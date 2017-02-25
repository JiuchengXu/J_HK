#ifndef __LIBE2PROM_H__
#define __LIBE2PROM_H__

void e2prom_Reads(I2C_TypeDef *I2C, u8 slave_addr, u8 Address,u8 *ReadBuffer,u16 ReadNumber);

static void e2prom_WaitForComplete(I2C_TypeDef *I2C, u8 slave_addr);


void e2prom_WriteByte(I2C_TypeDef *I2C, u8 slave_addr, u8 Address,u8 WriteData);


void e2prom_WritePage(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *WriteData, u16 WriteNumber);


void e2prom_Writes(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *WriteData,u16 WriteNumber);

#endif /* __LIBE2PROM_H__ */
