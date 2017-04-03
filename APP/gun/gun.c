#include "includes.h"
#include "net.h"
#include "priority.h"
#include "esp8266.h"
#include "helper.h"
#include "key.h"
#include "power.h"

#ifdef GUN
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
#define BLOD_MAX					100

enum {
	INIT,
	ACTIVE,
	LIVE,
	DEAD,
	SUPPLY,
};

static CPU_STK  RecvTaskStk[OS_RECV_TASK_STACK_SIZE];
static OS_TCB RecvTaskStkTCB;

static CPU_STK  HBTaskStk[OS_HB_TASK_STACK_SIZE];
static OS_TCB HBTaskStkTCB;

static char recv_buf[1024];                                                    
static u16 characCode = 0x1122;
static s8 actived;
static u16 packageID = 0;
	
extern s8 get_buletLeft(void);
extern void set_buletLeft(s8 v);
extern void add_buletLeft(s8 v);
extern void reduce_bulet(void);
extern s8 trigger_handle(u16 charcode);
extern void key_get_ip_suffix(char *s);
extern void key_get_sn(char *s);
extern s8 key_get_fresh_status(void);
extern s8 key_get_blod(void);

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

static int upload_status_data(void)
{
	struct GunStatusData data;
	
	memset(&data, '0', sizeof(data));
	
	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, GUN_STATUS_TYPE);
	INT2CHAR(data.packageID, packageID++);
	INT2CHAR(data.deviceType, 1);
	INT2CHAR(data.deviceSubType, get_deviceSubType());
	
	memcpy(data.deviceSN, "0987654321abcdef", sizeof(data.deviceSN));
	INT2CHAR(data.bulletLeft, get_buletLeft());
	key_get_sn(data.keySN);
	INT2CHAR(data.characCode, characCode);

	INT2CHAR(data.PowerLeft, get_power());
	
	upload_lcd(LCD_GUN_BULLET, get_buletLeft());
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
		actived = 1;
	} else if (packTye == START_WORK)
		actived = 1;
	else if (packTye == STOP_WORK)
		actived = 0;
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
		sleep(20);	
		power_status_data();		
	}	
}
	
static void net_init(void)
{
	u8 i;
	char host[20] = "CSsub", host_passwd[20] = "12345678", \
	    ip[16];
	
	// ip最后一位作为一个标识
	key_get_ip_suffix(&host[5]);
	
	sprintf(ip, "192.168.4.%d", GUN_IP & 0xff);
	
	if (set_auto_conn(0) < 0)
		err_log("set_echo");
	
	if  (close_conn() < 0)
		err_log("set_echo");

	if (set_echo(1) < 0)
		err_log("set_echo");
	
	if (set_mode(1) < 0)
		err_log("set_mode");
	
	if (connect_ap(host, host_passwd, 3) < 0)
		err_log("connect_ap");
	
	if (set_ip(ip) < 0)
		err_log("set_ip");
		
	if (set_mux(1) < 0)
		err_log("set_mux");
	
	for (i = 0; i < 4; i++)
		udp_close(i);
	
	if (udp_setup(HOST_IP, HOST_PORT, HOST_PORT) < 0)
		err_log("");
	
	//if (udp_setup(GUN_IP, GUN_PORT, GUN_PORT) < 0)
	//	err_log("");
	
	if (udp_setup(LCD_IP, LCD_PORT, LCD_PORT) < 0)
		err_log("");
	
	ping(LCD_IP);
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
            (CPU_CHAR *)"heart beat task", 
            (OS_TASK_PTR)power_status_task, 
            (void * )0, 
            (OS_PRIO)OS_TASK_HB_PRIO, 
            (CPU_STK *)&HBTaskStk[0], 
            (CPU_STK_SIZE)OS_HB_TASK_STACK_SIZE/10, 
            (CPU_STK_SIZE)OS_HB_TASK_STACK_SIZE, 
            (OS_MSG_QTY) 0, 
            (OS_TICK) 0, 
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR*)&err);
}

s8 get_actived_state(void)
{
	return actived;
}

static void key_insert_handle(void)
{
	OS_ERR err;
	
	//OSTaskDel((OS_TCB *)&RecvTaskStkTCB, &err);
	//OSTaskDel((OS_TCB *)&HBTaskStkTCB, &err);
	
	OSTaskSuspend((OS_TCB *)&RecvTaskStkTCB, &err);
	//OSTaskSuspend((OS_TCB *)&HBTaskStkTCB, &err);
	
	actived = 0;
	
	net_init();

	start_gun_tasks();
	
	while (!actived) {
		active_request();
		sleep(1);
	}
	
	OSTaskResume((OS_TCB *)&RecvTaskStkTCB, &err);
	//OSTaskResume((OS_TCB *)&HBTaskStkTCB, &err);
}

void main_loop(void)
{
	s8 bulet_bak = get_buletLeft();
	s8 bulet_used_nr;
	s8 i;
	int active_retry = 30;
	
	blue_led_on();	
	
#if 1
retry:	
	active_retry = 30;
	
	key_init();
	
	net_init();
					
	start_gun_tasks();
	
	while (!actived && --active_retry) {
		active_request();
		sleep(2);
	}
	
	if (active_retry == 0)
		goto retry;
#endif	
	
	//set_buletLeft(100);
	
	upload_status_data();
	
	green_led_on();
	
	ok_notice();
	
	//watch_dog_feed_task_init();
	
	while (1) {
		#if 1
		if (key_get_fresh_status())
			key_insert_handle();
		#endif
		
		bulet_used_nr = trigger_handle(characCode);
		
		if (bulet_used_nr > 0) {
			for (i = 0; i < bulet_used_nr; i++)
				reduce_bulet();
			
			upload_status_data();
		}
		
		msleep(100);
	}
}
#endif
