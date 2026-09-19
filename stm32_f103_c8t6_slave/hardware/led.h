#ifndef __LED_H
#define __LED_H

void LED_Init(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

void LED_Off(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

void LED_On(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

void LED_Toggle(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
#endif
