#include "includes.h"
#include "net.h"
#include "priority.h"
#include "esp8266.h"
#include "helper.h"
#include "key.h"
#include "power.h"

#ifdef GUN
#if 1

#define OS_TASK_STACK_SIZE   	 128

static CPU_STK  TaskStk[OS_TASK_STACK_SIZE];
static OS_TCB TaskStkTCB;

static void led_ctrl_task(void *data)
{
	while (1) {
		if (status_get_actived() && !bulet_empty() && !saver_on() && bulet_box_online()) {
			green_led_on();
		} else
			blue_led_on();
		
		sleep(1);
	}
}

static void led_status_task_create(void)
{
	OS_ERR err;
	
	blue_led_on();
	
	OSTaskCreate((OS_TCB *)&TaskStkTCB, 
		(CPU_CHAR *)"led control", 
		(OS_TASK_PTR)led_ctrl_task, 
		(void * )0, 
		(OS_PRIO)OS_TASK_LED_PRIO, 
		(CPU_STK *)&TaskStk[0], 
		(CPU_STK_SIZE)OS_TASK_STACK_SIZE/10, 
		(CPU_STK_SIZE)OS_TASK_STACK_SIZE, 
		(OS_MSG_QTY) 0, 
		(OS_TICK) 0, 
		(void *)0,
		(OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
		(OS_ERR*)&err);
}

static void key_insert_handle(void)
{
	int bulet;
		
	if (net_reinit() < 0)
		NVIC_SystemReset();
	
	bulet_read_from_key();

	ok_notice();
}
#endif
void main_loop(void)
{
	int i;
	
	//led_status_task_create();

	bulet_box_init();
#if 1
retry:	
	key_init();

	blue_led_on();

	if (net_init() < 0)
		goto retry;

	bulet_box_init();
#endif	

	ok_notice();

	//watch_dog_feed_task_init();
	
	while (1) {
		if (key_get_fresh_status())
			key_insert_handle();

		if (status_get_actived() && tirgger_pressed())
			bulet_dec();
		
		if (bolt_pulled())
			bulet_inc();

		msleep(100);
	}
#if 0
	while (1) {		
		if (key_get_fresh_status())
			key_insert_handle();

		if (is_actived()) {	
			if (check_pull_bolt() > 0) {
				play_bolt();

				err_log("press bolt\r\n");

				msleep(100);

				handle_bolt_pull();
			}

			if (is_single_mode() && bulet_one_bolt > 0) {
				s8 a = trigger_get_status();

				switch (a) {
					case 1 :
						if (status == 0) {
							play_bulet();


							send_charcode(characCode);

							err_log("press trigger\r\n");

							status = 1;

							bulet_one_bolt--;
							local_bulet--;

							upload_status_data();

							msleep(500);
						}
						break;
					case 0 :
						if (status == 1) {
							status =  0;
						}
						break;				
				}
			} else if (is_auto_mode(mode) && bulet_one_bolt > 0) {
				if (trigger_get_status()) {
					wav_play(2);


					for (i = 0; i < 4 && bulet_one_bolt > 0; i++) {
						send_charcode(characCode);
						msleep(200);
						bulet_one_bolt--;
						local_bulet--;
					}

					err_log("press trigger\r\n");

					upload_status_data();

				}
			}

			if (local_bulet <= 0) {
				local_bulet = 0;
				actived = 0;
				err_log("local bulet is 0\r\n");
			}

			if (bulet_one_bolt <= 0)
				bulet_one_bolt = 0;
		}	

		if (actived && bulet_one_bolt > 0 && mode != 0)
			green_led_on();
		else
			blue_led_on();			

		msleep(100);
	}
#endif
}
#endif
