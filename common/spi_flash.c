#include "stm32f10x.h"
#include "stm32f10x_spi.h"
#include "includes.h"
#include "helper.h"

#define FLASH_PAGE_SIZE		256
#define FLASH_SECTOR_SIZE	4096
#define FLASH_SECTOR_COUNT	2048


/* Select SPI FLASH: ChipSelect pin low  */
#define Select_Flash()     GPIO_ResetBits(GPIOA, GPIO_Pin_15)
/* Deselect SPI FLASH: ChipSelect pin high */
#define NotSelect_Flash()    GPIO_SetBits(GPIOA, GPIO_Pin_15)

#define FLASH_CS_0()     GPIO_ResetBits(GPIOA,GPIO_Pin_15)
#define FLASH_CS_1()     GPIO_SetBits(GPIOA,GPIO_Pin_15)

#define W25X_WriteEnable		0x06 
#define W25X_WriteDisable		0x04 
#define W25X_ReadStatusReg		0x05 
#define W25X_WriteStatusReg		0x01 
#define W25X_ReadData			0x03 
#define W25X_FastReadData		0x0B 
#define W25X_FastReadDual		0x3B 
#define W25X_PageProgram		0x02 
#define W25X_64K_BlockErase		0xD8
#define W25X_32K_BlockErase		0x52
#define W25X_SectorErase		0x20 
#define W25X_ChipErase			0xC7 
#define W25X_PowerDown			0xB9 
#define W25X_ReleasePowerDown	0xAB 
#define W25X_DeviceID			0xAB 
#define W25X_ManufactDeviceID	0x90 
#define W25X_JedecDeviceID		0x9F

#define Dummy_Byte								0x00

void SPI3_init(void)
{	
	SPI_InitTypeDef  SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable SPI3 GPIOA and GPIOB clocks */

	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB , ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_4 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Deselect the FLASH: Chip Select high */
	//NotSelect_Flash();
	Select_Flash();
	/* SPI3 configuration */ 
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI3, &SPI_InitStructure);

	//SPI_I2S_DMACmd(SPI3,SPI_I2S_DMAReq_Rx | SPI_I2S_DMAReq_Tx,ENABLE);

	/* Enable SPI2 Rx buffer DMA transfer request */
	//  SPI_DMACmd(SPI3, SPI_DMAReq_Rx | SPI_DMAReq_Tx, ENABLE);

	/* Enable SPI3  */
	SPI_Cmd(SPI3, ENABLE);
}

int32_t SPIFLASH_disk_initialize(void)
{
	SPI3_init();
	return 0;

}

uint8_t spi_write_byte(uint8_t data)
{
	int timeout = 7200;

	/*u32 Count = 0;*/
	/* Loop while DR register in not emplty */
	while(SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET);

	/* Send byte through the SPI3 peripheral */
	SPI_I2S_SendData(SPI3, data);

	/* Wait to receive a byte */
	while(SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET && --timeout);


	/* Return the byte read from the SPI bus */
	return SPI_I2S_ReceiveData(SPI3);
}


uint8_t spi_read_byte(void)
{
	return (spi_write_byte(Dummy_Byte));
}

void flash_write_enable(void)
{
	Select_Flash();
	spi_write_byte(W25X_WriteEnable);    //???????????   
	NotSelect_Flash();	
}

void flash_write_disable(void)
{
	Select_Flash();
	spi_write_byte(W25X_WriteDisable); 
	NotSelect_Flash();
}

void FlashWaitBusy(void)
{
	u8 val;

	while (1) {
		Select_Flash();
		spi_write_byte(W25X_ReadStatusReg);    //???????????     

		val = spi_read_byte() & 0x01;

		NotSelect_Flash();

		if (val != 0x01)
			break;
	}
}

void FlashWEL(void)
{
	u8 val;

	flash_write_enable();

	while (1) {
		Select_Flash();
		spi_write_byte(W25X_ReadStatusReg);    //???????????     

		val = spi_read_byte() & 0x02;

		NotSelect_Flash();

		if (val & 0x02)
			break;
	}
}

void flash_page_read(uint32_t page, uint8_t *data)
{
	uint32_t i;
	OS_ERR err;

	u32 ReadAddr = page * FLASH_PAGE_SIZE;

	OSSchedLock(&err);

	FlashWaitBusy();

	Select_Flash();	

	spi_write_byte(W25X_ReadData);         //??????   
	spi_write_byte((u8)((ReadAddr)>>16));  //??24bit??    
	spi_write_byte((u8)((ReadAddr)>>8));   
	spi_write_byte((u8)ReadAddr);   

	for(i = 0; i < FLASH_PAGE_SIZE; i++)
		data[i] = spi_read_byte();   //????  

	NotSelect_Flash();

	OSSchedUnlock(&err);
}

int voice_value = 1;

void increase_voice() {
	voice_value = 2;
}

void flash_bytes_read(u32 addr, u8 *buf, u16 len)
{
	uint32_t i;
	OS_ERR err;

	u32 ReadAddr = addr;

	FlashWaitBusy();

	Select_Flash();	

	OSSchedLock(&err);

	spi_write_byte(W25X_ReadData);         //??????   
	spi_write_byte((u8)((ReadAddr)>>16));  //??24bit??    
	spi_write_byte((u8)((ReadAddr)>>8));   
	spi_write_byte((u8)ReadAddr);   

	for(i = 0; i < len; i++)
		buf[i] = spi_read_byte() * voice_value;   //????  

	OSSchedUnlock(&err);

	NotSelect_Flash();	
}

void flash_page_write(uint32_t page, uint8_t *data)
{
	uint16_t i;	
	u32 WriteAddr = page * FLASH_PAGE_SIZE;

	FlashWEL();

	FLASH_CS_0();

	spi_write_byte(W25X_PageProgram);      //??????   
	spi_write_byte((u8)((WriteAddr)>>16)); //??24bit??    
	spi_write_byte((u8)((WriteAddr)>>8));   
	spi_write_byte((u8)WriteAddr);   

	for(i = 0; i < FLASH_PAGE_SIZE; i++)
		spi_write_byte(data[i]);

	FLASH_CS_1();
	FlashWaitBusy();
}

void flash_page_write_addr(uint32_t addr, uint8_t *data)

{
	uint16_t i;	
	u32 WriteAddr = addr;

	FlashWEL();

	FLASH_CS_0();

	spi_write_byte(W25X_PageProgram);      //??????   
	spi_write_byte((u8)((WriteAddr)>>16)); //??24bit??    
	spi_write_byte((u8)((WriteAddr)>>8));   
	spi_write_byte((u8)0);   

	for(i = 0; i < FLASH_PAGE_SIZE; i++)
		spi_write_byte(data[i]);

	FLASH_CS_1();
	FlashWaitBusy();
}

void flash_sector_erase(uint32_t WriteAddr)
{
	FlashWEL();

	FLASH_CS_0();

	spi_write_byte(W25X_SectorErase);      //??????   
	spi_write_byte((u8)((WriteAddr)>>16)); //??24bit??    
	spi_write_byte((u8)((WriteAddr)>>8));   
	spi_write_byte((u8)WriteAddr);   	

	FLASH_CS_1();
	msleep(100);

	FlashWaitBusy();	
}

void flash_block_erase32k(uint32_t WriteAddr)
{
	FlashWEL();

	FLASH_CS_0();

	spi_write_byte(W25X_32K_BlockErase);      //??????   
	spi_write_byte((u8)((WriteAddr)>>16)); //??24bit??    
	spi_write_byte((u8)((WriteAddr)>>8));   
	spi_write_byte((u8)WriteAddr);   	

	FLASH_CS_1();

	msleep(120);
	FlashWaitBusy();
}

void flash_block_erase64k(uint32_t WriteAddr)
{
	FlashWEL();

	FLASH_CS_0();

	spi_write_byte(W25X_64K_BlockErase);      //??????   
	spi_write_byte((u8)((WriteAddr)>>16)); //??24bit??    
	spi_write_byte((u8)((WriteAddr)>>8));   
	spi_write_byte((u8)WriteAddr);   	

	FLASH_CS_1();

	msleep(150);
	FlashWaitBusy();
}

void flash_erase(uint32_t addr, int len)
{	
	int i;

	len = len / (64 * 1024 - 1) + 1;

	for (i = 0; i < len; i++)
		flash_block_erase64k(addr + i * (64 * 1024));
}

u32 get_sector_size(void)
{
	return FLASH_SECTOR_SIZE;
}

u32 get_page_size(void)
{
	return FLASH_PAGE_SIZE;
}

void flash_chip_erase(void)
{
	FlashWEL();

	FLASH_CS_0();

	spi_write_byte(W25X_ChipErase);  

	FLASH_CS_1();
	FlashWaitBusy();	
}

u32 SPI_FLASH_ReadDeviceID(void)
{
	u32 Temp = 0;

	/* Select the FLASH: Chip Select low */
	Select_Flash();   //????,?????

	/* Send "RDID " instruction */
	spi_write_byte(0x90);//??????ID,???????????ID?(??????) 0XAB
	spi_write_byte(Dummy_Byte);     //??3???????,25X16????24??,??????????,?????????DEVICEID!
	spi_write_byte(Dummy_Byte);
	spi_write_byte(0);

	/* Read a byte from the FLASH */
	Temp = spi_read_byte();
	Temp <<= 8;
	Temp |= spi_read_byte(); 

	/* Deselect the FLASH: Chip Select high */
	NotSelect_Flash();

	return Temp;
}

u8 spi_data[256] = "abcdefghijklmnopqrstuvwxyz";
u8 spi_r_data[256];

void spi_flash_test(void)
{
	//flash_sector_erase(0);
	flash_erase(0, 1024 * 1024);

	flash_page_write(0, spi_data);
	flash_bytes_read(0, spi_r_data, 256);
}
