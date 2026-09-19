#ifndef __CONSOLE_CMD_H
#define __CONSOLE_CMD_H

#include <unistd.h>//ssize_t
#include <string.h>
#include <stdio.h>
#include <stddef.h>//size_t
#include <stdint.h>//uint8_t
/**
 * @brief 控制台stdin处理模块
 * @note 职责：按行读取用户键盘输入；剥离\r \n换行；**原始字节透传给串口下发STM32**；
 * ✅本阶段：Linux侧**不解析下发指令语义**；完整@xxx#!帧原样下发；全部指令解析交给STM32下位机三段状态机；
 * @TODO后续扩展：简易命令映射层，输入led_on自动组装@LED:ON#!下发。
 */

/**
 * @brief 从stdin读取一行；剥离末尾 \r \n；
 * @param buf 输出行缓冲区
 * @param buf_size 缓冲区总字节
 * @return >0：读到有效行（已经裁剪换行），返回有效字节长度；
 *          0：stdin读到EOF；
 *         -1：缓冲区满，输入行被截断；
 */
ssize_t console_read_line(uint8_t *buf,size_t buf_size);

#endif 