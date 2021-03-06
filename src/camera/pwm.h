/*!
*****************************************************************************
** \file        ./adi/test/src/pwm.h
**
**
** \brief       adi test pwm module header file.
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2013-2014 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef _PWM_H_
#define _PWM_H_

#include "stdio.h"

//*****************************************************************************
//*****************************************************************************
//** Defines and Macros
//*****************************************************************************
//*****************************************************************************



//*****************************************************************************
//*****************************************************************************
//** Enumerated types
//*****************************************************************************
//*****************************************************************************




//*****************************************************************************
//*****************************************************************************
//** Data Structures
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//*****************************************************************************
//** API Functions
//*****************************************************************************
//*****************************************************************************

#ifdef __cplusplus
extern "C" {
#endif
GADI_ERR pwm_init(void);
GADI_ERR pwm_exit(void);
GADI_ERR pwm_open(GADI_U8 channel);
GADI_ERR pwm_close(void);
GADI_ERR pwm_start(void);
GADI_ERR pwm_stop(void);
GADI_ERR pwm_set_mode(GADI_U8 mode);
GADI_ERR pwm_set_speed(GADI_U32 speed);
GADI_ERR pwm_set_speed(GADI_U32 speed);
GADI_ERR pwm_set_duty(GADI_U32 duty);
GADI_ERR pwm_look(void);
int pwm_register_testcase(void);


#ifdef __cplusplus
    }
#endif


#endif

