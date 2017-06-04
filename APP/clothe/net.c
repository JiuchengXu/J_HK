#include "includes.h"
#include "net.h"
#include "helper.h"
#include "esp8266.h"
#include "priority.h"

static u32 HOST_IP;

#define GUN_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 2)
#define RIFLE_IP	(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 3)
#define LCD_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 5)

#define HOST_PORT			(u16)5135
#define HOST_MSG_PORT			(u16)5999
#define GUN_PORT			(u16)8889
#define RIFLE_PORT			(u16)8890
#define LCD_PORT			(u16)8891


#define OS_RECV_TASK_STACK_SIZE     128
#define OS_HB_TASK_STACK_SIZE   	 64
#define OS_PWR_TASK_STACK_SIZE		OS_HB_TASK_STACK_SIZE

static CPU_STK  RecvTaskStk[OS_RECV_TASK_STACK_SIZE];
static OS_TCB RecvTaskStkTCB;

static CPU_STK  HBTaskStk[OS_HB_TASK_STACK_SIZE];
static OS_TCB HBTaskStkTCB;

static char recv_buf[1024];                                                    
static u16 characCode = 0xffff;
static s8 actived;
static volatile s16 blod, bulet;
static u16 packageID = 0;

static int offline_mode = 0;


static s8 sendto_host(char *buf, u16 len)
{
	if (offline_mode == 1)
		return 0;

	return send_data((u32)HOST_IP, (u16)HOST_PORT, (u16)HOST_PORT, buf, len);
}

static s8 sendto_host_msg(char *buf, u16 len)
{
	if (offline_mode == 1)
		return 0;

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

	return sendto_lcd((char *)&data, sizeof(data));
}

static int get_deviceSubType(void)
{
	return 0;
}

int upload_status_data(int blod)
{
	struct ClothesStatusData data;

	memset(&data, '0', sizeof(data));

	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, CLOTHES_STATUS_TYPE);
	INT2CHAR(data.packageID, packageID++);
	INT2CHAR(data.deviceType, 0);
	INT2CHAR(data.deviceSubType, get_deviceSubType());	
	INT2CHAR(data.lifeLeft, blod);
	INT2CHAR(data.PowerLeft, get_power());

	get_attack_info(&data.atck_info);
	key_get_sn(data.keySN);

	upload_lcd(LCD_LIFE, blod);
	msleep(20);
	return sendto_host((char *)&data, sizeof(data));
}

int power_status_data(void)
{	
	upload_lcd(LCD_PWR_INFO, get_power());
}

int upload_ip_info(void)
{
	struct sta_ip_change_pkg data;

	memset(&data, '0', sizeof(data));

	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, IP_CHANGE_TYPE);
	INT2CHAR(data.packageID, packageID++);
	key_get_sn(data.KeySN);

	return sendto_host((char *)&data, sizeof(data));	
}

int upload_heartbeat(void)
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

int work_flag_dipatch_gun(int flag)
{
	struct WorkFlahgDipatch data;

	memset(&data, '0', sizeof(data));

	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, flag);

	return sendto_gun((char *)&data, sizeof(data));
}

static void wait_for_response(void)
{
	int active_retry = 6;	
	
	while (offline_mode == 0 && !actived && --active_retry) {
		active_request();
		err_log("retry %d\r\n", active_retry);
		sleep(2);
	}

	if (active_retry == 0)
		offline_mode = 1;
}

static void recv_gun_handler(char *buf, u16 len)
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
		tmp = characCode + 0x100;
		INT2CHAR(ask_data.characCode, tmp);
		sendto_gun((void *)&ask_data, sizeof(ask_data));
		err_log("#to gun%04x\r\n", tmp);
	} else {
		if (packTye == GUN_STATUS_TYPE) {
			int value = CHAR2INT(((struct GunStatusData *)data)->bulletLeft);
			bulet = value;

			err_log("bulet value is %d\r\n", bulet);
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

		err_log("#from host%04x\r\n", characCode);	
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

		ask_data.mode = offline_mode == 1? '1' : '0';

		sendto_lcd((char *)&ask_data, sizeof(ask_data));
		msleep(20);
		upload_lcd(LCD_LIFE, get_blod());
		msleep(20);
		upload_lcd(LCD_GUN_BULLET, bulet);
	} 
}

void recv_task(void)
{
	u32 ip;
	u16 remote_port;
	u16 len;

	while (1) {
		recv_data(&ip, &remote_port, recv_buf, &len);
		
		if (ip == HOST_IP) {
			if (remote_port == HOST_MSG_PORT)
				recv_host_msg_handler(recv_buf, len);
			else
				recv_host_handler(recv_buf, len);
		} else if (ip == LCD_IP)
			recv_lcd_handler(recv_buf, len);
		else if (ip == GUN_IP) {
			err_log("receive gun\r\n");
			recv_gun_handler(recv_buf, len);
		} else if (ip == RIFLE_IP)
			recv_rifle_handler(recv_buf, len);
		else
			sendto_host(recv_buf, len);
	}
}

void power_status_task(void)
{	
	int mul = 0;
	
	while (1) {	
		sleep(5);
		power_status_data();
		
		if (mul == 10) {
			upload_heartbeat();
			mul = 0;
		} else
			mul++;			
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

void net_init(void)
{
	u8 i;
	char sid[20] = "CSsub", passwd[20] = "12345678", \
			host[20], host_passwd[20], \
			ip[16] = "192.168.1.";

	// ip最后一位作为一个标识
	key_get_ip_suffix(&sid[5]);

	key_get_ip_suffix(&ip[10]);

	HOST_IP = key_get_host_ip();

	get_wifi_info(host, host_passwd);

	if (set_echo(0) < 0)
		err_log("set_echo");

	if  (close_conn() < 0)
		err_log("set_echo");

	if (set_auto_conn(0) < 0)
		err_log("set_echo");

	if (set_mode(3) < 0)
		err_log("set_mode");

	if (set_dhcp() < 0)
		err_log("set_dhcp");

	//if (set_ip(ip) < 0)
	//	err_log("set_ip");

	if (set_ap(sid, passwd) < 0)
		err_log("set_ap");

	if (set_mux(1) < 0)
		err_log("set_mux");

	for (i = 0; i < 5; i++)
		udp_close(i);

	if (connect_ap(host, host_passwd, 3) < 0) {
		err_log("connect_ap %s %s  work in offline\r\n", host, host_passwd);
		offline_mode = 1;
	} else {
		if (udp_setup(HOST_IP, HOST_MSG_PORT, HOST_MSG_PORT) < 0)
			err_log("");

		if (udp_setup(HOST_IP, HOST_PORT, HOST_PORT) < 0)
			err_log("");
	}

	if (udp_setup(GUN_IP, GUN_PORT, GUN_PORT) < 0)
		err_log("");

	if (udp_setup(RIFLE_IP, RIFLE_PORT, RIFLE_PORT) < 0)
		err_log("");

	if (udp_setup(LCD_IP, LCD_PORT, LCD_PORT) < 0)
		err_log("");

	ping(HOST_IP);

	get_ip();
	
	start_clothe_tasks();
	
	wait_for_response();
	
	upload_ip_info();
}

