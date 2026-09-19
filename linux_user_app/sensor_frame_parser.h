#ifndef __SENSOR_FRAME_PARSER_H
#define __SENSOR_FRAME_PARSER_H

#include "app_config.h"
#include <stdint.h>

/**
 * @brief STM32上报传感器帧解析器：独立三段状态机，专门解析 $T:xxx#\r\n
 * @details 架构：parser内部自带frame_busy忙标记；
 *  1) 收到完整帧#，拷贝载荷生成独立SensorFrame_t副本返回；
 *  2) frame_busy置true；此时所有输入字节全部丢弃，不修改内部组帧缓存；
 *  3) 业务处理完毕，必须调用sensor_parser_release()把busy=false，才允许解析下一帧；
 *  4) 内置恶意半帧防护：parse_work_buf=128字节；RECV_PAYLOAD状态达到上限未遇见#，告警，丢弃半帧，切IDLE；ring_buf字节保留。
 * @warning 高危契约，必须遵守
 * 如果sensor_parser_input_byte返回1（out_frame有效），**无论业务正常、字段解析失败、CSV IO失败，全部代码路径必须调用sensor_parser_release()**。
 * 漏调用 → frame_busy永久true，解析器永久丢弃全部上报字节，完全失效。
 * @TODO 后续扩展：业务规模上涨，可以评估goto统一出口，从语法层面强制release调用；本项目L1不使用goto。
 */

/**
 * @brief 向解析器喂入单个字节；来自ring_buf单字节取出
 * @param byte 原始串口字节
 * @param out_frame 输出参数；仅返回值==1时，该结构体为有效帧副本
 * @return  0：未完成一帧；
 *          1：完整帧解析完成，out_frame存有独立副本；
 *         -1：发生异常：恶意超长半帧，已经告警丢弃半帧；
 */
int sensor_parser_input_byte(uint8_t byte,SensorFrame_t *out_frame);

/**
 * @brief 【契约强制调用】当input_byte返回1，业务处理全部完成（解析、写CSV、打印）之后调用；
 * 释放frame_busy忙标记；parser恢复正常接收下一帧字节。
 * @note   release执行时，如果之前busy丢弃过字节，打印"parser recover from busy state"恢复提示。
 */
void sensor_parser_release(void);

SensorParseState_t sensor_parser_get_state(void);

#endif