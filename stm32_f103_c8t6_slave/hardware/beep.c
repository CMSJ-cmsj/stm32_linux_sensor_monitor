#include "stm32f10x.h"                  // Device header
#include "beep.h"  
#include "delay.h"
/**
 * @brief 蜂鸣器初始化
 * @param GPIOx:GPIOA/GPIOB/GPIOC...
 * @param GPIO_Pin:GPIO_Pin_x
 * @retval none
 */
void BEEP_Init(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
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
	//......
	
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOx, &GPIO_InitStruct);
	
	BEEP_Off(GPIOx,GPIO_Pin);//默认关闭
	
}

void BEEP_Off(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	GPIO_SetBits(GPIOx, GPIO_Pin);
}

void BEEP_On(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	GPIO_ResetBits(GPIOx, GPIO_Pin);
}

/**
 * @brief 蜂鸣器鸣叫一段时间
 * @note 禁止在中断回调和中断下半部软中断调用
 */
void BEEP_Beep(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint32_t ms)
{
    BEEP_On(GPIOx, GPIO_Pin);
    delay_ms(ms);
    BEEP_Off(GPIOx, GPIO_Pin);
}

