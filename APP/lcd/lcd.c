#include "includes.h"
#include "net.h"
#include "priority.h"
#include "esp8266.h"
#include "helper.h"
#include "key.h"
#include "power.h"
#include "malloc.h"
#include "GUI.h"
#include "display.h"

#ifdef LCD
#define HOST_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 1)

#define GUN_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 2)
#define RIFLE_IP	(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 3)
#define LCD_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)4 << 8) | 5)

#define HOST_PORT			(u16)8891
#define GUN_PORT			(u16)8892
#define RIFLE_PORT			(u16)8890
#define LCD_PORT			(u16)8891

#define OS_RECV_TASK_STACK_SIZE     256
#define OS_HB_TASK_STACK_SIZE   	256
#define ITERM_TASK_STACK_SIZE		1024

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

static OS_TCB iterm_task_tcb;
static CPU_STK iterm_task_stk[ITERM_TASK_STACK_SIZE];

static char recv_buf[1024];                                                    
static s8 actived;

static u16 packageID = 0;
static s16 life_left = 0, bulet_left = 0, clothe_power = 0, gun_power = 0;

static int need_reflash = 0;

static int offline_mode;

extern void lcd_display_line_16bpp(int x, int y, u16 * p, int xsize, int ysize);
extern int spi_flash_get_data(void * p, const U8 ** ppData, unsigned NumBytes, U32 Off);
extern int get_keyboard_value(void);
extern void key_get_ip_suffix(char *s);

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
	
	packTye = char2u32_16(data->packTye, sizeof(data->packTye));
	
	if (packTye == CLOTHES_STATUS_TYPE) {
		life_left = (s16)char2u32_16(data1->lifeLeft, sizeof(data1->lifeLeft));
	} else if (packTye == GUN_STATUS_TYPE) {
		bulet_left = (s16)char2u32_16(data->bulletLeft, sizeof(data->bulletLeft));	
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
				set_ammo(bulet_left);
				need_update();
				break;
			case LCD_LIFE :
				life_left = msg_value;
				set_blod(life_left);
				need_update();
				break;
			case LCD_PWR_INFO:
				clothe_power = msg_value;
				break;
		}
	} else if (packType == ACTIVE_RESPONSE_TYPE) {
		struct LcdActiveAskData *data = (void *)buf;

		set_time(char2u32_16(data->rtc, sizeof(data->rtc)));
		actived = 1;
		
		if (data->mode == '1')
			enable_offline(1);
		else
			enable_offline(0);
		
		err_log("work in offline\r\n");
		
	} else if (packType == MESSAGE_TYPE) {
		struct MsgPkg *msg = (void *)buf;
		struct statistic_info info;
		
		switch (msg->msg_type[0]) {
			case 'a':				
				info.bekillCount = CHAR2INT(msg->bekillCount);
				info.headBeShootCount = CHAR2INT(msg->headBeShootCount);
				info.headShootCount = CHAR2INT(msg->headShootCount);
				info.killCount = CHAR2INT(msg->killCount);
				info.myTeamBeKillCount = CHAR2INT(msg->myTeamBeKillCount);
				info.myTeamKillCount = CHAR2INT(msg->myTeamKillCount);
				
				set_killed(info.bekillCount);
				set_kill(info.killCount);
				set_enemy(info.myTeamBeKillCount);
				set_our(info.myTeamKillCount);
				set_headshot(info.headShootCount);
				set_headshoted(info.headBeShootCount);
			
				printf("killed %d kill %d score %d live %d headshot %d headshoted %d\r\n",
						info.bekillCount, info.killCount, info.myTeamBeKillCount,
						info.myTeamKillCount, info.headShootCount, info.headBeShootCount);
			
				need_update();
				break;
			case '0':
				
			case '1':
				save_msg_id(char2u32_16(msg->msg_type, sizeof(msg->msg_type)));
				break;
		}
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
				set_ammo(bulet_left);
				need_update();
				break;
			case LCD_LIFE :
				life_left = msg_value;
				break;
			case LCD_PWR_INFO:
				gun_power = msg_value;
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

static volatile int prog_val = 1;

static void key_insert_handle(void)
{
	NVIC_SystemReset();
}

static int net_init(void)
{
	u8 i;
		
	char host[20] = "CSsub", host_passwd[20] = "12345678", \
	    ip[16];
	int timeout = 5;
		
	// ip最后一位作为一个标识
	key_get_ip_suffix(&host[5]);
	
	sprintf(ip, "192.168.4.%d", LCD_IP & 0xff);
	
	if (set_auto_conn(0) < 0)
		err_log("set_echo");
	
	prog_val = 10;
	
	if  (close_conn() < 0)
		err_log("set_echo");
	
	prog_val = 20;

	if (set_echo(1) < 0)
		err_log("set_echo");
	
	prog_val = 30;
	
	if (set_mode(1) < 0)
		err_log("set_mode");
	
	prog_val = 40;
			
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
	
	return 0;
}

u8 blod[100], bulet[100];

static void update_iterm_task(void)
{
	struct iterm_info info = {-1};
	
	while (1) {
		get_time(&info.hour, &info.mini, &info.sec);
		
		info.clothe_pwr = clothe_power;
		info.gun_pwr = gun_power;
		info.lcd_pwr = get_power();
		info.main_bulet = bulet_left;
		info.sub_bulet= 0;
		
		upiterm_show(&info);
		sleep(1);
	}
}

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

	OSTaskCreate((OS_TCB *)&iterm_task_tcb, 
			(CPU_CHAR *)"iterm task", 
			(OS_TASK_PTR)update_iterm_task, 
			(void * )0, 
			(OS_PRIO)OS_TASK_TIMER_PRIO, 
			(CPU_STK *)&iterm_task_stk[0], 
			(CPU_STK_SIZE)ITERM_TASK_STACK_SIZE/10, 
			(CPU_STK_SIZE)ITERM_TASK_STACK_SIZE, 
			(OS_MSG_QTY) 0, 
			(OS_TICK) 0, 
			(void *)0,
			(OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
			(OS_ERR*)&err);					
}

void main_loop(void)
{
	int bulet_left_bak, life_left_bak;
	int keyboard_bak = 1;
	int tmp_progress = 1;
	int active_retry = 30;
	int menu_idx = 1;
	
	GUI_Init();

	GUI_SetBkColor(GUI_BLACK);
	
	GUI_Clear();
	
	lcd_trunoff_backlight_countdown();
	
	read_spi_flash_header();
	
	XBF_font_init();
		
	GUI_Delay(100);	
	
	display_key();	
	
	pic_preload();
		
#if 1
retry:
	active_retry = 100;
	tmp_progress = 0;
	
	key_init();
			
	GUI_Delay(100);
	
	show_background();
	
	ProgBarInit();
	
	start_net_init_task();
	
	while (1) {
		if (tmp_progress == prog_val) {
			active_retry--;			
			if (active_retry == 0)
				NVIC_SystemReset();
		} else {
			tmp_progress += 2;
			ProgBarShow(tmp_progress);					
		}			
		
		if (tmp_progress == 120)
			break;
		
		GUI_Delay(100);
	}
	
	ProgBarDelete();
	
	GUI_Delay(100);
#endif	
	
	show_home();
	
	ok_notice();
	
	//watch_dog_feed_task_init();
						
	while (1) {
		int key_val = get_keyboard_value();
		
		if (key_val) {
			menu_idx = key_val;
			lcd_trunoff_backlight_countdown();
			err_log("press %d\r\n", menu_idx);
			need_update();
		}
		
		if (menu_idx > 0) {
			display_menu(menu_idx);
		}
		
		if (key_get_fresh_status())
			key_insert_handle();
		
		msleep(100);
	}
}
#endif
