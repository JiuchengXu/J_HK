#include "includes.h"
#include "libe2prom.h"

#define I2C                   I2C2

#define AT24Cx_Address           0xa0 
#define AT24Cx_PageSize          8  

#define EEPROM1_WP				GPIOC
#define EEPROM1_WP_Pin			GPIO_Pin_4

void e2prom_onboard_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); 
	
	GPIO_InitStructure.GPIO_Pin    = EEPROM1_WP_Pin;
	GPIO_InitStructure.GPIO_Speed  = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_Out_PP;
	
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_ResetBits(EEPROM1_WP, EEPROM1_WP_Pin);	
}

void e2prom_onboard_Reads(u8 Address, u8 *ReadBuffer, u16 ReadNumber)
{
	e2prom_Reads(I2C, AT24Cx_Address, Address, ReadBuffer, ReadNumber);
}

void e2prom_onboard_Writes(u8 Address,u8 *WriteData,u16 WriteNumber)
{
	e2prom_WritePage(I2C, AT24Cx_Address, Address, WriteData, WriteNumber);
}

void e2prom_onboard_test(void)
{
	char a[]="1234567890";
	
	e2prom_onboard_Writes(0, (u8 *)a, sizeof(a));
	memset(a, 0, sizeof(a));
	e2prom_onboard_Reads(0, (u8 *)a, sizeof(a));
}
