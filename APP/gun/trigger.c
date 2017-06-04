#include "includes.h"
#include "priority.h"
#include "helper.h"
#include "net.h"
#include "bulet.h"
#include "wav.h"

#ifdef  GUN

#define GUN_MODE_OFF	0
#define GUN_MODE_AUTO	1
#define GUN_MODE_SINGLE	2
#define GUN_MODE_OTHER	3


#define GUN_MOTOR_GPIO		GPIOB
#define GUN_MOTOR_GPIO_PIN	GPIO_Pin_6

#define GUN_MODE1_GPIO		GPIOD
#define GUN_MODE1_GPIO_PIN	GPIO_Pin_2

#define GUN_MODE2_GPIO		GPIOC
#define GUN_MODE2_GPIO_PIN	GPIO_Pin_1

#define GUN_BOLT_GPIO		GPIOC
#define GUN_BOLT_GPIO_PIN	GPIO_Pin_2

#define GUN_TRIGER_GPIO		GPIOC
#define GUN_TRIGER_GPIO_PIN	GPIO_Pin_13

#define BULET_ONE_BOLT	30

extern void send_charcode(u16 code);
extern void wav_play_up(void);
extern void wav_play_down(void);

static int mode;

void TIM3_Int_Init(u16 arr, u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //ʱ��ʹ��
	
	//��ʱ��TIM3��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //ʹ��ָ����TIM3�ж�,��������ж�

	//�ж����ȼ�NVIC����
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //��ռ���ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //�����ȼ�3��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);  //��ʼ��NVIC�Ĵ���


	//TIM_Cmd(TIM3, ENABLE);  //ʹ��TIMx					 
}

//��ʱ��3�жϷ������
void TIM3_IRQHandler(void)   //TIM3�ж�
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //���TIM3�����жϷ������
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //���TIMx�����жϱ�־ 
		
		TIM_Cmd(TIM3, DISABLE);
		GPIO_WriteBit(GUN_MOTOR_GPIO, GUN_MOTOR_GPIO_PIN, Bit_RESET);
	}
}

void startup_motor(void)
{
	//GPIO_WriteBit(GUN_MOTOR_GPIO, GUN_MOTOR_GPIO_PIN, Bit_RESET);
	//TIM3_Int_Init(2999, 7199);
	TIM_Cmd(TIM3, ENABLE);
	GPIO_WriteBit(GUN_MOTOR_GPIO, GUN_MOTOR_GPIO_PIN, Bit_SET);
	//msleep(300);
	
}

s8 check_pull_bolt(void)
{	
	s8 ret =  GPIO_ReadInputDataBit(GUN_BOLT_GPIO, GUN_BOLT_GPIO_PIN) == Bit_RESET;
	
	msleep(50);
	
	return ret && (GPIO_ReadInputDataBit(GUN_BOLT_GPIO, GUN_BOLT_GPIO_PIN) == Bit_RESET);
}

s8 is_auto_mode(void)
{
	return mode == GUN_MODE_AUTO;
}

s8 is_single_mode(void)
{
	return mode == GUN_MODE_SINGLE;
}

s8 get_mode(void)
{	
	//return GUN_MODE_SINGLE;
	
	if (GPIO_ReadInputDataBit(GUN_MODE1_GPIO, GUN_MODE1_GPIO_PIN) == Bit_RESET) {
		//msleep(20);
		//if (GPIO_ReadInputDataBit(GUN_MODE1_GPIO, GUN_MODE1_GPIO_PIN) == Bit_RESET)
		mode = GUN_MODE_AUTO;
		err_log("in auto\r\n");
		
	} else if (GPIO_ReadInputDataBit(GUN_MODE2_GPIO, GUN_MODE2_GPIO_PIN) == Bit_RESET) {
		//msleep(20);
		//if (GPIO_ReadInputDataBit(GUN_MODE2_GPIO, GUN_MODE2_GPIO_PIN) == Bit_RESET)
		mode = GUN_MODE_SINGLE;
		err_log("in single\r\n");
	} else
		mode = GUN_MODE_OFF;
		
	return mode;
}

int saver_on(void)
{
	return get_mode() == GUN_MODE_OFF;
}

s8 trigger_get_status(void)
{
	static int status = 0;	
	s8 ret = (GPIO_ReadInputDataBit(GUN_TRIGER_GPIO, GUN_TRIGER_GPIO_PIN) == Bit_RESET);
	
	//msleep(20);
	
	ret = ret && (GPIO_ReadInputDataBit(GUN_TRIGER_GPIO, GUN_TRIGER_GPIO_PIN) == Bit_RESET);

	return ret;
}

int tirgger_pressed(void)
{
	return trigger_get_status();
}

int bolt_pulled(void)
{
	return check_pull_bolt() > 0;
}
/*
void handle_bolt_pull(void)
{
	s16 bulet = bulet_get();
	s16 bulet_one_bolt = bulet_one_bolt_get();

	if (check_pull_bolt() > 0) {
		play_bolt();

		err_log("press bolt\r\n");

		msleep(100);

		if (bulet_one_bolt > 0 && (BULET_ONE_BOLT - bulet_one_bolt) > bulet)
			bulet_one_bolt += bulet;
		else
			bulet_one_bolt = BULET_ONE_BOLT;

		bulet_one_bolt_set(bulet_one_bolt);
	}
}


void handle_trigger(void)
{
	s8 status;
	s16 characCode;
	s8 bulet_one_bolt;
	int i;
	s8 local_bulet;
	
	if (is_single_mode()) {
		s8 a = trigger_get_status();
		switch (a) {
			case 1 :
				if (status == 0) {
					play_bulet();

					send_charcode(characCode);

					err_log("press trigger\r\n");

					status = 1;

					msleep(500);
				}
				break;
			case 0 :
				if (status == 1) {
					status =  0;
				}
				break;				
		}
	} else if (is_auto_mode()) {
		if (trigger_get_status()) {
			wav_play(2);

			for (i = 0; i < 4 && bulet_one_bolt > 0; i++) {
				send_charcode(characCode);
				msleep(200);
				bulet_one_bolt--;
				local_bulet--;
			}

			err_log("press trigger\r\n");

			upload_status_data();

		}
	}

}
*/
void trigger_init(void)
{
 	GPIO_InitTypeDef GPIO_InitStructure;
	
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin  = GUN_TRIGER_GPIO_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
 	GPIO_Init(GUN_TRIGER_GPIO, &GPIO_InitStructure);	

	GPIO_InitStructure.GPIO_Pin  = GUN_BOLT_GPIO_PIN; 
 	GPIO_Init(GUN_BOLT_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin  = GUN_MODE1_GPIO_PIN; 
 	GPIO_Init(GUN_MODE1_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin  = GUN_MODE2_GPIO_PIN; 
 	GPIO_Init(GUN_MODE2_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin  = GUN_MOTOR_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
 	GPIO_Init(GUN_MOTOR_GPIO, &GPIO_InitStructure);	
	
	TIM3_Int_Init(2999, 7199);
}

#endif
