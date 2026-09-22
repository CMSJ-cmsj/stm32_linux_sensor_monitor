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

int console_process_once(int uart_fd,uint8_t *tx_buf,size_t buf_size)
{
    ssize_t line_len = console_read_line(tx_buf, buf_size);
    if(line_len > 0)
    {
        fprintf(stderr,"[SEND] raw cmd to uart len=%zd\n",line_len);
        (void)write(uart_fd, tx_buf, (size_t)line_len);
        return 0;
    }
    else if(line_len == 0)
    {
        fprintf(stderr,"[INFO] stdin EOF,exit program\n");
        return 1;
    }
    return 0;
}
