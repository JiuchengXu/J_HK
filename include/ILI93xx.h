#ifndef __LCD_H
#define __LCD_H		
#include "stdlib.h"
#include "includes.h"

#define Bank1_LCD_D    ((u32)0x6C100000)    //Disp Data ADDR
#define Bank1_LCD_C    ((u32)0x6C000000)	   //Disp Reg ADDR


#define White          0xFFFF
#define Black          0x0000
#define Blue           0x001F
#define Blue2          0x051F
#define Red            0xF800
#define Magenta        0xF81F
#define Green          0x07E0
#define Cyan           0x7FFF
#define Yellow         0xFFE0

#define Set_Rst GPIOA->BSRR = GPIO_Pin_12
#define Clr_Rst GPIOA->BRR  = GPIO_Pin_12

#define Lcd_Light_ON   GPIOA->BSRR = GPIO_Pin_11
#define Lcd_Light_OFF  GPIOA->BRR  = GPIO_Pin_11

static inline void delay_us(int s)
{
	int i = 72 * s;
	
	while (i--);
}

static inline void WriteComm(u16 CMD)
{
	*(__IO u16 *) (Bank1_LCD_C) = CMD;
}

static inline void WriteData(u16 tem_data)
{		
	*(__IO u16 *) (Bank1_LCD_D) = tem_data;
}

static inline u16 ReadData(void)
{
	return *(__IO u16 *) (Bank1_LCD_D);
}

static inline void BlockWrite(unsigned int Xstart,unsigned int Xend,unsigned int Ystart,unsigned int Yend) 
{
	WriteComm(0x2a);   
	WriteData(Xstart>>8);
	WriteData(Xstart&0xff);
	WriteData(Xend>>8);
	WriteData(Xend&0xff);

	WriteComm(0x2b);   
	WriteData(Ystart>>8);
	WriteData(Ystart&0xff);
	WriteData(Yend>>8);
	WriteData(Yend&0xff);
	
	WriteComm(0x2C);
}
  
static inline void LCD_WR_REG(u16 regval)
{   
	WriteComm(regval);
}

static inline void LCD_WR_DATA(u16 data)
{	 
	WriteData(data);
}

static inline u16 LCD_RD_DATA(void)
{
	return ReadData();	
}					   

static inline void LCD_WriteReg(u16 LCD_Reg, u16 LCD_RegValue)
{	
    LCD_WR_REG(LCD_Reg);
	LCD_WR_DATA(LCD_RegValue);
}	   

static inline u16 LCD_ReadReg(u16 LCD_Reg)
{										   
	LCD_WR_REG(LCD_Reg);
	delay_us(5);		  
	return LCD_RD_DATA();		//返回读到的值
}   

static inline u16 ReadPixel(u16 x,u8 y)
{
	u16 dat, dummy;
	
	WriteComm(0x2a);   
	WriteData(x>>8);
	WriteData(x&0xff);
	WriteComm(0x2b);   
	WriteData(y>>8);
	WriteData(y&0xff);
	WriteComm(0x2e);
	
	dummy = LCD_RD_DATA();
	dummy = dummy;
	
	dat = LCD_RD_DATA();
	
	return dat;
}


static inline void DrawPixel(int x, int y, int Color)
{
	BlockWrite(x,x,y,y);		
	LCD_WR_DATA(Color);		
}

void LCD_Fill(int sx,int sy,int ex,int ey, u32 color);

#endif  
	 
	 



