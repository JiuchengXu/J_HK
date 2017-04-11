#include "includes.h"
#include "helper.h"

#define KEY1_PIN	GPIO_Pin_15
#define KEY1_GPIO	GPIOG
#define EXIT_KEY1_GPIO GPIO_PortSourceGPIOG
#define EXIT_KEY1_PIN PIO_PinSource15
#define KEY1_EXTI_Line	EXTI_Line15

#define KEY2_GPIO	GPIOD
#define KEY2_PIN	GPIO_Pin_2
#define EXIT_KEY2_GPIO GPIO_PortSourceGPIOD
#define EXIT_KEY2_PIN PIO_PinSource2
#define KEY2_EXTI_Line	EXTI_Line2

#define KEY3_GPIO	GPIOC
#define KEY3_PIN	GPIO_Pin_10
#define EXIT_KEY3_GPIO GPIO_PortSourceGPIOC
#define EXIT_KEY3_PIN PIO_PinSource10
#define KEY3_EXTI_Line	EXTI_Line10

#define KEY4_GPIO	GPIOA
#define KEY4_PIN	GPIO_Pin_0
#define EXIT_KEY4_GPIO GPIO_PortSourceGPIOA
#define EXIT_KEY4_PIN PIO_PinSource0
#define KEY4_EXTI_Line	EXTI_Line0

#define KEY5_GPIO	GPIOA
#define KEY5_PIN	GPIO_Pin_2
#define EXIT_KEY5_GPIO GPIO_PortSourceGPIOA
#define EXIT_KEY5_PIN PIO_PinSource2
#define KEY5_EXTI_Line	EXTI_Line2

static int key_map[6] = {0, 5, 4, 3, 2, 1};

static int press_key_counts = 0;

static int __get_keyboard_value(void)
{
	int ret = -1;
	
	if (GPIO_ReadInputDataBit(KEY1_GPIO, KEY1_PIN) == Bit_RESET)
		ret = 1;
	
	if (GPIO_ReadInputDataBit(KEY2_GPIO, KEY2_PIN) == Bit_RESET)
		ret = 2;
	
	if (GPIO_ReadInputDataBit(KEY3_GPIO, KEY3_PIN) == Bit_RESET)
		ret = 3;
	
	if (GPIO_ReadInputDataBit(KEY4_GPIO, KEY4_PIN) == Bit_RESET)
		ret = 4;
	
	if (GPIO_ReadInputDataBit(KEY5_GPIO, KEY5_PIN) == Bit_RESET)
		ret = 5;
	
	if (ret > 0 && get_backlight_status() == 0) {
		lcd_trunoff_backlight_countdown();
		ret = -1;
	}
	
	return ret;
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
	EXTI_InitTypeDef EXTI_InitStructure;
	
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
