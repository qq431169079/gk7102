/*
****************************************************************************
** \file      /applications/adidemo/demo/src/onvif.h
**
** \version   $Id: onvif.h du: cannot access 2016-10-21 11:34:08Z dengbiao $
**
** \brief     videc abstraction layer header file.
**
** \attention THIS SAMPLE CODE IS PROVIDED AS IS. GOFORTUNE SEMICONDUCTOR
**            ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**            OMMISSIONS.
**
** (C) Copyright 2015-2016 by GOKE MICROELECTRONICS CO.,LTD
**
****************************************************************************
*/

#ifndef __ONVIF_H__
#define __ONVIF_H__
//**************************************************************************
//**************************************************************************
//** Defines and Macros
//**************************************************************************
//**************************************************************************

//**************************************************************************
//**************************************************************************
//** Enumerated types
//**************************************************************************
//**************************************************************************

//**************************************************************************
//**************************************************************************
//** Data Structures
//**************************************************************************
//**************************************************************************

//**************************************************************************
//**************************************************************************
//** Global Data
//**************************************************************************
//**************************************************************************

//**************************************************************************
//**************************************************************************
//** API Functions
//**************************************************************************
//**************************************************************************
#ifdef __cplusplus
extern "C" {
#endif
int onvif_open(void);
int onvif_register_testcase(void);
#ifdef __cplusplus
}
#endif
#endif /* ONVIF_H */

