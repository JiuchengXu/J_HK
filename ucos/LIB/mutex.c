#include "os.h"

struct mutex_lock {
	OS_SEM sem;
};

struct wait {
	OS_SEM sem;
};

void mutex_init(struct mutex_lock *lock)
{
	OS_ERR	err;
	
	OSSemCreate(&lock->sem, "mutex_lock", 1, &err);
}

void mutex_lock(struct mutex_lock *lock)
{
	OS_ERR	err;
	
	OSSemPend(&lock->sem, 0, OS_OPT_PEND_BLOCKING, 0, &err);
}

void mutex_unlock(struct mutex_lock *lock)
{
	OS_ERR	err;
	
	OSSemPost(&lock->sem, OS_OPT_POST_1, &err);	
}

void wait_init(struct wait *wait)
{
	OS_ERR	err;
	
	OSSemCreate(&wait->sem, "mutex_lock", 0, &err);
}

void wait_for(struct wait *wait)
{
	OS_ERR	err;
	
	OSSemPend(&wait->sem, 0, OS_OPT_PEND_BLOCKING, 0, &err);
}

void wake_up(struct wait *wait)
{
	OS_ERR	err;
	
	OSSemPost(&wait->sem, OS_OPT_POST_1, &err);	
}


