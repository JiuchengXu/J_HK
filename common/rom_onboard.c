#include "includes.h"
#include "libe2prom.h"

#ifdef CLOTHE
#define I2C                   I2C2
#define EEPROM1_WP				GPIOC
#define EEPROM1_WP_Pin			GPIO_Pin_4
#define EEPROM_WP_RCC			RCC_APB2Periph_GPIOC
#endif

#ifdef GUN
#define I2C                   I2C2
#define EEPROM1_WP				GPIOC
#define EEPROM1_WP_Pin			GPIO_Pin_4
#define EEPROM_WP_RCC			RCC_APB2Periph_GPIOC
#endif

#ifdef LCD
#define I2C                   	I2C1
#define EEPROM1_WP				GPIOC
#define EEPROM1_WP_Pin			GPIO_Pin_12
#define EEPROM_WP_RCC			RCC_APB2Periph_GPIOC
#endif

#define AT24Cx_Address           0xa0
#define AT24Cx_PageSize          8  

void e2prom_onboard_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(EEPROM_WP_RCC, ENABLE); 
	
	GPIO_InitStructure.GPIO_Pin    = EEPROM1_WP_Pin;
	GPIO_InitStructure.GPIO_Speed  = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_Out_PP;
	
	GPIO_Init(EEPROM1_WP, &GPIO_InitStructure);
	
	GPIO_ResetBits(EEPROM1_WP, EEPROM1_WP_Pin);	
}

void e2prom_onboard_Reads(u8 Address, u8 *ReadBuffer, u16 ReadNumber)
{
	e2prom_Reads(I2C, AT24Cx_Address, Address, ReadBuffer, ReadNumber);
}

void e2prom_onboard_Writes(u8 Address,u8 *WriteData, u16 WriteNumber)
{
	e2prom_WriteBytes(I2C, AT24Cx_Address, Address, WriteData, WriteNumber, AT24Cx_PageSize);
}

static u8 eeprom[] = "SN145784541458890000015332453214253064064", test_eeprom[100];

void e2prom_test(void)
{
	e2prom_onboard_Reads(0, test_eeprom, sizeof(eeprom));
	//IrDA_Reads(I2C1, 0XA0, 0, test_eeprom, sizeof(eeprom));
	
	memset(test_eeprom, 0, 100);
	
	e2prom_onboard_Writes(0, eeprom, sizeof(eeprom));
	
	e2prom_onboard_Reads(0, test_eeprom, sizeof(eeprom));
	
	printf("%s\n", test_eeprom);
	
	while (1);
	
	//printf("%s\n", test_eeprom);
}
