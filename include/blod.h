#ifndef __BLOD__H__
#define __BLOD__H__

void wake_up_blod_task(void);
void wait_for_blod_change(void);
s8 get_lifeLeft(void);
void add_lifeLeft(s8 v);
void reduce_blod(s8 i);


#endif
