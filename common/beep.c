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

	RCC_APBPeriphClockCmd(BEEP_RCC, ENABLE);  //ʹ��GPIO�����AFIO���ù���ģ��ʱ��

	//GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE); //Timer3������ӳ��  TIM3_CH2->PB5    

	GPIO_InitStructure.GPIO_Pin = BEEP_GPIO_Pin; //TIM1_CH4
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(BEEP_GPIO, &GPIO_InitStructure);//��ʼ��GPIO

	TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ 
	TIM_TimeBaseStructure.TIM_ClockDivision = 1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(BEEP_TIM, &TIM_TimeBaseStructure); //����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //ѡ��ʱ��ģʽ:TIM�����ȵ���ģʽ2
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //�Ƚ����ʹ��
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //�������:TIM����Ƚϼ��Ը�
	TIM_OCInitStructure.TIM_Pulse = 0x9000;
	TIM_OCInit(BEEP_TIM, &TIM_OCInitStructure);  //����Tָ���Ĳ�����ʼ������TIM1 OC4

	TIM_OCPreloadConfig(BEEP_TIM, TIM_OCPreload_Enable);  //ʹ��TIM1��CCR2�ϵ�Ԥװ�ؼĴ���

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

