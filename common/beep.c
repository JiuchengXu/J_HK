#include "includes.h"
#include "helper.h"

#if defined(CLOTHE) || defined(GUN)
#define BEEP_GPIO		GPIOA
#define BEEP_GPIO_Pin	GPIO_Pin_11
#define BEEP_TIM		TIM1
#define BEEP_RCC		(RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO)
#define RCC_APBPeriphClockCmd	RCC_APB2PeriphClockCmd

#define TIM_OCInit		TIM_OC4Init
#define TIM_OCPreloadConfig	TIM_OC4PreloadConfig
#endif

#if defined(LCD)
#define BEEP_GPIO		GPIOA
#define BEEP_GPIO_Pin	GPIO_Pin_1
#define BEEP_TIM		TIM5
#define BEEP_RCC		(RCC_APB1Periph_TIM5 | RCC_APB2Periph_GPIOA)
#define RCC_APBPeriphClockCmd	RCC_APB1PeriphClockCmd

#define TIM_OCInit		TIM_OC2Init
#define TIM_OCPreloadConfig	TIM_OC2PreloadConfig
#endif

static void TIM_PWM_Init(u16 arr,u16 psc)
{  
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	RCC_APBPeriphClockCmd(BEEP_RCC, ENABLE);  //使能GPIO外设和AFIO复用功能模块时钟

	//GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE); //Timer3部分重映射  TIM3_CH2->PB5    

	GPIO_InitStructure.GPIO_Pin = BEEP_GPIO_Pin; //TIM1_CH4
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(BEEP_GPIO, &GPIO_InitStructure);//初始化GPIO

	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值 
	TIM_TimeBaseStructure.TIM_ClockDivision = 1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(BEEP_TIM, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //选择定时器模式:TIM脉冲宽度调制模式2
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性:TIM输出比较极性高
	TIM_OCInitStructure.TIM_Pulse = 0x9000;
	TIM_OCInit(BEEP_TIM, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM1 OC4

	TIM_OCPreloadConfig(BEEP_TIM, TIM_OCPreload_Enable);  //使能TIM1在CCR2上的预装载寄存器

	TIM_Cmd(BEEP_TIM, DISABLE); 

	TIM_CtrlPWMOutputs(BEEP_TIM, DISABLE);

}

void beep_on(void)
{  
	TIM_CtrlPWMOutputs(BEEP_TIM, ENABLE);
	TIM_Cmd(BEEP_TIM, ENABLE);
}

void beep_off(void)
{
	TIM_Cmd(BEEP_TIM, DISABLE);  
	TIM_CtrlPWMOutputs(BEEP_TIM, DISABLE);
}

void beep_init(void)
{
	TIM_PWM_Init(0xa000,1000);
}

void ok_notice(void)
{
	beep_on();
	sleep(1);
	beep_off();
}

