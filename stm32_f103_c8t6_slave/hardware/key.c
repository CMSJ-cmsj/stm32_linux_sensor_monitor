#include "delay.h"
#include "key.h"


/**
 * @brief 按键GPIO初始化，输入上拉模式
 */
void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(KEY_GPIO_RCC, ENABLE);

    GPIO_InitStruct.GPIO_Pin = KEY_GPIO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;   // 上拉输入，按下拉低
    GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStruct);
}


/**
 * @brief 按键扫描函数，软件消抖
 * @retval KEY_NULL 无按键；KEY_DOWN检测到一次按下事件
 * @note 调用频率建议：10ms左右调用一次，不要死循环疯狂调用
 */
uint8_t KEY_Scan(void)
{
	static uint8_t key_state = 0; //记住按键当前状态，static函数内变量，不会每次调用重置
	switch(key_state)
	{
		case 0:      //【状态0：空闲等待，没有按键按下】
			if(GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_GPIO_PIN) == 0)
			{
				key_state = 1; //检测到电平变低，怀疑按键按下，进入消抖状态
			}
			break;

		case 1:      //【状态1：消抖确认阶段】
			delay_ms(20);   //等待抖动过去 20ms
			if(GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_GPIO_PIN) == 0)
			{
				key_state = 2; //确认：真的按下了，切换到等待松开状态
				return KEY_DOWN; //上报：按键按下事件，给上层main
			}
			else
			{
				key_state = 0; //是抖动干扰，假触发，回到空闲
			}
			break;

		case 2:      //【状态2：已经确认按下，等待按键松开】
			if(GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_GPIO_PIN) == 1)
			{
				key_state = 0; //按键松开，回到空闲状态，等待下一次按键
			}
			break;

		default:
			key_state = 0;
			break;
	}
	return KEY_NULL;
}











