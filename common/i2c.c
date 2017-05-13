#include "includes.h"
#include "helper.h"

#define I2C1_RCC               RCC_APB1Periph_I2C1

#define I2C1_GPIO_RCC    	   RCC_APB2Periph_GPIOB
#define I2C1_GPIO       	   GPIOB
#define I2C1_SCL_Pin           GPIO_Pin_8

#define I2C1_SDA_GPIO_RCC      I2C1_GPIO_RCC
#define I2C1_SDA_Pin           GPIO_Pin_9


#define I2C2_RCC               RCC_APB1Periph_I2C2

#define I2C2_GPIO_RCC      		RCC_APB2Periph_GPIOB
#define I2C2_GPIO        	   GPIOB
#define I2C2_SCL_Pin           GPIO_Pin_10

#define I2C2_SDA_GPIO_RCC      I2C2_GPIO_RCC
#define I2C2_SDA_Pin           GPIO_Pin_11

#define udelay(x)	{int ii = 72 * x; do {} while (ii--);}
#define TIMEOUT			7200

static void fix_I2C_busy(I2C_TypeDef *I2C)
{
	GPIO_TypeDef* GPIO;
	u16 scl_pin,sda_pin;
	int times = 100;

	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef I2C_InitStructure;

	I2C_Cmd(I2C, DISABLE);

	if (I2C == I2C2) {
		GPIO = I2C2_GPIO;
		scl_pin = I2C2_SCL_Pin;
		sda_pin = I2C2_SDA_Pin;
	} else {
		GPIO = I2C1_GPIO;
		scl_pin = I2C1_SCL_Pin;
		sda_pin = I2C1_SDA_Pin;
	}

	GPIO_InitStructure.GPIO_Pin    = scl_pin ;
	GPIO_InitStructure.GPIO_Speed  = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_Out_OD;

	GPIO_Init(GPIO, &GPIO_InitStructure);

	while (GPIO_ReadInputDataBit(GPIO, sda_pin) != Bit_SET && --times) {
		GPIO_WriteBit(GPIO, scl_pin, Bit_RESET);
		udelay(10);
		GPIO_WriteBit(GPIO, scl_pin, Bit_SET);
		udelay(10);
	}

	GPIO_WriteBit(GPIO, scl_pin, Bit_RESET);
	udelay(10);
	GPIO_WriteBit(GPIO, scl_pin, Bit_SET);

	GPIO_InitStructure.GPIO_Pin    = scl_pin | sda_pin;
	GPIO_InitStructure.GPIO_Speed  = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_AF_OD;           

	GPIO_Init(GPIO, &GPIO_InitStructure);

	I2C_SoftwareResetCmd(I2C, ENABLE);

	msleep(1);

	I2C_SoftwareResetCmd(I2C, DISABLE);

	I2C_Init(I2C, &I2C_InitStructure);

	I2C_Cmd(I2C, ENABLE);

	I2C_AcknowledgeConfig(I2C, ENABLE);		
}

int i2c_Reads(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *ReadBuffer, u16 ReadNumber)
{
	int timeout;
	int ret = -1;
	OS_ERR err;

	if(ReadNumber == 0)  
		return 0 ;

	OSSchedLock(&err);

	I2C_GenerateSTART(I2C, ENABLE);
	timeout = TIMEOUT;
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_MODE_SELECT) && --timeout);

	if (timeout == 0)
		goto out;

	I2C_Send7bitAddress(I2C, slave_addr, I2C_Direction_Transmitter);
	timeout = TIMEOUT;
	while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && --timeout);
	if (timeout == 0)
		goto out;

	I2C_SendData(I2C, Address);
	timeout = TIMEOUT;

	while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED)  && --timeout ); 
	if (timeout == 0)
		goto out;

	I2C_GenerateSTART(I2C, ENABLE);
	timeout = TIMEOUT;
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_MODE_SELECT) && --timeout);
	if (timeout == 0)
		goto out;

	I2C_Send7bitAddress(I2C, slave_addr, I2C_Direction_Receiver);
	timeout = TIMEOUT;	
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)&& --timeout);
	if (timeout == 0)
		goto out;

	while (ReadNumber) {
		timeout = TIMEOUT;
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

int i2c_Writes(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *WriteData, u16 WriteNumber)
{
	OS_ERR err;
	int timeout;
	int ret = -1;

	OSSchedLock(&err);

	timeout = TIMEOUT;
	while(I2C_GetFlagStatus(I2C, I2C_FLAG_BUSY) && --timeout);

	if (timeout == 0)
		goto out;

	I2C_GenerateSTART(I2C, ENABLE);

	timeout = TIMEOUT;
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_MODE_SELECT) && --timeout);
	if (timeout == 0)
		goto out;

	I2C_Send7bitAddress(I2C, slave_addr, I2C_Direction_Transmitter);
	timeout = TIMEOUT;
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && --timeout);
	if (timeout == 0)
		goto out;

	I2C_SendData(I2C, Address);
	timeout = TIMEOUT;
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && --timeout);
	if (timeout == 0)
		goto out;

	while (WriteNumber--)  {
		timeout = TIMEOUT;
		I2C_SendData(I2C, *WriteData);
		WriteData++;
		while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && --timeout);
		if (timeout == 0)
			goto out;

	}

	I2C_GenerateSTOP(I2C, ENABLE);

	ret = 0;

out:
	OSSchedUnlock(&err);
	return ret;
}

void i2c_init(void)
{	
	RCC_APB1PeriphClockCmd(I2C1_RCC, ENABLE);
	RCC_APB2PeriphClockCmd(I2C1_GPIO_RCC, ENABLE);

	GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef I2C_InitStructure;

	/*********************************I2C1**************************************/
	GPIO_InitStructure.GPIO_Pin    = I2C1_SCL_Pin | I2C1_SDA_Pin;
	GPIO_InitStructure.GPIO_Speed  = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_AF_OD;           

	GPIO_Init(I2C1_GPIO, &GPIO_InitStructure);

	I2C_DeInit(I2C1);

	RCC_APB1PeriphClockCmd(I2C1_RCC, ENABLE);

	I2C_InitStructure.I2C_ClockSpeed          = 100000;    //100KHz I2C
	I2C_InitStructure.I2C_Mode                = I2C_Mode_I2C;   
	I2C_InitStructure.I2C_DutyCycle           = I2C_DutyCycle_2; 
	I2C_InitStructure.I2C_OwnAddress1         = 0x30;     
	I2C_InitStructure.I2C_Ack                 = I2C_Ack_Enable;  
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

	I2C_Init(I2C1, &I2C_InitStructure);

	I2C_Cmd(I2C1, ENABLE);

	I2C_AcknowledgeConfig(I2C1, ENABLE);

	//≈–∂œ «∑Ò «
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))		
		fix_I2C_busy(I2C1);

#if defined(CLOTHE) || defined(GUN)	
	/*********************************I2C2**************************************/
	RCC_APB1PeriphClockCmd(I2C2_RCC, ENABLE);
	RCC_APB2PeriphClockCmd(I2C2_GPIO_RCC, ENABLE);

	GPIO_InitStructure.GPIO_Pin    = I2C2_SCL_Pin | I2C2_SDA_Pin;
	GPIO_InitStructure.GPIO_Speed  = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_AF_OD;           

	GPIO_Init(I2C2_GPIO, &GPIO_InitStructure);

	I2C_DeInit(I2C2);

	I2C_InitStructure.I2C_ClockSpeed          = 100000;    //100KHz I2C
	I2C_InitStructure.I2C_Mode                = I2C_Mode_I2C;   
	I2C_InitStructure.I2C_DutyCycle           = I2C_DutyCycle_2; 
	I2C_InitStructure.I2C_OwnAddress1         = 0x30;     
	I2C_InitStructure.I2C_Ack                 = I2C_Ack_Enable;  
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

	I2C_Init(I2C2, &I2C_InitStructure);

	I2C_Cmd(I2C2, ENABLE);

	I2C_AcknowledgeConfig(I2C2, ENABLE);

	if (I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))
		fix_I2C_busy(I2C2);
#endif
}
