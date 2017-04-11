#include "ILI93xx.h"
#include "stdlib.h"
#include "font.h" 
#include "usart.h"
#include "helper.h"
#include "mutex.h"

static struct mutex_lock lcd_lock;
static OS_TMR timer;
static DMA_InitTypeDef DMA_InitStructure;
static struct wait waiter;

void LCD_FSMC_Config(void)
{
    FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
    FSMC_NORSRAMTimingInitTypeDef  p; 
    
    
    p.FSMC_AddressSetupTime = 0x02;	 //��ַ����ʱ��
    p.FSMC_AddressHoldTime = 0x00;	 //��ַ����ʱ��
    p.FSMC_DataSetupTime = 0x05;		 //���ݽ���ʱ��
    p.FSMC_BusTurnAroundDuration = 0x00;
    p.FSMC_CLKDivision = 0x00;
    p.FSMC_DataLatency = 0x00;

    p.FSMC_AccessMode = FSMC_AccessMode_B;	 // һ��ʹ��ģʽB������LCD
    
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

	Lcd_Light_OFF;
 
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

static void dma_config(u32 dst, u32 src, u16 len)
{
	DMA_InitStructure.DMA_PeripheralBaseAddr = dst;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) src;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = len * 2;
		
	DMA_Init(DMA2_Channel5, &DMA_InitStructure);
	
	DMA_Cmd(DMA2_Channel5, ENABLE); 
}

void DMA2_Channel4_5_IRQHandler(void)
{  
	GPIO_InitTypeDef GPIO_InitStructure;
  	
  	if (DMA_GetFlagStatus(DMA2_FLAG_TC5) != RESET) {         
 
      	DMA_Cmd(DMA2_Channel5, DISABLE);
		
      	DMA_ClearFlag(DMA2_FLAG_TC5);
		
		wake_up(&waiter);
  	}
}

void lcd_display_bitmap(int x, int y, const u16 * p, int xsize, int ysize)
{
	int Xstart = x;
	int Xend = x + xsize-1;
	int Ystart = y;
	int Yend = y + ysize-1;
	int i, j;
	OS_ERR err;

	OSSchedLock(&err);
	BlockWrite(Xstart, Xend, Ystart, Yend);

#if 1	
	for (j = 0; j < ysize; j++)
		for (i = 0; i < xsize; i++) {
			LCD_WR_DATA(*p++);
			//delay_us(1);
		}
#endif
		
	OSSchedUnlock(&err);

	//dma_config(Bank1_LCD_D, (u32)p, xsize * ysize);
	//wait_for(&waiter);
}

int init_flag = 0;
static int lcd_light_status = 0;

void backlight_on(void)
{
	lcd_light_status = 1;
	Lcd_Light_ON;
}

void backlight_off(void)
{
	lcd_light_status = 0;
	Lcd_Light_OFF;
}

int get_backlight_status(void)
{
	return lcd_light_status;
}

int lcd_trunoff_backlight_countdown(void)
{
	OS_ERR err;
	
	backlight_on();
	
	if (OSTmrStop(&timer, OS_OPT_TMR_NONE, NULL, &err) != DEF_TRUE)
		return -1;
	
	OSTmrStart(&timer, &err);
	
	if (err == OS_ERR_NONE)
		return 0;
	
	return -1;
}
 
void TFTLCD_Init(void)
{
	u16 id[3];
	u16 dummy;
	OS_ERR err;
	
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC,ENABLE);//ʹ��CRCʱ�ӣ�����STemWin����ʹ��
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Channel4_5_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);   

    DMA_Cmd(DMA2_Channel5, DISABLE);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Enable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Enable;
    DMA_Init(DMA2_Channel5, &DMA_InitStructure);
	DMA_ITConfig(DMA2_Channel5, DMA_IT_TC, ENABLE);
	
	if (init_flag == 0) {
		dummy = dummy;
		
		LCD_GPIO_Config();
		LCD_FSMC_Config();
		
		Lcd_Light_OFF;
		
		init_flag = 1;
		
		/* tft control gpio init */	 
		GPIO_ResetBits(GPIOA, GPIO_Pin_12);
		msleep(200);
		GPIO_SetBits(GPIOA, GPIO_Pin_12);
		msleep(200);
	}
		
	WriteComm(0x01);
	msleep(200);

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
	//sleep(2);
	
    //writecommand(0x21); //Display inversion on
	
	mutex_init(&lcd_lock);
	wait_init(&waiter);
	
	OSTmrCreate(&timer, "LCD backlight", 600, 0, OS_OPT_TMR_ONE_SHOT, (OS_TMR_CALLBACK_PTR)backlight_off, NULL, &err);
	
	//lcd_trunoff_backlight_countdown();
} 
