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
#ifndef __VENC_COMMON_H
#define __VENC_COMMON_H

#include <sys/time.h>
#include "mi_venc_datatype.h"

#define ASCII_COLOR_RED                          "\033[1;31m"
#define ASCII_COLOR_WHITE                        "\033[1;37m"
#define ASCII_COLOR_YELLOW                       "\033[1;33m"
#define ASCII_COLOR_BLUE                         "\033[1;36m"
#define ASCII_COLOR_GREEN                        "\033[1;32m"
#define ASCII_COLOR_END                          "\033[0m"

typedef struct ChnPrivate_s ChnPrivate_t;
typedef struct Chn_s
{
    MI_VENC_ModType_e eModType;
    MI_U32 u32ChnId;//ID to self so that output thread could have channel ID info
    pthread_t tid;
    MI_SYS_BUF_HANDLE hBuf;
    ChnPrivate_t *pstPrivate;
} Chn_t;

typedef struct VENC_FPS_s
{
    struct timeval stTimeStart, stTimeEnd;
    struct timezone stTimeZone;
    MI_U32 u32DiffUs;
    MI_U32 u32TotalBits;
    MI_U32 u32FrameCnt;
    MI_BOOL bRestart;
} VENC_FPS_t;
#define TIMEVAL_US_DIFF(start, end)     ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct VENC_Rc_s
{
    MI_VENC_RcMode_e eRcMode;
    MI_U32 u32SrcFrmRateNum;
    MI_U32 u32SrcFrmRateDen;
    MI_U32 u32Bitrate; //u32MaxBitRate for VBR, u32BitRate for CBR, u32AvgBitRate for ABR
    MI_U32 u32AvrMaxBitrate; //u32MaxBitRate for ABR
    MI_U32 u32Gop;
    MI_U32 u32FixQp;
    MI_U32 u32VbrMinQp;
    MI_U32 u32VbrMaxQp;
} VENC_Rc_t;

//==== configuration variables ====
#ifdef DEF_CFG_VAR
#define CFG_VAR
#else
#define CFG_VAR extern
#endif
//These global variables are used by multiple files.
//They are set by a configuration system, which might be environment variable or #define somewhere.
CFG_VAR int VENC_GLOB_TRACE;
CFG_VAR int VENC_GLOB_TRACE_QUERY;
CFG_VAR int VENC_GLOB_IN_MSG_LEVEL;
CFG_VAR int VENC_GLOB_OUT_MSG_LEVEL;

//These variables are used in frame-end operations
CFG_VAR int iRequestIdr;
CFG_VAR int iEnableIdr;
CFG_VAR int iEnableIdrFrame;
CFG_VAR int iEnableIdrCnt;
CFG_VAR int VENC_CHANGE_CHN_ATTR;
CFG_VAR int VENC_ChangeHeight;
CFG_VAR int VENC_ChangeQp;
CFG_VAR int VENC_ChangeGop;
CFG_VAR int VENC_ChangeToFpsN;
CFG_VAR int VENC_ChangeToFpsM;
CFG_VAR int VENC_ChangeBitrate;

extern const MI_U32 u32QueueLen;
extern volatile MI_BOOL gbPanic;

#define VENC_MAX_CHN (VENC_MAX_CHN_NUM)
extern Chn_t _astChn[VENC_MAX_CHN];

#define DBG_INFO(fmt, args...)     ({do{printf(ASCII_COLOR_WHITE"[APP INFO]:%s[%d]: ", __FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_END);}while(0);})
#define DBG_ERR(fmt, args...)      ({do{printf(ASCII_COLOR_RED  "[APP ERR ]:%s[%d]: ", __FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_END);}while(0);})

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        DBG_ERR("[%4d]exec function failed\n", __LINE__);\
        return 1;\
    }\
    else if(VENC_GLOB_TRACE)\
    {\
        printf("(%4d)exec %s passed.\n", __LINE__, __FUNCTION__);\
    }

void trace_venc_channel(char * title, MI_VENC_CHN VeChn);

int setup_venc_fixed_rc(MI_U32 u32Gop, MI_U8 u8QpI, MI_U8 u8QpP);
int create_venc_channel(MI_VENC_ModType_e eModType, MI_VENC_CHN VencChannel, MI_U32 u32Width,
                        MI_U32 u32Height, VENC_Rc_t *pstVencRc);
int destroy_venc_channel(MI_VENC_CHN VencChannel);
void venc_stream_print(char* title, int iCh, MI_VENC_Stream_t *pstStream);
void *venc_channel_func(void *arg);
void sleep_ms(int milliseconds);

#endif
