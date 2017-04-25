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

static struct info_statistac info, info_bak;

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

int statistic_changed(void)
{
	int ret = memcmp(&info, &info_bak, sizeof(info));
	
	info_bak = info;
	
	return ret == 0;
}

int get_kill(void)
{	
	return info.kill;	
}

int get_killed(void)
{
	return info.killed;	
}

int get_headshot(void)
{
	return info.headshot;	
}

int get_headshoted(void)
{
	return info.headshoted;
}

int get_blod(void)
{
	return info.blod;	
}

int get_ammo(void)
{
	return info.ammo;	
}

int get_our(void)
{
	return info.our;
}

int get_enemy(void)
{
	return info.enemy;	
}