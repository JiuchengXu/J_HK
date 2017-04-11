#ifndef __DISPLAY_H__
#define __DISPLAY_H__

struct iterm_info {
	s8 hour;
	s8 mini;
	s8 sec;
	
	u8 clothe_pwr;
	u8 gun_pwr;
	u8 lcd_pwr;
	
	u16 main_bulet;
	u16 sub_bulet;	
};

struct statistic_info {
	char killCount;
	char bekillCount;
	char myTeamKillCount;
	char myTeamBeKillCount;
	char headShootCount;
	char headBeShootCount;		
};

extern void pic_preload(void);
extern void show_background(void);
extern void show_home(void);
extern void ProgBarShow(int);
void upiterm_show(struct iterm_info *info);

void set_kill(int kill);

void set_killed(int killed);

void set_headshot(int headshot);


void set_headshoted(int headshoted);


void set_blod(int blod);


void set_ammo(int ammo);


void set_our(int our);

void set_enemy(int enemy);


#endif