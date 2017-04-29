#include "GUI.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sram.h"
#include "helper.h"
#include "PROGBAR.h"
#include "priority.h"
#include "mutex.h"
#include "display.h"

#define FONT16_BASE_ADDR		(U32)(3 * 0x100000)

#define FONT33_BASE_ADDR		(U32)(2 * 0x100000)

#define BIG_FONT	1
#define LITTLE_FONT	0

#define MSG_SUM		5


struct msg_queue {
	struct {
		int id;
		s8 h, m, s;
	} msg[MSG_SUM];
	
	int idx;
	int count;	
	int fresh;
};

struct flash_img_section {
	u32 pic_num;
	u32 img_addr[63];//256 bytes
};

#if 0
/*********************************************************************
*
*       Palette
*
*  Description
*    The following are the entries of the palette table.
*    The entries are stored as a 32-bit values of which 24 bits are
*    actually used according to the following bit mask: 0xBBGGRR
*
*    The lower   8 bits represent the Red   component.
*    The middle  8 bits represent the Green component.
*    The highest 8 bits represent the Blue  component.
*/
static GUI_CONST_STORAGE GUI_COLOR _Colorsmsg_notify1[] = {
  0xFFFFFF, 0xFFFFFF, 0x4D55EE, 0x4E56EE,
  0x4C54EE, 0x4149ED, 0x4B53EE, 0x4048ED,
  0x4149EE, 0x424BEE, 0x4A52EE, 0x555DEE,
  0x555EEE, 0x414AEE, 0x424AEE, 0x454DEE,
  0x454EEE, 0x464EEE, 0x464FEE, 0x474FEE,
  0x4A52ED, 0x4850EE, 0x4A51EE, 0x4953EE,
  0x4E55ED, 0x4D56EE, 0x5058EE, 0x5059EE,
  0x545CED, 0x575EEE, 0x5962EE, 0x5C63ED,
  0x5C64EE, 0x5D66EE, 0x5F67EF, 0x5F68EE,
  0x6971ED, 0x6971EF, 0x6E75EE, 0xA2A9F1,
  0xB1B6F1, 0xC2C7F3, 0xC9CDF3, 0xCACDF3,
  0xCACEF3, 0xD1D3F4, 0xD6DAF4, 0xD7DDF4,
  0xE1E4F4, 0xE2E7F4, 0xE3E8F3, 0xEAEEF6,
  0xF6F8F6
};

static GUI_CONST_STORAGE GUI_LOGPALETTE _Palmsg_notify1 = {
  53,  // Number of entries
  1,   // Has transparency
  &_Colorsmsg_notify1[0]
};

static GUI_CONST_STORAGE unsigned char _acmsg_notify1[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x15, 0x13, 0x02, 0x03, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x19, 0x04, 0x16, 0x1C, 0x0C, 0x06, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x04, 0x0D, 0x0B, 0x2B, 0x32, 0x23, 0x05, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x06, 0x08, 0x20, 0x2E, 0x34, 0x25, 0x07, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x04, 0x08, 0x0C, 0x2D, 0x33, 0x22, 0x07, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x0E, 0x1B, 0x2C, 0x31, 0x1E, 0x05, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x09, 0x03, 0x2A, 0x30, 0x1D, 0x05, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x10, 0x17, 0x27, 0x28, 0x1A, 0x0F, 0x03, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x0A, 0x14, 0x24, 0x26, 0x18, 0x0A, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x09, 0x04, 0x29, 0x2F, 0x0B, 0x05, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x04, 0x06, 0x1F, 0x21, 0x02, 0x04, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x12, 0x11, 0x02, 0x03, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

GUI_CONST_STORAGE GUI_BITMAP bmmsg_notify1 = {
  30, // xSize
  29, // ySize
  30, // BytesPerLine
  8, // BitsPerPixel
  _acmsg_notify1,  // Pointer to picture data (indices)
  &_Palmsg_notify1   // Pointer to palette
};

#endif



extern int GUI_PNG_Draw      (const void * pFileData, int DataSize, int x0, int y0);
extern void flash_bytes_read(u32 addr, u8 *buf, u32 len);
extern GUI_CONST_STORAGE GUI_FONT GUI_Fontsongti20;
extern GUI_CONST_STORAGE GUI_FONT GUI_Fontfont_task;
extern GUI_CONST_STORAGE GUI_BITMAP bmmsg_notify1;

extern void battery_show(int i);
extern u32 get_time(u8 *, u8 *, u8 *);

static GUI_MEMDEV_Handle  bg_hmem = 0, home1_hmem = 0;
static u8 get_data_ex[1440];
static struct flash_img_section img_section;

static int menu_index = 0;

static PROGBAR_Handle ahProgBar[2];
static struct mutex_lock lock;
static int need_update_flag;

static int offline_enable;

static GUI_FONT     XB16FFont, XB33FFont;
static GUI_XBF_DATA XB16F_Data, XB33F_Data;

static char iterm[][50] = {
"\xe6\x9d\x80\xe6\x95\x8c",
"\xe8\xa2\xab\xe6\x9d\x80",
"\xe7\x88\x86\xe5\xa4\xb4",
"\xe8\xa2\xab\xe7\x88\x86",
"\xe8\xa1\x80\xe9\x87\x8f",
"\xe5\xbc\xb9\xe8\x8d\xaf",
"\xe5\xb9\xb8\xe5\xad\x98",
"\xe7\xa7\xaf\xe5\x88\x86",
};
static char seccess_fail_font[2][50] = {
"\xe8\x83\x9c\xe5\x88\xa9",
"\xe5\xa4\xb1\xe8\xb4\xa5",
};

static char task_font[][50] = {
"\xe6\x95\x8c\xe6\x83\x85\xe9\x80\x9a\xe6\x8a\xa5",
"\xe8\x83\x9c\xe8\xb4\x9f\xe5\x88\xa4\xe5\xae\x9a"	
};

/*
沙场点兵方知好男儿
马革裹尸才是真英雄
唯有尝尽摸爬滚打
历经枪林弹雨
方可百炼成钢。

任何一队累计积分率
先达到120即为胜利。
*/
static char wanfa[8][40] = {
"\xe6\xb2\x99\xe5\x9c\xba\xe7\x82\xb9\xe5\x85\xb5\xe6\x96\xb9\xe7\x9f\xa5\xe5\xa5\xbd\xe7\x94\xb7\xe5\x84\xbf",
"\xe9\xa9\xac\xe9\x9d\xa9\xe8\xa3\xb9\xe5\xb0\xb8\xe6\x89\x8d\xe6\x98\xaf\xe7\x9c\x9f\xe8\x8b\xb1\xe9\x9b\x84",
"\xe5\x94\xaf\xe6\x9c\x89\xe5\xb0\x9d\xe5\xb0\xbd\xe6\x91\xb8\xe7\x88\xac\xe6\xbb\x9a\xe6\x89\x93",
"\xe5\x8e\x86\xe7\xbb\x8f\xe6\x9e\xaa\xe6\x9e\x97\xe5\xbc\xb9\xe9\x9b\xa8",
"\xe6\x96\xb9\xe5\x8f\xaf\xe7\x99\xbe\xe7\x82\xbc\xe6\x88\x90\xe9\x92\xa2\xe3\x80\x82",
"\xe4\xbb\xbb\xe4\xbd\x95\xe4\xb8\x80\xe9\x98\x9f\xe7\xb4\xaf\xe8\xae\xa1\xe7\xa7\xaf\xe5\x88\x86\xe7\x8e\x87",
"\xe5\x85\x88\xe8\xbe\xbe\xe5\x88\xb0""120\xe5\x8d\xb3\xe4\xb8\xba\xe8\x83\x9c\xe5\x88\xa9\xe3\x80\x82",
};

static struct msg_queue msg_queue;

static char msg[16][100] = {	
"\xe6\x80\xbb\xe5\x8f\xb0\xef\xbc\x9a\xe5\x89\x8d\xe5\x8f\xb0\xe6\x9c\x89\xe4\xba\xba\xe6\x89\xbe",
"\xe6\x80\xbb\xe5\x8f\xb0\xef\xbc\x9a\xe4\xbd\x99\xe9\xa2\x9d\xe4\xb8\x8d\xe8\xb6\xb3"
};

static char task_iterm[2][25] = {
"\xe6\x95\x8c\xe6\x83\x85\xe9\x80\x9a\xe6\x8a\xa5",//敌情通报
"\xe8\x83\x9c\xe8\xb4\x9f\xe5\x88\xa4\xe5\xae\x9a" //胜负判定
};

static int spi_flash_get_bg(void * p, const U8 ** ppData, unsigned NumBytes, U32 Off)
{
	if (NumBytes > sizeof(get_data_ex))
		NumBytes = sizeof(get_data_ex);
	
	flash_bytes_read(img_section.img_addr[0] + Off, get_data_ex, NumBytes);
	
	*ppData = get_data_ex;
	
	return NumBytes;
}

static int spi_flash_get_head_pic(void * p, const U8 ** ppData, unsigned NumBytes, U32 Off)
{
#if 0
	flash_bytes_read(img_section.img_addr[1] + Off, (u8 *)*ppData, NumBytes);

#else
	
	if (NumBytes > sizeof(get_data_ex))
		NumBytes = sizeof(get_data_ex);
	
	flash_bytes_read(img_section.img_addr[1] + Off, get_data_ex, NumBytes);
	
	*ppData = get_data_ex;
#endif
	return NumBytes;
}

static int spi_flash_get_msg(void * p, const U8 ** ppData, unsigned NumBytes, U32 Off)
{
	if (NumBytes > sizeof(get_data_ex))
		NumBytes = sizeof(get_data_ex);
	
	flash_bytes_read(256 * 3602 + Off, get_data_ex, NumBytes);
	
	*ppData = get_data_ex;
	
	return NumBytes;
}

static void display_iterm(char *s, int x, int y, int val)
{
	char str[100];

	sprintf(str, "%s%d", s, val);

	GUI_DispStringAt(str, x, y);	
}

static int has_message(void)
{
	return msg_queue.fresh;
}

static void show_statistac(void)
{
	int i = 0;
	
			
	GUI_SetColor(GUI_WHITE);
	
	GUI_SetTextMode(GUI_TEXTMODE_TRANS);
	
	GUI_SetFont(&GUI_Fontsongti20);
	GUI_UC_SetEncodeUTF8();
	
	if (offline_enable == 1) {
		display_iterm(iterm[4], 170, 120, get_blod());
		display_iterm(iterm[5], 308, 120, get_ammo());		
	} else {	
		display_iterm(iterm[0], 170, 40, get_kill());
		display_iterm(iterm[1], 308, 40, get_killed());

		display_iterm(iterm[2], 170, 80, get_headshot());
		display_iterm(iterm[3], 308, 80, get_headshoted());

		display_iterm(iterm[4], 170, 120, get_blod());
		display_iterm(iterm[5], 308, 120, get_ammo());

		display_iterm(iterm[6], 170, 160, get_our());
		display_iterm(iterm[7], 308, 160, get_enemy());
	}		
}

static void display_hanzi(char *src, int x, int y, GUI_COLOR color, int font_size)
{	
	GUI_COLOR bak = GUI_GetColor();
	
	GUI_SetColor(color);
	
	GUI_SetTextMode(GUI_TEXTMODE_TRANS);
	
	GUI_UC_SetEncodeUTF8();
	
	if (font_size == LITTLE_FONT)
		GUI_SetFont(&XB16FFont);
	else
		GUI_SetFont(&XB33FFont);
	
	GUI_DispStringAt(src, x, y);
	
	GUI_SetColor(bak);
}

static void display_en(char *src, int x, int y, GUI_COLOR color)
{
	GUI_COLOR bak = GUI_GetColor();
	
	GUI_SetColor(color);
	
	GUI_SetTextMode(GUI_TEXTMODE_TRANS);

	GUI_SetFont(GUI_FONT_8X16);
	
	GUI_DispStringAt(src, x, y);

	GUI_SetColor(bak);	
}

static void display_task_info(void)
{
	int i, j;
	
	for (i = 0, j = 0; i < 5; i++, j+= 30)
		display_hanzi(wanfa[i], 20, 100 + j, GUI_GREEN, LITTLE_FONT);
	
	for (j = 0; i < 7; i++, j+= 30)
		display_hanzi(wanfa[i], 234, 100 + j, GUI_GREEN, LITTLE_FONT);
}

static void clock_show(s8 hour, s8 min, s8 sec)
{
	char time[10];
	int ret;
	
	if (hour == -1 || min == -1 || sec == -1)
		return;

	memset(time, 0 , sizeof(time));
	
	sprintf(time, "%02d:%02d:%02d", hour, min, sec);
	
	GUI_SetFont(&GUI_Font16_ASCII);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt(time, 240, 4);		
}

static int _cbGetData16(U32 Off, U16 NumBytes, void * pVoid, void * pBuffer)
{
	flash_bytes_read(FONT16_BASE_ADDR + Off, pBuffer, NumBytes);
	
	return 0;
}

static int _cbGetData33(U32 Off, U16 NumBytes, void * pVoid, void * pBuffer)
{
	flash_bytes_read(FONT33_BASE_ADDR + Off, pBuffer, NumBytes);
	
	return 0;
}

static void display_msg_info(void)
{
	int i, j;
	char time[20];
	int cnt;
	
	display_hanzi("\xe6\xb6\x88\xe6\x81\xaf", 190, 50, GUI_WHITE, BIG_FONT);
	
	cnt = msg_queue.count > MSG_SUM ? MSG_SUM : msg_queue.count;
	
	for (i =0, j = 0; i < cnt; i++, j += 30) {
		sprintf(time, "%02d:%02d:%02d", msg_queue.msg[i].h, msg_queue.msg[i].m, msg_queue.msg[i].s);
		display_en(time, 30, 100 + j, GUI_GREEN);
		display_hanzi(msg[msg_queue.msg[i].id], 150, 100 + j, GUI_GREEN, LITTLE_FONT);	
	}
}

void enable_offline(int mode)
{
	offline_enable = mode;
}

void display_success(void)
{
	extern GUI_CONST_STORAGE GUI_FONT GUI_Fontsuccess78;
	
	mutex_lock(&lock);
	
	GUI_MEMDEV_Select(home1_hmem);
	
	GUI_MEMDEV_Write(bg_hmem);

	GUI_SetColor(GUI_GREEN);
	
	GUI_SetTextMode(GUI_TEXTMODE_TRANS);
	
	GUI_SetFont(&GUI_Fontsuccess78);
	
	GUI_UC_SetEncodeUTF8();
	
	GUI_DispStringAt(seccess_fail_font[0], 160, 100);	
	
	GUI_MEMDEV_Select(0);
	
	GUI_MEMDEV_CopyToLCDAt(home1_hmem, 0, 20);

	mutex_unlock(&lock);
}

void display_fail(void)
{
	extern GUI_CONST_STORAGE GUI_FONT GUI_Fontsuccess78;
	
	mutex_lock(&lock);
	
	GUI_MEMDEV_Select(home1_hmem);
	
	GUI_MEMDEV_Write(bg_hmem);

	GUI_SetColor(GUI_RED);
	
	GUI_SetTextMode(GUI_TEXTMODE_TRANS);
	
	GUI_SetFont(&GUI_Fontsuccess78);
	
	GUI_UC_SetEncodeUTF8();
	
	GUI_DispStringAt(seccess_fail_font[1], 160, 100);	
	
	GUI_MEMDEV_Select(0);
	
	GUI_MEMDEV_CopyToLCDAt(home1_hmem, 0, 20);

	mutex_unlock(&lock);	
}

void save_msg_id(int msg_id)
{	
	msg_queue.msg[msg_queue.idx].id = msg_id;
	
	get_time(&msg_queue.msg[msg_queue.idx].h, &msg_queue.msg[msg_queue.idx].m, &msg_queue.msg[msg_queue.idx].s);
	
	if (++msg_queue.idx == MSG_SUM)
		msg_queue.idx = 0;
	
	msg_queue.count++;
}

void read_spi_flash_header(void)
{
	flash_bytes_read(0, (u8 *)&img_section, sizeof(img_section));

	if (img_section.pic_num >= 1 && img_section.pic_num <= 64)
		return;
	
	GUI_Clear();
	
	display_en("Please download Image file", 10, 100, GUI_WHITE);
	
	while (1)
		sleep(1);
}

void show_background(void)
{
	mutex_lock(&lock);
	
	GUI_MEMDEV_Select(0);	
	GUI_MEMDEV_CopyToLCDAt(bg_hmem, 0, 20);
	
	mutex_unlock(&lock);
}

void show_msg(void)
{
	mutex_lock(&lock);
	
	GUI_MEMDEV_Select(home1_hmem);

	GUI_MEMDEV_Write(bg_hmem);
	
	GUI_SetColor(GUI_DARKGRAY);
	
	GUI_EnableAlpha(1);	
	
	GUI_SetAlpha(50);
	
	GUI_FillRect(20, 40, 440, 90);

	GUI_SetColor(GUI_GRAY);	
	
	GUI_FillRect(20, 90, 440, 240);

	display_msg_info();

	GUI_SetAlpha(0);

	GUI_MEMDEV_Select(0);	
	
	GUI_MEMDEV_CopyToLCDAt(home1_hmem, 0, 20);
	
	mutex_unlock(&lock);
}

void show_task(void)
{
	
	mutex_lock(&lock);
	
	GUI_MEMDEV_Select(home1_hmem);
	
	//GUI_BMP_DrawEx(spi_flash_get_bg, NULL, 0, 0);
	
	GUI_MEMDEV_Write(bg_hmem);
	
	GUI_SetColor(GUI_WHITE);
	
	GUI_SetTextMode(GUI_TEXTMODE_TRANS);
	
	GUI_SetFont(&GUI_Fontfont_task);
	GUI_UC_SetEncodeUTF8();
	
	GUI_SetColor(GUI_DARKGRAY);
	
	GUI_EnableAlpha(1);
	
	GUI_SetAlpha(50);
	
	GUI_FillRect(20, 40, 226, 90);
	
	GUI_FillRect(234, 40, 440, 90);
	
	//GUI_SetAlpha(50);
	
	GUI_SetColor(GUI_GRAY);
	
	GUI_FillRect(20, 90, 226, 240);
	
	GUI_FillRect(234, 90, 440, 240);
	
		//GUI_DispStringAt(task_font[0], 45, 50);
	display_hanzi(task_iterm[0], 45, 50, GUI_WHITE, BIG_FONT);
	
	//GUI_DispStringAt(task_font[1], 260, 50);
	display_hanzi(task_iterm[1], 260, 50, GUI_WHITE, BIG_FONT);
	
	display_task_info();
	
	GUI_SetAlpha(0);
	
	GUI_MEMDEV_Select(0);
	
	GUI_MEMDEV_CopyToLCDAt(home1_hmem, 0, 20);	
	
	mutex_unlock(&lock);
}

void show_home(void)
{
	mutex_lock(&lock);
	
	GUI_MEMDEV_Select(home1_hmem);
	
	GUI_MEMDEV_Write(bg_hmem);

	GUI_SetColor(GUI_DARKGRAY);
	
	GUI_EnableAlpha(1);
	
	GUI_SetAlpha(200);
	
	GUI_FillRect(20, 40, 440, 240);
	//
	//GUI_FillRect(234, 40, 440, 90);
	
	GUI_SetAlpha(0);
	
	GUI_EnableAlpha(0);
		
	GUI_BMP_DrawEx(spi_flash_get_head_pic, NULL, 20, 40);
	
	//GUI_PNG_DrawEx(spi_flash_get_head_pic, NULL, 20, 40);
	
	show_statistac();
	
	GUI_MEMDEV_Select(0);
	
	GUI_MEMDEV_CopyToLCDAt(home1_hmem, 0, 20);

	mutex_unlock(&lock);
}

void upiterm_show(struct iterm_info *info)
{
	static GUI_MEMDEV_Handle iterm_hmem = 0;
	char bulet[20];
	
	mutex_lock(&lock);
	
	if (iterm_hmem == 0) 
		iterm_hmem = GUI_MEMDEV_Create(0, 0, 480, 20);
	
	GUI_MEMDEV_Select(iterm_hmem);
	
	GUI_SetColor(GUI_BLACK);
	
	GUI_FillRect(0, 0, 479, 20);

	lcd_battery_show(info->lcd_pwr);
	
	clothe_battery_show(info->clothe_pwr);
	
	gun_battery_show(info->gun_pwr);
	
	sprintf(bulet, "Z %d    F %d", info->main_bulet, info->sub_bulet);
	
	display_en(bulet, 20, 5, GUI_WHITE);

	clock_show(info->hour, info->mini, info->sec);
	
	GUI_MEMDEV_Select(0);
	
	GUI_MEMDEV_CopyToLCD(iterm_hmem);
	
	mutex_unlock(&lock);	
}

void ProgBarInit(void) 
{
	ahProgBar[1] = PROGBAR_Create(120,240,240,30, WM_CF_SHOW);

	PROGBAR_EnableMemdev(ahProgBar[1]);
	PROGBAR_SetMinMax(ahProgBar[1], 0, 120);
	PROGBAR_SetBarColor(ahProgBar[1], 0, GUI_GREEN);//������ɫ
	PROGBAR_SetBarColor(ahProgBar[1], 1, GUI_TRANSPARENT | GUI_GRAY);//������ɫ
}

void ProgBarShow(int v)
{	
	if (v > 120)
		v = 120;
	
	PROGBAR_SetValue(ahProgBar[1], v);
	msleep(10);
}

void ProgBarDelete(void)
{
	GUI_Delay(500);
	
	PROGBAR_Delete(ahProgBar[1]);
}

void pic_preload(void)
{
	OS_ERR err;
	
	mutex_init(&lock);
	
	if (home1_hmem == 0)
		home1_hmem = GUI_MEMDEV_Create(0, 0, 480, 320);

	if (bg_hmem == 0)
		bg_hmem = GUI_MEMDEV_Create(0, 0, 480, 320);
	
	GUI_MEMDEV_Select(bg_hmem);	
		
	GUI_BMP_DrawEx(spi_flash_get_bg, NULL, 0, 0);

	GUI_MEMDEV_Select(0);
}

void display_key(void)
{
	char msg[] = "\请插入\\";
	
	display_hanzi(msg, 100, 100, GUI_WHITE, LITTLE_FONT);
	display_en("key...", 170, 101, GUI_WHITE);
}

void XBF_font_init(void)
{
	int ret = GUI_XBF_CreateFont(&XB16FFont, 
						&XB16F_Data, 
						GUI_XBF_TYPE_PROP, 
						_cbGetData16, 
						NULL); 
	if (ret != 0) {
		GUI_Clear();
		
		display_en("Please downlaod 16 font file", 10, 200, GUI_WHITE);
		
		while (1)
			sleep(1);
	}
	
	ret = GUI_XBF_CreateFont(&XB33FFont, 
						&XB33F_Data, 
						GUI_XBF_TYPE_PROP, 
						_cbGetData33, 
						NULL); 
	
	
	if (ret != 0) {
		GUI_Clear();
		
		display_en("Please downlaod 33 font file", 10, 200, GUI_WHITE);
		
		while (1)
			sleep(1);
	}
}

void need_update(void)
{
	need_update_flag = 1;
}

void display_menu(int menu_idx, int offline_mode)
{
	if (need_update_flag == 0)
		return;
	
	switch (menu_idx) {
		case 1 :	
			show_home();
			err_log("show home\r\n");
			break;	
		case 2:
			show_task();
			err_log("show task\r\n");
			break;
		case 3:
			if (offline_enable == 1)
				break;
			
			show_msg();
			err_log("show message\r\n");
			break;
		case 4:

			break;
		default:		
			
			break;
	}

	need_update_flag = 0;
}
