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
#ifndef _MIXER_COMMON_H_
#define _MIXER_COMMON_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <getopt.h>
#include "mi_sys.h"
#include "mi_vif.h"
#include "mi_vpe.h"
#include "mi_venc.h"
#include "mi_divp.h"
#include "mi_disp.h"
#include "mi_hdmi.h"


typedef enum
{
    MIXER_DBG_NONE = 0,
    MIXER_DBG_ERR,
    MIXER_DBG_WRN,
    MIXER_DBG_INFO,
    MIXER_DBG_DEBUG,
    MIXER_DBG_ALL
} MIXER_DBG_LEVEL_e;

#define ASCII_COLOR_RED                          "\033[1;31m"
#define ASCII_COLOR_WHITE                        "\033[1;37m"
#define ASCII_COLOR_YELLOW                       "\033[1;33m"
#define ASCII_COLOR_BLUE                         "\033[1;36m"
#define ASCII_COLOR_GREEN                        "\033[1;32m"
#define ASCII_COLOR_END                          "\033[0m"

extern MIXER_DBG_LEVEL_e mixer_debug_level;
extern MI_BOOL mixer_func_trace;

#define DBG_DEBUG(fmt, args...)     ({do{if(mixer_debug_level>=MIXER_DBG_DEBUG){printf(ASCII_COLOR_GREEN"[APP INFO]:%s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__,##args);}}while(0);})
#define DBG_INFO(fmt, args...)     ({do{if(mixer_debug_level>=MIXER_DBG_INFO){printf(ASCII_COLOR_GREEN"[APP INFO]:%s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__,##args);}}while(0);})
#define DBG_WRN(fmt, args...)      ({do{if(mixer_debug_level>=MIXER_DBG_WRN){printf(ASCII_COLOR_YELLOW"[APP WRN ]: %s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__, ##args);}}while(0);})
#define DBG_ERR(fmt, args...)      ({do{if(mixer_debug_level>=MIXER_DBG_ERR){printf(ASCII_COLOR_RED"[APP ERR ]: %s[%d]: " fmt ASCII_COLOR_END, __FUNCTION__,__LINE__, ##args);}}while(0);})
#define DBG_EXIT_ERR(fmt, args...) ({do{printf(ASCII_COLOR_RED"<<<%s[%d] " fmt ASCII_COLOR_END,__FUNCTION__,__LINE__,##args);}while(0);})
#define DBG_ENTER()                ({do{if(mixer_func_trace){printf(ASCII_COLOR_BLUE">>>%s[%d] \n" ASCII_COLOR_END,__FUNCTION__,__LINE__);}}while(0);})
#define DBG_EXIT_OK()              ({do{if(mixer_func_trace){printf(ASCII_COLOR_BLUE"<<<%s[%d] \n" ASCII_COLOR_END,__FUNCTION__,__LINE__);}}while(0);})


#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__,__LINE__);\
        return -1;\
    }

#endif //_MIXER_COMMON_H_