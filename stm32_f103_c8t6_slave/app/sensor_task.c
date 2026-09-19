#include "sensor_task.h"

SensorDataTypeDef g_sensor_data;

/* 光照滑动平均滤波，窗口大小8次采样 */
#define LIGHT_FILTER_WIN 8U
static uint16_t light_buf[LIGHT_FILTER_WIN];
static uint8_t light_idx = 0U;
static uint8_t sample_cnt = 0U;//记录当前有效样本数量

/**
 * @brief 光照滑动平均滤波
 * @param new_val ADC新采样原始值
 * @return 滤波输出
 * @note 上电样本逐步累积：1~7个样本时除以实际样本数；满8之后固定除以8
 */
static uint16_t Light_Filter(uint16_t new_val)
{
	uint32_t sum = 0U;
	uint8_t i;
	
	//存入新采样点
	light_buf[light_idx] = new_val;
	
	//有效样本计数，最多等于窗口大小
	if(sample_cnt < LIGHT_FILTER_WIN)
	{
		sample_cnt++;
	}
	
	//移动索引，环形回卷
	light_idx++;
	if(light_idx >= LIGHT_FILTER_WIN)
	{
		light_idx = 0U;
	}
	
	//只累加已经采集的有效样本，未使用的数组垃圾位置不会参与计算
	for(i=0;i < sample_cnt;i++)
	{
		sum += light_buf[i];
	}
	return (uint16_t)(sum / sample_cnt);
}


/**
 * @brief 读取全部传感器：MPU6050姿态温度 + ADC光照
 * @param hi2c_mpu MPU6050使用的软件I2C句柄指针
 */
void Sensor_ReadAllTask(SoftI2C_TypeDef *hi2c_mpu)
{
	MPU6050_DataDef mpu_raw;
	MPU6050_ReadBurst(hi2c_mpu, &mpu_raw);
	
	// MPU6050芯片内部温度换算，来自datasheet
	// 灵敏度340LSB/℃；25℃时raw=-521，推导得 T = raw/340.0f + 36.53f
	// 注意：此为芯片硅片温度，不等于环境室温
	g_sensor_data.temp = (mpu_raw.temp_raw /340.0f) +36.53f;
	
	g_sensor_data.ax = mpu_raw.ax;
    g_sensor_data.ay = mpu_raw.ay;
    g_sensor_data.az = mpu_raw.az;
    g_sensor_data.gx = mpu_raw.gx;
    g_sensor_data.gy = mpu_raw.gy;
    g_sensor_data.gz = mpu_raw.gz;

    //读取光照ADC原始值 +滑动平均滤波
    uint16_t light_raw = ADC_Get_Sample_Value();
    g_sensor_data.light = Light_Filter(light_raw);
}












