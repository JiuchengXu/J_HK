#include "includes.h"
#include "GUI.h"
#include "sram.h"
#include "malloc.h"


#define USE_EXRAM  0//ʹ���ⲿRAM
//����EMWIN�ڴ��С
#define GUI_NUMBYTES  (1024 * 1024)
#define GUI_BLOCKSIZE 0X80  //���С

static char gui_buf[GUI_NUMBYTES] __attribute__((at(0x68000000)));

//GUI_X_Config
//��ʼ����ʱ�����,��������emwin��ʹ�õ��ڴ�
void GUI_X_Config(void) {
	if(USE_EXRAM) //ʹ���ⲿRAM
	{
		//U32 *aMemory = mymalloc(SRAMEX,GUI_NUMBYTES); //���ⲿSRAM�з���GUI_NUMBYTES�ֽڵ��ڴ�
		//GUI_ALLOC_AssignMemory((void*)aMemory, GUI_NUMBYTES); //Ϊ�洢����ϵͳ����һ���洢��
		GUI_ALLOC_AssignMemory((void*)(0x68000000), GUI_NUMBYTES);
		GUI_ALLOC_SetAvBlockSize(GUI_BLOCKSIZE); //���ô洢���ƽ���ߴ�,����Խ��,���õĴ洢������Խ��
		//GUI_SetDefaultFont(GUI_FONT_6X8); //����Ĭ������
	}else  //ʹ���ڲ�RAM
	{
		//U32 *aMemory = mymalloc(SRAMIN,GUI_NUMBYTES); //���ڲ�RAM�з���GUI_NUMBYTES�ֽڵ��ڴ�
		U32 *aMemory = (void *)gui_buf; 
		if (aMemory == NULL) {
			printf("error\n");
			return;
		}
	
		GUI_ALLOC_AssignMemory((U32 *)aMemory, GUI_NUMBYTES); //Ϊ�洢����ϵͳ����һ���洢��
		GUI_ALLOC_SetAvBlockSize(GUI_BLOCKSIZE); //���ô洢���ƽ���ߴ�,����Խ��,���õĴ洢������Խ��
		//GUI_SetDefaultFont(GUI_FONT_6X8); //����Ĭ������
	}
}
