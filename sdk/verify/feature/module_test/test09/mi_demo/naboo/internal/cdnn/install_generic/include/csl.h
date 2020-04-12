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

#ifndef __CSL_DRIVER_H__
#define __CSL_DRIVER_H__

/**
*
* @file csl.h
* @brief The main header file for the CEVA-XM CSL (Core Support Library)
*/
#include "cevaxm.h"
#include "ceva_hw_cfg.h"
#include "dma_driver.h"
#include "ocem.h"
#ifdef CEVAXM
	// This functionality is not available in MSVS simulation
	#include "cpm_io.h"
	#include "mss.h"
#endif

#endif //__CSL_DRIVER_H__
