#ifndef __MUTEX_H__
#define __MUTEX_H__

struct mutex_lock {
	OS_SEM sem;
};

struct wait {
	OS_SEM sem;
};

void mutex_init(struct mutex_lock *lock);
void mutex_lock(struct mutex_lock *lock);
void mutex_unlock(struct mutex_lock *lock);

void wait_init(struct wait *wait);

void wait_for(struct wait *wait);
void wake_up(struct wait *wait);

#endif /* __MUTEX_H__ */
