#ifndef __TIM_PWM_H
#define __TIM_PWM_H

#include "stm32f10x.h"  

/* ============ 引脚/定时器宏定义（要换脚只改这里） ============ */
#define SG90_TIM            TIM3         // 使用的定时器
#define SG90_TIM_RCC        RCC_APB1Periph_TIM3      // 定时器所在总线：TIM3在APB1
#define SG90_GPIO_RCC       RCC_APB2Periph_GPIOA     // GPIOA时钟(APB2)
#define SG90_GPIO           GPIOA        // 端口
#define SG90_PIN            GPIO_Pin_6   // PA6
#define SG90_CHANNEL        TIM_Channel_1  // TIM3通道1
#define SG90_GPIO_PinSource GPIO_PinSource6  // 复用功能选择源

/* ============ PWM参数（已算好，见下方说明） ============ */
/* 系统时钟72MHz，PSC=719 -> 计数频率 = 72MHz/(719+1)=100kHz，每计数1次=10us */
/* ARR=1999 -> 周期 = (1999+1)*10us = 20ms = 50Hz（舵机标准周期） */
#define SG90_PSC            719        // 预分频值
#define SG90_ARR            1999       // 自动重装载值
#define SG90_CCR_0DEG       50         // 0度：  0.5ms -> 50*10us = 0.5ms
#define SG90_CCR_90DEG      150        // 90度： 1.5ms -> 150*10us = 1.5ms
#define SG90_CCR_180DEG     250        // 180度：2.5ms -> 250*10us = 2.5ms

/* ============ 对外接口 ============ */
void SG90_Init(void);                // 初始化定时器PWM，舵机归零
void SG90_SetAngle(uint8_t angle);   // 设置舵机角度 0~180

#endif


