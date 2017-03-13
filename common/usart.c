#include "includes.h"
#include "usart.h"
#include "lib_str.h"
#include "bus.h"
#include "helper.h"
#include <priority.h>
#include "mutex.h"

#define BUF_LENGTH	2048
static char RxBuf[BUF_LENGTH];
static volatile int w_index, r_start, r_start1;

static char usb_rxbuf[512];
static volatile int usb_w_idx, usb_r_idx;

void (*usb_uart_recv_hook)(char c);
void (*wifi_uart_recv_hook)(char c);

//重定义fputc函数

#define USB_UART_IRQHandler	USART1_IRQHandler
#define USB_USART			USART1
#define USB_UART_GPIO		GPIOA
#define USB_UART_TX			GPIO_Pin_9
#define USB_UART_RX			GPIO_Pin_10
#define USB_UART_IRQ		USART1_IRQn

#if defined(GUN) || defined(CLOTHE)

#define WIFI_USART		USART2
#define WIFI_UART_GPIO	GPIOA
#define WIFI_UART_TX	GPIO_Pin_2
#define WIFI_UART_RX	GPIO_Pin_3
#define WIFI_IRQHandler USART2_IRQHandler
#define WIFI_UART_IRQ	USART2_IRQn

#define RCC2_WIFI_USB 		(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1)
#define RCC1_WIFI_USB		(RCC_APB1Periph_USART2)

#else


#define WIFI_USART		USART3
#define WIFI_UART_GPIO	GPIOB
#define WIFI_UART_TX	GPIO_Pin_10
#define WIFI_UART_RX	GPIO_Pin_11
#define WIFI_IRQHandler USART3_IRQHandler
#define WIFI_UART_IRQ	USART3_IRQn

#define RCC2_WIFI_USB 		(RCC_APB2Periph_AFIO | \
							 RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA)
#define RCC1_WIFI_USB		(RCC_APB1Periph_USART3)

#endif

static struct wait usb_waiter, wifi_waiter;

int fputc(int ch, FILE *f)
{      
	while((USB_USART->SR&0X40)==0);//循环发送,直到发送完毕   
	USB_USART->DR = (u8) ch;      
	return ch;
}

void WIFI_IRQHandler(void) 
{
	char c;
	
	//OSIntEnter(); 
	
	if(USART_GetITStatus(WIFI_USART, USART_IT_RXNE) != RESET) {
		c = RxBuf[w_index++] = USART_ReceiveData(WIFI_USART);
		
		if (wifi_uart_recv_hook)
			wifi_uart_recv_hook(c);
		
		if (w_index == BUF_LENGTH)
			w_index = 0;
		
		//wake_up(&wifi_waiter);
	}
	
	//OSIntExit();
}

void USB_UART_IRQHandler(void) 
{ 
	char c;
	
	//OSIntEnter();
	
	if (USART_GetITStatus(USB_USART, USART_IT_RXNE) != RESET) {
		c = USART_ReceiveData(USB_USART);

		//if (usb_w_idx == usb_r_idx)
			//wake_up(&usb_waiter);
			
		usb_rxbuf[usb_w_idx++] = c;
		
		if (usb_w_idx == sizeof(usb_rxbuf))
			usb_w_idx = 0;
		
		
	}
	
	//////OSIntExit();
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

char usb_uart_get_char(void)
{
	char s;

	while (usb_w_idx == usb_r_idx)
		msleep(30);
		//wait_for(&usb_waiter);
		
	s = usb_rxbuf[usb_r_idx++];
		
	if (usb_r_idx == sizeof(usb_rxbuf))
		usb_r_idx = 0;	
	
	return s;
}


void usb_uart_get_string(u8 *buf, u16 len)
{
	int i;
	
	for (i = 0; i < len; i++) {
		while (usb_w_idx == usb_r_idx)
			msleep(30);
			//wait_for(&usb_waiter);
			
		buf[i] = usb_rxbuf[usb_r_idx++];
			
		if (usb_r_idx == sizeof(usb_rxbuf))
			usb_r_idx = 0;	
	}
}

static char wifi_uart_recieve(void)
{
	char c;
	
	while (r_start == w_index)
		msleep(50);
		//return '\0';
		//wait_for(&wifi_waiter);

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
	
	RCC_APB2PeriphClockCmd(RCC2_WIFI_USB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC1_WIFI_USB, ENABLE);
	
	/* Enable the USB_USART Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = USB_UART_IRQ;
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
	
	GPIO_InitStructure.GPIO_Pin = USB_UART_TX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(USB_UART_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = USB_UART_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(USB_UART_GPIO, &GPIO_InitStructure);
			
	USART_Cmd(USB_USART, ENABLE);
	USART_Init(USB_USART, &USART_InitStructure);
	USART_ITConfig(USB_USART, USART_IT_RXNE, ENABLE); 

	/************************************************************************************/
	NVIC_InitStructure.NVIC_IRQChannel = WIFI_UART_IRQ;
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
	
	//usb_uart_recv_hook = wifi_uart_putc;
	//wifi_uart_recv_hook = usb_uart_putc;
	
	register_bus(wifi_uart_send, wifi_uart_recieve);
	
	wait_init(&usb_waiter);
	wait_init(&wifi_waiter);
	
	usb_w_idx = usb_r_idx = 0;
}

void disable_usb_pass_through(void)
{
	wifi_uart_recv_hook = NULL;
}
