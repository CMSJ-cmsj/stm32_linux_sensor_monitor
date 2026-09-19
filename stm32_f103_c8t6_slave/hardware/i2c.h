#ifndef __I2C_H
#define __I2C_H
#include "stm32f10x.h"

/**
 * @brief 软件I2C实例结构体
 * @note 每一路独立的软件I2C，对应一个该结构体变量
 *       把GPIO端口、SCL引脚、SDA引脚、GPIO时钟封装在一起
 *       实现一套I2C驱动，支持多路I2C硬件实例
 */
typedef struct
{
    GPIO_TypeDef*        GPIOx;         // GPIO端口，例：GPIOB
    uint16_t             SCL_Pin;       // I2C SCL引脚
    uint16_t             SDA_Pin;       // I2C SDA引脚
    uint32_t             RCC_APB2Periph;// 该GPIO对应的APB2时钟使能位
} SoftI2C_TypeDef;


/**
 * @brief 初始化一路软件I2C的GPIO
 * @param hi2c 软件I2C实例指针
 * @note 配置为开漏输出；空闲状态SCL=1，SDA=1
 */
void Soft_I2C_Init(SoftI2C_TypeDef *hi2c);

/**
 * @brief I2C起始信号：SCL为高电平时，SDA由高变低
 * @param hi2c 软件I2C实例指针
 */
void Soft_I2C_Start(SoftI2C_TypeDef *hi2c);

/**
 * @brief I2C停止信号：SCL为高电平时，SDA由低变高
 * @param hi2c 软件I2C实例指针
 */
void Soft_I2C_Stop(SoftI2C_TypeDef *hi2c);

/**
 * @brief 主机等待从机应答
 * @param hi2c 软件I2C实例指针
 * @retval 0：收到ACK应答；1：NACK无应答
 */
uint8_t Soft_I2C_WaitAck(SoftI2C_TypeDef *hi2c);

/**
 * @brief 主机发送应答或者非应答信号
 * @param hi2c 软件I2C实例指针
 * @param ack  0发送ACK，1发送NACK
 */
void Soft_I2C_SendAck(SoftI2C_TypeDef *hi2c,uint8_t ack);

/**
 * @brief I2C发送1字节，高位MSB先行
 * @param hi2c 软件I2C实例指针
 * @param dat 待发送的字节数据
 */
void Soft_I2C_SendByte(SoftI2C_TypeDef *hi2c,uint8_t dat);

/**
 * @brief I2C读取1字节
 * @param hi2c 软件I2C实例指针
 * @param ack 读完之后主机发送 0=ACK，1=NACK
 * @retval 返回读到的一字节数据
 */
uint8_t Soft_I2C_ReadByte(SoftI2C_TypeDef *hi2c,uint8_t ack);

#endif
