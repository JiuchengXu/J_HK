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
#define I2C_RW_OP		3
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

static struct i2c_buf i2c_buf;
static volatile s8 i2c_status;

void i2c_Reads(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *ReadBuffer, u16 ReadNumber)
{	
	OS_ERR err;

	if (ReadNumber <= 0)
		return;
	
	i2c_buf.op = I2C_RW_OP;
	i2c_buf.step = I2C_OP_STEP_1;
	i2c_buf.slave_addr = slave_addr;
	i2c_buf.reg_addr = Address;
	i2c_buf.idx = 0;
	i2c_buf.buf = ReadBuffer;
	i2c_buf.len = ReadNumber;
	
	I2C_ITConfig(I2C, I2C_IT_EVT, ENABLE);
	I2C_ITConfig(I2C, I2C_IT_BUF, ENABLE);
	
	while(I2C_GetFlagStatus(I2C, I2C_FLAG_BUSY));
	
	I2C_GenerateSTART(I2C, ENABLE);
				
	OSSemPend(&i2c_buf.sem, NULL, OS_OPT_PEND_BLOCKING, NULL, &err);
}

void i2c_Writes(I2C_TypeDef *I2C, u8 slave_addr, u8 Address, u8 *WriteData, u16 WriteNumber)
{
	OS_ERR err;

	if (WriteNumber <= 0)
		return;
	
	i2c_buf.op = I2C_WW_OP;
	i2c_buf.step = I2C_OP_STEP_1;
	i2c_buf.slave_addr = slave_addr;
	i2c_buf.reg_addr = Address;
	i2c_buf.idx = 0;
	i2c_buf.buf = WriteData;
	i2c_buf.len = WriteNumber;
	
	I2C_ITConfig(I2C, I2C_IT_EVT, ENABLE);
	I2C_ITConfig(I2C, I2C_IT_BUF, ENABLE);
	
	while(I2C_GetFlagStatus(I2C, I2C_FLAG_BUSY));
	
	I2C_GenerateSTART(I2C, ENABLE);
	
	OSSemPend(&i2c_buf.sem, NULL, OS_OPT_PEND_BLOCKING, NULL, &err);
	
	msleep(5);
}

void I2C1_EV_IRQHandler(void)
{
#define I2C 	I2C1
	CPU_SR_ALLOC();
	
	CPU_INT_DIS(); 
	
	switch (I2C_GetLastEvent(I2C)) {
		case I2C_EVENT_MASTER_MODE_SELECT:
			I2C_AcknowledgeConfig(I2C, ENABLE);
		
			if (i2c_buf.op == I2C_RW_OP) {
				if (i2c_buf.step == I2C_OP_STEP_1)
					I2C_Send7bitAddress(I2C, i2c_buf.slave_addr, I2C_Direction_Transmitter);
				else
					I2C_Send7bitAddress(I2C, i2c_buf.slave_addr, I2C_Direction_Receiver);
				
			} else if (i2c_buf.op == I2C_WW_OP)
				I2C_Send7bitAddress(I2C, i2c_buf.slave_addr, I2C_Direction_Transmitter);
			
			break;
			
		case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED:
			break;
		
		case I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED:
			if (i2c_buf.len == 1) {
				I2C_AcknowledgeConfig(I2C, DISABLE);
				I2C_GenerateSTOP(I2C, ENABLE);
				break;
			}
			
			break;
		
		case I2C_EVENT_MASTER_BYTE_TRANSMITTING:
			if (i2c_buf.op == I2C_WW_OP && i2c_buf.step == I2C_OP_STEP_1) {
				I2C_SendData(I2C, i2c_buf.reg_addr);
				i2c_buf.step = I2C_OP_STEP_2;
				
				break;
			}
			
			if (i2c_buf.op == I2C_WW_OP && i2c_buf.step == I2C_OP_STEP_2) {
				if (i2c_buf.idx < i2c_buf.len)
					I2C_SendData(I2C, i2c_buf.buf[i2c_buf.idx++]);
				
				break;
			}
			
			if (i2c_buf.op == I2C_RW_OP && i2c_buf.step == I2C_OP_STEP_1) {
				I2C_SendData(I2C, i2c_buf.reg_addr);
				i2c_buf.step = I2C_OP_STEP_2;
				
				break;
			}
			
			break;
		case I2C_EVENT_MASTER_BYTE_TRANSMITTED:
			if (i2c_buf.op == I2C_RW_OP)
				I2C_GenerateSTART(I2C, ENABLE);
			else if (i2c_buf.op == I2C_WW_OP) {
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
	
	CPU_INT_EN();
	
#undef I2C 	
}

void i2c_init(void)
{
	OS_ERR err;
	
	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef I2C_InitStructure;
#if 1
	RCC_APB2PeriphClockCmd(I2C1_GPIO_RCC, ENABLE);
	
	GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
		
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
	
#if 1	
	//ÅÐ¶ÏÊÇ·ñÊÇ
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY)) {
		
		GPIO_InitStructure.GPIO_Pin    = I2C1_SCL_Pin ;
		GPIO_InitStructure.GPIO_Speed  = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_Out_OD;
		
		GPIO_Init(I2C1_GPIO, &GPIO_InitStructure);
		
		while (GPIO_ReadInputDataBit(I2C1_GPIO, I2C1_SDA_Pin) != Bit_SET) {
			GPIO_WriteBit(I2C1_GPIO, I2C1_SCL_Pin, Bit_RESET);
			udelay(10);
			GPIO_WriteBit(I2C1_GPIO, I2C1_SCL_Pin, Bit_SET);
			udelay(10);
		}
			
		GPIO_WriteBit(I2C1_GPIO, I2C1_SCL_Pin, Bit_RESET);
		udelay(10);
		GPIO_WriteBit(I2C1_GPIO, I2C1_SCL_Pin, Bit_SET);
		
		GPIO_InitStructure.GPIO_Pin    = I2C1_SCL_Pin | I2C1_SDA_Pin;
		GPIO_InitStructure.GPIO_Speed  = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_AF_OD;           

		GPIO_Init(I2C1_GPIO, &GPIO_InitStructure);
		
		I2C_SoftwareResetCmd(I2C1, ENABLE);
		msleep(1);
		
		I2C_SoftwareResetCmd(I2C1, DISABLE);
		
		I2C_Init(I2C1, &I2C_InitStructure);
	
		I2C_Cmd(I2C1, ENABLE);

		I2C_AcknowledgeConfig(I2C1, ENABLE);	
	}
#endif
		
	OSSemCreate(&i2c_buf.sem, "blod Sem", 0, &err);
	#endif
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
		
	if (I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY)) {
		
		GPIO_InitStructure.GPIO_Pin    = I2C2_SCL_Pin ;
		GPIO_InitStructure.GPIO_Speed  = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_Out_OD;
		
		GPIO_Init(I2C2_GPIO, &GPIO_InitStructure);
		
		while (GPIO_ReadInputDataBit(I2C2_GPIO, I2C2_SDA_Pin) != Bit_SET) {
			GPIO_WriteBit(I2C2_GPIO, I2C2_SCL_Pin, Bit_RESET);
			udelay(10);
			GPIO_WriteBit(I2C2_GPIO, I2C2_SCL_Pin, Bit_SET);
			udelay(10);
		}
			
		GPIO_WriteBit(I2C2_GPIO, I2C2_SCL_Pin, Bit_RESET);
		udelay(10);
		GPIO_WriteBit(I2C2_GPIO, I2C2_SCL_Pin, Bit_SET);
		
		GPIO_InitStructure.GPIO_Pin    = I2C2_SCL_Pin | I2C2_SDA_Pin;
		GPIO_InitStructure.GPIO_Speed  = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_AF_OD;           

		GPIO_Init(I2C2_GPIO, &GPIO_InitStructure);
		
		I2C_SoftwareResetCmd(I2C2, ENABLE);
		
		msleep(1);
		
		I2C_SoftwareResetCmd(I2C2, DISABLE);
		
		I2C_Init(I2C2, &I2C_InitStructure);
	
		I2C_Cmd(I2C2, ENABLE);

		I2C_AcknowledgeConfig(I2C2, ENABLE);	
	}
#endif
}
