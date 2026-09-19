#ifndef __SENSOR_TASK_H
#define __SENSOR_TASK_H
#include "stm32f10x.h"
#include "mpu6050.h"
#include "i2c.h"
#include "adc.h"

/* 传感器统一数据结构体 */
typedef struct
{
    float temp;
    uint16_t light;
    int16_t ax,ay,az;
    int16_t gx,gy,gz;
}SensorDataTypeDef;

extern SensorDataTypeDef g_sensor_data;

void Sensor_ReadAllTask(SoftI2C_TypeDef *hi2c_mpu);

#endif



