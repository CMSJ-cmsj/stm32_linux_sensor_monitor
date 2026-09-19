#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

// ========== 根据你的实际硬件修改引脚 ==========
#define KEY_GPIO_PORT     GPIOA
#define KEY_GPIO_RCC      RCC_APB2Periph_GPIOA
#define KEY_GPIO_PIN      GPIO_Pin_0

// 按键返回值定义
#define KEY_NULL     0   // 无按键按下
#define KEY_DOWN     1   // 按键按下（消抖完成）

void KEY_Init(void);       // 按键GPIO初始化
uint8_t KEY_Scan(void);    // 按键扫描，返回KEY_NULL / KEY_DOWN

#endif
