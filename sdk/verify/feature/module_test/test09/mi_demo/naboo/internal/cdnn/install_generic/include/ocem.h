/*****************************************************************************\
* CEVA Confidential property.
* Copyright (C) Ceva Inc. All rights reserved.
*
* This file constitutes proprietary and confidential information of CEVA Inc.
* Any use or copying of this file or any information contained in this file other
* than as expressly approved in writing by Ceva, Inc. is strictly prohibited.
* Any disclosure or distribution of this file or any information contained in
* this file except to the intended recipient is strictly prohibited.
\*****************************************************************************/

/**
@file ocem.h
@brief CEVA-XM OCEM driver library

@par Description:
The OCEM driver is used to configure the XM On-Chip Emulation Module hardware.
*/


#ifndef _INCLUDE_OCEM_H_
#define _INCLUDE_OCEM_H_


#if defined XM4
#include "xm4/ocem.h"
#elif defined XM6
#include "xm6/ocem.h"
#else
// OCEM funcionality is not available in MSVS simulation
#define prof_counter_config(id,e,l,h)  ((void)0)
#define PROF_RESET(mask)     ((void)0)
#define PROF_PAUSE(mask)     ((void)0)
#define PROF_RESUME(mask)    ((void)0)
#define PROF_START(mask)     ((void)0)
#define PROF_READ_CNT(id)    (0)
#define PROF_READ_FRCC()     (0)
#endif


#endif /* _INCLUDE_OCEM_H_ */
