#include <includes.h>


#define ACTIVED		1
#define NON_ACTIVED	0
static int actived_flag;


int status_get_actived(void)
{
	return actived_flag == ACTIVED;
}

void status_set_actived(void)
{
	actived_flag = ACTIVED;
}
void status_set_non_actived(void)
{
	actived_flag = NON_ACTIVED;
}

