#ifndef __SENSOR_FRAME_PROC_H
#define __SENSOR_FRAME_PROC_H
#include "app_config.h"
#include <stdint.h>
/**
 * @brief 完整传感器帧业务处理模块
 * 职责：原始载荷KV解析、打印调试信息、获取时间戳、写入CSV日志
 * 运行于主线程上下文
 */

/**
 * @brief 解析原始载荷字符串，完成KV分割，转换为传感器真实数值
 * @param raw_payload parser输出$与#之间原始0结尾载荷字符串
 * @param out_data 输出解析之后完整传感器结构体
 * @return 0全部字段解析成功；‑1解析失败
 */
int parse_sensor_kv_payload(const char *raw_payload,SensorFrame_t *out_data);

/**
 * @brief 一帧完整业务处理入口：KV解析、打印、写csv
 * @param raw_payload parser原始载荷
 * @param csv_fd csv日志文件fd
 * @return 0业务全部成功；‑1发生致命IO错误，上层需要release并退出
 */
int sensor_frame_process(const char *raw_payload,int csv_fd);

#endif
