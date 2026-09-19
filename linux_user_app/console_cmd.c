#include "console_cmd.h"

ssize_t console_read_line(uint8_t *buf,size_t buf_size)
{
    if(buf == NULL || buf_size == 0U)
    {
        return -1;
    }
    memset(buf,0x00,buf_size);
    size_t idx = 0U;
    uint8_t ch;
    ssize_t rd_ret;

    while(1)
    {
        rd_ret = read(STDIN_FILENO,&ch,1);
        if(rd_ret <= 0)
        {
            return 0;
        }
        if(ch == '\r' || ch == '\n')
        {
            break;
        }
        //留个空间写'\0'
        if(idx >=(buf_size - 1U))
        {
            fprintf(stderr,"[WARN] console input line buffer overflow,line truncted\n");
            return -1;
        }
        else
        {
            buf[idx] = ch;
            idx++;
        }
    }
    buf[idx] = '\0';
    return (ssize_t)idx;
}