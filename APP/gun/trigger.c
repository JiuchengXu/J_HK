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

#define GUN_BOLT_GPIO		GPIOC
#define GUN_BOLT_GPIO_PIN	GPIO_Pin_1

#define GUN_MODE1_GPIO		GPIOC
#define GUN_MODE1_GPIO_PIN	GPIO_Pin_2

#define GUN_MODE2_GPIO		GPIOC
#define GUN_MODE2_GPIO_PIN	GPIO_Pin_13

#define GUN_TRIGER_GPIO		GPIOD
#define GUN_TRIGER_GPIO_PIN	GPIO_Pin_2


#define GUN_MOTOR_GPIO		GPIOB
#define GUN_MOTOR_GPIO_PIN	GPIO_Pin_6

extern void send_charcode(u16 code);
extern void wav_play_up(void);
extern void wav_play_down(void);

//static s8 bolt = 0;

void startup_motor(void)
{
	GPIO_WriteBit(GUN_MOTOR_GPIO, GUN_MOTOR_GPIO_PIN, Bit_RESET);
	msleep(100);
	GPIO_WriteBit(GUN_MOTOR_GPIO, GUN_MOTOR_GPIO_PIN, Bit_SET);
	msleep(100);
	GPIO_WriteBit(GUN_MOTOR_GPIO, GUN_MOTOR_GPIO_PIN, Bit_RESET);
}

s8 is_bolt_on(void)
{
	return 0;
	
	if (GPIO_ReadInputDataBit(GUN_BOLT_GPIO, GUN_BOLT_GPIO_PIN) == Bit_RESET) {
		msleep(50);
		
		if (GPIO_ReadInputDataBit(GUN_BOLT_GPIO, GUN_BOLT_GPIO_PIN) == Bit_RESET)
			return 1;
	}
	
	return 0;
}

s8 get_mode(void)
{
	s8 mode = 0;
	return GUN_MODE_SINGLE;
	
	if (GPIO_ReadInputDataBit(GUN_MODE1_GPIO, GUN_MODE1_GPIO_PIN) == Bit_RESET) {
		msleep(50);
		if (GPIO_ReadInputDataBit(GUN_MODE1_GPIO, GUN_MODE1_GPIO_PIN) == Bit_RESET)
			mode |= 1;
	}
		
	if (GPIO_ReadInputDataBit(GUN_MODE2_GPIO, GUN_MODE2_GPIO_PIN) == Bit_RESET) {
		msleep(20);
		if (GPIO_ReadInputDataBit(GUN_MODE2_GPIO, GUN_MODE2_GPIO_PIN) == Bit_RESET)
			mode |= 2;
	}
	
	return mode;
}

s8 trigger_get_status(void)
{
#if 0
	static int s = 0;
	
	if (s++ == 20) {
		s = 0;
		return 1;
	}
	
	return 0;
#else	
	s8 ret = GPIO_ReadInputDataBit(GUN_TRIGER_GPIO, GUN_TRIGER_GPIO_PIN) == Bit_RESET;
	
	msleep(20);
	
	return ret && GPIO_ReadInputDataBit(GUN_TRIGER_GPIO, GUN_TRIGER_GPIO_PIN) == Bit_RESET;
	#endif
}

s8 trigger_handle(u16 charcode)
{
	s8 mode;
	s8 bulet_used_nr = 0;
	
	if (!is_bolt_on() && trigger_get_status()) {
		mode = get_mode();
		
		mode = GUN_MODE_SINGLE;
		switch(mode) {
			case GUN_MODE_SINGLE:
				send_charcode(charcode);
				
				wav_play_up();
			
				startup_motor();
				
				bulet_used_nr++;			
				
				break;
			
			case GUN_MODE_AUTO:
				while (trigger_get_status()) {
					send_charcode(charcode);
					wav_play_up();
					bulet_used_nr++;
				}
					
				wav_play_down();
				
				break;
		}
	}
	//sleep(3);
	return bulet_used_nr;
}

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
	
	
}

#endif
