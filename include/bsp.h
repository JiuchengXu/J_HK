/*
*********************************************************************************************************
*                                     MICIRUM BOARD SUPPORT PACKAGE
*
*                             (c) Copyright 2007; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                        BOARD SUPPORT PACKAGE
*
*                                     ST Microelectronics STM32
*                                              on the
*
*                                     Micrium uC-Eval-STM32F107
*                                        Evaluation Board
*
* Filename      : bsp.h
* Version       : V1.00
* Programmer(s) : EHS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                                 MODULE
*
* Note(s) : (1) This header file is protected from multiple pre-processor inclusion through use of the
*               BSP present pre-processor macro definition.
*********************************************************************************************************
*/

#ifndef  BSP_PRESENT
#define  BSP_PRESENT


/*
*********************************************************************************************************
*                                                 EXTERNS
*********************************************************************************************************
*/

#ifdef   BSP_MODULE
#define  BSP_EXT
#else
#define  BSP_EXT  extern
#endif


/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/

#include  <stdarg.h>
#include  <stdio.h>

#include  <cpu.h>
#include  <cpu_core.h>

#include  <lib_ascii.h>
#include  <lib_def.h>
#include  <lib_mem.h>
#include  <lib_str.h>

#include  <stm32f10x.h>

#include  <app_cfg.h>


void  BSP_Init (void);
CPU_INT32U   BSP_CPU_ClkFreq             (void);


#if ((APP_CFG_PROBE_OS_PLUGIN_EN == DEF_ENABLED) && \
     (OS_PROBE_HOOKS_EN          == 1))
void  OSProbe_TmrInit (void);
#endif

#if ((APP_CFG_PROBE_OS_PLUGIN_EN == DEF_ENABLED) && \
     (OS_PROBE_HOOKS_EN          == 1))
CPU_INT32U  OSProbe_TmrRd (void);
#endif

#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
void  CPU_TS_TmrInit (void);
#endif

#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
CPU_TS_TMR  CPU_TS_TmrRd (void);
#endif

void LED_Init(void);
void uart_inint(void);

void wifi_init(void);

void net_init(void);
void i2c_init(void);
void at24c02_init(void);

void esp8266_gpio_init(void);
void I2S_Bus_Init(void);
void adx345_init(void);
void SPI3_init(void);

#ifdef CLOTHE
void timer_init(void);
void irda_init(void);
void blod_init(void);
#endif

#ifdef GUN
void led_38k_init(void);
void wav_play(void);
void wav_pre_read(void);
void irda_init(void);
void bulet_box_init(void);
void trigger_init(void);
#endif

void beep_init(void);

#endif                                                          /* End of module include.                               */
