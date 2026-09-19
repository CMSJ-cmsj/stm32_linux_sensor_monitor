#include "tim_pwm.h"

/* =================================================================
 * SG90_Init : 初始化TIM3通道1 PWM输出，驱动SG90舵机
 * 步骤: 开时钟 -> 配GPIO(复用推挽) -> 配定时器时基 -> 配PWM通道 -> 使能
 * ================================================================= */
void SG90_Init(void)
{
	/* 1. 开启时钟
       - 定时器TIM3在 APB1 总线 -> 用 RCC_APB1PeriphClockCmd
       - 引脚PA6在 GPIOA(APB2) -> 用 RCC_APB2PeriphClockCmd
       - 注意：GPIO复用功能(重映射)只需开启AFIO时钟，普通复用推挽不需要AFIO */
    RCC_APB1PeriphClockCmd(SG90_TIM_RCC, ENABLE);
    RCC_APB2PeriphClockCmd(SG90_GPIO_RCC, ENABLE);
	
	/* 2. 配置 PA6 为 复用推挽输出 (AF_PP)
       因为PA6要作为TIM3_CH1的PWM输出脚，必须复用，不能用普通推挽 */
	GPIO_InitTypeDef      GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin   = SG90_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;   // 复用推挽
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SG90_GPIO, &GPIO_InitStructure);
	
	/* 3. 配置定时器时基单元
       PSC=719 -> 计数频率100kHz
       ARR=1999 -> 计数溢出周期20ms(=50Hz) */
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period        = SG90_ARR;             // 自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler     = SG90_PSC;             // 预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;         // 时钟分割，本例不用
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;   // 向上计数
    TIM_TimeBaseInit(SG90_TIM, &TIM_TimeBaseStructure);	
	
	/* 4. 配置输出比较通道1(PWM模式)
       - PWM模式1: 计数 < CCR时输出有效(高电平)，>=CCR时无效(低电平)
       - 通过改变CCR值即可改变占空比，从而改变舵机角度 */
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;          // PWM模式1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;   // 使能通道1输出
    TIM_OCInitStructure.TIM_Pulse       = SG90_CCR_0DEG;            // 初始CCR=50 -> 舵机0度
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;      // 高电平有效
    TIM_OC1Init(SG90_TIM, &TIM_OCInitStructure);

    /* 5. 使能定时器TIM3开始计数 */
    TIM_Cmd(SG90_TIM, ENABLE);
}

/* =================================================================
 * SG90_SetAngle : 设置舵机角度(0~180)
 * 原理: 高电平脉宽 0.5ms(0度) ~ 2.5ms(180度)
 *       CCR = 高电平时间 / 10us
 *       CCR = 50 + angle * 200/180
 * ================================================================= */
void SG90_SetAngle(uint8_t angle)
{
    uint16_t ccr;

    if (angle > 180)          // 角度限幅，保护舵机
        angle = 180;

    /* 角度换算成CCR: 0度=50, 180度=250 */
    ccr = SG90_CCR_0DEG + (uint16_t)((uint16_t)angle * (SG90_CCR_180DEG - SG90_CCR_0DEG) / 180);

    /* 写通道1的比较寄存器 */
    TIM_SetCompare1(SG90_TIM, ccr);
}










