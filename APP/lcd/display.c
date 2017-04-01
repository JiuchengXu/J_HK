#include "GUI.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sram.h"
#include "helper.h"
#include "PROGBAR.h"
#include "priority.h"

extern GUI_CONST_STORAGE GUI_BITMAP bmmsg_notify1;

#define ITERM_TASK_STACK_SIZE		1024
#define FONT_BASE_ADDR		(U32)(3 * 0x100000)

static OS_TCB iterm_task_tcb;
static CPU_STK iterm_task_stk[ITERM_TASK_STACK_SIZE];
static PROGBAR_Handle ahProgBar[2];

GUI_FONT     XBFFont;
GUI_XBF_DATA XBF_Data;


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
struct info_statistac {
	int kill;
	int killed;
	int headshot;
	int headshoted;
	int blod;
	int ammo;
	int our;
	int enemy;
};

struct vip_info {
	char nickname[100];
	char level[100];
	int score;
	int team;
	int kill;
	int killed;
	int headshot;
	int headshoted;
	int success;
	int fail;
};

struct flash_img_section {
	u32 pic_num;
	u32 img_addr[63];//256 bytes
};

extern int GUI_PNG_Draw      (const void * pFileData, int DataSize, int x0, int y0);
extern void flash_bytes_read(u32 addr, u8 *buf, u32 len);
extern GUI_CONST_STORAGE GUI_FONT GUI_Fontsongti20;
extern GUI_CONST_STORAGE GUI_FONT GUI_Fontfont_task;
//extern const unsigned char _achead_pic[43681UL + 1];

extern void battery_show(int i);
extern u32 get_time(u8 *, u8 *, u8 *);

static struct info_statistac info;
static GUI_MEMDEV_Handle  bg_hmem = 0, home1_hmem = 0;
static u8 get_data_ex[1440];
static struct flash_img_section img_section;

static int menu_index = 0;

static char iterm[][100] = {
"\xe6\x9d\x80\xe6\x95\x8c ",
"\xe8\xa2\xab\xe6\x9d\x80 ",
"\xe7\x88\x86\xe5\xa4\xb4 ",
"\xe8\xa2\xab\xe7\x88\x86 ",
"\xe8\xa1\x80\xe9\x87\x8f ",
"\xe5\xbc\xb9\xe8\x8d\xaf ",
"\xe6\x88\x91\xe6\x96\xb9 ",
"\xe6\x95\x8c\xe6\x96\xb9 ",
};

static char task_font[][50] = {
"\xe6\x95\x8c\xe6\x83\x85\xe9\x80\x9a\xe6\x8a\xa5",
"\xe8\x83\x9c\xe8\xb4\x9f\xe5\x88\xa4\xe5\xae\x9a"	
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

static void get_statistac(struct info_statistac *info)
{
/*	info->ammo = 10;
	info->blod = 10;
	info->enemy = 100;
	info->headshot = 10;
	info->headshoted = 100;
	info->kill = 10;
	info->killed = 10;
	info->our = 100;*/
}

static void display_iterm(char *s, int x, int y, int val)
{
	char str[100];

	sprintf(str, "%s%d", s, val);

	GUI_DispStringAt(str, x, y);	
}

static void show_statistac(void)
{
	int i = 0;
		
	get_statistac(&info);
			
	GUI_SetColor(GUI_WHITE);
	
	GUI_SetTextMode(GUI_TEXTMODE_TRANS);
	
	GUI_SetFont(&GUI_Fontsongti20);
	GUI_UC_SetEncodeUTF8();
	
	display_iterm(iterm[i++], 180, 40, info.kill);

	display_iterm(iterm[i++], 308, 40, info.killed);

	display_iterm(iterm[i++], 180, 80, info.headshot);
	display_iterm(iterm[i++], 308, 80, info.headshoted);

	display_iterm(iterm[i++], 180, 120, info.blod);
	display_iterm(iterm[i++], 308, 120, info.ammo);

	display_iterm(iterm[i++], 180, 160, info.our);
	display_iterm(iterm[i++], 308, 160, info.enemy);	
}

void show_background(void)
{
	GUI_MEMDEV_Select(0);	
	GUI_MEMDEV_CopyToLCDAt(bg_hmem, 0, 20);
}

void show_msg(void)
{
	GUI_MEMDEV_Select(home1_hmem);	
		
	GUI_BMP_DrawEx(spi_flash_get_msg, NULL, 0, 0);

	GUI_MEMDEV_Select(0);
	GUI_DrawBitmap(&bmmsg_notify1, 400, 55);
	
	GUI_MEMDEV_CopyToLCDAt(home1_hmem, 0, 20);

	//GUI_PNG_Draw(_acmsg_notify, sizeof(_acmsg_notify), 100, 100);
	
	//GUI_DrawBitmap(&bmmsg_notify1, 400, 35);
}

void show_task(void)
{
	GUI_MEMDEV_Select(home1_hmem);
	
	//GUI_BMP_DrawEx(spi_flash_get_bg, NULL, 0, 0);
	
	GUI_MEMDEV_Write(bg_hmem);
	
	GUI_SetColor(GUI_WHITE);
	
	GUI_SetTextMode(GUI_TEXTMODE_TRANS);
	
	GUI_SetFont(&GUI_Fontfont_task);
	GUI_UC_SetEncodeUTF8();
		
	GUI_DispStringAt(task_font[0], 45, 50);
	
	GUI_DispStringAt(task_font[1], 260, 50);
	
	GUI_SetColor(GUI_GRAY);
	
	GUI_EnableAlpha(1);
	
	GUI_SetAlpha(200);
	
	GUI_FillRect(20, 40, 226, 90);
	
	GUI_FillRect(234, 40, 440, 90);
	
	GUI_SetAlpha(120);
	
	GUI_SetColor(GUI_DARKGRAY);
	
	GUI_FillRect(20, 90, 226, 240);
	
	GUI_FillRect(234, 90, 440, 240);;
	
	GUI_SetAlpha(0);
	
	GUI_MEMDEV_Select(0);
	
	GUI_MEMDEV_CopyToLCDAt(home1_hmem, 0, 20);	
}

void show_home(void)
{		
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
}

void set_kill(int kill)
{	
	info.kill = kill;	
}

void set_killed(int killed)
{
	info.killed = killed;	
}

void set_headshot(int headshot)
{
	info.headshot = headshot;	
}

void set_headshoted(int headshoted)
{
	info.headshoted = headshoted;
}

void set_blod(int blod)
{
	info.blod = blod;	
}

void set_ammo(int ammo)
{
	info.ammo = ammo;	
}

void set_our(int our)
{
	info.our = our;
}

void set_enemy(int enemy)
{
	info.enemy = enemy;	
}

void clock_show(void)
{
	char time[10];
	u8 hour, min, sec;
	int ret;

	memset(time, 0 , sizeof(time));
	
	//GUI_SetBkColor(GUI_TRANSPARENT);
	
	if (get_time(&hour, &min, &sec) == 0)
		return;
	
	sprintf(time, "%02d:%02d:%02d", hour, min, sec);
	
	GUI_SetFont(&GUI_Font16_ASCII);
	GUI_SetColor(GUI_WHITE);
	GUI_DispStringAt(time, 240, 4);		
}

void upiterm_show(void)
{
	static GUI_MEMDEV_Handle iterm_hmem = 0;
	
	if (iterm_hmem == 0) 
		iterm_hmem = GUI_MEMDEV_Create(0, 0, 480, 20);
	
	GUI_MEMDEV_Select(iterm_hmem);
	
	GUI_SetColor(GUI_BLACK);
	
	GUI_FillRect(0, 0, 479, 20);

	battery_show(get_power());

	clock_show();
	
	GUI_MEMDEV_Select(0);
	
	GUI_MEMDEV_CopyToLCD(iterm_hmem);
	
}

static void Draw(void *data)
{
	GUI_AUTODEV_INFO *pAutoDevInfo = data;

	if (pAutoDevInfo->DrawFixed) {
		GUI_SetColor(GUI_BLACK);	
		GUI_FillRect(0, 0, 480, 20);		
	}
	
	battery_show(20);
	clock_show();	
}

void upiterm_show1(void)
{
	GUI_AUTODEV AutoDev;
	GUI_AUTODEV_INFO AutoDevInfo;
	
	GUI_MEMDEV_CreateAuto(&AutoDev);
	
	GUI_MEMDEV_DrawAuto(&AutoDev, /* Use GUI_AUTODEV-object for drawing */
							&AutoDevInfo,
							&Draw,
							&AutoDevInfo);
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

static void update_iterm_task(void)
{
	//GUI_AUTODEV AutoDev;
	//GUI_AUTODEV_INFO AutoDevInfo;
	
	//GUI_MEMDEV_CreateAuto(&AutoDev);
	
	//set_time(0x58587480);
	
	while (1) {
		//GUI_MEMDEV_DrawAuto(&AutoDev, /* Use GUI_AUTODEV-object for drawing */
						//	&AutoDevInfo,
							//&Draw,
							//&AutoDevInfo);
		upiterm_show();
		msleep(500);
	}
}

void pic_preload(void)
{
	OS_ERR err;
#if 1		
    OSTaskCreate((OS_TCB *)&iterm_task_tcb, 
            (CPU_CHAR *)"net reciv task", 
            (OS_TASK_PTR)update_iterm_task, 
            (void * )0, 
            (OS_PRIO)OS_TASK_TIMER_PRIO, 
            (CPU_STK *)&iterm_task_stk[0], 
            (CPU_STK_SIZE)ITERM_TASK_STACK_SIZE/10, 
            (CPU_STK_SIZE)ITERM_TASK_STACK_SIZE, 
            (OS_MSG_QTY) 0, 
            (OS_TICK) 0, 
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR*)&err);	
#endif		
	if (home1_hmem == 0)
		home1_hmem = GUI_MEMDEV_Create(0, 0, 480, 320);

	if (bg_hmem == 0)
		bg_hmem = GUI_MEMDEV_Create(0, 0, 480, 320);
	
	GUI_MEMDEV_Select(bg_hmem);	
		
	GUI_BMP_DrawEx(spi_flash_get_bg, NULL, 0, 0);
}

int _cbGetData16(U32 Off, U16 NumBytes, void * pVoid, void * pBuffer)
{
	flash_bytes_read(FONT_BASE_ADDR + Off, pBuffer, NumBytes);
	
	return 0;
}

void XBF_font_init(void)
{
	int ret = GUI_XBF_CreateFont(&XBFFont, 
						&XBF_Data, 
						GUI_XBF_TYPE_PROP, 
						_cbGetData16, 
						NULL); 
	if (ret != 0) {
		GUI_Clear();
		
		GUI_DispStringAt("Please downlaod font file", 10, 200);
		
		while (1)
			sleep(1);
	}
}

void display_hanzi(char *src, int x, int y)
{
	GUI_SetColor(GUI_WHITE);
	
	GUI_SetTextMode(GUI_TEXTMODE_TRANS);
	
	GUI_UC_SetEncodeUTF8();
	
	GUI_SetFont(&XBFFont);
	
	GUI_DispStringAt(src, x, y);
}

void display_en(char *src, int x, int y)
{
	GUI_SetColor(GUI_WHITE);
	
	GUI_SetTextMode(GUI_TEXTMODE_TRANS);

	GUI_SetFont(GUI_FONT_8X16);
	
	GUI_DispStringAt(src, x, y);	
}

void display_key(void)
{
	char msg[] = "\请插入\\";
	
	display_hanzi(msg, 100, 100);
	display_en("key...", 170, 101);
}

void read_spi_flash_header(void)
{
	flash_bytes_read(0, (u8 *)&img_section, sizeof(img_section));
	
	if (img_section.pic_num >= 1 && img_section.pic_num <= 64)
		return;
	
	GUI_Clear();
	
	display_en("Please download Image file", 10, 100);
	
	while (1)
		sleep(1);
}
