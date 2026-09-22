#include "app_config.h"
#include "uart_port.h"
#include "sensor_frame_parser.h"
#include "sensor_frame_proc.h"
#include "console_cmd.h"
#include "csv_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

/*========= 全局资源 =========*/
int g_uart_fd = -1;
int g_csv_fd = -1;
static volatile uint8_t g_program_exit_flag = 0U;

/*===== 控制台下发送 行缓冲区=====*/
static uint8_t console_tx_line_buf[CONSOLE_LINE_BUF_SIZE];

static void sigint_handler(int sig)
{
    (void)sig;
    g_program_exit_flag = 1U;
    uart_set_exit_flag();
}

static void release_all_resource(void)
{
    csv_log_close(g_csv_fd);
    uart_close(g_uart_fd);
    g_csv_fd = -1;
    g_uart_fd = -1;
    fprintf(stderr,"[Exit] All resource released safety.\n");
}

int main(int argc,char ** argv)
{
    const char *uart_dev_name = APP_DEFAULT_UART_DEV;
    if(argc >= 2)
    {
        uart_dev_name = argv[1];
    }

    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT,&sa,NULL);

    g_uart_fd = uart_open(uart_dev_name);
    if(g_uart_fd < 0)
    {
        release_all_resource();
        return EXIT_FAILURE;
    }
    fprintf(stderr,"[INIT] UART open success dev=\"%s\" baud=9600 8N1\n",uart_dev_name);

    g_csv_fd = csv_log_open(APP_CSV_LOG_FILE);
    if(g_csv_fd < 0)
    {
        release_all_resource();
        return EXIT_FAILURE;
    }
    fprintf(stderr,"[INIT] CSV log open success,path=%s\n",APP_CSV_LOG_FILE);

    if(uart_start_recv_thread() != 0)
    {
        release_all_resource();
        return EXIT_FAILURE;
    }

    struct pollfd poll_fds[1];
    poll_fds[0].fd = STDIN_FILENO;
    poll_fds[0].events = POLLIN;

    uint8_t temp_byte;
    SensorFrame_t raw_parse_frame;
    int poll_ret;
    int parser_ret;
    int proc_ret;

    fprintf(stderr,"\n Linux Stm32‑sensor host Ready \n");
    fprintf(stderr,"Usage: console input complete frame @xx:yy#! then enter,will send to Stm32\n");
    fprintf(stderr,"Ctrl+C safe exit.\n");

    while(g_program_exit_flag == 0U)
    {
        poll_ret = poll(poll_fds,1,APP_POLL_TIMEOUT_MS);
        if(poll_ret <0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            fprintf(stderr,"[ERROR] poll system call failed errno=%d\n",errno);
            break;
        }
        if(poll_ret == 0)
        {
        }
        else
        {
            if((poll_fds[0].revents & POLLIN )!= 0)
            {
                int ret = console_process_once(g_uart_fd, console_tx_line_buf, sizeof(console_tx_line_buf));
                if(ret != 0)
                {
                    g_program_exit_flag = 1U;
                    uart_set_exit_flag();
                }
            }
        }

        while(1)
        {
            int rb_cnt = (int)uart_ringbuf_get_count();
            if(rb_cnt <= 0)
            {
                break;
            }
            int rb_ret = uart_ringbuf_read_byte(&temp_byte);
            if(rb_ret != 0)
            {
                break;
            }
            parser_ret = sensor_parser_input_byte(temp_byte,&raw_parse_frame);
            if(parser_ret == 1)
            {
                fprintf(stderr,"[RECV_RAW] sensor raw payload=[%s]\n",(char *)raw_parse_frame.raw_payload);
                proc_ret = sensor_frame_process((const char *)raw_parse_frame.raw_payload, g_csv_fd);
                sensor_parser_release();
                /*
                修复逻辑：
                proc_ret == -1 分两种：
                    1.KV解析失败：仅仅丢弃本帧，继续运行，不要退出
                    2.csv IO致命错误：才执行goto退出
                sensor_frame_process内部：只有csv_log_write_frame返回‑1时才打印[FATAL]日志
                我们依靠stderr打印的"[FATAL] csv log write fsync errno!"作为真正致命错误标记。
                现在简化处理：proc_ret=-1 只有发生CSV IO错误才退出；单纯KV解析失败直接continue。
                判别方法：看sensor_frame_process返回‑1时，是否是csv出错。
                当前最小修复：不在此处直接goto；只有真正IO故障才退出，解析失败仅丢弃帧。
                */
                if(proc_ret == 0)
                {
                    //正常，什么都不做
                }
                else
                {
                    // proc_ret == -1
                    // 注意：无法在外部直接区分是解析失败还是IO失败；
                    // 优化：我们重新调用一次parse_sensor_kv_payload，区分是解析问题还是IO问题
                    SensorFrame_t tmp;
                    int kv_ret = parse_sensor_kv_payload((const char *)raw_parse_frame.raw_payload,&tmp);
                    if(kv_ret == 0)
                    {
                        // KV解析成功，说明是CSV IO发生致命错误，必须退出
                        goto main_exit_loop;
                    }
                    else
                    {
                        // KV解析失败，应答报文，丢弃这一帧，继续循环
                        continue;
                    }
                }
            }
        }
    }
main_exit_loop:
    if(g_program_exit_flag)
    {
        fprintf(stderr,"\n[SIGINT] Receive Ctrl‑C, prepare safe exit, release resource...\n");
    }
    uart_stop_recv_thread();
    release_all_resource();
    return EXIT_SUCCESS;
}
