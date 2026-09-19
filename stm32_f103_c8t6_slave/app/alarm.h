#ifndef __ALARM_H
#define __ALARM_H
#include "stm32f10x.h"

/* 默认上电阈值（只做初始化赋值，不再作为运行时常量） */
#define TEMP_HIGH_DEFAULT     31.0f
#define TEMP_LOW_DEFAULT      22.0f

/* 蜂鸣器、LED硬件引脚定义，根据你的硬件修改 */
#define BEEP_GPIO_PIN GPIO_Pin_0
#define BEEP_GPIO_PORT GPIOA
#define LED_GPIO_PIN GPIO_Pin_2
#define LED_GPIO_PORT GPIOA

/* 全局可修改报警参数，uart_cmd_parse可以extern修改 */
extern float g_alarm_high_thr;
extern float g_alarm_low_thr;

// 1开启报警，0关闭报警
extern uint8_t g_alarm_enable;

void Alarm_Init(void);

/**
 * @brief 报警业务处理
 * @param temp_val 当前温度
 */
void Alarm_Process(float temp_val);

#endif
