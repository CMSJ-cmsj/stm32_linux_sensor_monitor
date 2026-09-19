#ifndef __BEEP_H
#define __BEEP_H

void BEEP_Init(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

void BEEP_Off(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

void BEEP_On(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

void BEEP_Beep(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint32_t ms);


#endif 
