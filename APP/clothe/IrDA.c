#include "includes.h"
#include "helper.h"
#include "time.h"
#include "blod.h"
#include "priority.h"
#include "mutex.h"

#ifdef CLOTHE
#define I2C_OP_CODE_GET_INFO	1
#define I2C_OP_CODE_FAYANQIU	2
#define I2C_OP_CODE_GET_SW_VR	3
#define I2C_OP_CODE_GET_HW_VR	4
#define I2C_OP_CODE_SET_LEDS	5

#define I2C_OP_CODE_SET_RED		0X80
#define I2C_OP_CODE_SET_GREEN	0x40
#define I2C_OP_CODE_SET_BLUE	0x20
#define I2C_OP_CODE_SET_YELLOW	0x10

#define RECV_MOD_SA_BASE			0x10
#define CLOTHE_RECEIVE_MODULE_NUMBER	8
#define CLOTHE_RECEIVE_MODULE_BODY_NUMBER	5
#define IRDA_I2C			I2C1
static int team = I2C_OP_CODE_SET_RED;
#define OS_TASK_STACK_SIZE 256

static CPU_STK  TaskStk[OS_TASK_STACK_SIZE];
static OS_TCB TaskStkTCB;
static OS_Q queue;
static struct mutex_lock lock;
static OS_TMR timer[CLOTHE_RECEIVE_MODULE_NUMBER];
static u16 broken_cnt[CLOTHE_RECEIVE_MODULE_NUMBER];

struct  recv_info {
	u16 charcode;
	u8 status;
	u8 revr;
};

struct led_ctl {
	u8 id;
	u8 color;
	u8 time;
	u8 resv;
};

static u32 recv_offline_map;

extern int i2c_Reads(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *ReadBuffer, u16 ReadNumber);
extern int i2c_Writes(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *WriteData, u16 WriteNumber);

#if 1
int IrDA_Reads(u8 slave_addr, u8 op, u8 *ReadBuffer, u16 ReadNumber)
{
	int ret;
	
	ret = i2c_Reads(IRDA_I2C, slave_addr, op, ReadBuffer, ReadNumber);
	
	return ret;
}

int IrDA_Writes(u8 slave_addr, u8 op, u8 *WriteData, u16 WriteNumber)
{
	return i2c_Writes(IRDA_I2C, slave_addr, op, WriteData, WriteNumber);
}

#else

	
int IrDA_cmd(int cmd, int id, u8 *buf, int len)
{
	if (IrDA_Reads((RECV_MOD_SA_BASE + id) << 1, cmd, buf, len) != 0)
		return -1;
	
	return 0;
}
#endif

int IrDA_led(int id, int color, int on)
{
	static u8 led_local[CLOTHE_RECEIVE_MODULE_NUMBER] = {0};
	struct recv_info status;
	int ret;
	
	//if (recv_offline_map & (1 << id))
	//	return -1;
	
	if (on == 1) 
		led_local[id] |= color;
	else
		led_local[id] &= ~color;
	
	ret = IrDA_Reads((RECV_MOD_SA_BASE + id) << 1, I2C_OP_CODE_SET_LEDS | led_local[id], (u8 *)&status, 3);
	
	if (ret != 0)
		broken_cnt[id]++;
	
	if (broken_cnt[id] == 10000) {
		while (1) {
			red_led_on();
			beep_on();
			sleep(1);
			led_off();
			beep_off();
			sleep(1);
		}
	}
	
	return ret;
}

void clothe_led_off(void *p_tmr, void *p_arg)
{
	int side = (int)p_arg + 1;
	int i;
	
	if (side == 1) {
		for (i = 2; i < 5; i++)
			IrDA_led(i, 0xf0, 0);
		
	} else if (side == 2) {
		for (i = 0; i < 2; i++)
			IrDA_led(i, 0xf0, 0);
		
	} else if (side == 3) {
		for (i = 5; i < 8; i++)
			IrDA_led(i, 0xf0, 0);
	}
}

int clothe_led_on_then_off(int side, int color, int time)
{
	OS_ERR err;
	int i;

	if (side == 1) {
		for (i = 2; i < 5; i++)
			IrDA_led(i, color, 1);
		
		OSTmrStart(&timer[0], &err);
		
	} else if (side == 2) {
		for (i = 0; i < 2; i++)
			IrDA_led(i, color, 1);
		
		OSTmrStart(&timer[1], &err);
		
	} else if (side == 3) {
		for (i = 5; i < 8; i++)
			IrDA_led(i, color, 1);
		
		OSTmrStart(&timer[2], &err);
	}	
	
	if (err == OS_ERR_NONE)
		return 0;

	return -1;
}

void clear_receive(void)
{
	int i;
	u8 status;
	int ret = 0;
	struct recv_info info;
	
	for (i = 0; i < CLOTHE_RECEIVE_MODULE_NUMBER; i++)
		while (ret == 0) {
			if (IrDA_Reads((RECV_MOD_SA_BASE + i) << 1, I2C_OP_CODE_GET_INFO, (u8 *)&info, 3) == 0) {
			
				status = info.status;	
				
				switch (status) {
					case 0:					
						break;
					case 1:
						ret = 1;
						break;
				}
			} else
				break;
		}
}

void clothe_led(char *s, int on)
{
	int i;
	int color;
	OS_ERR err;
	
	if (strcmp(s, "red") == 0)
		color = I2C_OP_CODE_SET_RED;
	else if (strcmp(s, "blue") == 0)
		color = I2C_OP_CODE_SET_BLUE;
	else if (strcmp(s, "green") == 0)
		color = I2C_OP_CODE_SET_GREEN;
	else if (strcmp(s, "yellow") == 0)
		color = I2C_OP_CODE_SET_YELLOW;
	else if (strcmp(s, "all") == 0)
		color = 0xf0;
	else
		return;
	
	for (i = 0; i < CLOTHE_RECEIVE_MODULE_NUMBER; i++)
		IrDA_led(i, color, on);
}

int irda_get_shoot_info(u16 *charcode, s8 *head_shoot)
{
	static 	u16 temp_charcode[CLOTHE_RECEIVE_MODULE_NUMBER];
	
	u8 status;
	int ret = -1;
	int i, char_cnt = 0, shoot_cnt = 0;
	struct recv_info info;
	
	for (i = 0; i < CLOTHE_RECEIVE_MODULE_NUMBER; i++) {		
		if (IrDA_Reads((RECV_MOD_SA_BASE + i) << 1, I2C_OP_CODE_GET_INFO, (u8 *)&info, 3) == 0) {
		
			status = info.status;	
			
			switch (status) {
				case 0:
					temp_charcode[i] = info.charcode;
				
					if (i >= CLOTHE_RECEIVE_MODULE_BODY_NUMBER)
						shoot_cnt++;
					
					char_cnt++;
					
					break;
				case 1:
					temp_charcode[i] = 0;
					ret = -1;
					break;
				case 255:
					err_log("cound't support command\n");
			}				
		} else 
			temp_charcode[i] = 0;			
	}
	
	//只打到衣服
	if (char_cnt > 0 && shoot_cnt == 0) {	
		if (temp_charcode[2] != 0) {
			*charcode = temp_charcode[2];
			*head_shoot = 0;
			ret = 1;
		} else if (temp_charcode[3] != 0) {
			*charcode = temp_charcode[3];
			*head_shoot = 0;
			ret = 1;
		} else if (temp_charcode[4] != 0) {
			*charcode = temp_charcode[4];
			*head_shoot = 0;
			ret = 1;
		} else if (temp_charcode[0] != 0) {
			*charcode = temp_charcode[0];
			*head_shoot = 0;
			ret = 2;
		} else if (temp_charcode[1] != 0) {
			*charcode = temp_charcode[1];
			*head_shoot = 0;
			ret = 2;
		}
	} else {
		if (temp_charcode[5] != 0) {
			*charcode = temp_charcode[5];
			*head_shoot = 1;
			ret = 3;
		} else if (temp_charcode[6] != 0) {
			*charcode = temp_charcode[6];
			*head_shoot = 1;
			ret = 3;
		} else if (temp_charcode[7] != 0) {
			*charcode = temp_charcode[7];
			*head_shoot = 1;
			ret = 3;
		} 
	}
	
	if (ret > 0)
		printf("#%04x\r\n", info.charcode);
		
	return ret;
}

void IrDA_init(void)
{
	int i;
	u16 charcode;
	u8 status[4];
	u8 buf[3];
	struct recv_info info;
	OS_ERR err;
	
	for (i = 0; i < CLOTHE_RECEIVE_MODULE_NUMBER; i++) {		
		if (IrDA_Reads((RECV_MOD_SA_BASE + i) << 1, I2C_OP_CODE_GET_INFO, (u8 *)&info, 3) != 0) {
			recv_offline_map |= 1 << i;
		} else if (IrDA_led(i, 0xf0, 1) != 0)
			printf("err\n");
	}
	
	msleep(500);
	
	for (i = 0; i < 8; i++) {			
		IrDA_led(i, 0xf0, 0);
	}
	
	for (i = 0; i < 3; i++)
		OSTmrCreate(&timer[i], "led timer", 5, 0, OS_OPT_TMR_ONE_SHOT, (OS_TMR_CALLBACK_PTR)clothe_led_off, (void *)i, &err);
	
	mutex_init(&lock);
}	
#endif
