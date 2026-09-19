#include "uart_cmd_parse.h"
#include "alarm.h"
#include "sensor_task.h"
#include "led.h"
#include "beep.h"
#include "tim_pwm.h"
#include "uart_protocol.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/**
 * =================================================================================
 * 【本工程串口完整业务指令集】
 * 通信协议版本：V3
 * 下发单片机报文格式：  @{业务载荷}#!
 *      帧头：@   (0x40)
 *      帧尾：#!  (0x23 0x21)  双可见字符，#在前 !在后；状态机会自动剥离#!，不传入Cmd_Dispatch
 *      注意：业务载荷内部严禁连续出现 "#!"，会被误识别帧结束
 *      ⚠️区分：单片机向上位机应答报文末尾的'#'仅仅是应答结束标记，不是接收帧尾#!
 *
 * ------------------------全部合法完整一帧【文本流 + HEX字节流】------------------------
 *
 * 1、LED打开
 *   文本报文： @LED:ON#!
 *   HEX字节： 40 4C 45 44 3A 4F 4E 23 21
 *
 * 2、LED关闭
 *   文本报文： @LED:OFF#!
 *   HEX字节： 40 4C 45 44 3A 4F 46 46 23 21
 *
 * 3、蜂鸣器打开
 *   文本报文： @BEEP:ON#!
 *   HEX字节： 40 42 45 45 50 3A 4F 4E 23 21
 *
 * 4、蜂鸣器关闭
 *   文本报文： @BEEP:OFF#!
 *   HEX字节： 40 42 45 45 50 3A 4F 46 46 23 21
 *
 * 5、舵机SG90设置角度（示例角度90，可修改0~180）
 *   文本报文： @SG90:90#!
 *   HEX字节： 40 53 47 39 30 3A 39 30 23 21
 *
 * 6、读取传感器数据
 *   文本报文： @GET_DATA#!
 *   HEX字节： 40 47 45 54 5F 44 41 54 41 23 21
 *
 * 7、设置报警高阈值（示例30.5，范围10.0 ~ 80.0）
 *   文本报文： @SET_H:30.5#!
 *   HEX字节： 40 53 45 54 5F 48 3A 33 30 2E 35 23 21
 *
 * 8、设置报警低阈值（示例20.0，范围10.0 ~ 80.0）
 *   文本报文： @SET_L:20.0#!
 *   HEX字节： 40 53 45 54 5F 4C 3A 32 30 2E 30 23 21
 *
 * 9、开启报警功能
 *   文本报文： @ALARM:ON#!
 *   HEX字节： 40 41 4C 41 52 4D 3A 4F 4E 23 21
 *
 * 10、关闭报警功能
 *    文本报文： @ALARM:OFF#!
 *    HEX字节： 40 41 4C 41 52 4D 3A 4F 46 46 23 21
 *
 * ------------------------调试测试帧 ------------------------
 * 【空合法帧】载荷为空，返回UNKNOWN CMD
 *    文本报文： @#!
 *    HEX字节： 40 23 21
 *
 * 【非法帧示例：#后面不是!，触发帧丢弃告警】
 *    文本报文： @ABC#X
 *    HEX字节： 40 41 42 43 23 58
 *    现象：无业务应答，串口打印 [WARN] Frame discard: # followed by non-!
 *
 * ------------------------单片机应答报文样例（向上位机输出）------------------------
 * 例：LED打开应答： Resp:LED ON OK#\r\n
 * 注意：应答末尾#只是应答结束标记，**不是接收帧尾#!，上位机不要拿应答#当做帧尾解析**
 *
 * ------------------------协议历史变更记录 ------------------------
 * V1：帧尾\r\n；坑：串口助手文本模式无法可靠输出控制字符
 * V2：帧尾~#；坑：~波浪号输入法/复制粘贴容易字节异常转换
 * V3【当前】：帧尾#!；#在前!在后；#后面非!直接丢弃整帧，主循环输出业务告警
 *      约束：不做半帧软件超时复位；后续可增加看门狗处理
 *      约束：单帧缓存；上一帧未处理完毕，新@帧头直接丢弃；适合人手逐条间隔发送
 *
 * 业务版本：全部调试打印、调试脚手架已移除；异常排查历史记录存放在log_record/DebugRecord.md
 * =================================================================================
 */

typedef void (*CmdHandlerFunc_t)(char *frame);

typedef struct
{
    const char        *cmd_prefix;
    CmdHandlerFunc_t   handler;
} CmdTable_t;

static void Cmd_LED_ON(char *frame)
{
    (void)frame;
    LED_On(LED_GPIO_PORT, LED_GPIO_PIN);
    Serial_Printf("$Resp:LED ON OK#\r\n");
}
static void Cmd_LED_OFF(char *frame)
{
    (void)frame;
    LED_Off(LED_GPIO_PORT, LED_GPIO_PIN);
    Serial_Printf("$Resp:LED OFF OK#\r\n");
}
static void Cmd_BEEP_ON(char *frame)
{
    (void)frame;
    BEEP_On(BEEP_GPIO_PORT, BEEP_GPIO_PIN);
    Serial_Printf("$Resp:BEEP ON OK#\r\n");
}
static void Cmd_BEEP_OFF(char *frame)
{
    (void)frame;
    BEEP_Off(BEEP_GPIO_PORT, BEEP_GPIO_PIN);
    Serial_Printf("$Resp:BEEP OFF OK#\r\n");
}
static void Cmd_SG90_Set(char *frame)
{
    uint8_t angle = 0U;
    int ret = sscanf(frame, "SG90:%hhu", &angle);
    if(ret != 1)
    {
        Serial_Printf("$Resp:SG90 param err#\r\n");
        return;
    }
    if(angle > 180U)
    {
        Serial_Printf("$Resp:SG90 range err(0~180)#\r\n");
        return;
    }
    SG90_SetAngle(angle);
    Serial_Printf("$Resp:SG90 set %d OK#\r\n", angle);
}
static void Cmd_GET_DATA(char *frame)
{
    (void)frame;
    Uart_SendSensorFrame(&g_sensor_data);
}
static void Cmd_SET_H(char *frame)
{
    float valf = 0.0f;
    int ret = sscanf(frame, "SET_H:%f", &valf);
    if(ret != 1)
    {
        Serial_Printf("$Resp:SET_H param err#\r\n");
        return;
    }
    if(valf < 10.0f || valf > 80.0f)
    {
        Serial_Printf("$Resp:SET_H range err(10.0~80.0)#\r\n");
        return;
    }
    g_alarm_high_thr = valf;
    Serial_Printf("$Resp:SET_H=%.1f OK#\r\n", g_alarm_high_thr);
}
static void Cmd_SET_L(char *frame)
{
    float valf = 0.0f;
    int ret = sscanf(frame, "SET_L:%f", &valf);
    if(ret != 1)
    {
        Serial_Printf("$Resp:SET_L param err#\r\n");
        return;
    }
    if(valf < 10.0f || valf > 80.0f)
    {
        Serial_Printf("$Resp:SET_L range err(10.0~80.0)#\r\n");
        return;
    }
    g_alarm_low_thr = valf;
    Serial_Printf("$Resp:SET_L=%.1f OK#\r\n", g_alarm_low_thr);
}
static void Cmd_ALARM_ON(char *frame)
{
    (void)frame;
    g_alarm_enable = 1U;
    Serial_Printf("$Resp:ALARM ENABLE OK#\r\n");
}
static void Cmd_ALARM_OFF(char *frame)
{
    (void)frame;
    g_alarm_enable = 0U;
    Serial_Printf("$Resp:ALARM DISABLE OK#\r\n");
}


static const CmdTable_t cmd_table[] =
{
    {"LED:ON",      Cmd_LED_ON},
    {"LED:OFF",     Cmd_LED_OFF},
    {"BEEP:ON",     Cmd_BEEP_ON},
    {"BEEP:OFF",    Cmd_BEEP_OFF},
    {"SG90:",       Cmd_SG90_Set},
    {"GET_DATA",    Cmd_GET_DATA},
    {"SET_H:",      Cmd_SET_H},
    {"SET_L:",      Cmd_SET_L},
    {"ALARM:ON",    Cmd_ALARM_ON},
    {"ALARM:OFF",   Cmd_ALARM_OFF},
    {NULL,          NULL}
};

static void Cmd_Dispatch(char *frame)
{
    for(uint16_t i = 0; cmd_table[i].cmd_prefix != NULL; i++)
    {
        const char *prefix = cmd_table[i].cmd_prefix;
        if(strncmp(frame, prefix, strlen(prefix)) == 0)
        {
            cmd_table[i].handler(frame);
            return;
        }
    }
    Serial_Printf("Resp:UNKNOWN CMD#\r\n");
}

void UartCmdParse_Init(void)
{
}

void UartCmdParse_Run(void)
{
    //业务告警：#后面不是!帧丢弃，业务版本保留
    if(uart_frame_discard_warn == 1U)
    {
        Serial_Printf("[WARN] Frame discard: # followed by non-!\r\n");
        uart_frame_discard_warn = 0U;
    }

    if(Serial_GetRxFlag() == 1U)   //只读标志，不修改
    {
        Cmd_Dispatch(Serial_RxPacket);
        Serial_RxFlag = 0U;        //业务全部处理完成后主循环手动清零
    }
}
