#include "includes.h"
#include "usart.h"
#include "lib_str.h"
#include "bus.h"
#include "helper.h"
#include <priority.h>

#define OS_TASK_STACK_SIZE 256
#define MAX_SECOND		(0xfff / (40000/256 + 1))

static CPU_STK  TaskStk[OS_TASK_STACK_SIZE];
static OS_TCB TaskStkTCB;
static int init_flag;

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


void watch_dog_feed_task(void)
{	
	while (1) {
		IWDG_Feed();
		sleep(1);
	}
}

void watch_dog_set(int sec)
{
	if (sec > MAX_SECOND)
		sec = MAX_SECOND;

	IWDG_Init(IWDG_Prescaler_256, sec * 157);
}

void watch_dog_feed_task_init(void)
{
	OS_ERR err;

	watch_dog_set(3);

	OSTaskCreate((OS_TCB *)&TaskStkTCB, 
			(CPU_CHAR *)"watch dog feed task", 
			(OS_TASK_PTR)watch_dog_feed_task, 
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

	init_flag = 1;
}

void watch_dog_feed_task_delete(void)
{
	OS_ERR err;

	if (init_flag) {		
		OSTaskDel(&TaskStkTCB, &err);

		if (err != OS_ERR_NONE)
			while(1);
	} else {
		watch_dog_set(3);
	}
}
