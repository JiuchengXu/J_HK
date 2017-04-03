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

#if 0
int IrDA_Reads(u8 slave_addr, u8 op, u8 *ReadBuffer, u16 ReadNumber)
{
	int ret;
	
	mutex_lock(&lock);
	ret = i2c_Reads(IRDA_I2C, slave_addr, op, ReadBuffer, ReadNumber);
	mutex_unlock(&lock);
	
	return ret;
}

int IrDA_Writes(u8 slave_addr, u8 op, u8 *WriteData, u16 WriteNumber)
{
	return i2c_Writes(IRDA_I2C, slave_addr, op, WriteData, WriteNumber);
}

#else
int IrDA_Reads(u8 slave_addr, u8 op, u8 *ReadBuffer, u16 ReadNumber)
{
	int timeout;
	int ret = -1;
	OS_ERR err;
	
	I2C_TypeDef *I2C = IRDA_I2C;
	
	if(ReadNumber == 0)  
		return 0 ;
	
	OSSchedLock(&err);
	
	I2C_GenerateSTART(I2C, ENABLE);
	timeout = 72000;
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_MODE_SELECT) && --timeout);

	if (timeout == 0)
		goto out;
	
	I2C_Send7bitAddress(I2C, slave_addr, I2C_Direction_Transmitter);
	timeout = 72000;
	while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && --timeout);
	if (timeout == 0)
		goto out;
	
	I2C_SendData(I2C, op);
	timeout = 72000;
	
	while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED)  && --timeout ); 
	if (timeout == 0)
		goto out;
		
	I2C_GenerateSTART(I2C, ENABLE);
	timeout = 72000;
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_MODE_SELECT) && --timeout);
	if (timeout == 0)
		goto out;

	I2C_Send7bitAddress(I2C, slave_addr, I2C_Direction_Receiver);
	timeout = 72000;	
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)&& --timeout);
	if (timeout == 0)
		goto out;
	
	while (ReadNumber) {
		timeout = 7200;
		if (ReadNumber == 1) {
			I2C_AcknowledgeConfig(I2C, DISABLE);  
			I2C_GenerateSTOP(I2C, ENABLE); 
		}

		while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_RECEIVED) && --timeout); 
		if (timeout == 0)
			goto out;
		//while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_RECEIVED)); 
		*ReadBuffer = I2C_ReceiveData(I2C);
		ReadBuffer++;
		ReadNumber--;
	}
	
	ret = 0;
	
out:	
	I2C_AcknowledgeConfig(I2C, ENABLE);
	OSSchedUnlock(&err);
	
	return ret;
}

void IrDA_Writes(I2C_TypeDef *I2C, u8 slave_addr, u8 op, u8 *WriteData, u16 WriteNumber)
{
	while(I2C_GetFlagStatus(I2C, I2C_FLAG_BUSY));

	I2C_GenerateSTART(I2C, ENABLE);
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_MODE_SELECT));

	I2C_Send7bitAddress(I2C, slave_addr, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	I2C_SendData(I2C, op);
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

	while (WriteNumber--)  {
		I2C_SendData(I2C, *WriteData);
		WriteData++;
		while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	}

	I2C_GenerateSTOP(I2C, ENABLE);
}

	
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
	
	if (recv_offline_map & (1 << id))
		return -1;
	
	if (on == 1) 
		led_local[id] |= color;
	else
		led_local[id] &= ~color;
		
	ret = IrDA_Reads((RECV_MOD_SA_BASE + id) << 1, I2C_OP_CODE_SET_LEDS | led_local[id], (u8 *)&status, 3);
	
	return ret;
}

int clothe_led_on_then_off(int id, int color, int time)
{
	OS_ERR err;
	struct led_ctl *tmp;
	
	if (recv_offline_map & (1 << id))
		return -1;

	tmp = malloc(sizeof(*tmp));
	
	if (tmp == NULL) {
		err_log("NO memory for clothe_led_on_then_off\n");
		return -1;
	}
	
	tmp->id = id;
	tmp->color = color;
	tmp->time = time;
	
	OSQPost(&queue, (void *)tmp, sizeof(*tmp), OS_OPT_POST_FIFO, &err);

	if (err == OS_ERR_NONE)
		return 0;

	return -1;
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

	team = color;
}

int irda_get_shoot_info(u16 *charcode, s8 *head_shoot)
{
	static 	u16 temp_charcode[CLOTHE_RECEIVE_MODULE_NUMBER];
	
	u8 status;
	int ret = 0;
	int i, char_cnt = 0, shoot_cnt = 0;
	struct recv_info info;
	
	for (i = 0; i < CLOTHE_RECEIVE_MODULE_NUMBER; i++) {
		if (recv_offline_map & (1 << i)) {
			temp_charcode[i] = 0;
			
			continue;
		}
		
		if (IrDA_Reads((RECV_MOD_SA_BASE + i) << 1, I2C_OP_CODE_GET_INFO, (u8 *)&info, 3) == 0) {
		
			status = info.status;	
			
			switch (status) {
				case 0:
					temp_charcode[i] = info.charcode;
				
					if (i >= CLOTHE_RECEIVE_MODULE_BODY_NUMBER)
						shoot_cnt++;
					
					ret = 1;
					
					char_cnt++;
					
					break;
				case 1:
					temp_charcode[i] = 0;
					break;
				case 255:
					err_log("cound't support command\n");
			}				
		} else 
			recv_offline_map |= 1 << i;
			
	}
	
	//只打到衣服
	if (char_cnt > 0 && shoot_cnt == 0) {	
		if (temp_charcode[1] != 0) {
			if (temp_charcode[0] != 0 && temp_charcode[2] == 0) {
				//打到a边
				*charcode = temp_charcode[0];
				*head_shoot = 0;
				clothe_led_on_then_off(0, team, 2);
			} else if (temp_charcode[0] !=0 && temp_charcode[2] != 0) {
				//打到中间
				*charcode = temp_charcode[1];
				*head_shoot = 0;
				clothe_led_on_then_off(1, team, 2);
			} else if (temp_charcode[0] ==0 && temp_charcode[2] != 0) {
				//打到b边
				*charcode = temp_charcode[2];
				*head_shoot = 0;
				clothe_led_on_then_off(2, team, 2);
			} else {
				//打到中间
			}		
		} else if (temp_charcode[0] != 0 || temp_charcode[2] != 0) {
			if (temp_charcode[0] != 0 && temp_charcode[2] == 0) {
				//打到a边
				*charcode = temp_charcode[0];
				*head_shoot = 0;
				clothe_led_on_then_off(0, team, 2);
				
			} else if (temp_charcode[0] ==0 && temp_charcode[2] != 0) {
				//打到b边
				
				*charcode = temp_charcode[2];
				*head_shoot = 0;
				clothe_led_on_then_off(2, team, 2);
			}
		} else {			
			if (temp_charcode[3] != 0 && temp_charcode[4] == 0) {
				//打到a边
				*charcode = temp_charcode[3];
				*head_shoot = 0;
				clothe_led_on_then_off(3, team, 2);
			} else if (temp_charcode[3] ==0 && temp_charcode[4] != 0) {
				//打到b边
				*charcode = temp_charcode[4];
				*head_shoot = 0;
				clothe_led_on_then_off(4, team, 2);
			} else {
				//打到a边
				*charcode = temp_charcode[3];
				*head_shoot = 0;
				clothe_led_on_then_off(3, team, 2);
			}
		}
	} else if (shoot_cnt > 0) { //打到衣服和头
		if (temp_charcode[6] != 0) {
			//前头
			*charcode = temp_charcode[6];
			*head_shoot = 1;
			clothe_led_on_then_off(6, team, 2);
		} else if (temp_charcode[5] != 0) {
			//后头
			*charcode = temp_charcode[5];
			*head_shoot = 1;
			clothe_led_on_then_off(5, team, 2);
		} else {
			//上头
			*charcode = temp_charcode[7];
			*head_shoot = 1;
			clothe_led_on_then_off(7, team, 2);
		}	
	}
		
	return ret;
}

static volatile int started_flag = 0;

static void clothe_led_ctrl(void *data)
{
	OS_ERR err;
	struct led_ctl *tmp;
	OS_MSG_SIZE size;
	
	while (1) {
		started_flag = 1;
		
		tmp = OSQPend(&queue, 0, OS_OPT_PEND_BLOCKING, &size, NULL, &err);
		
		if (tmp == 0)
			continue;
		
		if (err == OS_ERR_NONE && size == sizeof(*tmp)) {
			IrDA_led(tmp->id, tmp->color, 1);
			if (tmp->time == 0)
				continue;
			sleep(tmp->time);
			
			IrDA_led(tmp->id, tmp->color, 0);
		}
		
		free((void *)tmp);

	}
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
	
	sleep(1);
	
	for (i = 0; i < 8; i++) {			
		IrDA_led(i, 0xf0, 0);
	}
	
#if 0	
	for (i = 0; i < CLOTHE_RECEIVE_MODULE_NUMBER; i++)
		IrDA_led(i, 0x80, 1);
	
	sleep(1);
	
	for (i = 0; i < CLOTHE_RECEIVE_MODULE_NUMBER; i++)
		IrDA_led(i, 0x00, 0);
#endif
	
	OSQCreate(&queue, "led queue", 1000, &err);
	
	mutex_init(&lock);
	
    OSTaskCreate((OS_TCB *)&TaskStkTCB, 
            (CPU_CHAR *)"led control", 
            (OS_TASK_PTR)clothe_led_ctrl, 
            (void * )0, 
            (OS_PRIO)8, 
            (CPU_STK *)&TaskStk[0], 
            (CPU_STK_SIZE)OS_TASK_STACK_SIZE/10, 
            (CPU_STK_SIZE)OS_TASK_STACK_SIZE, 
            (OS_MSG_QTY) 0, 
            (OS_TICK) 0, 
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR*)&err);	

	while (!started_flag)
		msleep(100);

#if 0	
	for (i = 0; i < CLOTHE_RECEIVE_MODULE_NUMBER; i++)
		clothe_led_on_then_off(i, 0x80, 1);
		
	while (1)
		sleep(1);
#endif
	
}	
#endif
