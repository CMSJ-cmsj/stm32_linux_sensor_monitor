#include "uart_protocol.h"
#include "usart.h"

uint8_t uart_send_frame_flag = 0;

/**
 * @brief 自定义串口协议组包上报，#作为帧结束标记
 * @note 协议V3 STM32→Linux传感器上报帧
 * 完整报文：$T:25.1 L:1024 AX:120 AY:211 AZ:1630 GX:-11 GY:22 GZ:-33#\r\n
 * 帧头：$ ; 帧结束主标记：#；\r\n仅辅助串口助手换行，Linux解析器会丢弃
 * 【重要区分】
 * 1) Linux→STM32下发控制帧：@payload#! （@帧头，#!双字符帧尾，STM32三段状态机解析）
 * 2) STM32→Linux上报传感器帧：$payload#\r\n（$帧头，#单字符帧尾，Linux独立三段状态机解析）
 * 两个方向协议完全独立，不可混用！
 * @param p_sensor 传感器结构体指针
 */
void Uart_SendSensorFrame(SensorDataTypeDef *p_sensor)
{
    Serial_Printf("$T:%.1f L:%d AX:%d AY:%d AZ:%d GX:%d GY:%d GZ:%d#\r\n",
        p_sensor->temp,
        p_sensor->light,
        p_sensor->ax,
        p_sensor->ay,
        p_sensor->az,
        p_sensor->gx,
        p_sensor->gy,
        p_sensor->gz
    );
}















