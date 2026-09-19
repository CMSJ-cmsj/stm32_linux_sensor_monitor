#ifndef __TIM_PERIOD_H
#define __TIM_PERIOD_H
#include "stm32f10x.h"

/**
 * @brief  周期采集定时器，使用TIM2(APB1 16位通用定时器)
 * PSC=7199，实际分频系数7200
 * TIM内核72M → f_cnt =10000Hz，1个tick = 100微秒(0.1ms)
 * 最大定时：65536 * 0.1ms = 6553.6 ms ≈6.55秒
 * 中断内部只置标志位；I2C/OLED/串口业务全部放主循环
 */

// 外部全局标志，主循环使用，中断里面只写这个变量
// volatile：中断修改该变量，阻止编译器优化缓存读取
extern volatile uint8_t tim_period_flag;

/**
 * @brief  TIM2初始化，实现ms级周期中断
 * @param  ms: 需要的中断周期，单位毫秒，范围：1 ~ 6553
 * @note  1tick = 0.1ms；总tick = ms * 10U；ARR = tick‑1
 */
void TIM2_Period_Init(uint16_t ms);

#endif
