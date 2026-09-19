#include "stm32f10x.h"                  // Device header
#include "led.h"

/**
* @brief LED初始化
* @param GPIOx:GPIOA/GPIOB/GPIOC...
* @param GPIO_Pin:GPIO_Pin_0 ~GPIO_Pin_15
* @retval none
*/
void LED_Init(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	if(GPIOx == GPIOA)
	{	
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if(GPIOx == GPIOB)
	{	
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
	else if(GPIOx == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	}
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOx, &GPIO_InitStruct);
	
	LED_Off(GPIOx,GPIO_Pin);//默认关灯
	
}

void LED_On(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	GPIO_WriteBit(GPIOx,GPIO_Pin,Bit_RESET);
}

void LED_Off(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	GPIO_WriteBit(GPIOx,GPIO_Pin,Bit_SET);
}

void LED_Toggle(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	if(GPIO_ReadOutputDataBit(GPIOx,GPIO_Pin))
	{
		GPIO_WriteBit(GPIOx,GPIO_Pin,Bit_RESET);
	}
	else
	{
		GPIO_WriteBit(GPIOx,GPIO_Pin,Bit_SET);
	}
}	


