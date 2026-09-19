#ifndef __APP_UI_OLED_H
#define __APP_UI_OLED_H

#include "stm32f10x.h"
#include "i2c.h"

/**
 * @brief OLED业务界面刷新，显示传感器、阈值、报警状态
 * @param hi2c oled使用的软件i2c实例
 */
void App_Oled_Update(SoftI2C_TypeDef *hi2c);







#endif
