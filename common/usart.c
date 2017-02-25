#include "includes.h"

#if defined GUN || defined LCD

#include "usart.h"
#include "lib_str.h"
#include "bus.h"
#include "helper.h"
#include <priority.h>

#define BUF_LENGTH	2048
static char RxBuf[BUF_LENGTH];
static volatile int w_index, r_start, r_start1;

static char usb_rxbuf[512];
static volatile int usb_w_idx, usb_r_idx;

void (*usb_uart_recv_hook)(char c);
void (*wifi_uart_recv_hook)(char c);

//重定义fputc函数 

#define WIFI_USART		USART2
#define WIFI_UART_GPIO	GPIOA
#define WIFI_UART_TX	GPIO_Pin_2
#define WIFI_UART_RX	GPIO_Pin_3
#define WIFI_IRQHandler USART2_IRQHandler


#define USB_UART_IRQHandler	USART1_IRQHandler
#define USB_USART	USART1

int fputc(int ch, FILE *f)
{      
	while((USB_USART->SR&0X40)==0);//循环发送,直到发送完毕   
	USB_USART->DR = (u8) ch;      
	return ch;
}

void WIFI_IRQHandler(void) 
{
	char c;
	
	OSIntEnter(); 
	
	if(USART_GetITStatus(WIFI_USART, USART_IT_RXNE) != RESET) {
		c = RxBuf[w_index++] = USART_ReceiveData(WIFI_USART);
		
		if (wifi_uart_recv_hook)
			wifi_uart_recv_hook(c);
		
		if (w_index == BUF_LENGTH)
			w_index = 0;
	}
	
	OSIntExit();
}

void USB_UART_IRQHandler(void) 
{ 
	char c;
	
	OSIntEnter();
	
	if (USART_GetITStatus(USB_USART, USART_IT_RXNE) != RESET) {
		c = USART_ReceiveData(USB_USART);	
		
		if (usb_uart_recv_hook)
			usb_uart_recv_hook(c);

		usb_rxbuf[usb_w_idx++] = c;
		
		if (usb_w_idx == sizeof(usb_rxbuf))
			usb_w_idx = 0;
	}
	
	OSIntExit();
}

void wifi_uart_putc(char c)
{
	while(USART_GetFlagStatus(WIFI_USART, USART_FLAG_TXE) == RESET)
		;
	USART_SendData(WIFI_USART, c);
}

void usb_uart_putc(char c)
{
	while(USART_GetFlagStatus(USB_USART, USART_FLAG_TXE) == RESET)
		;
    USART_SendData(USB_USART, c);   
}

void usb_uart_start_rx(void)
{
	usb_w_idx = usb_r_idx = 0;
}

int usb_uart_get_data(u8 *s, u16 len)
{
	int i, j;
	
	for (i = 0; i < len; i++) {
		j = 0;
		while (usb_r_idx == usb_w_idx) {
			msleep(20);
			j++;
			
			if (j == 50 && usb_r_idx != 0)
				return -1;
		}
		
		s[i] = usb_rxbuf[usb_r_idx++];
		
		if (usb_r_idx == sizeof(usb_rxbuf))
			usb_r_idx = 0;	
	}
	
	return 0;
}

static char wifi_uart_recieve(void)
{
	char c;
	
	if (r_start == w_index)
		return '\0';

	c = RxBuf[r_start];
	
	r_start++;
	
	if (r_start == BUF_LENGTH)
		r_start = 0;
	
	return c;
}

char wifi_uart_recieve1(void)
{
	char c;
	
	if (r_start1 == w_index)
		return '\0';

	c = RxBuf[r_start1];
	
	r_start1++;
	
	if (r_start1 == BUF_LENGTH)
		r_start1 = 0;
	
	return c;
}

static void wifi_uart_send(char *buf, int len)
{
	int i;
	
	for (i = 0; i < len; i++)
		wifi_uart_putc(buf[i]);
}

void reset_buffer(void)
{
	r_start = w_index;
}


void reset_rx_buffer1(void)
{
	r_start1 = w_index;
}


void uart_inint(void)
{
	u32 bound = 115200;
	
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	
	/* Enable the USB_USART Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USB_UART_INT_PRIO;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
			
	USART_Cmd(USB_USART, ENABLE);
	USART_Init(USB_USART, &USART_InitStructure);
	USART_ITConfig(USB_USART, USART_IT_RXNE, ENABLE); 

	/************************************************************************************/
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = WIFI_UART_INT_PRIO;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = WIFI_UART_TX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(WIFI_UART_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = WIFI_UART_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(WIFI_UART_GPIO, &GPIO_InitStructure);
	
	USART_Cmd(WIFI_USART, ENABLE);
	USART_Init(WIFI_USART, &USART_InitStructure);
	USART_ITConfig(WIFI_USART, USART_IT_RXNE , ENABLE); 
	
	usb_uart_recv_hook = wifi_uart_putc;
	wifi_uart_recv_hook = usb_uart_putc;
	
	register_bus(wifi_uart_send, wifi_uart_recieve);
}

#else

#include "includes.h"
#include "usart.h"
#include "lib_str.h"
#include "bus.h"
#include <priority.h>

#define BUF_LENGTH	2048
static char RxBuf[BUF_LENGTH];
static volatile int w_index, r_start;
static int r_start1 = 0;

static  OS_SEM   UartSem;

void (*uart1_recv_hook)(char c);
void (*uart3_recv_hook)(char c);

//???¨ò?fputcoˉêy 

#define WIFI_USART		USART3
#define WIFI_UART_GPIO	GPIOB
#define WIFI_UART_TX	GPIO_Pin_10
#define WIFI_UART_RX	GPIO_Pin_11

#define USB_USART	USART1

int fputc(int ch, FILE *f)
{      
	while((WIFI_USART->SR&0X40)==0);
	WIFI_USART->DR = (u8) ch;      
	return ch;
}

void USART3_IRQHandler(void) 
{
	char c;
	OS_ERR err; 
	
	if(USART_GetITStatus(WIFI_USART, USART_IT_RXNE) != RESET) {
		c = RxBuf[w_index++] = USART_ReceiveData(WIFI_USART);
		
		if (uart3_recv_hook)
			uart3_recv_hook(c);
		
		if (w_index == BUF_LENGTH)
			w_index = 0;
	}
	
}

void USART1_IRQHandler(void) 
{ 
	char c;
	
	if (USART_GetITStatus(USB_USART, USART_IT_RXNE) != RESET) {
		c = USART_ReceiveData(USB_USART);	
		
		if (uart1_recv_hook)
			uart1_recv_hook(c);		
	}
}

void uart3_putc(char c)
{

	while(USART_GetFlagStatus(WIFI_USART, USART_FLAG_TXE) == RESET);
	USART_SendData(WIFI_USART, c);
#if 0
	if (c == 0x0d) {
		c = 0x0a;
		while(USART_GetFlagStatus(WIFI_USART, USART_FLAG_TXE) == RESET);
		USART_SendData(WIFI_USART, c);
	}
#endif

}

void uart1_putc(char c)
{
	while(USART_GetFlagStatus(USB_USART, USART_FLAG_TXE) == RESET);
    USART_SendData(USB_USART, c);   
}

void uart_puts(char *str)
{
    while(*str)
    {
       
		while(USART_GetFlagStatus(WIFI_USART, USART_FLAG_TXE) == RESET);
		USART_SendData(WIFI_USART, *str++);
    }
}

static char uart3_recieve(void)
{
	char c;
	
	if (r_start == w_index)
		return '\0';

	c = RxBuf[r_start];
	
	r_start++;
	
	if (r_start == BUF_LENGTH)
		r_start = 0;
	
	return c;
}

static char uart3_recieve2(void)
{
	char c;
	
	if (r_start1 == w_index)
		return '\0';

	c = RxBuf[r_start1];
	
	r_start1++;
	
	if (r_start1 == BUF_LENGTH)
		r_start1 = 0;
	
	return c;
}

char wifi_uart_recieve1(void)
{
	return uart3_recieve2();
}

static void uart3_send(char *buf, int len)
{
	int i;
	
	for (i = 0; i < len; i++) {
		uart3_putc(buf[i]);
		//uart1_putc(buf[i]);
	}	
}

void reset_buffer(void)
{
	r_start = w_index;
}

void reset_rx_buffer1(void)
{
	r_start1 = w_index;
}


void uart_inint(void)
{
	u32 bound = 115200;
	OS_ERR err;
	
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	
	/* Enable the USB_USART Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USB_UART_INT_PRIO;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
			
	USART_Cmd(USB_USART, ENABLE);
	USART_Init(USB_USART, &USART_InitStructure);
	USART_ITConfig(USB_USART, USART_IT_RXNE, ENABLE); 

	/************************************************************************************/
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = WIFI_UART_INT_PRIO;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = WIFI_UART_TX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(WIFI_UART_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = WIFI_UART_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(WIFI_UART_GPIO, &GPIO_InitStructure);
	
	USART_Cmd(WIFI_USART, ENABLE);
	USART_Init(WIFI_USART, &USART_InitStructure);
	USART_ITConfig(WIFI_USART, USART_IT_RXNE , ENABLE); 
	
	uart1_recv_hook = uart3_putc;
	uart3_recv_hook = uart1_putc;

	OSSemCreate(&UartSem, "Test Sem", 0, &err);
	
	register_bus(uart3_send, uart3_recieve);
}

#endif
