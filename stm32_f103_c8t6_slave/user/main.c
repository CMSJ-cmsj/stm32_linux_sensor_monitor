#include "stm32f10x.h"
#include "delay.h"
#include "i2c.h"
#include "oled.h"
#include "mpu6050.h"
#include "adc.h"
#include "tim_pwm.h"
#include "usart.h"
#include "uart_cmd_parse.h"
#include "sensor_task.h"
#include "alarm.h"
#include "tim_period.h"
#include "app_ui_oled.h"

SoftI2C_TypeDef oledi2c1 = {
    .GPIOx = GPIOB,
    .SCL_Pin = GPIO_Pin_8,
    .SDA_Pin = GPIO_Pin_9,
    .RCC_APB2Periph = RCC_APB2Periph_GPIOB
};
SoftI2C_TypeDef mpu6050i2c2 = {
    .GPIOx = GPIOB,
    .SCL_Pin = GPIO_Pin_10,
    .SDA_Pin = GPIO_Pin_11,
    .RCC_APB2Periph = RCC_APB2Periph_GPIOB
};

int main(void)
{
    Delay_Init();

    Soft_I2C_Init(&oledi2c1);
	Soft_I2C_Init(&mpu6050i2c2);
    OLED_Init(&oledi2c1);
	OLED_Clear(&oledi2c1);
    MPU6050_Init(&mpu6050i2c2);
    ADC_Init_Config();
    SG90_Init();
    Serial_Init();
    Serial_Printf("System Power Up, Ready!\r\n");

    TIM2_Period_Init(2000);
	UartCmdParse_Init();
    Alarm_Init();

    while(1)
    {
        if(tim_period_flag == 1U)
        {
            tim_period_flag = 0U;

            //========业务定时任务【业务版本恢复，OLED业务界面上电打开】========
            Sensor_ReadAllTask(&mpu6050i2c2);
            Alarm_Process(g_sensor_data.temp);
            App_Oled_Update(&oledi2c1);
            //====================================================================
        }

        //业务告警：#后面不是!帧丢弃，业务版本保留
        if(uart_frame_discard_warn == 1U)
        {
            Serial_Printf("[WARN] Frame discard: # followed by non-!\r\n");
            uart_frame_discard_warn = 0U;
        }

        //串口指令轮询，仅此处调用解析入口
        UartCmdParse_Run();
    }
}
