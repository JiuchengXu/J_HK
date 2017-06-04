#include "includes.h"
#include "priority.h"
#include "net.h"

#ifdef CLOTHE

static int blod;

int get_blod(void)
{
	return blod;
}

void set_blod(int b)
{
	if (b > 1000)
		b = 1000;
	
	if (b < 0)
		b = 0;
	
	blod = b;
}

void inc_blod(int v)
{
	v += get_blod();
	set_blod(v);
}

void dec_blod(int i)
{
	if (get_blod() <= 0)
		return;

	set_blod(get_blod() - i);
}

int is_live(void)
{
	return blod > 0;
}

int is_dead(void)
{
	return blod <= 0;
}

void blod_read_from_key(void)
{
	blod += key_get_blod();

	if (blod > 0x7fff)
		blod = 0x7fff;
	
	upload_status_data(blod);
}

int is_shot(void)
{	
	int ret = irda_get_shoot_info();
	
	if (ret > 0) {
		if (is_shoot_head(ret))
			dec_blod(50);
		else
			dec_blod(10);
		
		upload_status_data(blod);
	}	
	
	return ret;
}

void blod_init(void)
{
	blod = key_get_blod();

	upload_status_data(blod);
}
#endif

