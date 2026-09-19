#include "i2c.h"
#include "delay.h"

/**
 * @brief  软件I2C GPIO初始化
 * @param  hi2c I2C实例指针
 * @note   I2C总线空闲：SCL=1，SDA=1；软件模拟I2C必须使用开漏输出Out_OD
 */
void Soft_I2C_Init(SoftI2C_TypeDef *hi2c)
{
	//开启对应GPIO端口时钟
	RCC_APB2PeriphClockCmd(hi2c->RCC_APB2Periph, ENABLE);

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = hi2c->SCL_Pin | hi2c->SDA_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;   // 开漏输出，I2C核心！
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(hi2c->GPIOx, &GPIO_InitStruct);

	//空闲状态SCL、SDA全部置高
	GPIO_SetBits(hi2c->GPIOx, hi2c->SCL_Pin);
	GPIO_SetBits(hi2c->GPIOx, hi2c->SDA_Pin);
}

/**
 * @brief I2C起始信号：SCL高电平时，SDA由高→低
 * @param hi2c I2C实例指针
 */
void Soft_I2C_Start(SoftI2C_TypeDef *hi2c)
{
    GPIO_SetBits(hi2c->GPIOx, hi2c->SDA_Pin);
    GPIO_SetBits(hi2c->GPIOx, hi2c->SCL_Pin);
    delay_us(5);
    GPIO_ResetBits(hi2c->GPIOx, hi2c->SDA_Pin);    // SCL高，SDA拉低，产生起始
    delay_us(5);
    GPIO_ResetBits(hi2c->GPIOx, hi2c->SCL_Pin);    // 拉低SCL，准备收发数据
}

/**
 * @brief I2C停止信号：SCL高电平时，SDA由低→高
 * @param hi2c I2C实例指针
 */
void Soft_I2C_Stop(SoftI2C_TypeDef *hi2c)
{
    GPIO_ResetBits(hi2c->GPIOx, hi2c->SDA_Pin);
    GPIO_SetBits(hi2c->GPIOx, hi2c->SCL_Pin);
    delay_us(5);
    GPIO_SetBits(hi2c->GPIOx, hi2c->SDA_Pin);    // SCL高，SDA拉高，产生停止
    delay_us(5);
}

/**
 * @brief I2C发送一个字节（高位先行 MSB）
 * @param hi2c I2C实例指针
 * @param dat 需要发送的数据
 */
void Soft_I2C_SendByte(SoftI2C_TypeDef *hi2c,uint8_t dat)
{
	uint8_t i;
	for(i=0;i<8;i++)
	{
		if(dat & 0x80)
		{
			GPIO_SetBits(hi2c->GPIOx, hi2c->SDA_Pin);
		}
		else
		{
			GPIO_ResetBits(hi2c->GPIOx, hi2c->SDA_Pin);
		}
		dat<<=1;
		delay_us(2);
		GPIO_SetBits(hi2c->GPIOx, hi2c->SCL_Pin);
		delay_us(5);
		GPIO_ResetBits(hi2c->GPIOx, hi2c->SCL_Pin);
        delay_us(2);
	}
}

/**
 * @brief 主机等待从机应答
 * @param hi2c I2C实例指针
 * @retval 0:收到应答ACK  1:无应答NACK
 */
uint8_t Soft_I2C_WaitAck(SoftI2C_TypeDef *hi2c)
{
    uint8_t ack = 1;
    GPIO_SetBits(hi2c->GPIOx, hi2c->SDA_Pin);        // 主机释放SDA，交给从机控制
    delay_us(5);
    GPIO_SetBits(hi2c->GPIOx, hi2c->SCL_Pin);
    delay_us(5);
    if(GPIO_ReadInputDataBit(hi2c->GPIOx, hi2c->SDA_Pin) == 0)
    {
        ack = 0;      // SDA被从机拉低，收到ACK
    }
    GPIO_ResetBits(hi2c->GPIOx, hi2c->SCL_Pin);
    return ack;
}

/**
 * @brief 主机发送应答信号
 * @param hi2c I2C实例指针
 * @param ack 0:ACK应答 1:NACK非应答
 */
void Soft_I2C_SendAck(SoftI2C_TypeDef *hi2c,uint8_t ack)
{
    if(ack)
    {
        GPIO_SetBits(hi2c->GPIOx, hi2c->SDA_Pin);
    }
    else
    {
        GPIO_ResetBits(hi2c->GPIOx, hi2c->SDA_Pin);
    }
    delay_us(5);
    GPIO_SetBits(hi2c->GPIOx, hi2c->SCL_Pin);
    delay_us(5);
    GPIO_ResetBits(hi2c->GPIOx, hi2c->SCL_Pin);
    GPIO_SetBits(hi2c->GPIOx, hi2c->SDA_Pin);    // 释放SDA
}

/**
 * @brief I2C读取1字节
 * @param hi2c I2C实例指针
 * @param ack 读完之后主机发送 ACK/NACK
 * @retval 读到的数据
 */
uint8_t Soft_I2C_ReadByte(SoftI2C_TypeDef *hi2c,uint8_t ack)
{
	uint8_t i, dat = 0;
    GPIO_SetBits(hi2c->GPIOx, hi2c->SDA_Pin);    // 主机释放SDA
	delay_us(2);
	for(i=0;i<8;i++)
	{
		dat <<= 1;
		GPIO_SetBits(hi2c->GPIOx, hi2c->SCL_Pin);
		delay_us(5);
		if(GPIO_ReadInputDataBit(hi2c->GPIOx, hi2c->SDA_Pin) != 0)
        {
            dat |= 0x01;
        }
		GPIO_ResetBits(hi2c->GPIOx, hi2c->SCL_Pin);
		delay_us(5);
	}
	Soft_I2C_SendAck(hi2c, ack); // 读完发送应答
    return dat;
}
