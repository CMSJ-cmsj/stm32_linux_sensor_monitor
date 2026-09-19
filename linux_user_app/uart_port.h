#ifndef __UART_PORT_H
#define __UART_PORT_H

#include <stdint.h>
#include <unistd.h>

/**
 * @brief POSIX系统调用封装串口termios；不使用libserial第三方库
 * 全部接口：open/read/write/close，底层都是系统调用
 */

/**
 * @brief 打开串口，配置波特率9600，8N1，原始模式，关闭canonical；
 * @param dev_path 串口设备路径如"/dev/ttyUSB0"
 * @return >=0 成功返回fd；-1打开/配置失败
 */
int uart_open(const char *dev_path);

/**
 * @brief 关闭串口fd
 */
void uart_close(int fd);

/**
 * @brief 串口发送原始字节数组；write系统调用封装
 * @return 返回write系统调用返回值；-1代表IO错误
 */
ssize_t uart_write_buf(int fd,const uint8_t *buf,size_t len);

#endif