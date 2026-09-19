#include "adc.h"
#include "delay.h"

/**
 * @brief  ADC初始化配置，查询模式
 * @note   PA1 设置为模拟输入；ADC1开启，软件触发，12位分辨率
 * @retval none
 */
void ADC_Init_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	ADC_InitTypeDef ADC_InitStruct;
	
	// 1. 开启时钟：GPIOA + ADC1，ADC1挂APB2总线
    RCC_APB2PeriphClockCmd(ADC_GPIO_RCC | ADC_RCC_APB2, ENABLE);
	
	// ADC时钟6分频，72M→12M，F1必须要有
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	
	// 2. GPIO配置为模拟输入模式 GPIO_Mode_AIN
    // 模拟输入：关闭上下拉、关闭施密特触发器，直接读取外部模拟电压
    GPIO_InitStruct.GPIO_Pin = ADC_GPIO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(ADC_GPIO_PORT, &GPIO_InitStruct);
	
	// 3. ADC基础配置
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;        // 独立模式，多ADC才用双模式
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;              // 扫描模式关闭：单通道，不需要扫描
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;        // 关闭连续转换：每次需要软件手动触发一次转换
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 软件触发，不用外部定时器触发
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;     // 数据右对齐：低12位有效，高位补0，我们直接读值即可
    ADC_InitStruct.ADC_NbrOfChannel = 1;                    // 转换通道数量：1个通道

    ADC_Init(ADCx, &ADC_InitStruct);
	
	// 4. 设置通道、采样时间
    // ADC1_IN1 通道1，采样周期239.5周期，采样时间更长，抗干扰更好，适合传感器
    ADC_RegularChannelConfig(ADCx, ADC_Channel_1, 1, ADC_SampleTime_239Cycles5);

    // 5. 开启ADC模块
    ADC_Cmd(ADCx, ENABLE);
	
	delay_ms(1);
	
	// 6. ADC校准！F1系列必须做校准，提升采样精度，不能省略
    ADC_ResetCalibration(ADCx);                     // 复位校准
    while(ADC_GetResetCalibrationStatus(ADCx));     // 等待复位校准完成，完成返回reset
    ADC_StartCalibration(ADCx);                     // 启动校准
    while(ADC_GetCalibrationStatus(ADCx));          // 等待校准结束
}

/**
 * @brief  获取一次ADC采样原始数值 0~4095
 * @note   软件触发转换，等待转换结束，返回DR寄存器的值
 * @retval uint16_t 原始采样值 0‑4095
 */
uint16_t ADC_Get_Sample_Value(void)
{
	// 软件触发，开启一次转换
    ADC_SoftwareStartConvCmd(ADCx, ENABLE);
	
	// 等待转换结束 EOC标志位
    while(ADC_GetFlagStatus(ADCx, ADC_FLAG_EOC) == RESET);
	
	// 读取转换结果，读DR寄存器会自动清除EOC标志位
    return ADC_GetConversionValue(ADCx);
}







