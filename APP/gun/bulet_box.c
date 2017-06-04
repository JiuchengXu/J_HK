#include "includes.h"
#include "libe2prom.h"
#include "helper.h"

#if 0

#define I2C                   	I2C1

#define AT24Cx_Address           0xa2 
#define AT24Cx_PageSize          8 

static s8 local_bulet = 100;

static s8 bulet_box_online(void)
{
	//return GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3) == Bit_RESET;
	return 1;
}

s8 bulet_Read(void)
{
	//s8 bulet = 0;
	s8 bulet = 100;
	if (bulet_box_online()) {
		//e2prom_Reads(I2C, AT24Cx_Address, 0, (u8 *)&bulet, 1);
		bulet = local_bulet;
	}
	
	return bulet;
}

void bulet_Write(s8 bulet)
{
	if (bulet_box_online()) {
	//	e2prom_Writes(I2C, AT24Cx_Address, 0, (u8 *)&bulet, 1);
		local_bulet = bulet;
	}
}

s8 get_buletLeft(void)
{
	if (local_bulet == 0)
		local_bulet = bulet_Read();
	
	return local_bulet;
}

void set_buletLeft(s8 v)
{
	s8 bulet = v;
	if (bulet > 100)
		bulet = 100;
	
	bulet_Write(bulet);
}

void add_buletLeft(s8 v)
{
	v += get_buletLeft();
	set_buletLeft(v);
}

void reduce_bulet(void)
{
	s8 bulet = get_buletLeft();
	
	if (bulet > 0)
		bulet_Write(--bulet);
}

void bulet_box_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
 	GPIO_Init(GPIOC, &GPIO_InitStructure);	
}

void bulet_box_test(void)
{
	s8 a = 100;
	
	while (1) {
		bulet_Write(a);
		a = 0;
		a = bulet_Read();
		a += 1;
		msleep(1);
	}
}

#endif

#define BULET_ONE_BOLT	30
#define AUTO_MODE_BULET_DEC_NUM		4

static s16 local_bulet, bulet_one_bolt;

static s8 bulet_box_online(void)
{
	//return GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3) == Bit_RESET;
	return 1;
}

static void bulet_set(s16 bulet)
{
	if (bulet_box_online()) {
	//	e2prom_Writes(I2C, AT24Cx_Address, 0, (u8 *)&bulet, 1);
		if (bulet < 0)
			bulet = 0;
		
		local_bulet = bulet;
		upload_status_data(bulet);
	}
}

static s16 bulet_get(void)
{
//	if (local_bulet == 0)
//		local_bulet = bulet_Read();
	
	return local_bulet;
}

static s16 bulet_one_bolt_get(void)
{
	return bulet_one_bolt;
}

static void bulet_one_bolt_set(s16 bulet)
{
	if (bulet < 0)
		bulet = 0;
	
	bulet_one_bolt = bulet;
}

int bulet_empty(void)
{
	return bulet_one_bolt <= 0;
}

void bulet_dec(void)
{
	if (bulet_empty() || saver_on())
		return;
	
	if (is_auto_mode()) {
		s16 b = bulet_one_bolt_get();
		b = b - AUTO_MODE_BULET_DEC_NUM < 0? 0 : b - AUTO_MODE_BULET_DEC_NUM;
		bulet_one_bolt_set(b);
		
		b = bulet_get();
		b = b - AUTO_MODE_BULET_DEC_NUM < 0? 0 : b - AUTO_MODE_BULET_DEC_NUM;
		bulet_set(b);
		play_bulets();
		
	} else if (is_single_mode()) {
		s16 b = bulet_one_bolt_get();
		b = b - 1 < 0? 0 : b - 1;
		bulet_one_bolt_set(b);
		
		b = bulet_get();
		b = b - 1 < 0? 0 : b - 1;
		bulet_set(b);
		
		play_bulet();
	}
}

void bulet_inc(void)
{
	u16 b = bulet_get();
	b = b > BULET_ONE_BOLT ? BULET_ONE_BOLT : b;
	
	bulet_one_bolt_set(b);
	
	err_log("bolt is pulled\n\r");
	
	play_bolt();
}

void bulet_read_from_key(void)
{
	s16 b;
	
	b = key_get_bulet();
	
	bulet_set(b);
}

void bulet_box_init(void)
{
	local_bulet = key_get_bulet();
	bulet_one_bolt = 0;
	
	upload_status_data(local_bulet);
	
	err_log("bulet %d\r\n", local_bulet);
}
