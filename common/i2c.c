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

#define I2C_OR_OP		1
#define I2C_OW_OP		2
#define I2C_WR_OP		3
#define I2C_WW_OP		4

#define I2C_OP_STEP_1	1
#define I2C_OP_STEP_2	2

struct i2c_buf {
	volatile u8 op;
	volatile u8 step;
	volatile u8 reg_addr;
	volatile u8 slave_addr;
	u8 *buf;
	volatile u8 idx;
	u16 len;
	OS_SEM sem;
};

static struct i2c_buf i2c1_buf, i2c2_buf;
static volatile s8 i2c_status;

void fix_I2C_busy(I2C_TypeDef *I2C)
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

int i2c_opt(I2C_TypeDef *I2C, u8 Op, u8 slave_addr, u8 Address, u8 *Buf, u16 Len)
{	
	OS_ERR err;
	int retry = 5, timeout;
	struct i2c_buf * i2c_buf;
	
	if (I2C == I2C2)
		i2c_buf = &i2c2_buf;
	else
		i2c_buf = &i2c1_buf;
	
	do { 	
		i2c_buf->op = Op;
		i2c_buf->step = I2C_OP_STEP_1;
		i2c_buf->slave_addr = slave_addr;
		i2c_buf->reg_addr = Address;
		i2c_buf->idx = 0;
		i2c_buf->buf = Buf;
		i2c_buf->len = Len;
		
		timeout = 72000 / 3;
		while(I2C_GetFlagStatus(I2C, I2C_FLAG_BUSY) && --timeout);
		if (timeout == 0) 
			fix_I2C_busy(I2C);
		
		I2C_ITConfig(I2C, I2C_IT_EVT | I2C_IT_BUF, ENABLE);
		//I2C_ITConfig(I2C, I2C_IT_BUF, ENABLE);
					
		I2C_GenerateSTART(I2C, ENABLE);
					
		OSSemPend(&i2c_buf->sem, 100, OS_OPT_PEND_BLOCKING, NULL, &err);
		
		if (err != OS_ERR_NONE) {
			I2C_GenerateSTOP(I2C, ENABLE);
			I2C_ITConfig(I2C, I2C_IT_BUF | I2C_IT_EVT, DISABLE);
		}
			
		
	} while (err != OS_ERR_NONE && --retry);
	
	if (retry == 0 && err != OS_ERR_NONE)
		return -1;
	
	return 0;
}

int i2c_Reads(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *ReadBuffer, u16 ReadNumber)
{
	return i2c_opt(I2C, I2C_WR_OP, slave_addr, Address, ReadBuffer, ReadNumber);
}

int i2c_Writes(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *WriteData, u16 WriteNumber)
{
	int ret = i2c_opt(I2C, I2C_WW_OP, slave_addr, Address, WriteData, WriteNumber);
	
	return ret;
}

static u8 cc[10]= {0,1,2,3,4,5,6,7,8,9};

void i2c_isr_test(void)
{	
	i2c_Writes(I2C1, 0xa6, 0, cc, 8);
	memset(cc, 0, 10);
	
	i2c_Reads(I2C1, 0xa6, 0, cc, 8);
	
	while (1);
}

void I2C1_EV_IRQHandler(void)
{
#define I2C 	I2C1	
	switch (I2C_GetLastEvent(I2C)) {
		case I2C_EVENT_MASTER_MODE_SELECT:		
			if (i2c1_buf.op == I2C_WR_OP) {
				if (i2c1_buf.step == I2C_OP_STEP_1)
					I2C_Send7bitAddress(I2C, i2c1_buf.slave_addr, I2C_Direction_Transmitter);
				else {
					I2C_AcknowledgeConfig(I2C, ENABLE);
					I2C_Send7bitAddress(I2C, i2c1_buf.slave_addr, I2C_Direction_Receiver);
				}
				
			} else 
				I2C_Send7bitAddress(I2C, i2c1_buf.slave_addr, I2C_Direction_Transmitter);
			
			break;
		case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED:
			
		case I2C_EVENT_MASTER_BYTE_TRANSMITTING:
			if (i2c1_buf.op == I2C_WW_OP) {
				if (i2c1_buf.step == I2C_OP_STEP_1) {
					I2C_SendData(I2C, i2c1_buf.reg_addr);
					i2c1_buf.step = I2C_OP_STEP_2;
				} else {
					if (i2c1_buf.idx < i2c1_buf.len)
						I2C_SendData(I2C, i2c1_buf.buf[i2c1_buf.idx++]);
				}
					break;
			} else {
				if (i2c1_buf.step == I2C_OP_STEP_1) {
					I2C_SendData(I2C, i2c1_buf.reg_addr);
					i2c1_buf.step = I2C_OP_STEP_2;
				} else
					I2C_GenerateSTART(I2C, ENABLE);
			}
			
			break;
		case I2C_EVENT_MASTER_BYTE_TRANSMITTED:
			if (i2c1_buf.op == I2C_WR_OP)
				I2C_GenerateSTART(I2C, ENABLE);
			else {
				OS_ERR err;
				
				if (i2c1_buf.idx == i2c1_buf.len) {
					I2C_GenerateSTOP(I2C, ENABLE);
					I2C_ITConfig(I2C, I2C_IT_BUF | I2C_IT_EVT, DISABLE);
					OSSemPost(&i2c1_buf.sem, OS_OPT_POST_ALL, &err);
				}
			}
			
			break;		
		case I2C_EVENT_MASTER_BYTE_RECEIVED:
			i2c1_buf.buf[i2c1_buf.idx++] = I2C_ReceiveData(I2C);
		
			if ((u16)i2c1_buf.idx == (i2c1_buf.len - 1)) {
				I2C_AcknowledgeConfig(I2C, DISABLE);
				I2C_GenerateSTOP(I2C, ENABLE);
				break;
			}
			
			if ((u16)i2c1_buf.idx == i2c1_buf.len) {
				OS_ERR err;
				
				I2C_ITConfig(I2C, I2C_IT_BUF | I2C_IT_EVT, DISABLE);
				OSSemPost(&i2c1_buf.sem, OS_OPT_POST_ALL, &err);	
				
				break;
			}
		break;
	}
	
#undef I2C 	
}

void I2C2_EV_IRQHandler(void)
{
#define I2C 	I2C2
#define i2c_buf		i2c2_buf	
	switch (I2C_GetLastEvent(I2C)) {
		case I2C_EVENT_MASTER_MODE_SELECT:		
			if (i2c_buf.op == I2C_WR_OP) {
				if (i2c_buf.step == I2C_OP_STEP_1)
					I2C_Send7bitAddress(I2C, i2c_buf.slave_addr, I2C_Direction_Transmitter);
				else {
					I2C_AcknowledgeConfig(I2C, ENABLE);
					I2C_Send7bitAddress(I2C, i2c_buf.slave_addr, I2C_Direction_Receiver);
				}
				
			} else 
				I2C_Send7bitAddress(I2C, i2c_buf.slave_addr, I2C_Direction_Transmitter);
			
			break;
		case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED:
			
		case I2C_EVENT_MASTER_BYTE_TRANSMITTING:
			if (i2c_buf.op == I2C_WW_OP) {
				if (i2c_buf.step == I2C_OP_STEP_1) {
					I2C_SendData(I2C, i2c_buf.reg_addr);
					i2c_buf.step = I2C_OP_STEP_2;
				} else {
					if (i2c_buf.idx < i2c_buf.len)
						I2C_SendData(I2C, i2c_buf.buf[i2c_buf.idx++]);
				}
					break;
			} else {
				if (i2c_buf.step == I2C_OP_STEP_1) {
					I2C_SendData(I2C, i2c_buf.reg_addr);
					i2c_buf.step = I2C_OP_STEP_2;
				} else
					I2C_GenerateSTART(I2C, ENABLE);
			}
			
			break;
		case I2C_EVENT_MASTER_BYTE_TRANSMITTED:
			if (i2c_buf.op == I2C_WR_OP)
				I2C_GenerateSTART(I2C, ENABLE);
			else {
				OS_ERR err;
				
				if (i2c_buf.idx == i2c_buf.len) {
					I2C_GenerateSTOP(I2C, ENABLE);
					I2C_ITConfig(I2C, I2C_IT_BUF | I2C_IT_EVT, DISABLE);
					OSSemPost(&i2c_buf.sem, OS_OPT_POST_ALL, &err);
				}
			}
			
			break;		
		case I2C_EVENT_MASTER_BYTE_RECEIVED:
			i2c_buf.buf[i2c_buf.idx++] = I2C_ReceiveData(I2C);
		
			if ((u16)i2c_buf.idx == (i2c_buf.len - 1)) {
				I2C_AcknowledgeConfig(I2C, DISABLE);
				I2C_GenerateSTOP(I2C, ENABLE);
				break;
			}
			
			if ((u16)i2c_buf.idx == i2c_buf.len) {
				OS_ERR err;
				
				I2C_ITConfig(I2C, I2C_IT_BUF | I2C_IT_EVT, DISABLE);
				OSSemPost(&i2c_buf.sem, OS_OPT_POST_ALL, &err);	
				
				break;
			}
		break;
	}
	
#undef I2C 
#undef i2c_buf	
}

void i2c_init(void)
{
	OS_ERR err;
		
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(I2C1_RCC, ENABLE);
	RCC_APB2PeriphClockCmd(I2C1_GPIO_RCC, ENABLE);
	
	GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;		
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		
	NVIC_Init(&NVIC_InitStructure);	
	
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
	
	//ÅÐ¶ÏÊÇ·ñÊÇ
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))		
		fix_I2C_busy(I2C1);
		
	OSSemCreate(&i2c1_buf.sem, "blod Sem", 0, &err);
	OSSemCreate(&i2c2_buf.sem, "blod Sem", 0, &err);

#if defined(CLOTHE) || defined(GUN)	
	/*********************************I2C2**************************************/
	RCC_APB1PeriphClockCmd(I2C2_RCC, ENABLE);
	RCC_APB2PeriphClockCmd(I2C2_GPIO_RCC, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel = I2C2_EV_IRQn;		
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		
	NVIC_Init(&NVIC_InitStructure);
	
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
	
	//i2c_isr_test();
}
