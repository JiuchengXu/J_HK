#include "includes.h"
#include "helper.h"

extern void i2c_Reads(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *ReadBuffer, u16 ReadNumber);
extern void i2c_Writes(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *WriteData, u16 WriteNumber);

void e2prom_Reads(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *ReadBuffer,u16 ReadNumber)
{
	i2c_Reads(I2C, slave_addr, Address, ReadBuffer, ReadNumber);
}

void e2prom_WriteByte(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 WriteData)
{
	i2c_Writes(I2C, slave_addr, Address, &WriteData, 1);
	msleep(6);
}

void e2prom_WritePage(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *WriteData, u8 page_size)
{
	i2c_Writes(I2C, slave_addr, Address, WriteData, page_size);
	msleep(6);
}

void e2prom_WriteBytes(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *WriteData, u16 WriteNumber, u8 page_size)
{
	int page_num  = WriteNumber / page_size;
	int page_mod = WriteNumber % page_size;
	int i, j;

	for (i = 0; i < page_num; i++)
		e2prom_WritePage(I2C, slave_addr, Address + 8 * i, &WriteData[8 * i], page_size);

	for (j = 0; j < page_mod; j++)
		e2prom_WriteByte(I2C, slave_addr, Address + 8 * i + j, WriteData[8 * i + j]);
}
