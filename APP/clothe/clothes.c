#include "includes.h"
#include "net.h"
#include "esp8266.h"
#include "priority.h"
#include "helper.h"
#include "key.h"
#include "power.h"
#include "time.h"
#include "led.h"
#include "blod.h"

#ifdef CLOTHE

//#define HOST_IP		(((u32)192 << 24) | ((u32)168 << 16) | ((u32)0 << 8) | 102)

void main_loop(void)
{
	u32 cha5rcode;
	
	int ret;
	int tmp;

	key_init();
	
	blue_led_on();

	net_init();

	blod_init();
	
	green_led_on();

	ok_notice();

	//watch_dog_feed_task_init();

	clear_receive();

	while (1) {	
recheck:		
		if ((ret = is_shot()) > 0) {
			startup_motor();

			if (is_dead()) {
				clothe_led("all", 1);

				work_flag_dipatch_gun(STOP_WORK);

				while (1) {
					if (key_get_fresh_status()) {
						if (is_dead())
							work_flag_dipatch_gun(START_WORK);

						blod_read_from_key();

						//upload_status_data();

						ok_notice();

						clothe_led("all", 0);
						msleep(200);
						clothe_led("all", 1);
						msleep(200);
						clothe_led("all", 0);

						clear_receive();

						break;
					}

					msleep(200);

				}
			} else {
				clothe_led_on_then_off(ret, 0xf0, 1);				
				//upload_status_data();
			}

			goto recheck;
		}

		if (key_get_fresh_status()) {
			blod_read_from_key();

			ok_notice();

			//upload_status_data();			
		}

		msleep(250);
	}	
}
#endif
