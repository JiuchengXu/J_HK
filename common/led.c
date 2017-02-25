#include "includes.h"

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


void yellow_led_on(void)
{	
	led_off();
	GPIO_WriteBit(GPIOC, GPIO_Pin_7, Bit_RESET);
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

	// yellow led
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	yellow_led_on();
}

