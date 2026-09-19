 #ifndef __ADC_H
#define __ADC_H

#include "stm32f10x.h"

// ADC 硬件宏定义
#define ADCx                    ADC1
#define ADC_RCC_APB2            RCC_APB2Periph_ADC1
#define ADC_GPIO_PORT           GPIOA
#define ADC_GPIO_RCC            RCC_APB2Periph_GPIOA
#define ADC_GPIO_PIN            GPIO_Pin_1   // ADC1_IN1 PA1

void ADC_Init_Config(void);               // ADC初始化
uint16_t ADC_Get_Sample_Value(void);      // 获取一次ADC采样原始值 0~4095

#endif


