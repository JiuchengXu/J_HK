#include "includes.h"
#include "libe2prom.h"
#include "helper.h"
#include "priority.h"
#include "key.h"

#ifdef CLOTHE
#define I2C                   I2C2
#define KEY_INT_GPIO			GPIOA
#define KEY_INT_GPIO_PIN		GPIO_Pin_0
#define KEY_INT_RCC				RCC_APB2Periph_GPIOA
#endif

#ifdef GUN
#define I2C                   I2C1
#define KEY_INT_GPIO			GPIOA
#define KEY_INT_GPIO_PIN		GPIO_Pin_0
#define KEY_INT_RCC			RCC_APB2Periph_GPIOA
#endif

#ifdef LCD
#define I2C                   I2C1
#define KEY_INT_GPIO			GPIOB
#define KEY_INT_GPIO_PIN		GPIO_Pin_7
#define KEY_INT_RCC				RCC_APB2Periph_GPIOB
#endif


#define AT24Cx_Address           0xa6
#define AT24Cx_PageSize          8 


struct eeprom_key_info {
	char sn[16];
	char user_id[16];
	char ip_suffix[3];
	char blod_def[3];
	char menoy[3];
};

struct eeprom_special_key {
	char key[16];
};

union key {
	struct eeprom_key_info gen_key;
	struct eeprom_special_key spec_key;
};

enum {
	KEY_UNINSERT = 0,
	KEY_INSERTING,
	KEY_INSERTED,
	KEY_UNUSED,
};

#define OS_TASK_STACK_SIZE   	 128

static CPU_STK  TaskStk[OS_TASK_STACK_SIZE];
static OS_TCB TaskStkTCB;

static struct eeprom_key_info gen_key;
static struct eeprom_special_key spec_key;

static volatile s8 key_fresh_status;

//char eeprom[] = "SN145784541458900000015332453213252064064";//假衣服
static char eeprom[] = "SN145784541458890000015332453214253064064"; //真衣服
//char eeprom[] = "SN145784541458880000015332453215252064064";

extern void upload_spec_key(u8 *key);

static void key_Reads(u8 Address, u8 *ReadBuffer, u16 ReadNumber)
{
	e2prom_Reads(I2C, AT24Cx_Address, Address, ReadBuffer, ReadNumber);
}

static void key_Writes(u8 Address, u8 *WriteData, u16 WriteNumber)
{

	e2prom_WriteBytes(I2C, AT24Cx_Address, Address, WriteData, WriteNumber, AT24Cx_PageSize);
}

static void read_key_from_eeprom(void)
{
	memcpy(&gen_key, eeprom, sizeof(eeprom));
	
	return;
#ifdef CLOTHE
	union key tmp_key;
	
	key_Reads(0, (u8 *)&tmp_key, sizeof(tmp_key));
	
	if (memcmp(tmp_key.spec_key.key, "AKey", 4) == 0) {
		upload_spec_key((u8 *)tmp_key.spec_key.key);
		
	} else {		
		int2chars(tmp_key.gen_key.blod_def, 0, sizeof(tmp_key.gen_key.blod_def));
		
		key_Writes(0, (u8 *)&tmp_key, sizeof(tmp_key));
		
		gen_key = tmp_key.gen_key;
	}
#endif
	
#ifdef GUN
	key_Reads(0, (u8 *)&gen_key, sizeof(gen_key));
	
#endif
	
#ifdef LCD
	key_Reads(0, (u8 *)&gen_key, sizeof(gen_key));
#endif
}

static s8 key_get_gpio(void)
{	
	return GPIO_ReadInputDataBit(KEY_INT_GPIO, KEY_INT_GPIO_PIN) == Bit_RESET;
	//return 1;
}

static void key_state_machine_task(void)
{
	s8 status = KEY_UNUSED;
	s8 key_insert_flag;
	
	while (1) {
		key_insert_flag = key_get_gpio();
		
		switch (status) {
			case KEY_UNINSERT:
				if (key_insert_flag)
					status = KEY_INSERTING;
				break;
				
			case KEY_INSERTING:
				if (key_insert_flag)
					status = KEY_INSERTED;
				break;
				
			case KEY_INSERTED:
				read_key_from_eeprom();
			
				key_fresh_status = 1;

				status = KEY_UNUSED;
				// fall through
			case KEY_UNUSED:
				if (!key_insert_flag)
					status = KEY_UNINSERT;
				break;
		}
		msleep(250);
	}
}

void key_test(void)
{
	char a[]="SN145784541458890000015332453214253064064";
	
	key_Writes(0, (u8 *)a, sizeof(a));
	memset(a, 0, sizeof(a));
	key_Reads(0, (u8 *)a, sizeof(a));
}

void key_get_sn(char *s)
{
	memcpy(s, gen_key.sn, 16);
}

void key_get_ip_suffix(char *s)
{
	memcpy(s, gen_key.ip_suffix, 3);
}


s8 key_get_blod(void)
{
	s8 ret = (s8)char2u32(gen_key.blod_def, sizeof(gen_key.blod_def));
	int2chars(gen_key.blod_def, 0, sizeof(gen_key.blod_def));
	
	return ret;
}

static s8 _key_get_blod(void)
{
	return (s8)char2u32(gen_key.blod_def, sizeof(gen_key.blod_def));
}

s8 key_get_fresh_status(void)
{
	s8 ret = key_fresh_status;
	
	key_fresh_status = 0;
	
	return ret;
}


void key_init(void)
{
	OS_ERR err;
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
 	RCC_APB2PeriphClockCmd(KEY_INT_RCC, ENABLE);

	GPIO_InitStructure.GPIO_Pin  = KEY_INT_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
 	GPIO_Init(KEY_INT_GPIO, &GPIO_InitStructure);
	
	while (1) {
		red_intival();
		
		if (key_get_gpio()) {
			msleep(200);
			if (!key_get_gpio())
				continue;
			
			read_key_from_eeprom();
			
			if (_key_get_blod() == 0) //即使是新的key，但是key里面没有钱(血量)
				continue;
			
			break;
		}		
		
		msleep(200);
	}
	
	key_fresh_status = 0;
		
    OSTaskCreate((OS_TCB *)&TaskStkTCB, 
            (CPU_CHAR *)"key_state_machine_task", 
            (OS_TASK_PTR)key_state_machine_task, 
            (void * )0, 
            (OS_PRIO)OS_TASK_KEY_PRIO, 
            (CPU_STK *)&TaskStk[0], 
            (CPU_STK_SIZE)OS_TASK_STACK_SIZE/10, 
            (CPU_STK_SIZE)OS_TASK_STACK_SIZE, 
            (OS_MSG_QTY) 0, 
            (OS_TICK) 0, 
            (void *)0,
            (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
            (OS_ERR*)&err);
}

