#include "includes.h"
#include "helper.h"

#define I2C                   I2C2

/* ADX345的I2C地址 */
#define ADX345_ADDR        0xA6   //ADX345的I2C地址

/* ADXL345的指令表 */
#define ADX_DEVID          0x00   //器件ID
#define ADX_DATA_FORMAT    0x31   //数据格式控制
#define ADX_BW_RATE        0x2C   //数据速率及功率模式控制
#define ADX_POWER_CTL      0x2D   //省电特性控制
#define ADX_INT_ENABLE     0x2E   //中断使能控制
#define ADX_OFSX           0x1E   //敲击阈值
#define ADX_OFSY           0x1F   //X轴偏移
#define ADX_OFSZ           0x20   //Y轴偏移


#define ADX345_DelayMs(x)     {msleep(x);}  //延时函数

/****************************************************************************
* Function Name  : ADX345_WriteReg
* Description    : 设置ADX345寄存器
* Input          : addr：寄存器地址
*                * dat：吸入数据
* Output         : None
* Return         : None
****************************************************************************/

static int8_t ADX345_WriteReg(uint8_t addr, uint8_t dat)
{
	I2C_GenerateSTART(I2C, ENABLE);
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_MODE_SELECT)); 


	I2C_Send7bitAddress(I2C, ADX345_ADDR, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); 


	I2C_SendData(I2C, addr);
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));


	I2C_SendData(I2C, dat);
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED));


	I2C_GenerateSTOP(I2C, ENABLE);  
    return 0;   
}

/****************************************************************************
* Function Name  : ADX345_ReadReg
* Description    : 读取ADX345寄存器
* Input          : addr：读取地址
* Output         : None
* Return         : 读取到的8位数据
****************************************************************************/

static uint8_t ADX345_ReadReg(uint8_t addr)
{
	uint8_t ret;
	
	while(I2C_GetFlagStatus(I2C,I2C_FLAG_BUSY));


	I2C_AcknowledgeConfig(I2C, ENABLE);


	I2C_GenerateSTART(I2C,ENABLE);
	while(!I2C_CheckEvent(I2C,I2C_EVENT_MASTER_MODE_SELECT));   

	I2C_Send7bitAddress(I2C, ADX345_ADDR, I2C_Direction_Transmitter);
	while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));


	I2C_SendData(I2C, addr);
	while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); 

	I2C_GenerateSTART(I2C, ENABLE);
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_MODE_SELECT));


	I2C_Send7bitAddress(I2C, ADX345_ADDR, I2C_Direction_Receiver);  
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));


	I2C_AcknowledgeConfig(I2C, DISABLE);  
	I2C_GenerateSTOP(I2C,ENABLE); 


	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_RECEIVED)); 
	
	ret = I2C_ReceiveData(I2C);
	
	I2C_AcknowledgeConfig(I2C, ENABLE);

    return ret;
}

static uint8_t ADX345_ReadReg_M(uint8_t addr, uint8_t *buf, uint8_t len)
{
	uint8_t i;
	
	if (len <= 0)
		return 0;
	
	while(I2C_GetFlagStatus(I2C,I2C_FLAG_BUSY));


	I2C_AcknowledgeConfig(I2C, ENABLE);


	I2C_GenerateSTART(I2C,ENABLE);
	while(!I2C_CheckEvent(I2C,I2C_EVENT_MASTER_MODE_SELECT));   

	I2C_Send7bitAddress(I2C, ADX345_ADDR, I2C_Direction_Transmitter);
	while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	I2C_SendData(I2C, addr);
	while (!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); 

	I2C_GenerateSTART(I2C, ENABLE);
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_MODE_SELECT));

	I2C_Send7bitAddress(I2C, ADX345_ADDR, I2C_Direction_Receiver);  
	while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

	for (i = 0; i < len; i++) {
		while(!I2C_CheckEvent(I2C, I2C_EVENT_MASTER_BYTE_RECEIVED)); 
		
		buf[i] = I2C_ReceiveData(I2C);
	}

	I2C_AcknowledgeConfig(I2C, DISABLE);  
	I2C_GenerateSTOP(I2C,ENABLE); 

	I2C_AcknowledgeConfig(I2C, ENABLE);

	return len;
}

/****************************************************************************
* Function Name  : ADX345_ReadXYZ
* Description    : 读取X,Y,Z的AD值
* Input          : xValue：X轴的保存地址
*                * yValue：Y轴的保存地址
*                * zValue：Z轴的保存地址
* Output         : None
* Return         : 0：读取成功。0xFF：读取失败
****************************************************************************/

static int8_t ADX345_ReadXYZ(int16_t *xValue, int16_t *yValue, int16_t *zValue)
{
	uint8_t readValue[6];
	
	ADX345_ReadReg_M(0x32, readValue, sizeof(readValue));
	
    /* 处理读取到的数据 */
	*xValue = (uint16_t)(readValue[1] << 8) + readValue[0]; 	    
	*yValue = (uint16_t)(readValue[3] << 8) + readValue[2]; 	    
	*zValue = (uint16_t)(readValue[5] << 8) + readValue[4]; 	   

   return 0;
}

/****************************************************************************
* Function Name  : ADX345_Init
* Description    : 初始化ADX345，并核对ADX的ID
* Input          : None
* Output         : None
* Return         : 0：成功。0xFF：失败
****************************************************************************/

int ADX345_Init(void)
{
    if(ADX345_ReadReg(ADX_DEVID) == 0xE5)
    {
        ADX345_WriteReg(ADX_DATA_FORMAT, 0x2B);//13位全分辨率,输出数据右对齐,16g量程 
		ADX345_WriteReg(ADX_BW_RATE, 0x0A);	  //数据输出速度为100Hz
		ADX345_WriteReg(ADX_POWER_CTL, 0x28);  //链接使能,测量模式
		ADX345_WriteReg(ADX_INT_ENABLE, 0x00); //不使用中断		 
	 	ADX345_WriteReg(ADX_OFSX, 0x00);       //敲击阀值
		ADX345_WriteReg(ADX_OFSY, 0x00);       //X轴偏移
		ADX345_WriteReg(ADX_OFSZ, 0x00);       //Y轴偏移
        return 0;
    }
   
    return 0xFF; //返回初始化失败
}

/****************************************************************************
* Function Name  : ADX345_Adjust
* Description    : ADX345进行校正。
* Input          : None
* Output         : None
* Return         : None
****************************************************************************/

void ADX345_Adjust(void)
{
    int32_t offx = 0, offy = 0, offz = 0;
    int16_t xValue, yValue, zValue;
    uint8_t i;

	ADX345_WriteReg(ADX_POWER_CTL, 0x00);	 //先进入休眠模式.
	ADX345_DelayMs(100);
	ADX345_Init(); 
    ADX345_DelayMs(20);
    
    /* 读取十次数值 */
    for(i=0; i<10; i++)
    {
        ADX345_ReadXYZ(&xValue, &yValue, &zValue);
        offx += xValue;
        offy += yValue;
        offz += zValue;
        ADX345_DelayMs(10);   //才样频率在100HZ，10ms采样一次最好         
    }
    
    /* 求出平均值 */
    offx /= 10;
    offy /= 10;
    offz /= 10;
    
    /* 全分辨率下，每个输出LSB为3.9 mg或偏移寄存器LSB的四分之一，所以除以4 */
    xValue = -(offx / 4);
	yValue = -(offy / 4);
	zValue = -((offz - 256) / 4);
    
    /* 设置偏移量 */
	ADX345_WriteReg(ADX_OFSX, xValue);
	ADX345_WriteReg(ADX_OFSY, yValue);
	ADX345_WriteReg(ADX_OFSZ, zValue); 
            
}

/****************************************************************************
* Function Name  : ADX_GetXYZData
* Description    : 读取ADX的XYZ轴的值（进行过数据处理）
* Input          : xValue：X轴的保存地址
*                * yValue：Y轴的保存地址
*                * zValue：Z轴的保存地址
* Output         : None
* Return         : None
****************************************************************************/

void ADX_GetXYZData(int16_t *xValue, int16_t *yValue, int16_t *zValue)
{
    int32_t xTotal = 0, yTotal = 0, zTotal = 0;
    uint8_t i;
    
    *xValue = 0;
    *yValue = 0;
    *zValue = 0;

    /* 读取十次采样值 */
    for(i=0; i<10; i++)
    {
        ADX345_ReadXYZ(xValue, yValue, zValue);
        xTotal += *xValue;
        yTotal += *yValue;
        zTotal += *zValue;
        ADX345_DelayMs(10);  //才样频率在100HZ，10ms采样一次最好 
    }
    
    /* 求出平均值 */
    *xValue = xTotal / 10;
    *yValue = yTotal / 10;
    *zValue = zTotal / 10;       
}

/****************************************************************************
* Function Name  : ADX_GetAngle
* Description    : 将AD值转换成角度值
* Input          : xValue：x的值
*                * yValue：y的值
*                * zValue：z的值
*                * dir：0：与Z轴的角度;1：与X轴的角度;2：与Y轴的角度.
* Output         : None
* Return         : None
****************************************************************************/

int16_t ADX_GetAngle(float xValue, float yValue, float zValue, uint8_t dir)
{
	float temp;
 	float res = 0;

	switch(dir)
	{   
        /* 与自然Z轴的角度 */
		case 0:
 			temp = sqrt((xValue * xValue + yValue * yValue)) / zValue;
 			res = atan(temp);
 			break;
        
        /* 与自然X轴的角度 */
		case 1:
 			temp = xValue / sqrt((yValue * yValue + zValue * zValue));
 			res = atan(temp);
 			break;

        /* 与自然Y轴的角度 */
 		case 2:
 			temp = yValue / sqrt((xValue * xValue + zValue * zValue));
 			res = atan(temp);
 			break;

        default:
            break;
 	}

    res = res * 1800 / 3.14; 

	return res;
}

#define ADX345_CS		GPIOC
#define ADX345_CS_Pin	GPIO_Pin_11

void adx345_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); 
	
	GPIO_InitStructure.GPIO_Pin    = ADX345_CS_Pin;
	GPIO_InitStructure.GPIO_Speed  = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_Out_PP;
	
	GPIO_Init(ADX345_CS, &GPIO_InitStructure);
	
	GPIO_SetBits(ADX345_CS, GPIO_Pin_11);		
}

void adx345_init(void)
{
	int16_t Xval, Yval, Zval, Xang, Yang, Zang;
	int32_t i;
	
	adx345_gpio_init();
	
	while (ADX345_Init()) {
		msleep(100);
	}
	
	ADX345_Adjust();
	
	while (1) {
		i++;
        if(i > 0x02FFFF)
        {
            /* 读取X,Y,Z的加速度值 */
            ADX_GetXYZData(&Xval, &Yval, &Zval);

            /* 将读取到的加速度值转换为角度值 */
            Xang=ADX_GetAngle(Xval, Yval, Zval,1);    
			Yang=ADX_GetAngle(Xval, Yval, Zval,2);   
			Zang=ADX_GetAngle(Xval, Yval, Zval,0);
			
			Xang = Xang;
			Yang = Yang;
			Zang = Zang;
			
            i = 0; 
        }
	}
}
