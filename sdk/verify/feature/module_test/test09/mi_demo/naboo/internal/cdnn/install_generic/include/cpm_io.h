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

#ifndef _CPM_IO_H_
#define _CPM_IO_H_

#include "cevaxm.h"

#if defined XM4 || defined XM6
#include <vec-c.h>
#define cpm_out(reg_addr, val)	out(cpm, (val), (unsigned int*)(reg_addr))
#define cpm_in(reg_addr)		in(cpm, (unsigned int*)(reg_addr))
#else 
#define cpm_out(reg_addr, val) ((void)0)
#define cpm_in(reg_addr) (0)
#endif

#endif //#ifndef _CPM_IO_H_
