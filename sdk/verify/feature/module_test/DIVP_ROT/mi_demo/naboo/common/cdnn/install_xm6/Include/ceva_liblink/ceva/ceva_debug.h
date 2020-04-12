/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#ifndef CEVA_DEBUG_H
#define CEVA_DEBUG_H


#ifdef ANDROID
#include <android/log.h>
#define LIBLINK_DEBUG_LEVEL_ERROR			ANDROID_LOG_ERROR
#define LIBLINK_DEBUG_LEVEL_WARNING			ANDROID_LOG_WARN
#define LIBLINK_DEBUG_LEVEL_INFO			ANDROID_LOG_INFO
#define LIBLINK_DEBUG_LEVEL_DEBUG			ANDROID_LOG_DEBUG
#endif


#ifdef CEVA_LIBLINK_DEBUG
#define CEVA_LOG_TAG "CEVA_LIBLINK"
#ifdef ANDROID
#define DEBUG(n,fmt,args...) \
	__android_log_print(n, CEVA_LOG_TAG, "%s," fmt,__FUNCTION__,##args)

#define CEVA_LIBLINK_LOG_D(fmt,args...) DEBUG(LIBLINK_DEBUG_LEVEL_DEBUG, fmt, ##args)
#define CEVA_LIBLINK_LOG_I(fmt,args...) DEBUG(LIBLINK_DEBUG_LEVEL_INFO, fmt, ##args)
#define CEVA_LIBLINK_LOG_W(fmt,args...) DEBUG(LIBLINK_DEBUG_LEVEL_WARNING, fmt, ##args)
#define CEVA_LIBLINK_LOG_E(fmt,args...) DEBUG(LIBLINK_DEBUG_LEVEL_ERROR, fmt, ##args)

#else //android
#ifdef linux
#include <stdio.h>
#define CEVA_LOG(n, fmt, ...)	do { if (CEVA_LOG_LEVEL >= (n)){fprintf(stderr,   "%s" CEVA_LOG_TAG ": " fmt "\n", DEBUG_LEVEL(n), __VA_ARGS__);} } while (0)

#define CEVA_LIBLINK_LOG_D(fmt,args...) do {fprintf(stderr,   "D/" CEVA_LOG_TAG ": " fmt "\n",  ##args);} while (0)
#define CEVA_LIBLINK_LOG_I(fmt,args...) do {fprintf(stderr,   "I/" CEVA_LOG_TAG ": " fmt "\n",  ##args);} while (0)
#define CEVA_LIBLINK_LOG_W(fmt,args...) do {fprintf(stderr,   "W/" CEVA_LOG_TAG ": " fmt "\n",  ##args);} while (0)
#define CEVA_LIBLINK_LOG_E(fmt,args...) do {fprintf(stderr,   "E/" CEVA_LOG_TAG ": " fmt "\n",  ##args);} while (0)
#endif //linux
#endif //android
#else //CEVA_LIBLINK_DEBUG
#define CEVA_LIBLINK_LOG_D(fmt,args...)
#define CEVA_LIBLINK_LOG_I(fmt,args...)
#define CEVA_LIBLINK_LOG_W(fmt,args...)
#define CEVA_LIBLINK_LOG_E(fmt,args...)
#endif //CEVA_LIBLINK_DEBUG
#endif //CEVA_DEBUG_H
