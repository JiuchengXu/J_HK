#include "includes.h"
#include "net.h"
#include "priority.h"
#include "esp8266.h"
#include "helper.h"
#include "key.h"
#include "power.h"
#include "malloc.h"
#include "GUI.h"
#include "GUI.h"


#ifdef LCD
#define HOST_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 1)

#define GUN_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 2)
#define RIFLE_IP	(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 3)
#define LCD_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 5)

#define HOST_PORT			(u16)8891
#define GUN_PORT			(u16)8892
#define RIFLE_PORT			(u16)8890
#define LCD_PORT			(u16)8891

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

static CPU_STK  progbarTaskStk[128];
static OS_TCB progbarTaskStkTCB;

static char recv_buf[1024];                                                    
static s8 actived;

static u16 packageID = 0;
static s16 life_left = 0, bulet_left = 0;
	
extern s8 get_buletLeft(void);
extern void set_buletLeft(s8 v);
extern void add_buletLeft(s8 v);
extern void reduce_bulet(void);

extern void lcd_display_line_16bpp(int x, int y, u16 * p, int xsize, int ysize);
extern int spi_flash_get_data(void * p, const U8 ** ppData, unsigned NumBytes, U32 Off);
extern int get_keyboard_value(void);
extern void key_get_ip_suffix(char *s);


extern void pic_preload(void);
extern void show_background(void);
extern void show_home(void);
extern void upiterm_show(int);
extern void ProgBarShow(int);

static s8 sendto_host(char *buf, u16 len)
{
	return send_data((u32)HOST_IP, (u16)HOST_PORT, (u16)HOST_PORT, buf, len);
}

#if 1
static s8 sendto_rifle(char *buf, u16 len)
{
	return send_data(RIFLE_IP, RIFLE_PORT, RIFLE_PORT, buf, len);
}


static s8 sendto_lcd(char *buf, u16 len)
{
	return send_data(LCD_IP, LCD_PORT, LCD_PORT, buf, len);
}
#endif

static int active_request(void)
{
	struct ActiveRequestData data;
	
	memset(&data, '0', sizeof(data));
	
	INT2CHAR(data.transMod, 0);
	INT2CHAR(data.packTye, ACTIVE_REQUEST_TYPE);
	INT2CHAR(data.packageID, packageID++);
	
	return sendto_host((char *)&data, sizeof(data));
}

static void recv_host_handler1(char *buf, u16 len)
{
	struct GunStatusData *data = (void *)buf;
	struct ClothesStatusData *data1 = (void *)buf;	
	u32 packTye;
	
	packTye = char2u32(data->packTye, sizeof(data->packTye));
	
	if (packTye == CLOTHES_STATUS_TYPE) {
		life_left = (s16)char2u32(data1->lifeLeft, sizeof(data1->lifeLeft));
	} else if (packTye == GUN_STATUS_TYPE) {
		bulet_left = (s16)char2u32(data->bulletLeft, sizeof(data->bulletLeft));
	
	}	
}

static void recv_host_handler(char *buf, u16 len)
{
	struct LcdStatusData *data = (void *)buf;
	u32 packType;
	int msg_type, msg_value;
	
	packType = CHAR2INT(data->packTye);
	
	if (packType == LCD_MSG) {
		msg_type = (int)CHAR2INT(data->msg_type);
		msg_value = (int)CHAR2INT(data->msg_value);
		
		switch (msg_type) {
			case LCD_GUN_BULLET :
				bulet_left = msg_value;
				break;
			case LCD_LIFE :
				life_left = msg_value;
				break;
		}
	} else if (packType == ACTIVE_RESPONSE_TYPE) {
		struct LcdActiveAskData *data = (void *)buf;

		set_time(char2u32(data->rtc, sizeof(data->rtc)));
		actived = 1;
	}
}

static void recv_gun_handler(char *buf, u16 len)
{
	struct LcdStatusData *data = (void *)buf;
	u32 packType;
	int msg_type, msg_value;
	
	packType = CHAR2INT(data->packTye);
	
	if (packType == LCD_MSG) {
		msg_type = (int)CHAR2INT(data->msg_type);
		msg_value = (int)CHAR2INT(data->msg_value);
		
		switch (msg_type) {
			case LCD_GUN_BULLET :
				bulet_left = msg_value;
				break;
			case LCD_LIFE :
				life_left = msg_value;
				break;
		}
	}
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
			case GUN_IP:
				recv_gun_handler(recv_buf, len);
				break;
			default:
				sendto_host(recv_buf, len);
				break;			
		}
	}
}

s8 get_actived_state(void)
{
	return actived;
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
}

static int prog_val = 0;

static void net_init(void)
{
	u8 i;
		
	char host[20] = "CSsub", host_passwd[20] = "12345678", \
	    ip[16];
		
	// ip最后一位作为一个标识
	key_get_ip_suffix(&host[5]);
	
	sprintf(ip, "192.168.4.%d", LCD_IP & 0xff);
	
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
	
	prog_val = 50;
	
	if (set_ip(ip) < 0)
		err_log("set_ip");
	
	if (set_mux(1) < 0)
		err_log("set_mux");
	
	for (i = 0; i < 4; i++)
		udp_close(i);
	
	if (udp_setup(HOST_IP, HOST_PORT, HOST_PORT) < 0)
		err_log("");
	
	if (udp_setup(GUN_IP, GUN_PORT, GUN_PORT) < 0)
		err_log("");
	
	start_gun_tasks();
	
	prog_val = 80;
	
	ping(GUN_IP);
	
	prog_val = 110;
	
	while (!actived) {
		active_request();
		sleep(1);
	}
	
	prog_val = 120;
}

u8 blod[100], bulet[100];

void start_net_init_task(void)
{
	OS_ERR err;
		
    OSTaskCreate((OS_TCB *)&progbarTaskStkTCB, 
            (CPU_CHAR *)"net reciv task", 
            (OS_TASK_PTR)net_init, 
            (void * )0, 
            (OS_PRIO)OS_TASK_RECV_PRIO, 
            (CPU_STK *)&progbarTaskStk[0], 
            (CPU_STK_SIZE)128/10, 
            (CPU_STK_SIZE)128, 
            (OS_MSG_QTY) 0, 
            (OS_TICK) 0, 
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR*)&err);		
}

void main_loop(void)
{
	int bulet_left_bak, life_left_bak;
	int need_reflash = 0;
	int keyboard_bak = 1;
	int tmp_progress = 0;
	
	GUI_Init();
	
	GUI_SetBkColor(GUI_BLACK);
	
	GUI_Clear();
	
	GUI_Delay(100);
	
	pic_preload();
	
	show_background();
		
	backlight_on();
	
	GUI_Delay(100);
	
	ProgBarInit();
				
	key_init();
	
	start_net_init_task();
	
	while (tmp_progress < 110) {
		tmp_progress += 10;
		if (tmp_progress > prog_val)
			ProgBarShow(tmp_progress);
		else {
			tmp_progress = prog_val;
			ProgBarShow(tmp_progress);
		}
		
		GUI_Delay(1000);
	}
	
	ProgBarDelete();
	
	GUI_Delay(100);
	
	show_home();
	
	ok_notice();
			
#if 1			
	while (1) {
		int keyboard = get_keyboard_value();
		
		if (keyboard != keyboard_bak)
			need_reflash = 1;
		else
			need_reflash = 0;
		
		keyboard_bak = keyboard;
		
		switch (keyboard) {
			case 1 :
				if (bulet_left_bak != bulet_left) {
					set_ammo(bulet_left);
					bulet_left_bak = bulet_left;
					
					need_reflash = 1;
				}
				
				if (life_left != life_left_bak) {
					set_blod(life_left);
					life_left_bak = life_left;
					
					need_reflash = 1;					
				}
				
				if (need_reflash == 1)
					show_home();
				
				break;
		
			case 2:
				if (need_reflash == 1)
					show_task();
				
				break;
			case 3:
				if (need_reflash == 1)
					show_msg();
				
				break;
			case 4:
				break;
			default:
				
				break;
		}
		
		//upiterm_show(100);
		
		msleep(100);
	}
#endif
}
#endif
