#include "includes.h"
#include "helper.h"

#if defined(LCD)
#define CPU_BAT_ADC_GPIO	GPIOB
#define CPU_BAT_ADC_PIN		GPIO_Pin_0
#define CPU_BAT_ADC_RCC		RCC_APB2Periph_GPIOB
#define CPU_BAT_ADC_CHAN	ADC_Channel_8
#else
#define CPU_BAT_ADC_GPIO	GPIOC
#define CPU_BAT_ADC_PIN		GPIO_Pin_0
#define CPU_BAT_ADC_RCC		RCC_APB2Periph_GPIOC
#define CPU_BAT_ADC_CHAN	ADC_Channel_10
#endif

#define OS_TASK_STACK_SIZE 256

static CPU_STK  TaskStk[OS_TASK_STACK_SIZE];
static OS_TCB TaskStkTCB;

static u16 get_adc_val(void)
{
	/* Start ADC1 Software Conversion */ 
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);			
 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ))	
		msleep(100);
	
	return ADC_GetConversionValue(ADC1);			
}

s8 get_power(void)
{
	u16 adc_v = get_adc_val();
	float f_adc_v = (float)adc_v / 4096 * 3.3 * 2;
	int ret = (int)((f_adc_v - 3) * 100) ;
	
	if (ret > 100)
		ret = 100;
		
	return (s8) ret;
}

void power_monitor_task(void)
{
	while (1) {
		if (get_power() < 5) {
			beep_on();
			sleep(2);
			beep_off();
			sleep(1);
		}		
	}
}

void power_monitor(void)
{
	OS_ERR err;
	
    OSTaskCreate((OS_TCB *)&TaskStkTCB, 
            (CPU_CHAR *)"power monitor", 
            (OS_TASK_PTR)power_monitor_task, 
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
}

void adc_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	
	RCC_APB2PeriphClockCmd(CPU_BAT_ADC_RCC | RCC_APB2Periph_ADC1, ENABLE);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	
	GPIO_InitStructure.GPIO_Pin = CPU_BAT_ADC_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(CPU_BAT_ADC_GPIO, &GPIO_InitStructure);
	
	ADC_DeInit(ADC1);
	
	/* ADC1 configuration ------------------------------------------------------*/
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;	
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	
	ADC_InitStructure.ADC_NbrOfChannel = 1;	
	ADC_Init(ADC1, &ADC_InitStructure);	
	
	ADC_RegularChannelConfig(ADC1, CPU_BAT_ADC_CHAN, 1, ADC_SampleTime_239Cycles5 );
	
	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);	
	
	/* Enable ADC1 reset calibaration register */   
	ADC_ResetCalibration(ADC1);	
	/* Check the end of ADC1 reset calibration register */
	while(ADC_GetResetCalibrationStatus(ADC1));	
	
	/* Start ADC1 calibaration */
	ADC_StartCalibration(ADC1);		
	
	/* Check the end of ADC1 calibration */
	while(ADC_GetCalibrationStatus(ADC1));		
	
	power_monitor();
}
