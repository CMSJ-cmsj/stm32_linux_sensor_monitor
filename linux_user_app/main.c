#include "app_config.h"
#include "uart_port.h"
#include "ring_buf.h"
#include "sensor_frame_parser.h"
#include "console_cmd.h"
#include "csv_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

/*========= 全局资源 =========*/
int g_uart_fd = -1;
int g_csv_fd = -1;
static uint8_t g_program_exit_flag = 0U;

/*========= 全局模块实例 ======*/
static RingBuf_t g_uart_ring_buf;
static uint8_t ring_buf_raw_mem[RING_BUF_SIZE];

/*===== 控制台下发送 行缓冲区=*/
static uint8_t console_tx_line_buf[CONSOLE_LINE_BUF_SIZE];

/*===SIGINT CTRL-C信号处理问题*/
static void sigint_handler(int sig)
{
    (void)sig;
    g_program_exit_flag = 1U;
}

static int parse_sensor_kv_payload(const char *raw_payload,SensorFrame_t *out_data)
{
    if(raw_payload == NULL || out_data == NULL)
    {
        return -1;
    }
    memset(out_data,0x00,sizeof(SensorFrame_t));
    char work_copy[PARSE_WORK_BUF_LEN];
    //strncpy原理:
    /*
        char *strncpy(char *dest,const char *src,size_t n);
            从src逐个字节复制到dest有以下两个分支
                复制过程中,复制n个字节前遇到'\0',停止复制且剩下的全部补0
                复制了n个字节依旧没有'\0',此时末尾不会追加'\0'
            返回值永远是dest首地址
    */
    strncpy(work_copy,raw_payload,sizeof(work_copy)-1U);
    work_copy[sizeof(work_copy)-1U] = '\0';
    //strtok_r原理:
    /*
        char *strtok_r(char *str,const char *delim,char **saveptr);
            str--第一次调用传入待分割字符串首地址,后续循环调用时填NULL
            delim--分隔符集合,任意字符都可,可以写多个如",\r\n".分割符会被改写为'\0'.
                连续的分隔符自动跳过,如"a,,b"-->"a"和"b",不会得到空字符串
            save_ptr--一般自己定义char *ctx类型,传入&ctx,函数内部用它保存分割完后剩下字符串的起始位置,函数
                外部由自己保管这个状态.同一条分割流程里循环调用时,不要修改ctx的值
        返回当前取出片段的首地址,分割完成的返回NULL
        
    */
    char *save_ptr = NULL;
    char *token = strtok_r(work_copy," ",&save_ptr);
    int parse_ok_cnt = 0;
   //strchr原理:
   /*
        char *strchr(const char *s,int c);
            在字符串s里从前往后查找第一个等于c的字符,c虽然传int,但只会取第一字节当做unsigned char对比
        返回该等于字符c的地址,找不到返回NULL
   */
  //样例T:25.1 L:1024 AX:120 AY:211 AZ:1630 GX:-11 GY:22 GZ:-33
    while(token != NULL)
    {
        char *colon_ptr = strchr(token,':');
        if(colon_ptr == NULL)
        {
            token = strtok_r(NULL," ",&save_ptr);
            continue;
        }
        *colon_ptr = '\0';
        const char *key = token;
        const char *val_str = colon_ptr + 1U;
    //atof原理:
    /*
        double atof(const char *nptr);
            跳过前置空白符,识别正负号,解析整除部分数字,如果遇到小数段则解析小数段
                支持识别科学技术如1.2E3,2.5e-2
            碰到不属于浮点数合法字符时停止,返回double
        (缺陷:)
            溢出和下溢没有错误标记
            无法区分出错和输入0.0
    */
    //atoi原理:
    /*
        int atoi(const char *nptr);
            跳过所有空白字符,识别正负号,往后读取数字字符,直到遇到非数字字符.
            返回int型整数
        (缺陷:)
            超出int所能表示的范围时,不报错,返回值乱掉
            无法区分出错和输出0
    */
        if(strcmp(key,"T") == 0)
        {
            out_data->temp = atof(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"L") == 0)
        {
            out_data->light = (uint16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"AX") == 0)
        {
            out_data->ax = (int16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"AY") == 0)
        {
            out_data->ay = (int16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"AZ") == 0)
        {
            out_data->az = (int16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"GX") == 0)
        {
            out_data->gx = (int16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"GY") == 0)
        {
            out_data->gy = (int16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        else if(strcmp(key,"GZ") == 0)
        {
            out_data->gz = (int16_t)atoi(val_str);
            parse_ok_cnt++;
        }
        token = strtok_r(NULL," ",&save_ptr);
    }
    if(parse_ok_cnt == 8)
    {
        out_data->valid = 1U;
        return 0;
    }
    fprintf(stderr,"[WARN] KV parse incomplete, valid field count=%d, drop frame\n",parse_ok_cnt);
    return -1;
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

    //sigaction原理:
    /*
        int sigaction(int signum,const struct sigaction *act,struct sigaction *oldact);
            signum:要处理的信号
            act:入参,不为NULL,就代表设置这个信号新的处理方式
            oldact:出参,不为NULL,内核把这个信号旧的配置回填进来,可以用来备份恢复
        返回0成功,-1失败
        struct sigaction
        {
            void (*sa_handler) (int);   //自定义函数/SIG_IGN(忽略)/SIG_DFL(恢复系统默认行为)
            void (*sa_sigaction)(int,siginfo_t *,void *);  
            sigset_t sa_mask;   //执行当前信号回调的时候,登记给sa_mask的信号会被临时屏蔽,当前出发的这个信号自动加入屏蔽集
            int sa_flags;   //行为开关,常用有SA_RESTART(被信号打断的系统调用自动重启,不然系统调用返回-1)
            void (*sa_restorer)(void)   //用来恢复用户栈,应用层直接置NULL
        }
    */
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT,&sa,NULL);

    RingBuf_Init(&g_uart_ring_buf,ring_buf_raw_mem,sizeof(ring_buf_raw_mem));

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
    
    //poll原理:
    /*
        int poll(struct pollfd *fds,nfds_t nfds,int timeout);
            fds:struct pollfd数组首地址,每一项代表一个带监控的fd
            nfds:数组里面元素的个数
            timeout:超时时间,单位ms(-1-无线阻塞,0-立即返回)
        返回值>0为就绪fd的总数,0为超时没有事件就绪,-1出错errno被置
        struct pollfd //就是来保存单个fd的监控配置与返回结果
        {
            int fd;     //文件描述符,为-1可以原地禁用数组里某个条目
            short events;   //用户填写,想要监控什么事情(可以合并|)
                常用POLLIN(监控是否可读,如socket/串口)
            short revents;  //内核填写,返回这个fd真实触发的事情
        }
    */
    struct pollfd poll_fds[2];
    poll_fds[0].fd = g_uart_fd;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = STDIN_FILENO;
    poll_fds[1].events = POLLIN;

    uint8_t temp_byte;  //存接受到的一个字节
    SensorFrame_t raw_parse_frame;  //原始载荷字符串
    SensorFrame_t final_data_frame; //解析完成后的真实数据
    ssize_t console_line_len;
    int poll_ret;
    int parser_ret;
    int kv_ret;
    int csv_ret;

    fprintf(stderr,"\n Linux Stm32-sensor host Ready \n");
    fprintf(stderr,"Usage: console input complete frame @xx:yy#! then enter,will send to Stm32\n");
    fprintf(stderr,"Ctrl+C safe exit.\n");

    while(g_program_exit_flag == 0U)
    {
        poll_ret = poll(poll_fds,2,APP_POLL_TIMEOUT_MS);
        if(poll_ret <0)
        {
            //EINTR--interrupted system call
            /*
                当程序正在执行一个阻塞类的系统调用时,进程收到一个信号,内核提前终止这次系统调用
                处理方法就是再次重试这个系统调用
            */
            if(errno == EINTR)
            {
                continue;
            }
            fprintf(stderr,"[ERROR] poll system call failed errno=%d\n",errno);
            break;
        }
        if(poll_ret == 0)
        {
            continue;
        }

        if((poll_fds[0].revents & POLLIN ) != 0)
        {
            while(1)
            {
                ssize_t rd_cnt = read(g_uart_fd,&temp_byte,1);
                if(rd_cnt <= 0)
                {
                    if(rd_cnt <0 && errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        fprintf(stderr,"[FATAL] uart read IO errno=%d\n",errno);
                        g_program_exit_flag = 1U;
                    }
                    break;
                }
                RingBuf_WriteByte(&g_uart_ring_buf,temp_byte);
            }

            while(RingBuf_GetCount(&g_uart_ring_buf) > 0U )
            {
                int rb_ret = RingBuf_ReadByte(&g_uart_ring_buf,&temp_byte);
                if(rb_ret != 0)
                {
                    break;
                }
                parser_ret = sensor_parser_input_byte(temp_byte,&raw_parse_frame);
                if(parser_ret == 1)
                {
                    fprintf(stderr,"[RECV_RAW] sensor raw payload=[%s]\n",(char *)raw_parse_frame.raw_payload);
                    kv_ret = parse_sensor_kv_payload((const char *)raw_parse_frame.raw_payload,&final_data_frame);
                    if(kv_ret != 0)
                    {
                        fprintf(stderr,"[WARN] KV parse fail ,drop this sensor frame,call release\n");
                        sensor_parser_release();
                        continue;
                    }

                    fprintf(stderr,"[PARSED] T=%.1f L=%d AX=%d AY=%d AZ=%d GX=%d GY=%d GZ=%d\n",
                        final_data_frame.temp,
                        final_data_frame.light,
                        final_data_frame.ax,
                        final_data_frame.ay,
                        final_data_frame.az,
                        final_data_frame.gx,
                        final_data_frame.gy,
                        final_data_frame.gz
                    );
                    //tv.tv_sec:Unix时间戳,从1970-01-01 UTC到现在一共经过多少秒
                    //tv.tv_usec:微妙,1s=1000000微妙
                    struct timeval tv;
                    gettimeofday(&tv,NULL);
                    double ts = tv.tv_sec + tv.tv_usec / 1000000.0;

                    csv_ret = csv_log_write_frame(g_csv_fd,&final_data_frame,ts);
                    if(csv_ret != 0)
                    {
                        fprintf(stderr,"[FATAL] csv log write fsync errno! release parser then exit.\n");
                        sensor_parser_release();
                        release_all_resource();
                        return EXIT_FAILURE;
                    }

                    sensor_parser_release();
                }
                else if(parser_ret == -1)
                {
                    continue;
                }
                else
                {
                    continue;
                }
            }
        }

       if(((poll_fds[1]).revents & POLLIN )!= 0)
       {
        console_line_len = console_read_line(console_tx_line_buf,sizeof(console_tx_line_buf));
        if(console_line_len > 0)
        {
            fprintf(stderr,"[SEND] raw cmd to uart len=%zd\n",console_line_len);
            uart_write_buf(g_uart_fd,console_tx_line_buf,(size_t)console_line_len);
        }
        else if(console_line_len == 0)
        {
            fprintf(stderr,"[INFO] stdin EOF,exit program\n");
            g_program_exit_flag = 1U;
        }
       }
    }
    if(g_program_exit_flag)
    {
        fprintf(stderr,"\n[SIGINT] Receive Ctrl‑C, prepare safe exit, release resource...\n");
    }
    release_all_resource();
    return EXIT_SUCCESS;
}
