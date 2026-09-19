#ifndef __UART_RX_FIFO_H
#define __UART_RX_FIFO_H

#include "stm32f10x.h"
#include "stdio.h"

// 环形FIFO缓冲区大小，128字节足够我们项目使用
#define UART_RX_FIFO_SIZE 128U

/**
 * @brief  初始化串口接收环形FIFO
 * @retval 无
 */
void UartRxFifo_Init(void);

/**
 * @brief  向FIFO写入1字节，供串口接收中断调用
 * @param  ch：待存入字节
 * @retval 0：写入成功；1：FIFO已满，丢弃数据（溢出）
 * @note   这个函数会在中断里面执行，代码一定要简短，不能有延时、打印
 */
uint8_t UartRxFifo_Write(uint8_t ch);

/**
 * @brief  判断FIFO是否为空
 * @retval 1为空，0有数据
 */
uint8_t UartRxFifo_IsEmpty(void);

/**
 * @brief  获取FIFO当前有效数据个数
 * @retval 有效字节数量
 */
uint16_t UartRxFifo_GetCount(void);

uint8_t UartRxFifo_Read(uint8_t *p_ch);

uint32_t UartRxFifo_GetOverflowCnt(void);


#endif



