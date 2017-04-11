#include "GUI.h"
#include <stdio.h>
#include "string.h"

static GUI_CONST_STORAGE GUI_COLOR ColorsBatteryEmpty_27x14[] = {
     0x0000FF,0x000000,0xD7A50F,0xF0D26C
    ,0xFFFFFF
};

static GUI_CONST_STORAGE GUI_LOGPALETTE PalBatteryEmpty_27x14 = {
  5,	// number of entries
  1, 	// Has transparency
  &ColorsBatteryEmpty_27x14[0]
};

static GUI_CONST_STORAGE unsigned char acBatteryEmpty_27x14[] = {
  0x00, 0x04, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x00,
  0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x40,
  0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40,
  0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40,
  0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40,
  0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40,
  0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40,
  0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40,
  0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40,
  0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40,
  0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40,
  0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x40,
  0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x40,
  0x00, 0x04, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x00
};

static GUI_CONST_STORAGE GUI_BITMAP _bmBatteryEmpty_27x14 = {
  27, // XSize
  14, // YSize
  14, // BytesPerLine
  4, // BitsPerPixel
  acBatteryEmpty_27x14,  // Pointer to picture data (indices)
  &PalBatteryEmpty_27x14   // Pointer to palette
};

void battery_show_at(int p, int x, int y)
{
	float v;

	v = (float)p / 5;
	
	if (v == 0)
		return;
	
	GUI_DrawBitmap(&_bmBatteryEmpty_27x14, x, y);

	if (p <= 15)
		GUI_SetColor(GUI_RED);
	else
		GUI_SetColor(GUI_GREEN);
	
	GUI_FillRect(x + 23 - (int)v,  y +  2, x + 24, y + 11);		
}

void lcd_battery_show(int p)
{	
	float v;
			
	battery_show_at(p, 450, 4);
}

void clothe_battery_show(int p)
{
	float v;
			
	battery_show_at(p, 140, 4);	
}

void gun_battery_show(int p)
{
	float v;
			
	battery_show_at(p, 180, 4);	
}