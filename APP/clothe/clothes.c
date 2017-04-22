#include "includes.h"
#include "net.h"
#include "esp8266.h"
#include "priority.h"
#include "helper.h"
#include "key.h"
#include "power.h"
#include "time.h"
#include "led.h"
#include "blod.h"

#ifdef CLOTHE

//#define HOST_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)0 << 8) | 102)
static u32 HOST_IP;

#define GUN_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 2)
#define RIFLE_IP	(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 3)
#define LCD_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 5)

#define HOST_PORT			(u16)5135
#define HOST_MSG_PORT		(u16)5999
#define GUN_PORT			(u16)8889
#define RIFLE_PORT			(u16)8890
#define LCD_PORT			(u16)8891

#define OS_RECV_TASK_STACK_SIZE     256
#define OS_HB_TASK_STACK_SIZE   	 256
#define BLOD_MAX					100

enum {
	TEAM_COLOR_RED	=	0,
	TEAM_COLOR_BLUE	=	1,
};

#define CLOTHE_RECEIVE_MODULE_NUMBER	8

enum {
	INIT,
	ACTIVE,
	LIVE,
	DEAD,
	SUPPLY,
};

struct attacked_info_record {
	struct attacked_info info;
	s8 cnt;
};

static u16 g_characode[CLOTHE_RECEIVE_MODULE_NUMBER];
static s8 g_head_shoot[CLOTHE_RECEIVE_MODULE_NUMBER];

static CPU_STK  RecvTaskStk[OS_RECV_TASK_STACK_SIZE];
static OS_TCB RecvTaskStkTCB;

static CPU_STK  HBTaskStk[OS_HB_TASK_STACK_SIZE];
static OS_TCB HBTaskStkTCB;

static char recv_buf[1024];                                                    
static u16 characCode = 0xffff;
static s8 actived;
static volatile s16 blod, bulet;
static u16 packageID = 0;

static struct attacked_info_record att_info;

extern int irda_get_shoot_info(u16 *, s8 *);
extern void key_get_ip_suffix(char *s);
extern void key_get_sn(char *s);
extern s8 key_get_blod(void);
extern s8 key_get_fresh_status(void);
extern u32 get_time(u8 *, u8 *, u8 *);
extern void get_wifi_info(char *id, char *passwd);
extern void clothe_led(char *s, int);

static s8 sendto_host(char *buf, u16 len)
{
	return send_data((u32)HOST_IP, (u16)HOST_PORT, (u16)HOST_PORT, buf, len);
}

static s8 sendto_host_msg(char *buf, u16 len)
{
	return send_data((u32)HOST_IP, (u16)HOST_MSG_PORT, (u16)HOST_MSG_PORT, buf, len);
}

static s8 sendto_gun(char *buf, u16 len)
{
	return send_data(GUN_IP, GUN_PORT, GUN_PORT, buf, len);
}

static s8 sendto_rifle(char *buf, u16 len)
{
	return send_data(RIFLE_IP, RIFLE_PORT, RIFLE_PORT, buf, len);
}

static s8 sendto_lcd(char *buf, u16 len)
{
	return send_data(LCD_IP, LCD_PORT, LCD_PORT, buf, len);
}

static void set_attack_info(u16 *charcode, s8 *head_shoot, u32 time)
{
	int i = 0, j;
	
	INT2CHAR(att_info.info.characCode[att_info.cnt], charcode[i]);
	
	if (head_shoot[i] == 1)
		INT2CHAR(att_info.info.ifHeadShot[att_info.cnt], 1);
	else
		INT2CHAR(att_info.info.ifHeadShot[att_info.cnt], 0);
	
	INT2CHAR(att_info.info.attachTime[att_info.cnt], time);
	
	att_info.cnt++;
	
	if (att_info.cnt == 10)
		att_info.cnt = 0;
}

static void get_attack_info(struct attacked_info *info)
{
	if (att_info.cnt > 0)
		memcpy(info, &att_info.info, sizeof(*info));
}

static int active_request(void)
{
	struct ActiveRequestData data;
	
	memset(&data, '0', sizeof(data));
	
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
	struct ClothesStatusData data;
	
	memset(&data, '0', sizeof(data));
	
	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, CLOTHES_STATUS_TYPE);
	INT2CHAR(data.packageID, packageID++);
	INT2CHAR(data.deviceType, 0);
	INT2CHAR(data.deviceSubType, get_deviceSubType());	
	INT2CHAR(data.lifeLeft, (int)blod);
	get_attack_info(&data.atck_info);
	key_get_sn(data.keySN);
	INT2CHAR(data.PowerLeft, get_power());
	
	upload_lcd(LCD_LIFE, blod);
	msleep(20);
	return sendto_host((char *)&data, sizeof(data));
}

static int power_status_data(void)
{	
	upload_lcd(LCD_PWR_INFO, get_power());
}

static int upload_ip_info(void)
{
	struct sta_ip_change_pkg data;
	
	memset(&data, '0', sizeof(data));
	
	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, IP_CHANGE_TYPE);
	INT2CHAR(data.packageID, packageID++);
	key_get_sn(data.KeySN);
	
	return sendto_host((char *)&data, sizeof(data));	
}

static int upload_heartbeat(void)
{
	struct HeartBeat data;
	
	memset(&data, '0', sizeof(data));
	
	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, HEART_BEAT_TYPE);
	INT2CHAR(data.packageID, packageID++);
	INT2CHAR(data.deviceType, 0);
	INT2CHAR(data.deviceSubType, get_deviceSubType());
	key_get_sn(data.deviceSN);
	
	return sendto_host((char *)&data, sizeof(data));
}

static int work_flag_dipatch_gun(int flag)
{
	struct WorkFlahgDipatch data;
	
	memset(&data, '0', sizeof(data));
	
	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, flag);
	
	return sendto_gun((char *)&data, sizeof(data));
}

static void recv_gun_handler(char *buf, u16 len)
{
	struct ActiveRequestData *data = (void *)buf;
	u32 packTye;
	
	packTye = char2u32_16(data->packTye, sizeof(data->packTye));
	
	if (packTye == ACTIVE_REQUEST_TYPE) {
		struct GunActiveAskData ask_data;
		u16 tmp; 
		
		if (characCode == 0xffff) //æ— æ•ˆçš„
			return;
		
		INT2CHAR(ask_data.transMod, 0);
		INT2CHAR(ask_data.packTye, ACTIVE_RESPONSE_TYPE);
		INT2CHAR(ask_data.packageID, packageID++);
		tmp = characCode + 0x100;
		INT2CHAR(ask_data.characCode, tmp);
		sendto_gun((void *)&ask_data, sizeof(ask_data));
		printf("#to gun%04x\r\n", tmp);
	} else {
		if (packTye == GUN_STATUS_TYPE) {
			int value = CHAR2INT(((struct GunStatusData *)data)->bulletLeft);
			bulet = value;
		}
		
		sendto_host(buf, len);
	}
}

static void recv_rifle_handler(char *buf, u16 len)
{
	struct ActiveRequestData *data = (void *)buf;
	u32 packTye;
	
	packTye = char2u32_16(data->packTye, sizeof(data->packTye));
	
	if (packTye == ACTIVE_REQUEST_TYPE) {
		struct GunActiveAskData ask_data;
		u16 tmp;
		
		INT2CHAR(ask_data.transMod, 0);
		INT2CHAR(ask_data.packTye, ACTIVE_RESPONSE_TYPE);
		INT2CHAR(ask_data.packageID, packageID++);
		tmp = characCode + 0x200;
		INT2CHAR(ask_data.characCode, tmp);
		sendto_rifle((void *)&ask_data, sizeof(ask_data));
	} else
		sendto_host(buf, len);
}

static void recv_host_handler(char *buf, u16 len)
{
	struct ActiveAskData *data = (void *)buf;
	struct MsgPkgResp resp;
	u32 packType;
	
	packType = char2u32_16(data->packTye, sizeof(data->packTye));
	if (packType == ACTIVE_RESPONSE_TYPE) {
		actived = 1;
		characCode = (u16)char2u32_16(data->characCode, sizeof(data->characCode));
		set_time(char2u32_16(data->curTime, sizeof(data->curTime)));
		
		printf("#from host%04x\r\n", characCode);	
	} 
}

static void recv_host_msg_handler(char *buf, u16 len)
{
	struct MsgPkg *data = (void *)buf;
	struct MsgPkgResp resp;
	u32 packType;
	
	packType = char2u32_16(data->packTye, sizeof(data->packTye));
	
	if (packType == MESSAGE_TYPE) {
		int i;
		memcpy(&resp, data, sizeof(struct MsgPkg));
		INT2CHAR(resp.packTye, MESSAGE_RESPONSE_TYPE);
		INT2CHAR(resp.rx_ok, 0);
		sendto_host_msg((char *)&resp, sizeof(resp));
		msleep(20);
		sendto_lcd((char *)data, sizeof(*data));
	}	
}

static void recv_lcd_handler(char *buf, u16 len)
{
	struct ActiveRequestData *data = (void *)buf;
	u32 packTye;
	
	packTye = char2u32_16(data->packTye, sizeof(data->packTye));
	
	if (packTye == ACTIVE_REQUEST_TYPE) {
		struct LcdActiveAskData ask_data;
		u32 tmp;
		
		INT2CHAR(ask_data.transMod, 0);
		INT2CHAR(ask_data.packTye, ACTIVE_RESPONSE_TYPE);
		INT2CHAR(ask_data.packageID, packageID++);
		tmp = get_time(NULL, NULL, NULL);
		INT2CHAR(ask_data.rtc, tmp);
		
		sendto_lcd((char *)&ask_data, sizeof(ask_data));
		msleep(20);
		upload_lcd(LCD_LIFE, blod);
		msleep(20);
		upload_lcd(LCD_GUN_BULLET, bulet);
	} 
}

static void dead(void)
{
	//release_smoke();
	//disable_guns();
}

static void recv_task(void)
{
	u32 ip;
	u16 remote_port;
	u16 len;
	
	while (1) {
		recv_data(&ip, &remote_port, recv_buf, &len);
#if 0
		switch (ip) {
			case HOST_IP:
				recv_host_handler(recv_buf, len);
				break;
			case LCD_IP:
				recv_lcd_handler(recv_buf, len);
				break;
			case GUN_IP:
				recv_gun_handler(recv_buf, len);
				break;
			case RIFLE_IP:
				recv_rifle_handler(recv_buf, len);
				break;
			default:
				sendto_host(recv_buf, len);
				break;			
		}
#endif		
		if (ip == HOST_IP) {
			if (remote_port == HOST_MSG_PORT)
				recv_host_msg_handler(recv_buf, len);
			else
				recv_host_handler(recv_buf, len);
		} else if (ip == LCD_IP)
			recv_lcd_handler(recv_buf, len);
		else if (ip == GUN_IP)
			recv_gun_handler(recv_buf, len);
		else if (ip == RIFLE_IP)
			recv_rifle_handler(recv_buf, len);
		else
			sendto_host(recv_buf, len);
	}
}

static void power_status_task(void)
{	
	while (1) {	
		sleep(5);	
		power_status_data();		
	}
}

static void start_clothe_tasks(void)
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

static void net_init(void)
{
	u8 i;
	char sid[20] = "CSsub", passwd[20] = "12345678", \
		host[20], host_passwd[20], \
	    ip[16] = "192.168.1.";
	
	// ip×îºóÒ»Î»×÷ÎªÒ»¸ö±êÊ¶
	key_get_ip_suffix(&sid[5]);
	
	key_get_ip_suffix(&ip[10]);
	
	HOST_IP = key_get_host_ip();
	
	get_wifi_info(host, host_passwd);
	
	if (set_echo(1) < 0)
		err_log("set_echo");
	
	if  (close_conn() < 0)
		err_log("set_echo");
	
	if (set_auto_conn(0) < 0)
		err_log("set_echo");

	if (set_mode(3) < 0)
		err_log("set_mode");
	
	if (set_dhcp() < 0)
		err_log("set_dhcp");
		
	if (connect_ap(host, host_passwd, 3) < 0)
		err_log("connect_ap");
	
	//if (set_ip(ip) < 0)
	//	err_log("set_ip");
		
	if (set_ap(sid, passwd) < 0)
		err_log("set_ap");
	
	if (set_mux(1) < 0)
		err_log("set_mux");
	
	for (i = 0; i < 5; i++)
		udp_close(i);
	
	if (udp_setup(HOST_IP, HOST_PORT, HOST_PORT) < 0)
		err_log("");
	
	if (udp_setup(HOST_IP, HOST_MSG_PORT, HOST_MSG_PORT) < 0)
		err_log("");
	
	if (udp_setup(GUN_IP, GUN_PORT, GUN_PORT) < 0)
		err_log("");
	
	if (udp_setup(RIFLE_IP, RIFLE_PORT, RIFLE_PORT) < 0)
		err_log("");
	
	if (udp_setup(LCD_IP, LCD_PORT, LCD_PORT) < 0)
		err_log("");
	
	get_ip();
}

void upload_spec_key(u8 *key)
{
	struct sepcial_key tmp_key;
	
	memset(&tmp_key, '0', sizeof(tmp_key));
	
	INT2CHAR(tmp_key.transMod, 0);
	INT2CHAR(tmp_key.packTye, SPECIAL_KEY_TYPE);
	INT2CHAR(tmp_key.packageID, packageID++);
	key_get_sn(tmp_key.keySN);
	memcpy(tmp_key.AkeySN, key, sizeof(tmp_key.AkeySN));
	
	sendto_host((char *)&tmp_key, sizeof(tmp_key));
}

void main_loop(void)
{
	u32 charcode;
	s16 blod_bak;
	int active_retry = 30;
	int ret;
	memset(&att_info, '0', sizeof(att_info));
	att_info.cnt = 0;
	
#if 1	
	
retry:	
	active_retry = 30;
	
	key_init();
	
	blue_led_on();
		
	blod_bak = blod = key_get_blod();
	
	net_init();
		
	start_clothe_tasks();
		
	while (!actived && --active_retry) {
		active_request();
		sleep(2);
	}
	
	if (active_retry == 0)
		goto retry;
	
	upload_ip_info();
#endif
	
	green_led_on();
	
	ok_notice();	
	
	upload_status_data();
	
	//watch_dog_feed_task_init();
	
	while (1) {	
		recheck:		
		if ((ret = irda_get_shoot_info(g_characode, g_head_shoot)) >= 0 && blod > 0) {
			set_attack_info(g_characode, g_head_shoot, get_time(NULL, NULL, NULL));
			
			if (g_head_shoot[0] == 1)
				blod -= 50 ;
			else
				blod -= 10;
			
			if (blod <= 0) {
				blod = 0;
				
				upload_status_data();
				
				work_flag_dipatch_gun(STOP_WORK);
				
				while (blod <= 0) {
					if (key_get_fresh_status()) {
						if (blod <= 0)
							work_flag_dipatch_gun(START_WORK);
						
						blod += key_get_blod();

						ok_notice();
						
						clothe_led("all", 0);
						msleep(200);
						clothe_led("all", 1);
						msleep(200);
						clothe_led("all", 0);

						clear_receive();
						
						break;
					}
					
					clothe_led("all", 1);
					
					msleep(200);
				}
			} else
				clothe_led_on_then_off(ret, 0xf0, 1);
			
			if (ret > 0)
				goto recheck;
		}
#if 1		
		if (key_get_fresh_status()) {
			if (blod <= 0)
				work_flag_dipatch_gun(START_WORK);
			
			blod += key_get_blod();			
		}
		
		if (blod != blod_bak)
			upload_status_data();
		
		blod_bak = blod;
#endif		
		msleep(150);
	}	
}
#endif
