#include "csv_log.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

//内部组装一行csv字符串:静态局部缓冲
#define CSV_LINE_BUF_SIZE 256U
static char csv_line_buf[CSV_LINE_BUF_SIZE];

/*
    @brief 写字符串到fd
    @return 0-全部写完,-1-IO错误
*/
static int csv_log_write_raw(int fd,const char *text)
{
    if(text == NULL)
    {
        return -1;
    }
    size_t total_len = strlen(text);
    size_t offset = 0U;
    ssize_t w_ret;
    while(offset < total_len)
    {
        w_ret = write(fd,text + offset,total_len - offset);
        if(w_ret < 0)
        {
            fprintf(stderr,"[ERROR] csv write raw failed errno=%d\n",errno);
            return -1;
        }
        offset += (size_t)w_ret;
    }
    return 0;
}

int csv_log_open(const char *file_path)
{
    int fd = open(file_path,O_WRONLY | O_APPEND | O_CREAT,644);
    //打开日志文件失败,业务无法运行，上层main收到-1,释放所有资源
    if(fd<0)
    {
        fprintf(stderr,"[FATAL] open csv log file %s failed. errno:%d\n",file_path,errno);
        return -1;
    }
    //判断文件是否为空,lseek移动到文件末尾,获取偏移
    off_t file_end = lseek(fd,0,SEEK_END);
    if(file_end == 0)
    {
        //空文件，写入csv表头
        const char *csv_header = "timestamp,temp,light,ax,ay,az,gx,gy,gz\n";
        if(csv_log_write_raw(fd,csv_header) != 0)
        {
            close(fd);
            return -1;
        }
        if(fsync(fd) < 0)
        {
            fprintf(stderr,"[ERROR] fsync after write csv header errno=%d\n",errno);
            close(fd);
            return -1;
        }
    }
    return fd;
}

/*调用csv_log_write_frame之前,main主循环必须完成字符串解析*/
int csv_log_write_frame(int fd,const SensorFrame_t *frame,double timestamp)
{
    if(fd < 0 || frame == NULL || frame->valid == 0U)
    {
        return -1;
    }

    //按照format格式化字符串，输出到str缓冲区,最多写入size-1个有效字符,末尾自动补0
    snprintf(csv_line_buf,sizeof(csv_line_buf),
    "%.2lf,%.1f,%d,%d,%d,%d,%d,%d,%d\n",
    timestamp,//占位时间戳,mian层调用get time of day 填充时间字符串
    frame->temp,
    frame->light,
    frame->ax,
    frame->ay,
    frame->az,
    frame->gx,
    frame->gy,
    frame->gz);

    if(csv_log_write_raw(fd,csv_line_buf) != 0)
    {
        return -1;
    }

    if(fsync(fd) < 0)
    {
        fprintf(stderr,"[ERROR] csv fsync failed errno=%d\n",errno);
        return -1;
    }
    return 0;
}

void csv_log_close(int fd)
{
    if(fd >= 0)
    {
        close(fd);
    }
}