#ifndef __MPU6050_H
#define __MPU6050_H
#include "stm32f10x.h"
#include "i2c.h"
#include "mpu6050_reg.h"

#define MPU6050_ADDR		    0xD0   //7位从机地址0x68>>1|0=0xD0,为写操作

/**
 * @brief MPU6050读出的加速度、陀螺仪数据结构体
 */
typedef struct
{
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
	int16_t temp_raw;
}MPU6050_DataDef;

/* 函数声明：全部增加I2C实例指针hi2c */

/**
 * @brief MPU写单个寄存器
 * @param hi2c I2C实例指针
 * @param RegAddress 寄存器地址
 * @param Data 要写入的数据
 */
void MPU6050_WriteReg(SoftI2C_TypeDef *hi2c,uint8_t RegAddress, uint8_t Data);

/**
 * @brief MPU读单个寄存器
 * @param hi2c I2C实例指针
 * @param RegAddress 寄存器地址
 * @retval 读到的寄存器值
 */
uint8_t MPU6050_ReadReg(SoftI2C_TypeDef *hi2c,uint8_t RegAddress);

/**
 * @brief MPU6050初始化，江协原版寄存器配置
 * @param hi2c I2C实例指针
 * @note I2C初始化放到main主函数完成，本函数不再内部初始化I2C
 */
void MPU6050_Init(SoftI2C_TypeDef *hi2c);

/**
 * @brief 获取设备ID，调试用，正常返回0x68
 * @param hi2c I2C实例指针
 * @retval WHO_AM_I寄存器读到的值
 */
uint8_t MPU6050_GetID(SoftI2C_TypeDef *hi2c);

/**
 * @brief 多次单寄存器读取（江协原版逻辑，适合学习）
 * @param hi2c I2C实例指针
 */
void MPU6050_GetData(SoftI2C_TypeDef *hi2c,int16_t *AccX, int16_t *AccY, int16_t *AccZ,
                     int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ);

/**
 * @brief 连续Burst读14字节，工程实际使用，效率更高
 * @param hi2c I2C实例指针
 * @param dat 接收数据的结构体指针
 */
void MPU6050_ReadBurst(SoftI2C_TypeDef *hi2c,MPU6050_DataDef *dat);

#endif
