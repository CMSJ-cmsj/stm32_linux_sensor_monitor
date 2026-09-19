#ifndef __USART_H
#define __USART_H
#include <stdio.h>
#include <stdint.h>

#define SERIAL_RX_BUF_LEN 100U

extern char Serial_RxPacket[SERIAL_RX_BUF_LEN];
extern volatile uint8_t Serial_RxFlag;

// 中断状态机全局状态变量，volatile禁止编译器寄存器缓存；不回退IRQ内部static
extern volatile uint16_t RxState;
extern volatile uint16_t pRxPacket;

//业务告警：#后紧跟非!，帧丢弃标记，属于协议业务告警，非调试
extern volatile uint8_t uart_frame_discard_warn;

void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
void Serial_Printf(char *format, ...);

/**
 * @brief 获取接收帧标志，仅读取，不修改标志位
 * @retval 1:存在未处理完整帧  0:无新帧
 */
uint8_t Serial_GetRxFlag(void);

#endif
