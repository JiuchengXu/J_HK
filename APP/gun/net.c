#include "includes.h"
#include "net.h"
#include "priority.h"
#include "esp8266.h"
#include "helper.h"
#include "key.h"
#include "power.h"

#define HOST_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 1)

#define GUN_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 2)
#define RIFLE_IP	(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 3)
#define LCD_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 5)

#define HOST_PORT			(u16)8889
#define GUN_PORT			(u16)8889
#define RIFLE_PORT			(u16)8890
#define LCD_PORT			(u16)8892

#define OS_RECV_TASK_STACK_SIZE     128
#define OS_HB_TASK_STACK_SIZE   	 64

static CPU_STK  RecvTaskStk[OS_RECV_TASK_STACK_SIZE];
static OS_TCB RecvTaskStkTCB;

static CPU_STK  HBTaskStk[OS_HB_TASK_STACK_SIZE];
static OS_TCB HBTaskStkTCB;

static char recv_buf[1024];                                                    
static u16 characCode = 0x1122;
static u16 packageID = 0;
static char g_host[20] = "CSsub", g_host_passwd[20] = "12345678";

static s8 sendto_host(char *buf, u16 len)
{
	return send_data((u32)HOST_IP, (u16)HOST_PORT, (u16)HOST_PORT, buf, len);
}

static s8 sendto_rifle(char *buf, u16 len)
{
	return send_data(RIFLE_IP, RIFLE_PORT, RIFLE_PORT, buf, len);
}

static s8 sendto_lcd(char *buf, u16 len)
{
	return send_data(LCD_IP, LCD_PORT, LCD_PORT, buf, len);
}

static int active_request(void)
{
	struct ActiveRequestData data;

	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, ACTIVE_REQUEST_TYPE);
	key_get_sn(data.keySN);
	INT2CHAR(data.packageID, packageID++);

	return sendto_host((char *)&data, sizeof(data));
}

static int upload_lcd(u8 msg_type, u16 value)
{
	struct LcdStatusData data;

	memset(&data, '0', sizeof(data));

	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, LCD_MSG);

	INT2CHAR(data.msg_type, msg_type);
	INT2CHAR(data.msg_value, value);

	sendto_lcd((char *)&data, sizeof(data));
}

static int get_deviceSubType(void)
{
	return 0;
}

int upload_status_data(int bulet)
{
	struct GunStatusData data;

	memset(&data, '0', sizeof(data));

	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, GUN_STATUS_TYPE);
	INT2CHAR(data.packageID, packageID++);
	INT2CHAR(data.deviceType, 1);
	INT2CHAR(data.deviceSubType, get_deviceSubType());

	//memcpy(data.deviceSN, "0987654321abcdef", sizeof(data.deviceSN));
	INT2CHAR(data.bulletLeft, bulet);
	key_get_sn(data.keySN);
	INT2CHAR(data.characCode, characCode);

	INT2CHAR(data.PowerLeft, get_power());

	upload_lcd(LCD_GUN_BULLET, bulet);
	msleep(20);

	return sendto_host((char *)&data, sizeof(data));
}

static int power_status_data(void)
{	
	upload_lcd(LCD_PWR_INFO, get_power());
}

static int upload_heartbeat(void)
{
	struct HeartBeat data;

	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, HEART_BEAT_TYPE);
	INT2CHAR(data.packageID, packageID++);
	INT2CHAR(data.deviceType, 1);
	INT2CHAR(data.deviceSubType, get_deviceSubType());
	key_get_sn(data.deviceSN);

	return sendto_host((char *)&data, sizeof(data));
}

static void recv_rifle_handler(char *buf, u16 len)
{
	sendto_host(buf, len);
}

static void recv_host_handler(char *buf, u16 len)
{
	struct ActiveAskData *data = (void *)buf;
	u32 packTye;

	packTye = char2u32_16(data->packTye, sizeof(data->packTye));

	if (packTye == ACTIVE_RESPONSE_TYPE) {
		characCode = (u16)char2u32_16(data->characCode, sizeof(data->characCode));
		status_set_actived();
	} else if (packTye == START_WORK)
		status_set_actived();
	else if (packTye == STOP_WORK)
		status_set_non_actived();
}

static u16 get_characCode(void)
{
	return characCode;
}

static void recv_lcd_handler(char *buf, u16 len)
{
	sendto_host(buf, len);
}

static void recv_task(void)
{
	u32 ip;
	u16 remote_port;
	u16 len;

	while (1) {
		recv_data(&ip, &remote_port, recv_buf, &len);
		switch (ip) {
			case HOST_IP:
				recv_host_handler(recv_buf, len);
				break;
			case LCD_IP:
				recv_lcd_handler(recv_buf, len);
				break;
			case RIFLE_IP:
				recv_rifle_handler(recv_buf, len);
				break;
			default:
				sendto_host(recv_buf, len);
				break;			
		}
	}
}

static void power_status_task(void)
{
	while (1) {	
		sleep(5);	
		power_status_data();
	}	
}

static void start_gun_tasks(void)
{
	OS_ERR err;

	OSTaskCreate((OS_TCB *)&RecvTaskStkTCB, 
			(CPU_CHAR *)"net reciv task", 
			(OS_TASK_PTR)recv_task, 
			(void * )0, 
			(OS_PRIO)OS_TASK_RECV_PRIO, 
			(CPU_STK *)&RecvTaskStk[0], 
			(CPU_STK_SIZE)OS_RECV_TASK_STACK_SIZE/10, 
			(CPU_STK_SIZE)OS_RECV_TASK_STACK_SIZE, 
			(OS_MSG_QTY) 0, 
			(OS_TICK) 0, 
			(void *)0,
			(OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
			(OS_ERR*)&err);	

	OSTaskCreate((OS_TCB *)&HBTaskStkTCB, 
			(CPU_CHAR *)"power status task", 
			(OS_TASK_PTR)power_status_task, 
			(void * )0, 
			(OS_PRIO)OS_TASK_POWER_PRIO, 
			(CPU_STK *)&HBTaskStk[0], 
			(CPU_STK_SIZE)OS_HB_TASK_STACK_SIZE/10, 
			(CPU_STK_SIZE)OS_HB_TASK_STACK_SIZE, 
			(OS_MSG_QTY) 0, 
			(OS_TICK) 0, 
			(void *)0,
			(OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
			(OS_ERR*)&err);
}

int net_init(void)
{
	u8 i;
	char host[20] = "CSsub", host_passwd[20] = "12345678", \
			 ip[16];
	int ret = -1;
	int timeout = 5;
	int active_retry = 30;

	// ip×îºóÒ»Î»×÷ÎªÒ»¸ö±êÊ¶
	key_get_ip_suffix(&host[5]);

	strcpy(g_host, host);

	sprintf(ip, "192.168.4.%d", GUN_IP & 0xff);

	if (set_auto_conn(0) < 0) {
		err_log("set_echo");
		return -1;
	}

	if  (close_conn() < 0) {
		err_log("set_echo");
		return -1;
	}

	if (set_echo(1) < 0) {
		err_log("set_echo");
		return -1;
	}

	if (set_mode(1) < 0) {
		err_log("set_mode");
		return -1;
	}

	while (1) {
		if (connect_ap(host, host_passwd, 3) < 0) {
			err_log("connect_ap");
			//return -1;
			timeout--;
		} else
			break;
		
		if (timeout == 0)
			return -1;
	}

	if (set_ip(ip) < 0) {
		err_log("set_ip");
		return -1;
	}

	if (set_mux(1) < 0) {
		err_log("set_mux");
		return -1;
	}

	for (i = 0; i < 4; i++)
		udp_close(i);

	if (udp_setup(HOST_IP, HOST_PORT, HOST_PORT) < 0)
		err_log("");

	//if (udp_setup(GUN_IP, GUN_PORT, GUN_PORT) < 0)
	//	err_log("");

	if (udp_setup(LCD_IP, LCD_PORT, LCD_PORT) < 0)
		err_log("");

	ping(LCD_IP);

	start_gun_tasks();

	while (!status_get_actived() && --active_retry) {
		active_request();
		sleep(2);
	}

	if (active_retry == 0)
		return -1;

	return 0;
}

int net_reinit(void)
{
	u8 i;
	OS_ERR err;
	char host[20] = "CSsub", host_passwd[20] = "12345678", \
			 ip[16];

	// ipØ®Û³Ò»Î»Ø·ÎªÒ»Ù¶ÒªÊ¶
	key_get_ip_suffix(&host[5]);

	if (strcmp(host, g_host) == 0)
		goto out;

	OSTaskSuspend((OS_TCB *)&RecvTaskStkTCB, &err);
	OSTaskSuspend((OS_TCB *)&HBTaskStkTCB, &err);

	status_set_non_actived();

	sprintf(ip, "192.168.4.%d", GUN_IP & 0xff);

	if (set_auto_conn(0) < 0)
		err_log("set_echo");

	if  (close_conn() < 0)
		err_log("set_echo");

	if (set_echo(1) < 0)
		err_log("set_echo");

	if (set_mode(1) < 0)
		err_log("set_mode");

	if (connect_ap(host, host_passwd, 3) < 0) {
		err_log("connect_ap");
		return -1;
	}

	if (set_ip(ip) < 0)
		err_log("set_ip");

	if (set_mux(1) < 0)
		err_log("set_mux");

	for (i = 0; i < 4; i++)
		udp_close(i);

	if (udp_setup(HOST_IP, HOST_PORT, HOST_PORT) < 0)
		err_log("");

	if (udp_setup(LCD_IP, LCD_PORT, LCD_PORT) < 0)
		err_log("");

	ping(LCD_IP);

	OSTaskResume((OS_TCB *)&RecvTaskStkTCB, &err);
	OSTaskResume((OS_TCB *)&HBTaskStkTCB, &err);

	while (!status_get_actived()) {
		active_request();
		sleep(2);
	}

out:
	status_set_actived();

	return 0;
}



