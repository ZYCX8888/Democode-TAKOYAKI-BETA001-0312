/********************************************************************************************************************************************************
Copyright (C) CEVA(R) Inc. All rights reserved

This information embodies materials and concepts, which are proprietary and confidential to CEVA Inc., and is made available solely pursuant to the
terms of a written license agreement, or NDA, or another written agreement, as applicable (“CEVA Agreement?, with CEVA Inc. or any of its subsidiaries
(“CEVA?.

This information can be used only with the written permission from CEVA, in accordance with the terms and conditions stipulated in the CEVA Agreement,
under which the information has been supplied and solely as expressly permitted for the purpose specified in the CEVA Agreement.
This information is made available exclusively to licensees or parties that have received express written authorization from CEVA to download or receive
the information and have agreed to the terms and conditions of the CEVA Agreement.
IF YOU HAVE NOT RECEIVED SUCH EXPRESS AUTHORIZATION AND AGREED TO THE CEVA AGREEMENT, YOU MAY NOT DOWNLOAD, INSTALL OR USE THIS INFORMATION.

The information contained in this document is subject to change without notice and does not represent a commitment on any part of CEVA. Unless
specifically agreed otherwise in the CEVA Agreement, CEVA make no warranty of any kind with regard to this material, including, but not limited to
implied warranties of merchantability and fitness for a particular purpose whether arising out of law, custom, conduct or otherwise.

While the information contained herein is assumed to be accurate, CEVA assumes no responsibility for any errors or omissions contained herein,
and assumes no liability for special, direct, indirect or consequential damage, losses, costs, charges, claims, demands, fees or expenses, of any nature
or kind, which are incurred in connection with the furnishing, performance or use of this material.

This document contains proprietary information, which is protected by U.S. and international copyright laws. All rights reserved.
No part of this document may be reproduced, photocopied, or translated into another language without the prior written consent of CEVA.
********************************************************************************************************************************************************/

/*
 * ceva_log.h
 *
 *  Created on: May 7, 2014
 *      Author: nirg
 */

#ifndef CEVA_LOG_H_
#define CEVA_LOG_H_


#if _MSC_VER >= 1300
#define __func__ __FUNCTION__
#endif	//_MSC_VER >= 1300

#define CEVA_LOG_NO_OUTPUT		0
#define CEVA_LOG_LEVEL_ERR		1
#define CEVA_LOG_LEVEL_WARNING	2
#define CEVA_LOG_LEVEL_INFO		3
#define CEVA_LOG_LEVEL_VERBOSE	4
 
//#define CEVA_CONFIG_LOG_LEVEL (CEVA_LOG_LEVEL_VERBOSE)
#ifndef CEVA_CONFIG_LOG_LEVEL
#define CEVA_LOG_LEVEL (CEVA_LOG_LEVEL_ERR)
#else
#define CEVA_LOG_LEVEL (CEVA_CONFIG_LOG_LEVEL)
#endif

#ifndef CEVA_LOG_TAG
#define CEVA_LOG_TAG "CEVA_LOG"
#endif // CEVA_LOG_TAG

#if (CEVA_LOG_LEVEL > CEVA_LOG_NO_OUTPUT)

#ifdef WIN32
#include <stdio.h>
#define	DEBUG_LEVEL(n)			(n == CEVA_LOG_LEVEL_VERBOSE)? "V/":(n == CEVA_LOG_LEVEL_INFO)? "I/":(n == CEVA_LOG_LEVEL_WARNING)? "W/" : "E/"
#define CEVA_LOG(n, fmt, ...)	do { if (CEVA_LOG_LEVEL >= (n)){fprintf(stderr,   "%s" CEVA_LOG_TAG ": " fmt "\n", DEBUG_LEVEL(n), __VA_ARGS__);} } while (0)
#else //defined (WIN32)
#ifdef __ANDROID__
#include <android/log.h>
#define	DEBUG_LEVEL(n)					(n == CEVA_LOG_LEVEL_VERBOSE)? ANDROID_LOG_VERBOSE:(n == CEVA_LOG_LEVEL_INFO)? ANDROID_LOG_INFO:(n == CEVA_LOG_LEVEL_WARNING)?ANDROID_LOG_WARN:ANDROID_LOG_ERROR
#define CEVA_LOG(n,fmt, ...)			do {if (CEVA_LOG_LEVEL >= (n)){__android_log_print(DEBUG_LEVEL(n), CEVA_LOG_TAG,fmt, __VA_ARGS__); } } while (0)
#else // __ANDROID__
#include <stdio.h>
#include <unistd.h>
#define	DEBUG_LEVEL(n)			(n == CEVA_LOG_LEVEL_VERBOSE)? "V/":(n == CEVA_LOG_LEVEL_INFO)? "I/":(n == CEVA_LOG_LEVEL_WARNING)? "W/" : "E/"
#define CEVA_LOG(n, fmt, ...)	do { if (CEVA_LOG_LEVEL >= (n)){fprintf(stderr,   "%s" CEVA_LOG_TAG ": [%s, %s (%d)] : " fmt "\n", DEBUG_LEVEL(n), __FILE__, __func__, __LINE__, __VA_ARGS__); usleep(100000);} } while (0)
#endif // __ANDROID__
#endif //defined (WIN32)
#else //CEVA_LOG_LEVEL > 0
#define CEVA_LOG(n, fmt, ...) 			do {} while (0)
#endif //CEVA_LOG_LEVEL > 0

#define CEVA_LOG_TRACE(msg)				CEVA_LOG(CEVA_LOG_LEVEL_VERBOSE, "%s, %s", __func__, msg)
#define CEVA_LOG_TRACEF(fmt,...)		CEVA_LOG(CEVA_LOG_LEVEL_VERBOSE, fmt, __VA_ARGS__)
#define CEVA_LOG_ERROR(msg)				CEVA_LOG(CEVA_LOG_LEVEL_ERR, "%s", msg)
#define CEVA_LOG_ERRORF(fmt,...)		CEVA_LOG(CEVA_LOG_LEVEL_ERR, fmt, __VA_ARGS__)
#define CEVA_LOG_INFO(msg)				CEVA_LOG(CEVA_LOG_LEVEL_INFO, "%s",msg)
#define CEVA_LOG_INFOF(fmt,...)			CEVA_LOG(CEVA_LOG_LEVEL_INFO, fmt, __VA_ARGS__)
#define CEVA_LOG_WARN(msg)				CEVA_LOG(CEVA_LOG_LEVEL_WARNING, "%s", msg)
#define CEVA_LOG_WARNF(fmt,...)			CEVA_LOG(CEVA_LOG_LEVEL_WARNING, fmt, __VA_ARGS__)
#define CEVA_LOG_VERBOSE(msg)			CEVA_LOG(CEVA_LOG_LEVEL_VERBOSE, "%s", msg)
#define CEVA_LOG_VERBOSEF(fmt,...)		CEVA_LOG(CEVA_LOG_LEVEL_VERBOSE, fmt, __VA_ARGS__)
#endif /* CEVA_LOG_H_ */
