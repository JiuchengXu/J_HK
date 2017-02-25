#ifndef __LED_H
#define __LED_H 

#include "stm32f10x.h"


#define LED_ON(n) GPIO_ResetBits(GPIOA, GPIO_Pin_8) 
#define LED_OFF(n) GPIO_SetBits(GPIOA, GPIO_Pin_8)
#define LED_NF(n) do { \
						uint8_t v = GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_8); \
						if (v == Bit_RESET) \
							GPIO_SetBits(GPIOA, GPIO_Pin_8); \
						else \
							GPIO_ResetBits(GPIOA, GPIO_Pin_8) ; \
					} while (0)

void LED_Init(void);
					
void red_led_on(void);
void green_led_on(void);
void yellow_led_on(void);

#endif

