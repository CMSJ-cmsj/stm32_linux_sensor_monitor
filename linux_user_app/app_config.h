#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include <stdint.h>
#include <sys/poll.h>

// 默认串口设备
// 波特率：和STM32下位机保持9600
// poll主循环超时，单位ms；50ms
#define APP_DEFAULT_UART_DEV "/dev/ttyUSB0"
#define APP_UART_BAUDRATE B9600
#define APP_POLL_TIMEOUT_MS 50

// 环形底层字节缓冲区大小（ring_buf），硬件字节蓄水池
// 解析器内部组帧工作缓冲区：解析$...#上报帧，最大载荷128字节
#define RING_BUF_SIZE 256U
#define PARSE_WORK_BUF_LEN 128U

// CSV日志文件名
#define APP_CSV_LOG_FILE "./sensor_log.csv"

// 控制台stdin行缓冲区：下发指令最大长度
#define CONSOLE_LINE_BUF_SIZE 128U

/* ========== 传感器上报帧解析状态定义（STM32->Linux $T:xxx#） ========== */
typedef enum
{
    SENSOR_ST_IDLE = 0U,          // 空闲，等待$帧头
    SENSOR_ST_RECV_PAYLOAD,       // 接收载荷，收集$之后#之前字节
    SENSOR_ST_WAIT_TRAIL          // 收到#，本帧结束；丢弃\r\n辅助换行字符
}SensorParseState_t;

/* ========== 解析器输出：解析完成后的传感器帧副本结构体 ========== */
typedef struct
{
    char raw_payload[128];  //存放完整原始载荷字符串
    float temp;
    uint16_t light;
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
    uint8_t valid;  //1=本结构体数据有效；0=无效
}SensorFrame_t;

/* ========== 全局外部声明 ========== */
extern int g_uart_fd;
extern int g_csv_fd;

#endif

