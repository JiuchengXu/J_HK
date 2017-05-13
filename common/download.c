#include "includes.h"
#include "usart.h"
#include "lib_str.h"
#include "bus.h"
#include "helper.h"
#include <priority.h>

#define OS_TASK_STACK_SIZE 256

static CPU_STK  TaskStk[OS_TASK_STACK_SIZE];
static OS_TCB TaskStkTCB;
static u8 spi_buf[256 + 2];

extern void usb_uart_start_rx(void);
extern char usb_uart_get_char(void);
extern void esp8266_update_firmware(void);
extern void wifi_uart_putc(char c);
extern void IWDG_Init(u8 prer, u16 rlr) ;
extern void IWDG_Feed(void);
extern void usb_uart_get_string(u8 *buf, u16 len);
extern void flash_page_write(uint32_t page, uint8_t *data);
extern void flash_chip_erase(void);
extern void usb_uart_putc(char c);
extern void disable_usb_pass_through(void);
extern void flash_erase(uint32_t addr, int len);
extern u32 get_sector_size(void);
extern void watch_dog_feed_task_delete(void);

static int crc(int crc, const char *buf, int len)
{
	unsigned int byte;
	unsigned char k;
	unsigned short ACC,TOPBIT;
	unsigned short remainder = crc;

	TOPBIT = 0x8000;

	for (byte = 0; byte < len; ++byte)
	{
		ACC = buf[byte];
		remainder ^= (ACC <<8);
		for (k = 8; k > 0; --k)
		{
			if (remainder & TOPBIT)
			{
				remainder = (remainder << 1) ^0x8005;
			}
			else
			{
				remainder = (remainder << 1);
			}
		}
	}

	remainder=remainder^0x0000;
	return remainder;
}

static void handle_erase_info(u32 *addr, u32 *len)
{
	u32 *c_sector, *c_num;
	u8 tmp[8 + 2];
	int i;
	int retry = 5;

try:	
	usb_uart_get_string(tmp, sizeof(tmp));
	if (crc(0, (char *)tmp, sizeof(tmp)) == 0) {		
		c_sector = (u32 *)tmp;
		c_num = (u32 *)(tmp + 4);

		*addr = *c_sector;
		*len = *c_num;

		usb_uart_putc('o');

		flash_erase(*addr, *len);
	} else {
		usb_uart_putc('r');
		if (retry-- > 0)
			goto try;
	}
}

static void download_task(void)
{
	char cmd[3] = {0};
	char cmd_no;
	char c;
	int i;
	OS_ERR	err;
	u32 addr, len;

	while (1) {
		cmd[2] = usb_uart_get_char();

		if (memcmp(cmd, "cmd", 3) == 0)
			break;

		cmd[0] = cmd[1];
		cmd[1] = cmd[2];
	}

	disable_usb_pass_through();

	OSSchedLock(&err);

	usb_uart_putc('o');
	usb_uart_putc('k');

	cmd_no = usb_uart_get_char();

	switch (cmd_no) {
		case 'e': //更新8266
			esp8266_update_firmware();
			//IWDG_Init(IWDG_Prescaler_64, 625 * 10);
			while (1) {
				c = usb_uart_get_char();
				wifi_uart_putc(c);
				IWDG_Feed();
			}
		case 'f': //更新spi flash 文件		
			handle_erase_info(&addr, &len);

			usb_uart_putc('s');

			i = 0;

			while (i < len) {
				if (i + get_page_size() > len)
					break;

				usb_uart_get_string(spi_buf, sizeof(spi_buf));
				if (crc(0, spi_buf, sizeof(spi_buf)) == 0) {
					usb_uart_putc('o');
					flash_page_write_addr(addr + i, spi_buf);
					i += get_page_size();
				} else 
					usb_uart_putc('r');				
			}

			if (i == len)
				NVIC_SystemReset();
			else {
				while (1) {
					int tmp_len = len - i;

					usb_uart_get_string(spi_buf, tmp_len + 2);

					if (crc(0, spi_buf, tmp_len + 2) == 0) {
						usb_uart_putc('o');
						flash_page_write_addr(addr + i, spi_buf);
						i += get_page_size();
						NVIC_SystemReset();
					} else 
						usb_uart_putc('r');
				}
			}
	}
}

void download_task_init(void)
{
	OS_ERR err;

	OSTaskCreate((OS_TCB *)&TaskStkTCB, 
			(CPU_CHAR *)"download task", 
			(OS_TASK_PTR)download_task, 
			(void * )0, 
			(OS_PRIO)8, 
			(CPU_STK *)&TaskStk[0], 
			(CPU_STK_SIZE)OS_TASK_STACK_SIZE/10, 
			(CPU_STK_SIZE)OS_TASK_STACK_SIZE, 
			(OS_MSG_QTY) 0, 
			(OS_TICK) 0, 
			(void *)0,
			(OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
			(OS_ERR*)&err);	
}
