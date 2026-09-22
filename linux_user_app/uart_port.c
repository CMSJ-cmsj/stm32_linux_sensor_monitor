#include "uart_port.h"
#include "ring_buf.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <poll.h>
#include "app_config.h"

/* 全部移到uart_port模块内部私有，不再从main extern引用 */
static uint8_t ring_buf_raw_mem[RING_BUF_SIZE];
static RingBuf_t g_uart_ring_buf;
static pthread_t g_uart_recv_tid;
static int g_pipe_fd[2];
static pthread_mutex_t g_rb_mutex;
static uint8_t g_uart_thread_running = 0U;
static volatile uint8_t g_uart_exit_flag = 0U;

extern int g_uart_fd;

int uart_open(const char *dev_path)
{
    int fd = open(dev_path,O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd<0)
    {
        fprintf(stderr,"[ERROR] open uart dev %s failed,errno=%d\n",dev_path,errno);
        return -1;
    }
    fcntl(fd,F_SETFL,0);
    struct termios tty;
    if(tcgetattr(fd,&tty) != 0)
    {
        fprintf(stderr,"[ERROR] tcgetattr failed errno=%d\n",errno);
        close(fd);
        return -1;
    }
    cfsetispeed(&tty,B9600);
    cfsetospeed(&tty,B9600);

    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    tty.c_cflag |= CS8;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_lflag &= ~(INPCK | ISTRIP | IXON | IXOFF | IGNCR);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if(tcsetattr(fd,TCSANOW,&tty) != 0)
    {
        fprintf(stderr,"[ERROR] tcsetattr configure uart failed errno=%d\n",errno);
        close(fd);
        return -1;
    }
    tcflush(fd,TCIOFLUSH);
    return fd;
}

void uart_close(int fd)
{
    if(fd>0)
    {
        close(fd);
    }
}

ssize_t uart_write_buf(int fd,const uint8_t *buf,size_t len)
{
    if(fd<0 || buf ==NULL || len == 0U)
    {
        return -1;
    }
    return write(fd,buf,len);
}

static void *uart_recv_thread_entry(void *arg)
{
    (void)arg;
    struct pollfd poll_fds[2];
    poll_fds[0].fd = g_uart_fd;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = g_pipe_fd[0];
    poll_fds[1].events = POLLIN;

    uint8_t temp_byte;
    while(uart_get_exit_flag() == 0U)
    {
        int poll_ret = poll(poll_fds, 2, -1);
        if(poll_ret < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            fprintf(stderr,"[ERROR] uart thread poll errno=%d\n",errno);
            break;
        }
        if(poll_fds[1].revents & POLLIN)
        {
            break;
        }
        if(poll_fds[0].revents & POLLIN)
        {
            ssize_t rd_cnt = read(g_uart_fd, &temp_byte, 1);
            if(rd_cnt <= 0)
            {
                if(rd_cnt < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    fprintf(stderr,"[FATAL] uart thread read error errno=%d\n",errno);
                    uart_set_exit_flag();
                }
                continue;
            }
            pthread_mutex_lock(&g_rb_mutex);
            RingBuf_WriteByte(&g_uart_ring_buf, temp_byte);
            pthread_mutex_unlock(&g_rb_mutex);
        }
    }
    g_uart_thread_running = 0U;
    pthread_exit(NULL);
}

int uart_start_recv_thread(void)
{
    if(g_uart_fd < 0)
    {
        fprintf(stderr,"[ERROR] uart_start_recv_thread: uart fd invalid\n");
        return -1;
    }
    /* 初始化模块内环形缓冲区 */
    RingBuf_Init(&g_uart_ring_buf, ring_buf_raw_mem, sizeof(ring_buf_raw_mem));
    g_uart_exit_flag = 0U;

    if(pipe(g_pipe_fd) != 0)
    {
        fprintf(stderr,"[ERROR] pipe create failed errno=%d\n",errno);
        return -1;
    }
    pthread_mutex_init(&g_rb_mutex, NULL);
    g_uart_thread_running = 1U;

    int ret = pthread_create(&g_uart_recv_tid, NULL, uart_recv_thread_entry, NULL);
    if(ret != 0)
    {
        fprintf(stderr,"[ERROR] pthread_create uart recv thread failed\n");
        close(g_pipe_fd[0]);
        close(g_pipe_fd[1]);
        pthread_mutex_destroy(&g_rb_mutex);
        g_uart_thread_running = 0U;
        return -1;
    }
    return 0;
}

int uart_stop_recv_thread(void)
{
    if(g_uart_thread_running == 0U)
    {
        goto clean_fd_mutex;
    }
    uint8_t dummy = 0;
    (void)write(g_pipe_fd[1], &dummy, 1);
    pthread_join(g_uart_recv_tid, NULL);

clean_fd_mutex:
    close(g_pipe_fd[0]);
    close(g_pipe_fd[1]);
    pthread_mutex_destroy(&g_rb_mutex);
    return 0;
}

size_t uart_ringbuf_get_count(void)
{
    size_t cnt;
    pthread_mutex_lock(&g_rb_mutex);
    cnt = RingBuf_GetCount(&g_uart_ring_buf);
    pthread_mutex_unlock(&g_rb_mutex);
    return cnt;
}

int uart_ringbuf_read_byte(uint8_t *ch)
{
    int ret;
    pthread_mutex_lock(&g_rb_mutex);
    ret = RingBuf_ReadByte(&g_uart_ring_buf, ch);
    pthread_mutex_unlock(&g_rb_mutex);
    return ret;
}

void uart_set_exit_flag(void)
{
    g_uart_exit_flag = 1U;
}

int uart_get_exit_flag(void)
{
    return g_uart_exit_flag;
}
