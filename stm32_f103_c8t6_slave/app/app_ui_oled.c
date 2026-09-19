#include "app_ui_oled.h"
#include "oled.h"
#include "sensor_task.h"
#include "alarm.h"
#include <stdio.h>

/**
 * @brief OLED业务界面刷新
 * @param hi2c oled使用的软件i2c实例
 * @note 跟随采集周期调用，不要高频刷屏
 */
void App_Oled_Update(SoftI2C_TypeDef *hi2c)
{
    char buf[32];

    //第1行：温度
    sprintf(buf,"T:%.1f C",g_sensor_data.temp);
    OLED_ShowString(hi2c,1,1,buf);

    //第2行：光照
    sprintf(buf,"L:%d",g_sensor_data.light);
    OLED_ShowString(hi2c,2,1,buf);

    //第3行：高低温阈值
    sprintf(buf,"H:%.1f L:%.1f",g_alarm_high_thr,g_alarm_low_thr);
    OLED_ShowString(hi2c,3,1,buf);

    //第4行：报警开关状态
    if(g_alarm_enable)
    {
        OLED_ShowString(hi2c,4,1,"ALARM:ON ");
    }
    else
    {
        OLED_ShowString(hi2c,4,1,"ALARM:OFF");
    }
}

















