#include "alarm.h"
#include "beep.h"
#include "led.h"

// 全局报警参数
float g_alarm_high_thr;
float g_alarm_low_thr;
uint8_t g_alarm_enable;

static uint8_t alarm_state = 0; //0正常，1报警

/**
 * @brief 报警外设初始化，BEEP、LED引脚在这里统一配置
 * @note 使用默认宏给阈值变量上电赋值
 */
void Alarm_Init(void)
{
    BEEP_Init(BEEP_GPIO_PORT, BEEP_GPIO_PIN);
    LED_Init(LED_GPIO_PORT, LED_GPIO_PIN);
    BEEP_Off(BEEP_GPIO_PORT, BEEP_GPIO_PIN);
    LED_Off(LED_GPIO_PORT, LED_GPIO_PIN);
	
	//上电加载默认阈值
	g_alarm_high_thr = TEMP_HIGH_DEFAULT;
	g_alarm_low_thr = TEMP_LOW_DEFAULT;
	//默认报警功能打开
	g_alarm_enable = 1U;
	alarm_state = 0U;
}

/**
 * @brief 阈值报警业务处理，带状态防抖
 * @param temp_val 传入温度浮点数值
 * @note 如果g_alarm_enable=0，直接关闭声光，不判断阈值
 */
void Alarm_Process(float temp_val)
{
    uint8_t need_alarm = 0;

    // 如果报警总开关关闭，直接清除报警
    if(g_alarm_enable == 0U)
    {
        need_alarm = 0;
    }
    else
    {
        //温度超出上下限则需要报警
        if(temp_val > g_alarm_high_thr || temp_val < g_alarm_low_thr)
        {
            need_alarm = 1;
        }
        else
        {
            need_alarm = 0;
        }
    }

    //只有状态切换才操作外设，避免阈值附近抖动反复开关
    if(need_alarm != alarm_state)
    {
        alarm_state = need_alarm;
        if(alarm_state == 1)
        {
            BEEP_On(BEEP_GPIO_PORT, BEEP_GPIO_PIN);
            LED_On(LED_GPIO_PORT, LED_GPIO_PIN);
        }
        else
        {
            BEEP_Off(BEEP_GPIO_PORT, BEEP_GPIO_PIN);
            LED_Off(LED_GPIO_PORT, LED_GPIO_PIN);
        }
    }
}

