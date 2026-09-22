#include "sensor_frame_parser.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ========= parser模块内部私有静态变量，流控收敛在parser内部 ========= */
static SensorParseState_t  s_parse_state = SENSOR_ST_IDLE;
static uint8_t  s_parse_work_buf[PARSE_WORK_BUF_LEN]; //组帧工作缓存128字节
static size_t   s_parse_work_len = 0U;                //当前载荷有效字节计数
static uint8_t  s_frame_busy = 0U;                    //核心忙标记；1=已经产出完整帧，业务尚未release
static uint8_t  s_busy_drop_warn_printed = 0U;        //busy丢弃字节仅打印一次告警
static uint8_t  s_busy_recover_notice_print = 0U;

/**
 * @brief 内部：重置组帧工作缓冲区；**只有收到$帧头才调用memset清空**；收到#帧结束不清空，等待下一帧$
 */
static void parser_reset_work_buffer(void)
{
    memset(s_parse_work_buf,0x00,sizeof(s_parse_work_buf));
    s_parse_work_len = 0U;
}

/**
 * @brief 内部：将当前组帧缓存拷贝到对外输出SensorFrame_t副本；只拷贝原始载荷字符串，不做KV解析；KV解析交给main业务层
 */
 static void parser_copy_payload_to_output(SensorFrame_t *out_frame,const uint8_t *payload_buf,size_t payload_len)
{
    memset(out_frame,0x00,sizeof(SensorFrame_t));

    // 最大允许有效字符数，预留1字节给字符串结束符'\0'
    const size_t max_valid = sizeof(out_frame->raw_payload) - 1U;
    size_t copy_len;

    if(payload_len <= max_valid)
    {
        copy_len = payload_len;
    }
    else
    {
        //报文超长，截断
        copy_len = max_valid;
    }

    memcpy(out_frame->raw_payload, payload_buf, copy_len);
    out_frame->raw_payload[copy_len] = '\0';

    out_frame->valid = 1U;
}

 int sensor_parser_input_byte(uint8_t byte,SensorFrame_t *out_frame)
 {
    if(out_frame == NULL)
    {
        return -1;
    }
    out_frame->valid = 0U;
    /* ==========第一层防护：frame_busy为true，全部输入字节直接丢弃；仅首次打印告警 ========== */
    if(s_frame_busy != 0U)
    {
        if(s_busy_drop_warn_printed == 0U)
        {
            fprintf(stderr,"[WARN] parser frame_busy is true, drop incoming bytes, waiting for release()\n");
            s_busy_drop_warn_printed = 1U;
        }
        return 0; //丢弃字节，返回0：没有产生新帧
    }

    /* ========== 正常状态机流转 ========== */
    switch(s_parse_state)
    {
        case SENSOR_ST_IDLE:
            if(byte == '$')
            {
                // 识别上报帧帧头$；此时才清空组帧工作缓存
                parser_reset_work_buffer();
                s_parse_state = SENSOR_ST_RECV_PAYLOAD;
            }
            // 其他噪声字节：IDLE直接全部丢弃，不处理
            break;
        case SENSOR_ST_RECV_PAYLOAD:
            if(byte == '#')
            {
                // 识别主帧结束标记#；本帧载荷收集完毕
                // 把s_parse_work_buf拷贝到外部独立副本out_frame
                parser_copy_payload_to_output(out_frame,s_parse_work_buf,s_parse_work_len);
                // A‑1：收到完整帧，置忙标记true；**此时不清空work_buf，等待下一次$帧头再重置**
                s_frame_busy = 1U;
                s_busy_drop_warn_printed = 0U;
                s_parse_state = SENSOR_ST_WAIT_TRAIL;
                return 1;
            }
            else
            {
                if(s_parse_work_len >= PARSE_WORK_BUF_LEN)
                {
                    // 恶意无限半帧：$开头一直发载荷，永不发#；触发协议层防护
                    fprintf(stderr,"[WARN] parser half‑frame overflow(%u bytes), drop half‑frame, back to IDLE\n",PARSE_WORK_BUF_LEN);
                    parser_reset_work_buffer();
                    s_parse_state = SENSOR_ST_IDLE;
                    return -1;//异常半帧丢弃
                }
                s_parse_work_buf[s_parse_work_len] = byte;
                s_parse_work_len++;
            }
            break;
        case SENSOR_ST_WAIT_TRAIL:
            // 已经收到#；仅丢弃辅助 \r \n；其余异常噪声直接丢弃；等待下一个$帧头回到IDLE
            if(byte == '\r' || byte == '\n')
            {
                //合法辅助换行，静默丢弃
            }
            else if(byte == '$')
            {
                parser_reset_work_buffer();
                s_parse_state = SENSOR_ST_RECV_PAYLOAD;
            }
            else
            {
                //其他杂字节全部丢弃，等待新$
            }
            break;
        default:
            //异常状态兜底，强制回到IDLE
            s_parse_state = SENSOR_ST_IDLE;
            parser_reset_work_buffer();
            break;
    }
    return 0;
 }

 void sensor_parser_release(void)
 {
    if(s_frame_busy == 0U)
    {
        // 重复release，不做动作；防止业务多调用release，无害
        return ;
    }
    s_frame_busy = 0U;
    // busy状态结束，打印恢复提示
    fprintf(stderr,"[INFO] parser release busy flag, recover to accept new frame.\n");
    s_busy_recover_notice_print = 1U;
 }

 SensorParseState_t sensor_parser_get_state(void)
 {
    return s_parse_state;
 }