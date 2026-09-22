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
/**
 * @brief 启动串口接收子线程
 * 要求：调用前g_uart_fd必须已经有效，串口open成功
 * @return 0成功，‑1失败
 */
int uart_start_recv_thread(void);
/**
 * @brief 通知串口子线程退出，并且pthread_join回收线程资源
 * 运行于主线程上下文；join返回代表串口线程不再访问串口fd
 * @return 0成功
 */
int uart_stop_recv_thread(void);

/**
 * @brief 读取ringbuf有效字节数（内部自带互斥锁保护）
 */
size_t uart_ringbuf_get_count(void);
/**
 * @brief 从ringbuf读取1字节（内部自带互斥锁保护）
 * @param ch 输出字节指针
 * @return 0成功，1缓冲区空
 */
int uart_ringbuf_read_byte(uint8_t *ch);

/**
 * @brief 设置串口模块退出标记，通知接收线程准备退出
 */
void uart_set_exit_flag(void);
/**
 * @brief 获取串口模块退出标记
 * @retval 1 需要退出，0正常运行
 */
int uart_get_exit_flag(void);

#endif
