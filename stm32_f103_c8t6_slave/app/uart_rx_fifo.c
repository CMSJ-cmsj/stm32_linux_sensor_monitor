#include "uart_rx_fifo.h"

/*
环形FIFO原理：
head：写指针，下一个数据写到head位置；中断写
tail：读指针，下一个数据从tail取出；主循环读
head == tail：缓冲区为空；
(head +1)%SIZE == tail：缓冲区满；
为了区分满和空，牺牲1字节空间，不全部占满缓存
*/
static uint8_t g_uart_rx_buf[UART_RX_FIFO_SIZE];
static uint16_t g_fifo_head;// 写索引
static uint16_t g_fifo_tail;// 读索引
static uint32_t g_fifo_overflow_cnt = 0U; //FIFO溢出计数

void UartRxFifo_Init(void)
{
	g_fifo_head = 0U;
	g_fifo_tail =0U;
}

uint8_t UartRxFifo_Write(uint8_t ch)
{
	// 判断FIFO是否已满：head往后走一格就追上tail，代表满
	uint16_t next_head = (g_fifo_head + 1U) % UART_RX_FIFO_SIZE;
	if(next_head == g_fifo_tail)
	{
		//调试时，可以增加一条调试指令，例如`GET_FIFO#`，返回当前 FIFO 有效字节数 + 溢出计数值，快速定位是否发生丢字节
		g_fifo_overflow_cnt++; //溢出计数累加
		// 缓冲区溢出，直接丢弃当前字节，返回1标记溢出
		return 1U;
	}
	g_uart_rx_buf[g_fifo_head] = ch;
	g_fifo_head = next_head;
	return 0U;
}

//对外提供读取接口，调试打印溢出次数
uint32_t UartRxFifo_GetOverflowCnt(void)
{
    return g_fifo_overflow_cnt;
}

uint8_t UartRxFifo_Read(uint8_t *p_ch)
{
	if(p_ch == NULL)
	{
		return 1U;
	}
	if(g_fifo_head == g_fifo_tail)
	{
		// 没有数据
		return 1U;
	}
	*p_ch = g_uart_rx_buf[g_fifo_tail];
	g_fifo_tail = (g_fifo_tail + 1U) % UART_RX_FIFO_SIZE;
	return 0U;
}

uint8_t UartRxFifo_IsEmpty(void)
{
	return (g_fifo_head == g_fifo_tail) ? 1U : 0U;
}

uint16_t UartRxFifo_GetCount(void)
{
	if(g_fifo_head >= g_fifo_tail)
	{
		return g_fifo_head - g_fifo_tail;
	}
	else
	{
		return (UART_RX_FIFO_SIZE - g_fifo_tail) + g_fifo_head;
	}
}


































