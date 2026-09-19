#include "delay.h"
#include "misc.h"               // Device header

/**
 * @brief  初始化SysTick，72M系统时钟
 */
void Delay_Init(void)
{
    // SysTick使用HCLK时钟 =72MHz
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
}

/**
 * @brief 微秒级阻塞延时
 * @param nus 要延时多少微秒
 */
void delay_us(uint32_t nus)
{
    uint32_t temp;
    SysTick->LOAD = 72 * nus;
    SysTick->VAL  = 0x00;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    do
    {
        temp = SysTick->CTRL;
    }while((temp & 0x01) && !(temp & (1 << 16)));
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL  = 0x00;
}

/**
 * @brief 毫秒级阻塞延时
 * @param nms 延时多少毫秒
 */
void delay_ms(uint32_t nms)
{
    uint32_t i;
    for(i = 0; i < nms; i++)
    {
        delay_us(1000);
    }
}
