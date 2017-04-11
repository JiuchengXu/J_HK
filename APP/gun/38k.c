#include "includes.h"
#include "helper.h"

#ifdef GUN
static DMA_InitTypeDef DMA_InitStructure;
static u8 send_buf[4] = {0x55, 0, 0, 0};

void TIM5_PWM_Init(u16 arr,u16 psc)
{  
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA  | RCC_APB2Periph_AFIO, ENABLE);  //使能GPIO外设和AFIO复用功能模块时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
	
	//GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE); //Timer3部分重映射  TIM3_CH2->PB5    
 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //TIM5_CH2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIO
	
	GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_RESET);
 
   //初始化TIM5
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值 
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
	
	//初始化TIM3 Channel2 PWM模式	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //选择定时器模式:TIM脉冲宽度调制模式2
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //输出极性:TIM输出比较极性高
	TIM_OCInitStructure.TIM_Pulse = 12;
	TIM_OC2Init(TIM5, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM1 OC4

	TIM_OC4PreloadConfig(TIM5, TIM_OCPreload_Enable);  //使能TIM1在CCR2上的预装载寄存器
 
	//TIM_Cmd(TIM5, ENABLE);  //使能TIM3	
}

void DMA2_Channel4_5_IRQHandler(void)
{  
	GPIO_InitTypeDef GPIO_InitStructure;
  	
  	if (DMA_GetFlagStatus(DMA2_FLAG_TC5) != RESET) {         
 
      	DMA_Cmd(DMA2_Channel5, DISABLE);
		
      	DMA_ClearFlag(DMA2_FLAG_TC5);

		while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
		
		while(USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET);
		
      	TIM_Cmd(TIM5, DISABLE);
	
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
		GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_RESET);   
  	}
}

void led_uart_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;			

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Channel4_5_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);   

    DMA_Cmd(DMA2_Channel5, DISABLE);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(UART4->DR));
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) send_buf;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = 4;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA2_Channel5, &DMA_InitStructure);
	DMA_ITConfig(DMA2_Channel5, DMA_IT_TC, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化GPIO
	
	USART_InitStructure.USART_BaudRate = 2000;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx;
	
	USART_Init(UART4, &USART_InitStructure);
			
	USART_Cmd(UART4, ENABLE);
	
	USART_DMACmd(UART4, USART_DMAReq_Tx, ENABLE);
}

void uart4_putchar(char c)
{
	USART_SendData(UART4, c);
	while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);	
}

void led_38k_init(void)
{
	TIM5_PWM_Init(25,72);
	led_uart_init();
}

void send_charcode(u16 code)
{
	u8 c2 = (u8)(code >> 8);
	u8 c1 = (u8)(code & 0x00ff);
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //TIM5_CH2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIO

	printf("#%04x\r\n", code);
	
	TIM_Cmd(TIM5, ENABLE);	
#if 0	
	send_buf[1] = c1;
	send_buf[2] = c2;
	
	DMA_Init(DMA2_Channel5, &DMA_InitStructure);
	
	DMA_Cmd(DMA2_Channel5, ENABLE); 
#else	
	
	//uart4_putchar(0xff);
	//uart4_putchar(0xff);
	//uart4_putchar(0x00);
	uart4_putchar(0x55);
	uart4_putchar(c1);
	uart4_putchar(c2);
	uart4_putchar(0x00); //receive node couldn't recive the last byte without 0x00
	
	TIM_Cmd(TIM5, DISABLE);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIO
	
	GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_RESET);
#endif
}

void led_38k_test(void)
{
	//led_38k_init();
	
//	while (1) {
	//	uart4_putchar(0xff);
	//	uart4_putchar(0xff);
	//	uart4_putchar(0x00);
	//	uart4_putchar(0xaa);
	//	uart4_putchar(0x55);
	//	msleep(200);
//	}
}
#endif
