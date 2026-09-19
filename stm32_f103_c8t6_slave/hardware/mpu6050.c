#include "mpu6050.h"

/**
 * @brief MPU写单个寄存器
 * @param hi2c I2C实例指针
 * @param RegAddress 寄存器地址
 * @param Data 待写入数据
 */
void MPU6050_WriteReg(SoftI2C_TypeDef *hi2c,uint8_t RegAddress, uint8_t Data)
{
	Soft_I2C_Start(hi2c);
	Soft_I2C_SendByte(hi2c,MPU6050_ADDR);
	Soft_I2C_WaitAck(hi2c);

	Soft_I2C_SendByte(hi2c,RegAddress);
	Soft_I2C_WaitAck(hi2c);

	Soft_I2C_SendByte(hi2c,Data);
	Soft_I2C_WaitAck(hi2c);
	Soft_I2C_Stop(hi2c);
}

/**
 * @brief MPU读单个寄存器
 * @param hi2c I2C实例指针
 * @param RegAddress 寄存器地址
 * @retval 读到的寄存器数据
 */
uint8_t MPU6050_ReadReg(SoftI2C_TypeDef *hi2c,uint8_t RegAddress)
{
	uint8_t Data;
	Soft_I2C_Start(hi2c);
	Soft_I2C_SendByte(hi2c,MPU6050_ADDR);
	Soft_I2C_WaitAck(hi2c);
	Soft_I2C_SendByte(hi2c,RegAddress);
	Soft_I2C_WaitAck(hi2c);

	Soft_I2C_Start(hi2c);	                //重复起始信号
	Soft_I2C_SendByte(hi2c,MPU6050_ADDR | 0x01);
	Soft_I2C_WaitAck(hi2c);
	Data = Soft_I2C_ReadByte(hi2c,1);	    //读完发NACK
	Soft_I2C_Stop(hi2c);
	return Data;
}

/**
 * @brief MPU6050初始化，完全沿用江协寄存器配置
 * @param hi2c I2C实例指针
 * @note 已经删除内部Soft_I2C_Init，I2C由main统一初始化
 */
void MPU6050_Init(SoftI2C_TypeDef *hi2c)
{
	MPU6050_WriteReg(hi2c,MPU6050_PWR_MGMT_1, 0x01);	//唤醒，时钟源X轴陀螺仪
	MPU6050_WriteReg(hi2c,MPU6050_PWR_MGMT_2, 0x00);	//全部轴不待机
	MPU6050_WriteReg(hi2c,MPU6050_SMPLRT_DIV, 0x09);	//采样率分频
	MPU6050_WriteReg(hi2c,MPU6050_CONFIG, 0x06);		//DLPF滤波
	MPU6050_WriteReg(hi2c,MPU6050_GYRO_CONFIG, 0x18);	//陀螺仪 ±2000°/s
	MPU6050_WriteReg(hi2c,MPU6050_ACCEL_CONFIG, 0x18);	//加速度 ±16g
}

/**
 * @brief 获取设备ID，调试用，正常返回0x68
 * @param hi2c I2C实例指针
 * @retval WHO_AM_I寄存器值
 */
uint8_t MPU6050_GetID(SoftI2C_TypeDef *hi2c)
{
	return MPU6050_ReadReg(hi2c,MPU6050_WHO_AM_I);
}

/**
 * @brief 多次单寄存器读取（江协原版逻辑，适合学习）
 * @param hi2c I2C实例指针
 */
void MPU6050_GetData(SoftI2C_TypeDef *hi2c,int16_t *AccX, int16_t *AccY, int16_t *AccZ,
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
	uint8_t DataH, DataL;
	DataH = MPU6050_ReadReg(hi2c,MPU6050_ACCEL_XOUT_H);
	DataL = MPU6050_ReadReg(hi2c,MPU6050_ACCEL_XOUT_L);
	*AccX = (DataH << 8) | DataL;

	DataH = MPU6050_ReadReg(hi2c,MPU6050_ACCEL_YOUT_H);
	DataL = MPU6050_ReadReg(hi2c,MPU6050_ACCEL_YOUT_L);
	*AccY = (DataH << 8) | DataL;

	DataH = MPU6050_ReadReg(hi2c,MPU6050_ACCEL_ZOUT_H);
	DataL = MPU6050_ReadReg(hi2c,MPU6050_ACCEL_ZOUT_L);
	*AccZ = (DataH << 8) | DataL;

	DataH = MPU6050_ReadReg(hi2c,MPU6050_GYRO_XOUT_H);
	DataL = MPU6050_ReadReg(hi2c,MPU6050_GYRO_XOUT_L);
	*GyroX = (DataH << 8) | DataL;

	DataH = MPU6050_ReadReg(hi2c,MPU6050_GYRO_YOUT_H);
	DataL = MPU6050_ReadReg(hi2c,MPU6050_GYRO_YOUT_L);
	*GyroY = (DataH << 8) | DataL;

	DataH = MPU6050_ReadReg(hi2c,MPU6050_GYRO_ZOUT_H);
	DataL = MPU6050_ReadReg(hi2c,MPU6050_GYRO_ZOUT_L);
	*GyroZ = (DataH << 8) | DataL;
}

/**
 * @brief 连续Burst读14字节，工程实际使用，效率更高
 * @param hi2c I2C实例指针
 * @param dat 输出数据结构体指针
 */
void MPU6050_ReadBurst(SoftI2C_TypeDef *hi2c,MPU6050_DataDef *dat)
{
	uint8_t buf[14];
	uint8_t i;

	Soft_I2C_Start(hi2c);
	Soft_I2C_SendByte(hi2c,MPU6050_ADDR);
	Soft_I2C_WaitAck(hi2c);
	Soft_I2C_SendByte(hi2c,MPU6050_ACCEL_XOUT_H);
	Soft_I2C_WaitAck(hi2c);
	Soft_I2C_Start(hi2c);
	Soft_I2C_SendByte(hi2c,MPU6050_ADDR | 0x01);
	Soft_I2C_WaitAck(hi2c);

	for(i=0;i<14;i++)
	{
		if(i == 13)
			buf[i] = Soft_I2C_ReadByte(hi2c,1); //最后一字节NACK
		else
			buf[i] = Soft_I2C_ReadByte(hi2c,0);
	}
	Soft_I2C_Stop(hi2c);

	dat->ax = (int16_t)((buf[0] << 8) | buf[1]);
	dat->ay = (int16_t)((buf[2] << 8) | buf[3]);
	dat->az = (int16_t)((buf[4] << 8) | buf[5]);
	dat->temp_raw = (int16_t)((buf[6] << 8 | buf[7]));
	dat->gx = (int16_t)((buf[8] << 8) | buf[9]);
	dat->gy = (int16_t)((buf[10] << 8) | buf[11]);
	dat->gz = (int16_t)((buf[12] << 8) | buf[13]);
}
