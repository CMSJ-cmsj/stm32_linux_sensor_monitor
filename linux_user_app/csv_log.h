#ifndef __CSV_LOG_H
#define __CSV_LOG_H

#include "app_config.h"
#include <stdint.h>
#include <unistd.h>

/**
 * @brief CSV日志模块：**全部使用POSIX系统调用open/write/lseek/fsync/close
 * 业务规则：
 * 1) open打开日志文件，O_WRONLY | O_APPEND | O_CREAT；
 * 2) 文件为空（lseek到末尾==0），自动写入csv表头；
 * 3) 每解析到一帧传感器数据，组装一行csv文本；write写入；**执行fsync强制刷盘，降低异常断电丢日志**；
 * 4) 策略：
 *    - open打开日志文件失败：打印stderr，释放全部资源，程序直接exit(EXIT_FAILURE)；
 *    - 运行中write/fsync返回‑1磁盘IO致命错误：**先调用sensor_parser_release()遵守parser契约，再释放全部资源，进程退出**；
 * @warning 【红线】哪怕日志IO发生致命错误，exit之前必须先release；避免单元测试/代码路径违反parser契约；虽然进程销毁内存，代码路径必须合规。
 */

/**
 * @brief 打开csv日志文件；空文件自动写入表头；
 * @param file_path 日志文件路径
 * @return >=0成功返回fd；-1打开失败；失败内部会打印stderr，程序会exit
 */
int csv_log_open(const char *file_path);

/**
 * @brief 写入一帧传感器数据到csv；内部组装字符串，write + fsync；
 * @param fd csv文件fd
 * @param frame 已经解析完成的传感器帧副本
 * @return 0写入成功；-1发生IO致命错误；返回‑1调用方必须执行release，再释放资源退出程序
 */
int csv_log_write_frame(int fd,const SensorFrame_t *frame,double timestamp);

/**
 * @brief 关闭csv文件fd
 */
void csv_log_close(int fd);

#endif