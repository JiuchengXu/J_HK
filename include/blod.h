#ifndef __BLOD__H__
#define __BLOD__H__

int get_blod(void);

void dec_blod(int i);

void blod_init(void);

int is_live(void);

int is_dead(void);

void blod_read_from_key(void);


#endif
