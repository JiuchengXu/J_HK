#include "includes.h"
#include "priority.h"

void led_off(void)
{
	GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
	GPIO_WriteBit(GPIOC, GPIO_Pin_8, Bit_SET);
	GPIO_WriteBit(GPIOC, GPIO_Pin_7, Bit_SET);
}

void red_led_on(void)
{
	led_off();
	GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_RESET);
}

void green_led_on(void)
{
	led_off();
	GPIO_WriteBit(GPIOC, GPIO_Pin_8, Bit_RESET);
}

void blue_led_on(void)
{	
	led_off();
	GPIO_WriteBit(GPIOC, GPIO_Pin_7, Bit_RESET);
}

void red_intival(void)
{
	static s8 val = 1;

	if (val == 1) {
		red_led_on();
		val = 0;
	} else {
		led_off();
		val = 1;
	}
}

void blue_intival(void)
{
	static s8 val = 1;

	if (val == 1) {
		blue_led_on();
		val = 0;
	} else {
		led_off();
		val = 1;
	}
}

void LED_Init(void)
{	
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);	

	//red led
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//green led
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	// Blue led
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	blue_led_on();
}

