#include "includes.h"
#include "priority.h"
#include "net.h"
#if 0
#ifdef CLOTHE

static volatile u16 blod;
static OS_SEM blod_sem;

void wake_up_blod_task(void)
{
	OS_ERR err;
	OSSemPost(&blod_sem, OS_OPT_POST_ALL, &err);
}

void wait_for_blod_change(void)
{
	OS_ERR err;
	
	OSSemPend(&blod_sem, NULL, OS_OPT_PEND_BLOCKING, NULL, &err);
}

void set_lifeLeft(s8 v)
{
	blod = v;
	if (blod > 100)
		blod = 100;	
}

s8 get_lifeLeft(void)
{
	return blod;
}

void add_lifeLeft(s8 v)
{
	v += get_lifeLeft();
	set_lifeLeft(v);
}

int xjc_ct = 0;

void reduce_blod(s8 i)
{
	if (blod <= 0)
		return;
	
	if (i == 0)
		--blod;
	else {
		blod -= i;
		blod = blod < (s8)0 ? (s8)0 : (s8)blod;
	}
	xjc_ct++;
	wake_up_blod_task();
}

void blod_init(void)
{
	OS_ERR err;

	OSSemCreate(&blod_sem, "blod Sem", 0, &err);
}

#endif
#endif