#include "includes.h"
#include "usart.h"
#include "lib_str.h"
#include "bus.h"
#include "helper.h"
#include <priority.h>

void IWDG_Init(u8 prer, u16 rlr) 
{	
 	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);  //ʹ�ܶԼĴ���IWDG_PR��IWDG_RLR��д����
	
	IWDG_SetPrescaler(prer);  //����IWDGԤ��Ƶֵ:����IWDGԤ��ƵֵΪ64
	
	IWDG_SetReload(rlr);  //����IWDG��װ��ֵ
	
	IWDG_ReloadCounter();  //����IWDG��װ�ؼĴ�����ֵ��װ��IWDG������
	
	IWDG_Enable();  //ʹ��IWDG
}

void IWDG_Feed(void)
{   
 	IWDG_ReloadCounter();//reload										   
}

