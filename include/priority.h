#ifndef __PRIO_H__
#define __PRIO_H__

#include "includes.h"

/* task priority */
#define OS_TASK_KEY_PRIO		  8
#define OS_TASK_RECV_PRIO         8

#if defined(LCD)
#define OS_TASK_TIMER_PRIO           9
#endif

#define OS_TASK_POWER_PRIO		16

/* interrupt irq priority */
#define WIFI_UART_INT_PRIO		0
#define USB_UART_INT_PRIO		1
#define RTC_INT_PRIO			2

#endif
