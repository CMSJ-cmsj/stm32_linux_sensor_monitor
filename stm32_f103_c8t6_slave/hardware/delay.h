#include "stm32f10x.h"                  // Device header

#ifndef __DELAY_H

#define __DELAY_H

void Delay_Init(void);
	
void delay_us(uint32_t nus);

void delay_ms(uint32_t nms);


#endif
