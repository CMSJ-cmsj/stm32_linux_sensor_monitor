#include "uart_port.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

/*linux的串口属于tty终端设备,默认终端模式,给shell使用*/
/*我们做串口二进制通信,要改成原始模式,收到什么字节就给程序什么字节*/
int uart_open(const char *dev_path)
{
    //O_NOCTTY:表示不要把串口当成控制终端,不然当串口接收到Crtl+C,内核会直接给你的进程发SIGINT,程序直接退出
    //O_NDELAY: open调用不会被串口信号卡住
    int fd = open(dev_path,O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd<0)
    {
        fprintf(stderr,"[ERROR] open uart dev %s failed,errno=%d\n",dev_path,errno);
        return -1;
    }
    // 清除O_NDELAY，恢复阻塞语义,后续read默认阻塞等待数据，后面用poll多路IO来驱动
    fcntl(fd,F_SETFL,0);

    //串口属于TTY终端设备:open打开之后必须用专用结构体配置串口参数
    struct termios tty;

    /*-------------------------------------------------先读---在修改---最后写回---------------------------------------------------------*/
    //读取串口当前的termios配置
    if(tcgetattr(fd,&tty) != 0)
    {
        fprintf(stderr,"[ERROR] tcgetattr failed errno=%d\n",errno);
        close(fd);
        return -1;
    }

    //设置9600 8N1:原始模式
    cfsetispeed(&tty,B9600);
    cfsetospeed(&tty,B9600);

    //c_cflag:控制标志
    /*
        parenb:是否为奇偶校验   cstopb:为几个停止位
        csize:旧的数据位数      cs8:8位数据位
        cread:使能串口接收      clocal:忽略调制解调器控线,开发板串口必备,不然开发板接收不带数据
    */
    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    tty.c_cflag |= CS8;
    tty.c_cflag |= CREAD | CLOCAL;

    //c_lflag:输入标志
    /*
        一般关掉所有,关闭输入转换
    */
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_lflag &= ~(INPCK | ISTRIP | IXON | IXOFF | IGNCR);

    //c_oflag:输出标志
    /*
        关掉所有内核输出处理，不做如换行/回车转换,还是原样发送
    */
    tty.c_oflag &= ~OPOST;

    /*
        vmin:最小读取字节数
        vtime:超时时间
    */
    //配合poll，poll监测有数据,在调用read去拿字节
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    //把配置写回内核
    //tcsanow:立即生效,不等待缓冲区
    if(tcsetattr(fd,TCSANOW,&tty) != 0)
    {
        fprintf(stderr,"[ERROR] tcsetattr configure uart failed errno=%d\n",errno);
        close(fd);
        return -1;
    }

    //清空上次串口输入+输出缓冲区tcioflush
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