#ifndef __UART_PROTOCOL_H
#define __UART_PROTOCOL_H
#include "sensor_task.h"

extern uint8_t uart_send_frame_flag;

void Uart_SendSensorFrame(SensorDataTypeDef *p_sensor);

#endif







