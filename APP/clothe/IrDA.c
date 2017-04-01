#include "includes.h"
#include "helper.h"
#include "time.h"
#include "blod.h"
#include "priority.h"

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
#define IRDA_I2C			I2C1
static u8 tmp_led[CLOTHE_RECEIVE_MODULE_NUMBER];

struct  recv_info {
	u16 charcode;
	u8 status;
	u8 revr;
};

static u32 recv_offline_map;

extern int i2c_Reads(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *ReadBuffer, u16 ReadNumber);
extern int i2c_Writes(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *WriteData, u16 WriteNumber);

#if 1
int IrDA_Reads(u8 slave_addr, u8 op, u8 *ReadBuffer, u16 ReadNumber)
{
	return i2c_Reads(IRDA_I2C, slave_addr, op, ReadBuffer, ReadNumber);
}

int IrDA_Writes(u8 slave_addr, u8 op, u8 *WriteData, u16 WriteNumber)
{
	return i2c_Writes(IRDA_I2C, slave_addr, op, WriteData, WriteNumber);
}

#else
int IrDA_Reads(I2C_TypeDef *I2C, u8 slave_addr, u8 op, u8 *ReadBuffer, u16 ReadNumber)
{
	int timeout;
	int ret = -1;
	OS_ERR err;
	
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
#endif
	
int IrDA_cmd(int cmd, int id, u8 *buf, int len)
{
	if (IrDA_Reads((RECV_MOD_SA_BASE + id) << 1, cmd, buf, len) != 0)
		return -1;
	
	return 0;
}

int IrDA_led(int id, int color)
{
	if (recv_offline_map & (1 << id))
		return -1;
	u8 ret;
	struct recv_info status;
	
	ret = IrDA_Reads((RECV_MOD_SA_BASE + id) << 1, I2C_OP_CODE_SET_LEDS | color, (u8 *)&status, 3);
	
	return ret;
}

void IrDA_test(void);

int irda_get_shoot_info(u16 *charcode, s8 *head_shoot)
{
	u8 status;
	int ret = 0;
	int i;
	struct recv_info info;
	u16 temp_charcode;
	s8 temp_head_shoot;
	
	for (i = 0; i < CLOTHE_RECEIVE_MODULE_NUMBER; i++) {
		if (recv_offline_map & (1 << i)) {
			charcode[i] = 0;
			head_shoot[i] = 0;
			
			continue;
		}
		
		if (IrDA_Reads((RECV_MOD_SA_BASE + i) << 1, I2C_OP_CODE_GET_INFO, (u8 *)&info, 3) == 0) {
		
			status = info.status;	
			
			switch (status) {
				case 0:
					charcode[i] = info.charcode;
					IrDA_led(i, 0xf5);
					if (i >= 5)
						head_shoot[i] = 1;//Ѭͷ
					else
						head_shoot[i] = 0;
					
					ret = 1;
					break;
				case 1:
					charcode[i] = 0;
					head_shoot[i] = 0;
					break;
				case 255:
					err_log("cound't support command\n");
			}				
		} else 
			recv_offline_map |= 1 << i;
			
	}
	
	return ret;
}

void IrDA_init(void)
{
	int i;
	u16 charcode;
	u8 status[4];
	u8 buf[3];
	struct recv_info info;
	
	for (i = 0; i < CLOTHE_RECEIVE_MODULE_NUMBER; i++) {		
		if (IrDA_Reads((RECV_MOD_SA_BASE + i) << 1, I2C_OP_CODE_GET_INFO, (u8 *)&info, 3) != 0) {
			recv_offline_map |= 1 << i;
		} else if (IrDA_led(i, I2C_OP_CODE_SET_RED) != 0)
			printf("err\n");
	}
	
	sleep(1);
	
	for (i = 0; i < 8; i++) {			
		IrDA_led(i, 0x00);
	}
}

void IrDA_test(void)
{
	int i;
	struct recv_info info;
	int ret;

	restart:	
	for (i = 0; i < 8; i++) {
		if (IrDA_led(i, 0xf0) != 0)
				printf("err\n");
		
		//sleep(1);
#if 0		
		if (IrDA_led(i, I2C_OP_CODE_SET_GREEN) != 0)
				printf("err\n");
		
		sleep(1);
		
		if (IrDA_led(i, I2C_OP_CODE_SET_YELLOW) != 0)
				printf("err\n");
		
		sleep(1);
		
		if (IrDA_led(i, I2C_OP_CODE_SET_BLUE) != 0)
				printf("err\n");
		
		sleep(1);
		
		if (i == 7)
			i = 0;
#endif		
	}
	
	sleep(1);
	
	for (i = 0; i < 8; i++) {
		if (IrDA_led(i, 0x00) != 0)
				printf("err\n");
	}
	
	sleep(1);
	
	goto restart;
	
	while (1);
}

#endif
