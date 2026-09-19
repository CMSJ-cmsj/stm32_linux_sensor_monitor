#ifndef __RING_BUF_H
#define __RING_BUF_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief 纯字节环形FIFO缓冲区，无帧概念；硬件串口read原始字节蓄水池
 * @note 大小RING_BUF_SIZE=256；写满时丢弃新来字节，打印一次性严重告警；
 * 仅做字节缓存，完全不感知$ #协议；属于底层第二层防护。
 */

// 环形缓冲区句柄
typedef struct
{
    uint8_t *buf;
    size_t  head;  // 写指针：下一个写入位置
    size_t  tail;  // 读指针：下一个读取位置
    size_t  size;
    size_t count;  // 避免牺牲字节来区分满和空
    uint8_t overflow_warn_flag; // 溢出告警只打印一次，防止刷屏
} RingBuf_t;

/**
 * @brief 初始化环形缓冲区
 * @param rb 句柄指针
 * @param buf 外部提供数组缓冲区
 * @param buf_size 数组字节数
 */
void RingBuf_Init(RingBuf_t *rb,uint8_t *buf,size_t buf_size);

/**
 * @brief 写入1字节到环形缓冲区
 * @return 0：写入成功；1：缓冲区满，字节被丢弃（兜底溢出）
 */
int RingBuf_WriteByte(RingBuf_t *rb,uint8_t ch);

/**
 * @brief 读取1字节；
 * @param ch 输出字节指针
 * @return 0：读取成功；1：缓冲区为空无数据
 */
int RingBuf_ReadByte(RingBuf_t *rb,uint8_t *ch);

/**
 * @brief 获取当前缓冲区有效字节数量
 */
size_t RingBuf_GetCount(RingBuf_t *rb);

/**
 * @brief 清空整个环形缓冲区（紧急复位使用）
 */
void RingBuf_Clear(RingBuf_t *rb);

#endif