#include "includes.h"

#ifdef GUN
#include "helper.h"
#include "bus.h"
#include "mutex.h"

#define ESP8266_DOWNLOAD		GPIOB
#define ESP8266_DOWNLOAD_PIN	GPIO_Pin_0

#define ESP8266_RST				GPIOA
#define ESP8266_RST_PIN			GPIO_Pin_12

#define ESP8266_ENABLE			GPIOA
#define ESP8266_ENABLE_PIN		GPIO_Pin_8

#define ESP8266_GPIO15			GPIOA
#define ESP8266_GPIO15_PIN		GPIO_Pin_4

#define ESP8266_GPIO4			GPIOB
#define ESP8266_GPIO4_PIN		GPIO_Pin_6

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

s8 str_include(char *buf, char *str)
{	
	u16 i = 0, j = 0, k;

	for (i = 0; buf[i] != '\0'; i++) {
		for (j = 0, k = i; str[j] != '\0' && buf[k] != '\0'; j++, k++) {
			if (buf[k] == str[j])
				continue;
			else
				break;
		}
		
		if (str[j] == '\0')
			return 0;
		
		if (buf[k] == '\0')
			return -1;
	}
	
	return -1;
}

s8 set_mode(u8 mode)
{
	sleep(1);
	sprintf(temp, "AT+CWMODE=%d\r\n", mode);
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_show_ip(int mode)
{
	sprintf(temp, "AT+CIPDINFO=%d\r\n", mode);
	bus_send_string(temp);
		
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 connect_ap(char *id, char *passwd, s8 channel)
{
	uint8_t times = 5;
	int timeout;
	int priv_char;
	
	while (times-- > 0) {	
		sprintf(temp, "AT+CWJAP=\"%s\",\"%s\"\r\n", id, passwd);
		bus_send_string(temp);	

		timeout = 5000;
		
		while (timeout--) {	
			char c = wifi_uart_recieve1();
			
			if (priv_char == 'O' && c == 'K')
				break;
			
			priv_char = c;

			msleep(10);			
		}
		
		bus_recieve_string(output);
		
		if (str_include(output, "OK") == 0)
			return 0;
	}
	return -1;
}

s8 set_mac_addr(void)
{
	sprintf(temp, "AT+CIPSTAMAC=\"18:fe:35:98:d3:7b\"\r\n");
	bus_send_string(temp);
		
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_auto_conn(u8 i)
{
	sprintf(temp, "AT+CWAUTOCONN=%d\r\n", i);
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");	
}
s8 close_conn(void)
{
	sprintf(temp, "AT+CWQAP\r\n");
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_RF_power(s8 v)
{
	sprintf(temp, "AT+RFPOWER=%d\r\n", v);
	bus_send_string(temp);
	
	sleep(1);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_ap(char *sid, char *passwd)
{
	sprintf(temp, "AT+CWSAP=\"%s\",\"%s\",5,3,4\r\n", sid, passwd);
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_echo(s8 on)
{
	reset_buffer();
	
	sprintf(temp, "ATE%d\r\n", on);
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_mux(s8 mode)
{
	sprintf(temp, "AT+CIPMUX=%d\r\n", mode);
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");	
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
	
	//sprintf(temp, "AT+CIPSTART=%d,\"UDP\",\"%s\",%d,%d,2\r\n", gID++, ip_str, remote_port, local_port);
	sprintf(temp, "AT+CIPSTART=%d,\"UDP\",\"%s\",%d,%d\r\n", gID++, ip_str, remote_port, local_port);
	
	bus_send_string(temp);
	
	msleep(400);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

void ping(u32 ip)
{
	char ip_str[16];
	int timeout = 500;
	int priv_char;
	
	sprintf(ip_str, "%d.%d.%d.%d", ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip &0xff);
	
	sprintf(temp, "AT+PING=\"%s\"\r\n", ip_str);
	
	bus_send_string(temp);
	
	while (timeout--) {	
			char c = wifi_uart_recieve1();
			if (priv_char == 'O' && c == 'K')
				break;
			
			if (priv_char == 'O' && c == 'R')
				break;
			
			priv_char = c;
			
			msleep(10);			
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

s8 send_data_old(u32 ip, u16 src_port, u16 dst_port, char *data, u16 len)
{
	u8 id = get_id(ip, src_port, dst_port);
	s8 ret;
	
	mutex_lock(&lock);
	sprintf(temp, "AT+CIPSEND=%d,%d\r\n", id, len);
	
	bus_send_string(temp);
		
#if 0
	msleep(len >> 2);	
	bus_recieve_string(output);
		
	if (str_include(output, ">") == 0) {
		bus_send(data, len);
		msleep(10);
		ret = str_include(output, "SEND OK");
	} else
		ret = -1;
#else
	msleep(100);
	
	bus_send(data, len);
#endif
		
	mutex_unlock(&lock);
	
	return ret;
}

s8 send_data(u32 ip, u16 src_port, u16 dst_port, char *data, u16 len)
{
	u8 id = get_id(ip, src_port, dst_port);
	OS_ERR err;
	int timeout;
	
	sprintf(temp, "AT+CIPSEND=%d,%d\r\n", id, len);
		
	mutex_lock(&lock);
	
	bus_send_string(temp);
	
	timeout = 500;
	
	while (timeout--) {	
		char c = wifi_uart_recieve1();
		
		if (c == '>')
			break;

		msleep(10);			
	}
	
	bus_send(data, len);
	
	timeout = 500;
	
	while (timeout--) {
		char c = wifi_uart_recieve1();
		
		if (c == 'K')
			break;
		

		msleep(10);			
	}
	
	mutex_unlock(&lock);
	
	return 0;
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

void recv_data(u32 *ip, u16 *port, char *buf, u16 *buf_len)
{
	char tmp[10], c;
	u16 len;
	u8 fd;
	u16 i;
	struct ip_port_map *map;

	while (1) {		
		c = bus_recieve();
		
		if (c == '+') {
			msleep(100);		
		} else if (c == '\0') {
			msleep(100);			
			continue;
		} else			
			continue;
		
		/* read IPD */
		for (i = 0; i < 4; i++)
			tmp[i] = bus_recieve();
		
		if (strncmp(tmp, "IPD,", 4) != 0)
			continue;
		
		/* read fd */
		for (i = 0; i < 2; i++)
			tmp[i] = bus_recieve();
		
		//fd is from '0' to '9'
		fd  = tmp[0] - '0';
		
		/* get length */
		len = get_len(tmp);
		
		for (i = 0; i < len; i++)
			buf[i] = bus_recieve();
		
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
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_ip(char *ip)
{
	sprintf(temp, "AT+CIPSTA=\"%s\"\r\n", ip);
								
	bus_send_string(temp);
	
	msleep(20);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");	
}

s8 set_bound(void)
{
	sprintf(temp, "AT+UART=115200,8,1,0,0\r\n");
								
	bus_send_string(temp);
	
	msleep(20);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");	
}

s8 esp_reset(void)
{
	sprintf(temp, "AT+RST\r\n");
								
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");	
}

char data_i[100];

void send_test(void)
{
	u32 ip = ((u32)192 << 24) | ((u32)168 << 16) | ((u32)1 << 8) | 20;
	u16 src_port;
	u16 port = 8888;
	char data[] = "hello world!";
	u16 len;

	udp_close(0);
	
	if (udp_setup(ip, port, port) < 0)
		err_log("udp_setup");
	
	send_data(ip, port, port, data, strlen(data));
	
	recv_data(&ip, &src_port, data_i, &len);
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
		
#if 1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOA, ENABLE);	
	// download
	GPIO_InitStructure.GPIO_Pin = ESP8266_DOWNLOAD_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(ESP8266_DOWNLOAD, &GPIO_InitStructure);
	
	//rst
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

#else

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC |RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#endif

	//update_esp8266();
	work_esp8266();
	
	reset_esp8266();
	disable_esp8266();
	msleep(100);
	enbale_esp8266();
	
	mutex_init(&lock);
	
#if 0
	GPIO_WriteBit(ESP8266_GPIO4, ESP8266_GPIO4_PIN, Bit_RESET);
	msleep(100);
	
	GPIO_WriteBit(ESP8266_GPIO4, ESP8266_GPIO4_PIN, Bit_SET);
	sleep(1);
	GPIO_WriteBit(ESP8266_GPIO4, ESP8266_GPIO4_PIN, Bit_RESET);
#endif
	//while (1)
		//sleep(1);	
}

#else

#include "includes.h"
#include "helper.h"
#include "bus.h"
#include "mutex.h"


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

s8 str_include(char *buf, char *str)
{	
	u16 i = 0, j = 0, k;

	for (i = 0; buf[i] != '\0'; i++) {
		for (j = 0, k = i; str[j] != '\0' && buf[k] != '\0'; j++, k++) {
			if (buf[k] == str[j])
				continue;
			else
				break;
		}
		
		if (str[j] == '\0')
			return 0;
		
		if (buf[k] == '\0')
			return -1;
	}
	
	return -1;
}

s8 set_mode(u8 mode)
{
	sleep(1);
	sprintf(temp, "AT+CWMODE=%d\r\n", mode);
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_show_ip(int mode)
{
	sprintf(temp, "AT+CIPDINFO=%d\r\n", mode);
	bus_send_string(temp);
		
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 connect_ap(char *id, char *passwd, s8 channel)
{
	uint8_t times = 5;
	int timeout;
	char priv_char;
	
	while (times-- > 0) {	
		sprintf(temp, "AT+CWJAP=\"%s\",\"%s\"\r\n", id, passwd);
		bus_send_string(temp);	

		timeout = 5000;
		
		while (timeout--) {	
			char c = wifi_uart_recieve1();
			if (priv_char == 'O' && c == 'K')
				break;
			
			priv_char = c;
			
			msleep(10);			
		}
		
		bus_recieve_string(output);
		
		if (str_include(output, "OK") == 0)
			return 0;
	}
	return -1;
}

void ping(u32 ip)
{
	char ip_str[16];
	int timeout = 500;
	int priv_char;
	
	sprintf(ip_str, "%d.%d.%d.%d", ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip &0xff);
	
	sprintf(temp, "AT+PING=\"%s\"\r\n", ip_str);
	
	bus_send_string(temp);
	
	while (timeout--) {	
			char c = wifi_uart_recieve1();
			if (priv_char == 'O' && c == 'K')
				break;
			
			if (priv_char == 'O' && c == 'R')
				break;
			
			priv_char = c;
			
			msleep(10);			
		}
	
	msleep(100);
}

s8 set_mac_addr(void)
{
	sprintf(temp, "AT+CIPSTAMAC=\"18:fe:35:98:d3:7b\"\r\n");
	bus_send_string(temp);
		
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_auto_conn(u8 i)
{
	sprintf(temp, "AT+CWAUTOCONN=%d\r\n", i);
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");	
}
s8 close_conn(void)
{
	sprintf(temp, "AT+CWQAP\r\n");
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_RF_power(s8 v)
{
	sprintf(temp, "AT+RFPOWER=%d\r\n", v);
	bus_send_string(temp);
	
	sleep(1);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_ap(char *sid, char *passwd)
{
	sprintf(temp, "AT+CWSAP=\"%s\",\"%s\",5,3,4\r\n", sid, passwd);
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_echo(s8 on)
{
	reset_buffer();
	
	sprintf(temp, "ATE%d\r\n", on);
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_mux(s8 mode)
{
	sprintf(temp, "AT+CIPMUX=%d\r\n", mode);
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");	
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
	
	//sprintf(temp, "AT+CIPSTART=%d,\"UDP\",\"%s\",%d,%d,2\r\n", gID++, ip_str, remote_port, local_port);
	sprintf(temp, "AT+CIPSTART=%d,\"UDP\",\"%s\",%d,%d\r\n", gID++, ip_str, remote_port, local_port);
	
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
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
	OS_ERR err;
	int timeout;
	
	sprintf(temp, "AT+CIPSEND=%d,%d\r\n", id, len);
		
	mutex_lock(&lock);
	
	bus_send_string(temp);
	
	timeout = 100;
	
	while (timeout--) {	
		char c = wifi_uart_recieve1();
		
		if (c == '>')
			break;

		msleep(10);			
	}
	
	bus_send(data, len);
	
	timeout = 100;
	
	while (timeout--) {
		char c = wifi_uart_recieve1();
		
		if (c == 'K')
			break;
		

		msleep(10);			
	}
	
	mutex_unlock(&lock);
	
	return 0;
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

void recv_data(u32 *ip, u16 *port, char *buf, u16 *buf_len)
{
	char tmp[10], c;
	u16 len;
	u8 fd;
	u16 i;
	struct ip_port_map *map;

	while (1) {
		c = bus_recieve();
		
		if (c == '+') {
			msleep(100);		
		} else if (c == '\0') {
			msleep(100);			
			continue;
		} else {
			continue;
		}
		
		/* read IPD */
		for (i = 0; i < 4; i++)
			tmp[i] = bus_recieve();
		
		if (strncmp(tmp, "IPD,", 4) != 0) {
			//mutex_unlock(&lock);
			continue;
		}
		
		/* read fd */
		for (i = 0; i < 2; i++)
			tmp[i] = bus_recieve();
		
		//fd is from '0' to '9'
		fd  = tmp[0] - '0';
		
		/* get length */
		len = get_len(tmp);
		
		for (i = 0; i < len; i++)
			buf[i] = bus_recieve();
		
		//mutex_unlock(&lock);
		
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
	bus_send_string(temp);
	
	msleep(20);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");
}

s8 set_ip(char *ip)
{
	sprintf(temp, "AT+CIPSTA=\"%s\"\r\n", ip);
								
	bus_send_string(temp);
	
	msleep(20);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");	
}

s8 set_bound(void)
{
	sprintf(temp, "AT+UART=115200,8,1,0,0\r\n");
								
	bus_send_string(temp);
	
	msleep(20);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");	
}

s8 esp_reset(void)
{
	sprintf(temp, "AT+RST\r\n");
								
	bus_send_string(temp);
	
	msleep(200);
	
	bus_recieve_string(output);
	
	return str_include(output, "OK");	
}

char data_i[100];

void send_test(void)
{
	u32 ip = ((u32)192 << 24) | ((u32)168 << 16) | ((u32)1 << 8) | 20;
	u16 src_port;
	u16 port = 8888;
	char data[] = "hello world!";
	u16 len;

	udp_close(0);
	
	if (udp_setup(ip, port, port) < 0)
		err_log("udp_setup");
	
	send_data(ip, port, port, data, strlen(data));
	
	recv_data(&ip, &src_port, data_i, &len);
}

void update_esp8266(void)
{
//	GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_RESET); //update pin IO0
//	GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_RESET); //ESP GPIO15
	
	GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_RESET);
}

void work_esp8266(void)
{
	//GPIO_WriteBit(GPIOB, GPIO_Pin_0, Bit_SET); //update pin IO0
	//GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_RESET); //ESP GPIO15
	GPIO_WriteBit(GPIOB, GPIO_Pin_13, Bit_RESET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_12, Bit_SET); //update pin IO0
}

void enbale_esp8266(void)
{
	//GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_SET);
	
	GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
}

void disable_esp8266(void)
{
	//GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);
	GPIO_WriteBit(GPIOB, GPIO_Pin_14, Bit_SET);
}

void reset_esp8266(void)
{
	//msleep(100);
	//GPIO_WriteBit(GPIOA, GPIO_Pin_12, Bit_SET);
	//msleep(100);
	//GPIO_WriteBit(GPIOA, GPIO_Pin_12, Bit_RESET);
	//msleep(300);
	//GPIO_WriteBit(GPIOA, GPIO_Pin_12, Bit_SET);
	//msleep(100);
	
	GPIO_WriteBit(GPIOB, GPIO_Pin_15, Bit_SET);
}

void esp8266_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
		
#if 0
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB |RCC_APB2Periph_GPIOA, ENABLE);	
	// download
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// enable, rst
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_12 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
#endif
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	// update
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// GIO 15
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// enable
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// reset
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	
	
	//update_esp8266();
	work_esp8266();
	
	reset_esp8266();
	disable_esp8266();
	msleep(100);
	enbale_esp8266();	
	
	mutex_init(&lock);
	
	//while(1)
		//sleep(1);	
}


#endif
