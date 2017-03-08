#include "includes.h"
#include "helper.h"
#include "bus.h"
#include "mutex.h"

#if defined(GUN) || defined(CLOTHE)
#define ESP8266_DOWNLOAD		GPIOB
#define ESP8266_DOWNLOAD_PIN	GPIO_Pin_0

#define ESP8266_RST				GPIOA
#define ESP8266_RST_PIN			GPIO_Pin_12

#define ESP8266_ENABLE			GPIOA
#define ESP8266_ENABLE_PIN		GPIO_Pin_8

#define ESP8266_GPIO15			GPIOA
#define ESP8266_GPIO15_PIN		GPIO_Pin_4

#define ESP8266_GPIO4			GPIOB
#define ESP8266_GPIO4_PIN		GPIO_Pin_1
#endif

#if defined(LCD)
#define ESP8266_DOWNLOAD		GPIOC
#define ESP8266_DOWNLOAD_PIN	GPIO_Pin_1

#define ESP8266_RST				GPIOC
#define ESP8266_RST_PIN			GPIO_Pin_4

#define ESP8266_ENABLE			GPIOC
#define ESP8266_ENABLE_PIN		GPIO_Pin_5

#define ESP8266_GPIO15			GPIOA
#define ESP8266_GPIO15_PIN		GPIO_Pin_4

#define ESP8266_GPIO4			GPIOC
#define ESP8266_GPIO4_PIN		GPIO_Pin_2

#define ESP8266_PWR_EN			GPIOB
#define ESP8266_PWR_EN_PIN		GPIO_Pin_1
#endif

struct ip_port_map {
	u32 ip;
	u16 remote_port;
	u16 local_port;
};

struct ip_port_map ip_map[20];

static char temp[100];
static char output[2048];
static u8 gID;

static struct mutex_lock lock;

s8 close_udp(u8 id);

extern void reset_buffer(void);
extern void reset_rx_buffer1(void);
extern u8 wifi_uart_recieve1(void);

void bus_send_string(char *buf)
{
	u16 i;
	
	reset_rx_buffer1();
	
	for (i = 0; buf[i] != '\0'; i++)
		bus_send(&buf[i], 1);
}

void bus_recieve_string(char *buf)
{
	u16 i = 0;
	
	memset(output, 0, sizeof(output));
	
	while ((buf[i++] = bus_recieve()) != '\0');
}

static s8 wait_for_return(void)
{
	int timeout = 1500;
	char priv_char;
	
	while (timeout) {	
		char c = wifi_uart_recieve1();
			
		if (priv_char == 'O' && c == 'K')
			return 0;
		
		if (priv_char == 'E' && c == 'R')
			return 0;
		
		if (priv_char == 'F' && c == 'A')
			return -1;
		
		if (c == '\0') {
			msleep(10);
			timeout--;
		} else
			priv_char = c;					
	}

	return -1;
}

static s8 exe_cmd(char *cmd)
{
	int retry = 5;

	while (retry--) {
		bus_send_string(cmd);
		if (wait_for_return() == 0)
			return 0;
	}
	
	return -1;
}

s8 set_mode(u8 mode)
{
	sprintf(temp, "AT+CWMODE=%d\r\n", mode);	
	return exe_cmd(temp);
}

s8 set_show_ip(int mode)
{
	sprintf(temp, "AT+CIPDINFO=%d\r\n", mode);
	return exe_cmd(temp);
}

s8 connect_ap(char *id, char *passwd, s8 channel)
{		
	sprintf(temp, "AT+CWJAP=\"%s\",\"%s\"\r\n", id, passwd);
	return exe_cmd(temp);
}

s8 set_mac_addr(void)
{
	sprintf(temp, "AT+CIPSTAMAC=\"18:fe:35:98:d3:7b\"\r\n");
	return exe_cmd(temp);
}

s8 set_auto_conn(u8 i)
{
	sprintf(temp, "AT+CWAUTOCONN=%d\r\n", i);
	return exe_cmd(temp);	
}
s8 close_conn(void)
{
	sprintf(temp, "AT+CWQAP\r\n");
	return exe_cmd(temp);
}

s8 set_RF_power(s8 v)
{
	sprintf(temp, "AT+RFPOWER=%d\r\n", v);
	return exe_cmd(temp);
}

s8 set_ap(char *sid, char *passwd)
{
	sprintf(temp, "AT+CWSAP=\"%s\",\"%s\",5,3,4\r\n", sid, passwd);
	return exe_cmd(temp);
}

s8 set_echo(s8 on)
{
	reset_buffer();	
	sprintf(temp, "ATE%d\r\n", on);
	return exe_cmd(temp);
}

s8 set_mux(s8 mode)
{
	sprintf(temp, "AT+CIPMUX=%d\r\n", mode);
	return exe_cmd(temp);	
}

s8 udp_setup(u32 ip, u16 remote_port, u16 local_port)
{
	char ip_str[16];
	
	sprintf(ip_str, "%d.%d.%d.%d", ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip &0xff);
	
	if (gID >= sizeof(ip_map)/sizeof(ip_map[0]))
		return -1;
	
	ip_map[gID].ip = ip;
	ip_map[gID].remote_port = remote_port;
	ip_map[gID].local_port = local_port;
	
	sprintf(temp, "AT+CIPSTART=%d,\"UDP\",\"%s\",%d,%d\r\n", gID++, ip_str, remote_port, local_port);
	
	return exe_cmd(temp);
}

void ping(u32 ip)
{
	char ip_str[16];
	int timeout = 500;
	char priv_char;
	
	sprintf(ip_str, "%d.%d.%d.%d", ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip &0xff);
	
	sprintf(temp, "AT+PING=\"%s\"\r\n", ip_str);
	
	bus_send_string(temp);
	
	while (timeout) {	
		char c = wifi_uart_recieve1();
		if (priv_char == 'O' && c == 'K')
			break;
		
		if (priv_char == 'O' && c == 'R')
			break;
		
		if (c == '\0') {
			msleep(10);
			timeout--;
		} else
			priv_char = c;						
	}
	
	msleep(100);
}

static s8 get_id(u32 ip, u16 local_port, u16 remote_port)
{
	u8 i;
	
	for (i = 0; i < sizeof(ip_map) / sizeof(ip_map[0]); i++)
		if (ip_map[i].ip == ip &&
			local_port == ip_map[i].local_port &&
			remote_port == ip_map[i].remote_port)
			return i;
		
	return -1;
}

static struct ip_port_map *get_ip_port(u8 id)
{
	return &ip_map[id];
}

s8 send_data(u32 ip, u16 src_port, u16 dst_port, char *data, u16 len)
{
	u8 id = get_id(ip, src_port, dst_port);
	int timeout, ret = -1;
	char priv_char;
	
	sprintf(temp, "AT+CIPSEND=%d,%d\r\n", id, len);
		
	mutex_lock(&lock);
	
	bus_send_string(temp);
	
	timeout = 500;
	
	while (timeout) {	
		char c = wifi_uart_recieve1();
		
		if (c == '>') {
			ret = 0;
			break;
		}
		
		if (c == '\0') {
			msleep(10);	
			timeout--;
		}
	}
	
	if (timeout == 0 && ret == -1)
		goto out;
	
	bus_send(data, len);
	
	timeout = 500;
	ret = -1;
	
	while (timeout) {
		char c = wifi_uart_recieve1();
		
		if (priv_char == 'O' && c == 'K') {
			ret = 0;
			goto out;
		}
		
		if (c == '\0') {
			msleep(10);	
			timeout--;
		} else
			priv_char = c;
	}
out:	
	mutex_unlock(&lock);
	
	return ret;
}

static u16 get_len(char *buf)
{
	u8 i;
	u16 tmp = 0;
		
	for (i = 0; (buf[i] = bus_recieve()) != ':'; i++) {
		tmp *= 10;
		tmp += buf[i] - '0';
	}
	
	return tmp;
}

static char net_receive_sync(void)
{
	char c;
	
	while (1) {
		c = bus_recieve();
		if (c == '\0')
			msleep(10);
		else
			return c;
	}
}

void recv_data(u32 *ip, u16 *port, char *buf, u16 *buf_len)
{
	char tmp[10], c;
	u16 len;
	u8 fd;
	u16 i;
	struct ip_port_map *map;

	while (1) {		
		c = net_receive_sync();
		
		if (c != '+')
			continue;		
		
		/* read IPD */
		for (i = 0; i < 4; i++)
			tmp[i] = net_receive_sync();
		
		if (strncmp(tmp, "IPD,", 4) != 0)
			continue;
		
		/* read fd */
		for (i = 0; i < 2; i++)
			tmp[i] = net_receive_sync();
		
		//fd is from '0' to '9'
		fd  = tmp[0] - '0';
		
		/* get length */
		len = get_len(tmp);
		
		for (i = 0; i < len; i++)
			buf[i] = net_receive_sync();
		
		break;
	}
	
	map = get_ip_port(fd);
	*ip = map->ip;
	*port = map->remote_port;
	*buf_len = len;
}

s8 udp_close(u8 id)
{
	sprintf(temp, "AT+CIPCLOSE=%d\r\n", id);
	return exe_cmd(temp);
}

s8 set_ip(char *ip)
{
	sprintf(temp, "AT+CIPSTA=\"%s\"\r\n", ip);
	return exe_cmd(temp);	
}

s8 set_bound(void)
{
	sprintf(temp, "AT+UART=115200,8,1,0,0\r\n");
	return exe_cmd(temp);	
}

s8 esp_reset(void)
{
	sprintf(temp, "AT+RST\r\n");
	return exe_cmd(temp);	
}

void update_esp8266(void)
{
	GPIO_WriteBit(ESP8266_DOWNLOAD, ESP8266_DOWNLOAD_PIN, Bit_RESET);
}

void work_esp8266(void)
{
	GPIO_WriteBit(ESP8266_DOWNLOAD, ESP8266_DOWNLOAD_PIN, Bit_SET); //update pin IO0
}

void enbale_esp8266(void)
{
	GPIO_WriteBit(ESP8266_ENABLE, ESP8266_ENABLE_PIN, Bit_SET);
}

void disable_esp8266(void)
{
	GPIO_WriteBit(ESP8266_ENABLE, ESP8266_ENABLE_PIN, Bit_SET);
}

void reset_esp8266(void)
{
	GPIO_WriteBit(ESP8266_GPIO15, ESP8266_GPIO15_PIN, Bit_RESET);
	msleep(100);
	GPIO_WriteBit(ESP8266_RST, ESP8266_RST_PIN, Bit_SET);
	msleep(100);
	GPIO_WriteBit(ESP8266_RST, ESP8266_RST_PIN, Bit_RESET);
	msleep(300);
	GPIO_WriteBit(ESP8266_RST, ESP8266_RST_PIN, Bit_SET);
	msleep(100);
}

void esp8266_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
		
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);	
	// download
	GPIO_InitStructure.GPIO_Pin = ESP8266_DOWNLOAD_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(ESP8266_DOWNLOAD, &GPIO_InitStructure);
	

#if defined(LCD)	
	//rst
	GPIO_InitStructure.GPIO_Pin = ESP8266_PWR_EN_PIN;
	GPIO_Init(ESP8266_PWR_EN, &GPIO_InitStructure);
	
	GPIO_WriteBit(ESP8266_PWR_EN, ESP8266_PWR_EN_PIN, Bit_SET);
	msleep(200);
#endif
	
	GPIO_InitStructure.GPIO_Pin = ESP8266_RST_PIN;
	GPIO_Init(ESP8266_RST, &GPIO_InitStructure);

	//enable
	GPIO_InitStructure.GPIO_Pin =  ESP8266_ENABLE_PIN;
	GPIO_Init(ESP8266_ENABLE, &GPIO_InitStructure);

	//CS
	GPIO_InitStructure.GPIO_Pin = ESP8266_GPIO15_PIN;
	GPIO_Init(ESP8266_GPIO15, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = ESP8266_GPIO4_PIN;
	GPIO_Init(ESP8266_GPIO4, &GPIO_InitStructure);

	work_esp8266();	
	reset_esp8266();
	disable_esp8266();
	msleep(100);
	enbale_esp8266();
	
	mutex_init(&lock);

}
