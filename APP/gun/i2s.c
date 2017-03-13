#include "includes.h"
#include "wav.h"
#include "stm32f10x_dma.h"
#include "flash.h"
#include "helper.h"

#ifdef GUN

DMA_InitTypeDef DMA_InitStructure; 
static u32 file_idx;
static u16 *CurrentPos;
static OS_SEM dma_sem;

#define BUF_LEN		256
u16 buffer1[BUF_LEN];
u16 buffer2[BUF_LEN];

__IO int WaveLen, g_WaveLen;
__IO u32 XferCplt;
__IO u32 DataOffset;
__IO u32 WaveCounter;

u8 buffer_switch;

WAVE_TypeDef WAVE_Format;

void Audio_MAL_Play(u32 Addr, u32 Size);
u32 ReadUnit(u8 *buffer,u8 idx,u8 NbrOfBytes,Endianness BytesFormat);

/**
  * @brief  I2S_Stop 停止iis总线
  * @param  none
  * @retval none
  */
void I2S_Stop(void)
{		
	/* 禁能 SPI2/I2S2 外设 */
	//SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, DISABLE);
	I2S_Cmd(SPI2, DISABLE);	

	DMA_Cmd(DMA1_Channel5, DISABLE);
}


/**
  * @brief  I2S_Freq_Config 根据采样频率配置iis总线，在播放音频文件时可从文件中解码获取
  * @param  SampleFreq 采样频率
  * @retval none
  */
void I2S_Freq_Config(uint16_t SampleFreq)
{
	I2S_InitTypeDef I2S_InitStructure;

	I2S_Stop();
	I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;					// 配置I2S工作模式
	I2S_InitStructure.I2S_Standard = I2S_Standard_Phillips;				// 接口标准 
	I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;			// 数据格式，16bit 
	I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;		// 主时钟模式 
	I2S_InitStructure.I2S_AudioFreq = SampleFreq;					// 音频采样频率 
	I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;  			
	I2S_Init(SPI2, &I2S_InitStructure);
	
	I2S_Cmd(SPI2, ENABLE);	//使能iis总线
}

/**
  * @brief  DMA_I2S_Configuration 配置DMA总线
  * @param  addr:数据源地址
	*	@param	size:要传输的数据大小
  * @retval none
  */
void DMA_I2S_Configuration(uint32_t addr, uint32_t size)
{
	NVIC_InitTypeDef NVIC_InitStructure;				  

	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 13;//?????(0 ??)
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;//????(0 ??)
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);   

    /* DMA 通道配置*/
    DMA_Cmd(DMA1_Channel5, DISABLE);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI2->DR));		//目的地址，iis的数据寄存器
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) addr;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = size;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	//DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	//DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);//????DMA???
	
    SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);	//使能DMA请求
}

/**
  * @brief  I2S_GPIO_Config 配置IIS总线用到的GPIO引脚
  * @param  none
  * @retval none
  */
static void I2S_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* Enable GPIOB, GPIOC */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
	
	/* 打开 I2S2 APB1 时钟 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	
	/* I2S2 SD, CK  WS 配置 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	/* I2S2 MCK  */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	//GPIO_ResetBits(GPIOC, GPIO_Pin_12);
	GPIO_WriteBit(GPIOC, GPIO_Pin_12, Bit_SET);
}

/**
  * @brief  I2S_Mode_Config 配置IIS总线的工作模式(默认采样频率)
  * @param  none
  * @retval none
  */
static void I2S_Mode_Config(void)
{
	I2S_InitTypeDef I2S_InitStructure; 
			
	/* I2S2 外设配置 */
	I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;					// 配置I2S工作模式 
	I2S_InitStructure.I2S_Standard = I2S_Standard_Phillips;				// 接口标准 
	I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_16b;			// 数据格式，16bit
	I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Enable;		// 主时钟模式 
	I2S_InitStructure.I2S_AudioFreq = I2S_AudioFreq_44k;			// 音频采样频率
	I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;  					// 默认为低	
	I2S_Init(SPI2, &I2S_InitStructure);
}

/**
  * @brief  I2S_Bus_Init 初始化iis总线
  * @param  none
  * @retval none
  */
void I2S_Bus_Init(void)
{
	/* 配置I2S GPIO用到的引脚 */
	I2S_GPIO_Config(); 

	/* 配置I2S工作模式 */
	I2S_Mode_Config();

	I2S_Cmd(SPI2, ENABLE);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
	DMA_I2S_Configuration(0, 0xFFFE);
}

void Audio_MAL_Play(u32 Addr, u32 Size)
{	
  	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)Addr;
  	DMA_InitStructure.DMA_BufferSize = (uint32_t)Size >> 1; // Size/2
  	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
  	DMA_Cmd(DMA1_Channel5, ENABLE); 
	
  	if ((SPI2->I2SCFGR & I2S_ENABLE_MASK) == 0)
		I2S_Cmd(SPI2, ENABLE);
}

u8 WaveParsing(void)
{
  	u32 temp=0x00;
  	u32 extraformatbytes=0;

  	temp=ReadUnit((u8*)buffer1,0,4,BigEndian);//?'RIFF'
  	if(temp!=CHUNK_ID)return 1;
	
  	WAVE_Format.RIFFchunksize=ReadUnit((u8*)buffer1,4,4,LittleEndian);//?????
	
  	temp=ReadUnit((u8*)buffer1,8,4,BigEndian);//?'WAVE'
  	if(temp!=FILE_FORMAT)return 2;
	
  	temp=ReadUnit((u8*)buffer1,12,4,BigEndian);//?'fmt '
	
  	if(temp!=FORMAT_ID)return 3;
	
  	temp=ReadUnit((u8*)buffer1,16,4,LittleEndian);//?'fmt'????
	
  	if(temp!=0x10)extraformatbytes=1;
	
  	WAVE_Format.FormatTag=ReadUnit((u8*)buffer1,20,2,LittleEndian);//?????
	
  	if(WAVE_Format.FormatTag!=WAVE_FORMAT_PCM)return 4;  
	
  	WAVE_Format.NumChannels=ReadUnit((u8*)buffer1,22,2,LittleEndian);//???? 
	WAVE_Format.SampleRate=ReadUnit((u8*)buffer1,24,4,LittleEndian);//????
	WAVE_Format.ByteRate=ReadUnit((u8*)buffer1,28,4,LittleEndian);//????
	WAVE_Format.BlockAlign=ReadUnit((u8*)buffer1,32,2,LittleEndian);//????
	WAVE_Format.BitsPerSample=ReadUnit((u8*)buffer1,34,2,LittleEndian);//??????
	
	if(WAVE_Format.BitsPerSample!=BITS_PER_SAMPLE_16)return 5;
	
	DataOffset=36;
	
	if(extraformatbytes==1)
	{
		temp=ReadUnit((u8*)buffer1,36,2,LittleEndian);//???????
		if(temp!=0x00)return 6;
		temp=ReadUnit((u8*)buffer1,38,4,BigEndian);//?'fact'
		//if(temp!=FACT_ID)return 7;
		temp=ReadUnit((u8*)buffer1,42,4,LittleEndian);//?Fact????
		DataOffset+=10+temp;
	}
	
	temp=ReadUnit((u8*)buffer1,DataOffset,4,BigEndian);//?'data'
	
	DataOffset+=4;
	
	if(temp!=DATA_ID)return 8;
	
	WAVE_Format.DataSize=ReadUnit((u8*)buffer1,DataOffset,4,LittleEndian);//???????
	DataOffset+=4;
	WaveCounter=DataOffset;
	
	return 0;
}

u32 ReadUnit(u8 *buffer,u8 idx,u8 NbrOfBytes,Endianness BytesFormat)
{
  	u32 index=0;
  	u32 temp=0;
  
  	for(index=0;index<NbrOfBytes;index++)temp|=buffer[idx+index]<<(index*8);
  	if (BytesFormat == BigEndian)temp= __REV(temp);
		return temp;
}

void AUDIO_TransferComplete(void)
{
	OS_ERR err;
#if 0	
  	XferCplt=1;
	WaveLen=0;
	
	//OSSemPost(&dma_sem, OS_OPT_POST_ALL, &err);
#else
	XferCplt=1;
	if (WaveLen == 0) {
		GPIO_WriteBit(GPIOC, GPIO_Pin_12, Bit_SET);
		return;
	}
	
	if (WaveLen)
		WaveLen -= BUF_LEN;
	
	if (WaveLen <= BUF_LEN && WaveLen > 0) {	
		if (buffer_switch == 1)
			Audio_MAL_Play((u32)buffer1, WaveLen);
		else
			Audio_MAL_Play((u32)buffer2, WaveLen);
		
		WaveLen = 0;
				
	} else if (WaveLen > BUF_LEN){
		if (buffer_switch == 1) {
			Audio_MAL_Play((u32)buffer1, BUF_LEN);
			flash_bytes_read(file_idx, (u8 *)buffer2, BUF_LEN);
			file_idx += BUF_LEN;
			buffer_switch = 2;
		} else {
			Audio_MAL_Play((u32)buffer2, BUF_LEN);
			flash_bytes_read(file_idx, (u8 *)buffer1, BUF_LEN);
			file_idx += BUF_LEN;
			buffer_switch = 1;
		}
		
		
	}	
#endif
}

/*
void SPI2_IRQHandler(void)
{
  	if (SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE)!=RESET)
  	{     	
    	SPI_I2S_SendData(SPI2, 0);//??????I2S 
  	}
}
*/

void DMA1_Channel5_IRQHandler(void)
{    	
  	if (DMA_GetFlagStatus(DMA1_FLAG_TC5) != RESET)
  	{         
 
      	DMA_Cmd(DMA1_Channel5, DISABLE);   
      	DMA_ClearFlag(DMA1_FLAG_TC5);      
      	AUDIO_TransferComplete();       
  	}
}

void wav_pre_read(void)
{
	u32 index = 0;
	OS_ERR err;

	flash_bytes_read(index, (u8 *)buffer1, 0x100);
	
	while(WaveParsing())
		;
	
  	g_WaveLen = WaveLen = WAVE_Format.DataSize;
	
  	I2S_Freq_Config(WAVE_Format.SampleRate);

	OSSemCreate(&dma_sem, "dma Sem", 0, &err);
}

void wav_play_up(void)
{
	OS_ERR err;

	XferCplt = 0;
	buffer_switch = 1;
	
	file_idx = DataOffset;
	WaveLen = g_WaveLen;
	GPIO_WriteBit(GPIOC, GPIO_Pin_12, Bit_RESET);	
	flash_bytes_read(file_idx, (u8 *)buffer1, BUF_LEN);
	file_idx += BUF_LEN;
	flash_bytes_read(file_idx, (u8 *)buffer2, BUF_LEN);
	file_idx += BUF_LEN;
	
	buffer_switch = 2;
		
	Audio_MAL_Play((u32)buffer1, BUF_LEN);

#if 0	
	sleep(10);
	
	while (WaveLen > 0) {
		//OSSemPend(&dma_sem, NULL, OS_OPT_PEND_BLOCKING, NULL, &err);
		if (XferCplt == 0)
			continue;
		
		if (buffer_switch == 2) {
			flash_bytes_read(file_idx, (u8 *)buffer1, BUF_LEN);
			file_idx += BUF_LEN;
			buffer_switch = 1;
		} else {
			flash_bytes_read(file_idx, (u8 *)buffer2, BUF_LEN);
			file_idx += BUF_LEN;
			buffer_switch = 2;
		}
		
		XferCplt = 0;
	}
#endif	
}

void wav_play_down(void)
{
	u32 index = 0;
	OS_ERR err;
	
	XferCplt = 0;
	buffer_switch = 1;
	
	index = DataOffset + g_WaveLen / 4;
	WaveLen = g_WaveLen - g_WaveLen / 4;
	
	GPIO_WriteBit(GPIOC, GPIO_Pin_12, Bit_RESET);
		
	flash_bytes_read(index, (u8 *)buffer1, 1024);
	index += 1024;
	buffer_switch = 2;
	
	Audio_MAL_Play((u32)buffer1, 1024);
	
	while (WaveLen > 0) {		
		if (buffer_switch == 2) {
			flash_bytes_read(index, (u8 *)buffer2, 1024);
			index += 1024;
			OSSemPend(&dma_sem, NULL, OS_OPT_PEND_BLOCKING, NULL, &err);
			buffer_switch = 1;
		} else {
			flash_bytes_read(index, (u8 *)buffer1, 1024);
			index += 1024;
			OSSemPend(&dma_sem, NULL, OS_OPT_PEND_BLOCKING, NULL, &err);
			buffer_switch = 2;
		}
		
		
	}	
}

void wav_play(void)
{
	u32 index = 0;
	OS_ERR err;
	
	XferCplt = 0;
	buffer_switch = 1;
	
	index = DataOffset;
	WaveLen = g_WaveLen;
	
	//increase_voice();
	
#if 0	
	flash_bytes_read(index, (u8 *)buffer1, 1024);
	index += 1024;
	flash_bytes_read(index, (u8 *)buffer2, 1024);
	
  	Audio_MAL_Play((u32)buffer1, 1024);

  	buffer_switch=1;
  	XferCplt=0;  
  	while (WaveLen != 0) { 
	      	while (XferCplt == 0)//??DMA????
				OSSemPend(&dma_sem, NULL, OS_OPT_PEND_BLOCKING, NULL, &err);
			
	      	XferCplt=0;
	      	if (buffer_switch==0) {
		        	Audio_MAL_Play((u32)buffer1, 1024);//?buffer1??
					index += 1024;
		        	flash_bytes_read(index, (u8 *)buffer2, 1024);
		        	buffer_switch=1;
	      	} else {   
		        	Audio_MAL_Play((u32)buffer2, 1024);//?buffer2??
					index += 1024;
		        	flash_bytes_read(index, (u8 *)buffer1, 1024);
		        	buffer_switch = 0;
	      	} 
		i++;
  	}
#else
	flash_bytes_read(index, (u8 *)buffer1, 1024);
	index += 1024;
	buffer_switch = 0;
	
	Audio_MAL_Play((u32)buffer1, 1024);
	
	while (WaveLen > 0) {
		if (buffer_switch == 0) {
			flash_bytes_read(index, (u8 *)buffer2, 1024);
			index += 1024;
			buffer_switch = 1;
		} else {
			flash_bytes_read(index, (u8 *)buffer1, 1024);
			index += 1024;
			buffer_switch = 0;
		}
		
		OSSemPend(&dma_sem, NULL, OS_OPT_PEND_BLOCKING, NULL, &err);
	}
	
#endif
}
#endif
