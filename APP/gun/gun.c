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
static s16 local_bulet = 100;
static char g_host[20] = "CSsub", g_host_passwd[20] = "12345678";
	
extern s8 trigger_handle(u16 charcode, int);
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
	INT2CHAR(data.bulletLeft, local_bulet);
	key_get_sn(data.keySN);
	INT2CHAR(data.characCode, characCode);

	INT2CHAR(data.PowerLeft, get_power());
	
	upload_lcd(LCD_GUN_BULLET, local_bulet);
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
		sleep(5);	
		power_status_data();
	}	
}
	
static int net_init(void)
{
	u8 i;
	char host[20] = "CSsub", host_passwd[20] = "12345678", \
	    ip[16];
	int ret = -1;
	
	// ip最后一位作为一个标识
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
	
	if (connect_ap(host, host_passwd, 3) < 0) {
		err_log("connect_ap");
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
	
	return 0;
}

static int net_reinit(void)
{
	u8 i;
	OS_ERR err;
	char host[20] = "CSsub", host_passwd[20] = "12345678", \
	    ip[16];
	
	// ip禺鄢一位胤为一俣要识
	key_get_ip_suffix(&host[5]);
	
	if (strcmp(host, g_host) == 0)
		goto out;
	
	OSTaskSuspend((OS_TCB *)&RecvTaskStkTCB, &err);
	OSTaskSuspend((OS_TCB *)&HBTaskStkTCB, &err);
	
	actived = 0;
	
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
		
	while (!actived) {
		active_request();
		sleep(2);
	}
	
out:
	actived = 1;
	
	return 0;
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
	if (net_reinit() < 0)
		NVIC_SystemReset();
	
	ok_notice();	
	
	local_bulet += key_get_bulet();
		
	upload_status_data();
}

void main_loop(void)
{
	s8 bulet_used_nr;
	s8 i;
	int active_retry = 30;
	s8 bulet_one_bolt = 0;
	s8 mode, status = 0;
	
	blue_led_on();	
	
#if 1
retry:	
	active_retry = 30;
	
	key_init();
	
	local_bulet = key_get_bulet();
	
	blue_led_on();
	
	if (net_init() < 0)
		goto retry;
					
	start_gun_tasks();
	
	while (!actived && --active_retry) {
		active_request();
		sleep(2);
	}
	
	if (active_retry == 0)
		goto retry;
#endif	
	
	upload_status_data();
	
	ok_notice();
	
	//watch_dog_feed_task_init();
	
	while (1) {		
		if (key_get_fresh_status())
				key_insert_handle();
		
		if (actived) {	
			if (check_pull_bolt() > 0) {
				play_bolt();
				
				msleep(100);
				
				if (bulet_one_bolt > 0 && (30 - bulet_one_bolt) > local_bulet)
					bulet_one_bolt += local_bulet;
				else
					bulet_one_bolt = 30;
			}
			
			mode = get_mode();
			
			if (is_single_mode(mode) && bulet_one_bolt > 0) {
				s8 a = trigger_get_status();
					
				switch (a) {
					case 1 :
						if (status == 0) {
							play_bulet();
							
							send_charcode(characCode);
							
							status = 1;
							
							bulet_one_bolt--;
							local_bulet--;
							
							upload_status_data();
							
							msleep(500);
						}
						break;
					case 0 :
						if (status == 1) {
							status =  0;
						}
						break;				
				}
			} else if (is_auto_mode(mode) && bulet_one_bolt > 0) {
				if (trigger_get_status()) {
					wav_play(2);
						
					for (i = 0; i < 4 && bulet_one_bolt > 0; i++) {
						send_charcode(characCode);
						msleep(200);
						bulet_one_bolt--;
						local_bulet--;
					}
					
					upload_status_data();
					
					//msleep(500);
				}
			}
#if 0
					static s8 lianfa = 0;
					
					if (lianfa == 0) {
						wav_play(2);
						for (i = 0; i < 3 && local_bulet > 0; i++) {
							send_charcode(characCode);
							msleep(100);
							
							bulet_one_bolt--;
							local_bulet--;
						}
						sleep(1);
					} else {
						wav_play(3);
					
						for (i = 0; i < 4 && local_bulet > 0; i++) {
							send_charcode(characCode);
							msleep(100);
							
							bulet_one_bolt--;
							local_bulet--;
						}
						
						sleep(1);
					}
					
					lianfa ^= 1;
#endif				
				
			if (local_bulet <= 0) {
				local_bulet = 0;
				actived = 0;				 
			}
			
			if (bulet_one_bolt <= 0)
				bulet_one_bolt = 0;
		}	
		
		if (actived && bulet_one_bolt > 0 && mode != 0)
			green_led_on();
		else
			blue_led_on();			
		
		msleep(100);
	}
#if 0		
		if (actived) {
			s8 bulet_used_nr = 0;
			int status;
			
			is_bolt_on() && (status = trigger_get_status()) > 0) {
				send_charcode(characCode);
				
				if (status == 2)
					wav_play(2);
				else
					wav_play(0);
				
				bulet_used_nr++;
				
				if (bulet_used_nr == local_bulet)
					break;
				
				bulet_one_bolt--;
				local_bulet--;
				
				if (bulet_one_bolt == 0) {
					actived = 0;
			
					reset_bolt();
				}
				
				msleep(100);			
			}
			
			if (bulet_used_nr > 0)				
				upload_status_data();
		}
		
		if (local_bulet <= 0) {
			
			actived = 0;
			
			reset_bolt();
			
			while (key_get_fresh_status() == 0)
				msleep(100);
			
			key_insert_handle();
		}	
					
		msleep(100);
	}
#endif
}
#endif
