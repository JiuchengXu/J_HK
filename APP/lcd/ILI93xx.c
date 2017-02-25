#include "ILI93xx.h"
#include "stdlib.h"
#include "font.h" 
#include "usart.h"
#include "helper.h"
#include "mutex.h"

static struct mutex_lock lcd_lock;

void LCD_FSMC_Config(void)
{
    FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
    FSMC_NORSRAMTimingInitTypeDef  p; 
    
    
    p.FSMC_AddressSetupTime = 0x02;	 //地址建立时间
    p.FSMC_AddressHoldTime = 0x00;	 //地址保持时间
    p.FSMC_DataSetupTime = 0x05;		 //数据建立时间
    p.FSMC_BusTurnAroundDuration = 0x00;
    p.FSMC_CLKDivision = 0x00;
    p.FSMC_DataLatency = 0x00;

    p.FSMC_AccessMode = FSMC_AccessMode_B;	 // 一般使用模式B来控制LCD
    
    FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;
    FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
    FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_NOR;
    FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
    FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
    FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
    FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
    FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
    FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
    FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
    FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p; 
    
    FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure); 
    
    /* Enable FSMC Bank1_SRAM Bank */
    FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);  
}

static void Delay(u32 nCount)
{	
	int i;
	for(i=0;i<0XFFFF;i++)
		for(; nCount != 0; nCount--);
}

void LCD_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* config lcd gpio clock base on FSMC */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    /* config tft rst gpio */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;		
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
    /* config back_light gpio */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 ; 	 
    GPIO_Init(GPIOA, &GPIO_InitStructure); 

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;	
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; 
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; 
    GPIO_Init(GPIOG, &GPIO_InitStructure);  
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 ; 
    GPIO_Init(GPIOE, &GPIO_InitStructure); 

 
	//GPIO_SetBits(GPIOA, GPIO_Pin_11);
}

void Lcd_ColorBox(u16 xStart,u16 yStart,u16 xLong,u16 yLong,u16 Color)
{
	u32 temp;

	BlockWrite(xStart, xStart+xLong-1, yStart, yStart+yLong-1);
	
	for (temp=0; temp<xLong*yLong; temp++)
		LCD_WR_DATA(Color);
}

void LCD_Fill(int sx,int sy,int ex,int ey, u32 color)
{
	Lcd_ColorBox(sx, sy, ex - sx+1, ey - sy+1, color);
}      

void lcd_display_bitmap(int x, int y, const u16 * p, int xsize, int ysize)
{
	int Xstart = x;
	int Xend = x + xsize-1;
	int Ystart = y;
	int Yend = y + ysize-1;
	int i, j;

	mutex_lock(&lcd_lock);
	BlockWrite(Xstart, Xend, Ystart, Yend);
	
	for (j = 0; j < ysize; j++)
		for (i = 0; i < xsize; i++) {
			LCD_WR_DATA(*p++);
			//delay_us(1);
		}
		
	mutex_unlock(&lcd_lock);	
}

int init_flag = 0;
 
void TFTLCD_Init(void)
{
	u16 id[3];
	u16 dummy;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC,ENABLE);//使能CRC时钟，否则STemWin不能使用
	
	if (init_flag == 0) {
		dummy = dummy;
		
		LCD_GPIO_Config();
		LCD_FSMC_Config();
		
		Lcd_Light_OFF;
		
		init_flag = 1;
		
		   /* tft control gpio init */	 

		//GPIO_SetBits(GPIOA, GPIO_Pin_12);
		GPIO_SetBits(GPIOA, GPIO_Pin_12);
		msleep(20);
		//GPIO_ResetBits(GPIOA, GPIO_Pin_12);		 // RST = 1
		GPIO_ResetBits(GPIOA, GPIO_Pin_12);
		msleep(200);
		//GPIO_SetBits(GPIOA, GPIO_Pin_12);		 // RST = 1
		GPIO_SetBits(GPIOA, GPIO_Pin_12);
		msleep(200);
			
		WriteComm(0xda);
		dummy = ReadData();
		id[0] = ReadData();
		
		WriteComm(0xdb);
		dummy = ReadData();
		id[1] = ReadData();
		
		WriteComm(0xdc);
		dummy = ReadData();
		id[2] = ReadData();
		
		id[0] = id[0];
		id[1] = id[1];
		id[2] = id[2];
	}
	
	
			
	WriteComm(0x01);
	sleep(2);	
#if 0
	
	WriteComm(0x00B3);
	WriteData( 0x0000);//WriteData( 0x0002);
	WriteData( 0x0000);
	WriteData(0x0000);
	WriteData(0x0000);
	//WriteData(0x0000);

	WriteComm(0x00B4);
	WriteData(0x0000);

	WriteComm(0x00C0);
	WriteData(0x0013);//
	WriteData( 0x003B);//480
	WriteData( 0x0000);
	WriteData( 0x0003);//02
	WriteData( 0x0000);
	WriteData( 0x0001);
	WriteData( 0x0000);
	WriteData( 0x0043);

	WriteComm( 0x00C1);
	WriteData( 0x0008);
	WriteData( 0x0012);//CLOCK
	WriteData( 0x0008);
	WriteData( 0x0008);

	WriteComm( 0x00C4);
	WriteData( 0x0011);
	WriteData( 0x0007);
	WriteData( 0x0003);
	WriteData( 0x0003);

	WriteComm( 0x00C8);//GAMMA
	WriteData( 0x0004);
	WriteData( 0x0009);
	WriteData( 0x0016);
	WriteData( 0x005A);
	WriteData( 0x0002);
	WriteData( 0x000A);
	WriteData( 0x0016);
	WriteData( 0x0005);
	WriteData( 0x0000);
	WriteData( 0x0032);
	WriteData( 0x0005);
	WriteData( 0x0016);
	WriteData( 0x000A);
	WriteData( 0x0053);//43/55
	WriteData( 0x0008);
	WriteData( 0x0016);
	WriteData( 0x0009);
	WriteData( 0x0004);
	WriteData( 0x0032);
	WriteData( 0x0000);

	WriteComm( 0x002A);
	WriteData( 0x0000);
	WriteData( 0x0000);
	WriteData( 0x0001);
	WriteData( 0x003F);//320

	WriteComm( 0x002B);
	WriteData( 0x0000);
	WriteData( 0x0000);
	WriteData( 0x0001);
	WriteData( 0x00DF);//480

	WriteComm( 0x0035);
	WriteData( 0x0000);

	WriteComm( 0x003A);
	WriteData( 0x0055);

	WriteComm( 0x0044);
	WriteData( 0x0000);
	WriteData( 0x0001);

	//WriteComm( 0x002C);
	WriteComm( 0x0011);

	Delay(150);

	WriteComm( 0x00D0);
	WriteData( 0x0007);
	WriteData( 0x0007);
	WriteData( 0x001E);
	WriteData( 0x0703);

	WriteComm( 0x00D1);
	WriteData( 0x0003);
	WriteData( 0x0052);//VCM
	WriteData( 0x0010);//VDV

	WriteComm( 0x00D2);
	WriteData( 0x0003);
	WriteData( 0x0024);

	WriteComm(0x00B0);//{setc, [107], W, 0x000B0}
	WriteData(0x0000);//{setp, [104], W, 0x00000}

	WriteComm(0x0029);
	
	WriteComm(0x36); //Set_address_mode
	//WriteData(0xC8); //竖屏，从左上角开始，从左到右，从上到下
	WriteData(0xe8);
	
	sleep(2);
	
	Lcd_Light_ON;
	
	return;
#else
#define writecommand WriteComm
#define writedata	WriteData
#define delay		Delay

  /* writecommand(0xB9); // Enable extension command
    writedata(0xFF);
    writedata(0x83);
    writedata(0x57);
    delay(50);*/

     writecommand(0xB6); //Set VCOM voltage
    //writedata(0x2C);    //0x52 for HSD 3.0"
	writedata(0x40); 
	writedata(0x10); 
	
    writecommand(0x11); // Sleep off
    delay(200);

    writecommand(0x35); // Tearing effect on
    writedata(0x00);    // Added parameter

    writecommand(0x3A); // Interface pixel format
    writedata(0x55);    // 16 bits per pixel

    writecommand(0xB1); // Power control
    writedata(0x00);
    writedata(0x15);
    writedata(0x0D);
    writedata(0x0D);
    writedata(0x83);
    writedata(0x48);   
    
    writecommand(0xC0); // Does this do anything?
    writedata(0x24);
    writedata(0x24);
    writedata(0x01);
    writedata(0x3C);
    writedata(0xC8);
    writedata(0x08);
    
    writecommand(0xB4); // Display cycle
    writedata(0x02);
    writedata(0x40);
    writedata(0x00);
    writedata(0x2A);
    writedata(0x2A);
    writedata(0x0D);
    writedata(0x4F);
   
    writecommand(0xE0); // Gamma curve
    writedata(0x00);
    writedata(0x15);
    writedata(0x1D);
    writedata(0x2A);
    writedata(0x31);
    writedata(0x42);
    writedata(0x4C);
    writedata(0x53);
    writedata(0x45);
    writedata(0x40);
    writedata(0x3B);
    writedata(0x32);
    writedata(0x2E);
    writedata(0x28);
	
    writedata(0x24);
    writedata(0x03);
    writedata(0x00);
    writedata(0x15);
    writedata(0x1D);
    writedata(0x2A);
    writedata(0x31);
    writedata(0x42);
    writedata(0x4C);
    writedata(0x53);
    writedata(0x45);
    writedata(0x40);
    writedata(0x3B);
    writedata(0x32);
    
    writedata(0x2E);
    writedata(0x28);
    writedata(0x24);
    writedata(0x03);
    writedata(0x00);
    writedata(0x01);
	
	msleep(10);
	
	writecommand(0x36); // MADCTL Memory access control
    writedata(0xe8);
    delay(20);
	
	writecommand(0x29); // Display on
	sleep(2);
	
    //writecommand(0x21); //Display inversion on
	
	Lcd_Light_ON;
	
	mutex_init(&lcd_lock);
#endif
} 
