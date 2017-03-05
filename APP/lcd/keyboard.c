#include "includes.h"
#include "helper.h"

#define KEY1_PIN	GPIO_Pin_15
#define KEY1_GPIO	GPIOG

#define KEY2_GPIO	GPIOD
#define KEY2_PIN	GPIO_Pin_2

#define KEY3_GPIO	GPIOC
#define KEY3_PIN	GPIO_Pin_10

#define KEY4_GPIO	GPIOA
#define KEY4_PIN	GPIO_Pin_0

#define KEY5_GPIO	GPIOA
#define KEY5_PIN	GPIO_Pin_2

static int key_map[6] = {0, 5, 4, 3, 2, 1};
 
static int __get_keyboard_value(void)
{
	if (GPIO_ReadInputDataBit(KEY1_GPIO, KEY1_PIN) == Bit_RESET)
		return 1;
	
	if (GPIO_ReadInputDataBit(KEY2_GPIO, KEY2_PIN) == Bit_RESET)
		return 2;
	
	if (GPIO_ReadInputDataBit(KEY3_GPIO, KEY3_PIN) == Bit_RESET)
		return 3;
	
	if (GPIO_ReadInputDataBit(KEY4_GPIO, KEY4_PIN) == Bit_RESET)
		return 4;
	
	if (GPIO_ReadInputDataBit(KEY5_GPIO, KEY5_PIN) == Bit_RESET)
		return 5;
	
	
	return -1;
}

int get_keyboard_value(void)
{
	int i, ret;
	static int current_value = 1;
	
	ret = __get_keyboard_value();
	
	if (ret != -1)
		current_value = key_map[ret];

	
	return current_value;	
}

void keyboard_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA |
						   RCC_APB2Periph_GPIOG | RCC_APB2Periph_GPIOD, ENABLE);
	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

	GPIO_InitStructure.GPIO_Pin = KEY1_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(KEY1_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = KEY2_PIN;
	GPIO_Init(KEY2_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = KEY3_PIN;
	GPIO_Init(KEY3_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = KEY4_PIN;
	GPIO_Init(KEY4_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = KEY5_PIN;
	GPIO_Init(KEY5_GPIO, &GPIO_InitStructure);
	
}
