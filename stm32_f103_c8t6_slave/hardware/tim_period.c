#include "tim_period.h"

/**
 * @brief 周期中断标志位
 *  0 = 没有到采集时间
 *  1 = 定时时间到达，需要执行一次采集任务
 * @note 【重点】中断里面只把它置1；清零操作放在主循环！不要在中断清零！
 * volatile：告诉编译器该变量会被硬件中断修改，不要做寄存器缓存优化
 */
uint8_t volatile tim_period_flag = 0;

/**
 * @brief TIM2 周期中断初始化
 * @param ms 中断周期，单位ms，允许范围1 ~ 6553
 *
 * 系统SYSCLK=72MHz；标准库配置APB1预分频=2
 * APB1总线时钟 = HCLK/2 = 36MHz
 * STM32F1特殊规则：APB1预分频≠1时，APB1定时器时钟自动×2
 * 所以TIM2定时器内核时钟 = 36MHz * 2 = 72MHz
 *
 * PSC =7199，实际分频系数 = PSC+1 =7200
 * f_cnt = 72M / 7200 = 10000 Hz
 * 每1个tick = 1 / 10000 = 100 μs = 0.1 ms
 *
 * 总计数tick数量 = ms * 10U
 * ARR寄存器写入值 = 总tick数量 - 1
 *
 * 16位ARR最大65535，最大定时 65536 * 0.1ms = 6553.6ms（约6.55秒）
 */
void TIM2_Period_Init(uint16_t ms)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStruct;
	NVIC_InitTypeDef NVIC_InitStruct;

	const uint16_t psc_val = 7199;	//实际分频7200，tick=100us(0.1ms)
	uint32_t total_tick;

	// 计算需要多少个tick：1ms需要10个tick
	total_tick = (uint32_t)ms * 10U;

	//开启TIM2时钟，TIM2挂载在APB1总线上
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	//时基单元配置
	TIM_TimeBaseStruct.TIM_Prescaler = psc_val;          //预分频PSC
	TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up; //向上计数
	TIM_TimeBaseStruct.TIM_Period = total_tick - 1;         //ARR 自动重装载
	TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStruct.TIM_RepetitionCounter = 0;         //F1产品该位无效，写0

	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStruct);

	//使能TIM2更新中断（溢出中断）
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	
	// ========= NVIC中断优先级配置 =========
	NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;  //抢占优先级
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 2;          //子优先级
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);

	//TIM2使能计数器，定时器开始跑
	TIM_Cmd(TIM2, ENABLE);
}

/**
 * @brief TIM2全局中断服务函数
 * @note 启动文件固定函数名，名字写错中断不会进入！
 * 【核心原则】中断内部禁止任何耗时外设操作，只置标志位
 */
void TIM2_IRQHandler(void)
{
	//判断是否是更新(溢出)中断
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
	{
		tim_period_flag = 1;			//仅仅置采集标志，不做别的！
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);	//清除中断标志，必不可少，否则一直进中断
	}
}



















