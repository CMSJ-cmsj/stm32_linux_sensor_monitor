#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "usart.h"

char Serial_RxPacket[SERIAL_RX_BUF_LEN];
// 事件标志：中断仅置1；主循环业务处理完成后置0
volatile uint8_t Serial_RxFlag = 0U;

//================ 中断三段状态机全局状态变量【业务版本保留，禁止改回IRQ内部static】================
#define STATE_IDLE         0U
#define STATE_RECV_DATA    1U
#define STATE_WAIT_EXCL    2U

volatile uint16_t RxState = STATE_IDLE;
volatile uint16_t pRxPacket = 0U;
//================================================================================================

//业务告警：#后面不是!，帧丢弃标记
volatile uint8_t uart_frame_discard_warn = 0U;

void Serial_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &USART_InitStructure);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);

	USART_Cmd(USART1, ENABLE);

	Serial_RxFlag = 0U;
	memset(Serial_RxPacket, 0, SERIAL_RX_BUF_LEN);

	RxState = STATE_IDLE;
	pRxPacket = 0U;
}

void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1, Byte);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	for (i = 0; i < Length; i ++)
	{
		Serial_SendByte(Array[i]);
	}
}

void Serial_SendString(char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i ++)
	{
		Serial_SendByte((uint8_t)String[i]);
	}
}

uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y --)
	{
		Result *= X;
	}
	return Result;
}

void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i ++)
	{
		Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');
	}
}

int fputc(int ch, FILE *f)
{
	Serial_SendByte((uint8_t)ch);
	return ch;
}

/**
 * @brief 串口格式化输出，使用vsnprintf增加缓冲区边界保护
 * @note 业务行为完全等价原版vsprintf；输出超过99字节会自动截断，防止栈溢出
 */
void Serial_Printf(char *format, ...)
{
	char String[100];
	va_list arg;
	va_start(arg, format);
	vsnprintf(String, sizeof(String), format, arg);
	va_end(arg);
	Serial_SendString(String);
}

uint8_t Serial_GetRxFlag(void)
{
	return Serial_RxFlag;	//仅读取，不修改标志位
}

/**
 * @brief USART1接收中断，三段状态机 【业务版本，全部调试采集代码已移除】
 * ==================协议变更历史==================
 * 版本1：帧尾\r\n；坑：串口助手文本模式无法可靠输出控制字符
 * 版本2：帧尾~#；坑：~波浪号输入法/复制粘贴字节异常转换
 * 版本3【当前生效】：
 *      帧头：'@' (0x40)
 *      帧尾：严格连续双可见字符 #! (0x23 紧跟0x21)
 *      完整报文样例： @LED:ON#!
 *      规则：
 *      1. 收到'#'进入STATE_WAIT_EXCL，#不存入载荷；仅期待紧随的'!'；
 *      2. 如果#后面不是!：整帧全部丢弃；中断仅置业务告警标记，打印放到主循环；
 *      3. #!两个帧尾字节均不会写入业务载荷；
 *      4. 载荷内部严禁连续出现 "#!" 组合，否则会被误识别帧结束；
 *      5. 本版本不实现半帧软件超时复位；后续可增加看门狗/软件定时器；
 *      6. STM32F10：读DR硬件自动清除RXNE，不再手动清除中断挂起位
 *
 * 状态定义：
 * STATE_IDLE(0)：空闲，等待帧头@；Serial_RxFlag==0才允许接收新帧头
 * STATE_RECV_DATA(1)：接收业务载荷；收到'#'进入STATE_WAIT_EXCL
 * STATE_WAIT_EXCL(2)：已经收到'#'，仅等待紧随其后的'!'；非!直接整帧作废回到IDLE
 *
 * 重要工程坑记录：RxState/pRxPacket禁止放回IRQ内部static，会产生编译器‑中断异步访问未定义行为
 * =================================================================
 */
void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		uint8_t RxData = USART_ReceiveData(USART1);

		switch(RxState)
		{
		case STATE_IDLE:
			//上一帧必须被主循环处理完毕，才接收新帧头@
			if(RxData == '@' && Serial_RxFlag == 0U)
			{
				RxState = STATE_RECV_DATA;
				pRxPacket = 0U;
				Serial_RxPacket[0] = '\0'; //收到帧头，首字节置结束符，防止缓冲区脏数据
			}
			break;

		case STATE_RECV_DATA:
			if(RxData == '#')
			{
				//收到帧尾前置#，暂不写入载荷；进入等待!状态
				RxState = STATE_WAIT_EXCL;
			}
			else
			{
				if(pRxPacket < (SERIAL_RX_BUF_LEN - 1U))
				{
					Serial_RxPacket[pRxPacket++] = RxData;
				}
				else
				{
					//缓冲区溢出，整帧丢弃，回到空闲
					RxState = STATE_IDLE;
					pRxPacket = 0U;
				}
			}
			break;

		case STATE_WAIT_EXCL:
			if(RxData == '!')
			{
				// ✅匹配完整帧尾 #!；#与!均不存入业务载荷
				//====业务健壮性防御：防止非法下标写'\0'越界，业务版本保留====
				if(pRxPacket >= SERIAL_RX_BUF_LEN)
				{
					//下标非法，直接丢弃本帧，不置业务帧标志
					RxState = STATE_IDLE;
					pRxPacket = 0U;
					uart_frame_discard_warn = 1U;
					break;
				}
				//================================================

				Serial_RxPacket[pRxPacket] = '\0';

				Serial_RxFlag = 1U;	//中断仅允许置1
				RxState = STATE_IDLE;
				pRxPacket = 0U;
			}
			else
			{
				// ❌#后面不是!：整帧直接全部作废；中断仅置业务告警标记，打印交给主循环
				RxState = STATE_IDLE;
				pRxPacket = 0U;
				uart_frame_discard_warn = 1U;
			}
			break;

		default:
			//异常状态兜底，强制回到空闲
			RxState = STATE_IDLE;
			pRxPacket = 0U;
			break;
		}
	}
}
