#include "includes.h"
#include "helper.h"
#include "time.h"
#include "blod.h"
#include "priority.h"

#ifdef CLOTHE
#define I2C_OP_CODE_GET_INFO	1
#define I2C_OP_CODE_LED_COLOR	2

#if 0
void IrDA_Reads(I2C_TypeDef *I2C, u8 slave_addr, u8 op, u8 *ReadBuffer, u16 ReadNumber)
{
	i2c_Reads(I2C, slave_addr, op, ReadBuffer, ReadNumber);
}

void IrDA_WriteBytes(I2C_TypeDef *I2C, u8 slave_addr, u8 op, u8 *WriteData, u16 WriteNumber)
{
	
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
		goto restart;
	
	I2C_Send7bitAddress(I2C, slave_addr, I2C_Direction_Transmitter);
	timeout = 72000;
	while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && --timeout);
	if (timeout == 0)
		goto restart;
	
	I2C_SendData(I2C, op);
	timeout = 72000;
	
	while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED)  && --timeout ); 
	if (timeout == 0)
		goto restart;
		
	I2C_GenerateSTART(I2C, ENABLE);
	timeout = 72000;
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_MODE_SELECT) && --timeout);
	if (timeout == 0)
		goto restart;

	I2C_Send7bitAddress(I2C, slave_addr, I2C_Direction_Receiver);
	timeout = 72000;	
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)&& --timeout);
	if (timeout == 0)
		goto restart;
	
	while (ReadNumber) {
		timeout = 7200;
		if (ReadNumber == 1) {
			I2C_AcknowledgeConfig(I2C, DISABLE);  
			I2C_GenerateSTOP(I2C, ENABLE); 
		}

		while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_RECEIVED) && --timeout); 
		if (timeout == 0)
			goto restart;
		//while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_RECEIVED)); 
		*ReadBuffer = I2C_ReceiveData(I2C);
		ReadBuffer++;
		ReadNumber--;
	}
	
	ret = 0;
	
restart:	
	I2C_AcknowledgeConfig(I2C, ENABLE);
	OSSchedUnlock(&err);
	
	return ret;
}

void IrDA_WriteBytes(I2C_TypeDef *I2C, u8 slave_addr, u8 op, u8 *WriteData, u16 WriteNumber)
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
extern s8 get_actived_state(void);
	struct  recv_info {
		u16 charcode;
		u8 status;
		u8 revr;
	};
	
u16 irda_get_shoot_info(void)
{
	u16 charcode;
	u8 status[4];
	int i;
	u8 buf[3];
	struct recv_info info;

	memset(status, 254, sizeof(status));
	
	for (i = 0; i < 4; i++) {
		if (IrDA_Reads(I2C1, 0x50 + (i << 1), I2C_OP_CODE_GET_INFO, (u8 *)&info, 3) == 0) {
		
			status[i] = info.status;	
			
			switch (status[i]) {
				case 0:
					charcode = info.charcode;
					return charcode;
				case 1:
					break;
				case 255:
					err_log("cound't support command\n");
			}				
		}
	}		
	
	return 0;
}

#endif
