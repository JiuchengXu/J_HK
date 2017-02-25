#include "includes.h"
#include "helper.h"

#define KEY0_KEY3_KEY7_KEY8_GPIO	GPIOC
#define KEY4_GPIO		GPIOA

#define KEY0_PIN	GPIO_Pin_0
#define KEY1_PIN	GPIO_Pin_1
#define KEY2_PIN	GPIO_Pin_2
#define KEY3_PIN	GPIO_Pin_3
#define KEY4_PIN	GPIO_Pin_0
#define KEY7_PIN	GPIO_Pin_4
#define KEY8_PIN	GPIO_Pin_5

static int map[5] = {8, 7, 4, 3, 2};
 
static int __get_keyboard_value(void)
{
	if (GPIO_ReadInputDataBit(KEY0_KEY3_KEY7_KEY8_GPIO, KEY0_PIN) == Bit_RESET)
		return 0;
	
	if (GPIO_ReadInputDataBit(KEY0_KEY3_KEY7_KEY8_GPIO, KEY1_PIN) == Bit_RESET)
		return 1;
	
	if (GPIO_ReadInputDataBit(KEY0_KEY3_KEY7_KEY8_GPIO, KEY2_PIN) == Bit_RESET)
		return 2;
	
	if (GPIO_ReadInputDataBit(KEY0_KEY3_KEY7_KEY8_GPIO, KEY3_PIN) == Bit_RESET)
		return 3;
	
	if (GPIO_ReadInputDataBit(KEY4_GPIO, KEY4_PIN) == Bit_RESET)
		return 4;
	
	if (GPIO_ReadInputDataBit(KEY0_KEY3_KEY7_KEY8_GPIO, KEY7_PIN) == Bit_RESET)
		return 7;
	
	if (GPIO_ReadInputDataBit(KEY0_KEY3_KEY7_KEY8_GPIO, KEY8_PIN) == Bit_RESET)
		return 8;
	
	return -1;
}

int get_keyboard_value(void)
{
	int i, ret;
	static int current_value = 1;
	
	ret = __get_keyboard_value();
	
	
	for (i = 0; ret != -1 && i < sizeof(map); i++)
		if (ret == map[i]) {
			current_value = i;
			break;
		}
	
	return current_value;	
}

void keyboard_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = KEY0_PIN | KEY1_PIN | KEY2_PIN | KEY3_PIN | KEY7_PIN | KEY8_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(KEY0_KEY3_KEY7_KEY8_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = KEY4_PIN;
	GPIO_Init(KEY4_GPIO, &GPIO_InitStructure);
}
