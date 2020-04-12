/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (??Sigmastar Confidential Information??) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#ifndef _ST_COMMON_H
#define _ST_COMMON_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>

/*
 * MACROS FUNC
 */
 #ifndef ExecFunc
#define ExecFunc(_func_, _ret_) \
        if (_func_ != _ret_)\
        {\
            printf("Test [%d]exec function failed\n", __LINE__);\
            return 1;\
        }\
        else\
        {\
            printf("Test [%d]exec function pass\n", __LINE__);\
        }
#endif
#define STCHECKRESULT(result)\
    if (result != MI_SUCCESS)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("(%s %d)exec function pass\n", __FUNCTION__,__LINE__);\
    }

#define STDBG_ENTER() \
    printf("\n"); \
    printf("[IN] [%s:%s:%d] \n", __FILE__, __FUNCTION__, __LINE__); \
    printf("\n"); \

#define STDBG_LEAVE() \
    printf("\n"); \
    printf("[OUT] [%s:%s:%d] \n", __FILE__, __FUNCTION__, __LINE__); \
    printf("\n"); \

#define ST_RUN() \
    printf("\n"); \
    printf("[RUN] ok [%s:%s:%d] \n", __FILE__, __FUNCTION__, __LINE__); \
    printf("\n"); \

#define COLOR_NONE          "\033[0m"
#define COLOR_BLACK         "\033[0;30m"
#define COLOR_BLUE          "\033[0;34m"
#define COLOR_GREEN         "\033[0;32m"
#define COLOR_CYAN          "\033[0;36m"
#define COLOR_RED           "\033[0;31m"
#define COLOR_YELLOW        "\033[1;33m"
#define COLOR_WHITE         "\033[1;37m"

#define ST_NOP(fmt, args...)
#define ST_DBG(fmt, args...) ({do{printf(COLOR_GREEN"[DBG]:%s[%d]: "COLOR_NONE, __FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);})
#define ST_WARN(fmt, args...) ({do{printf(COLOR_YELLOW"[WARN]:%s[%d]: "COLOR_NONE, __FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);})
#define ST_INFO(fmt, args...) ({do{printf("[INFO]:%s[%d]: \n", __FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);})
#define ST_ERR(fmt, args...) ({do{printf(COLOR_RED"[ERR]:%s[%d]: "COLOR_NONE, __FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);})

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif
#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])

#define MAKE_YUYV_VALUE(y,u,v) ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK MAKE_YUYV_VALUE(0,128,128)
#define YUYV_WHITE MAKE_YUYV_VALUE(255,128,128)
#define YUYV_RED MAKE_YUYV_VALUE(76,84,255)
#define YUYV_GREEN MAKE_YUYV_VALUE(149,43,21)
#define YUYV_BLUE MAKE_YUYV_VALUE(29,225,107)

/***************************************************************************************************
 * MACROS VAL
 **************************************************************************************************/
#define MAX_CHANNEL_NUM 16
#define MAX_FILENAME_LEN 128
#define ALIGN_N(x, align)           (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_BACK(x, a)            (((x) / (a)) * (a))

#define MAX_VDF_NUM_PER_CHN         16

#define MAX_SUB_DESC_NUM            16
#define MAX_CHN_NUM                 64
#define NALU_PACKET_SIZE            512*1024
#define VIF_MAX_CHN_NUM 32 //16Main Vif + 16Sub Vif

#define DISP_PORT 3
#define VDISP_PORT 2
#define MAIN_VENC_PORT 0
#define SUB_VENC_PORT 1

/***************************************************************************************************
 * ENUM
 **************************************************************************************************/
typedef enum
{
    E_ST_SPLIT_MODE_ONE = 0,
    E_ST_SPLIT_MODE_TWO,
    E_ST_SPLIT_MODE_FOUR,
    E_ST_SPLIT_MODE_SIX,
    E_ST_SPLIT_MODE_EIGHT,
    E_ST_SPLIT_MODE_NINE,
    E_ST_SPLIT_MODE_NINE_EX,        /* 9画面扩展不规则分屏,实际显示6画面(1大+5小) */
    E_ST_SPLIT_MODE_SIXTEEN,
    E_ST_SPLIT_MODE_SIXTEEN_EX,
    E_ST_SPLIT_MODE_MAX,
} ST_SplitMode_e;

typedef enum
{
    E_ST_CHNTYPE_FILE = 0,
    E_ST_CHNTYPE_VIF,
    E_ST_CHNTYPE_VDEC_LIVE,
    E_ST_CHNTYPE_VDEC_PLAYBACK,
    E_ST_CHNTYPE_VIF_ZOOM,
    E_ST_CHNTYPE_VDEC_ZOOM,
    E_ST_CHNTYPE_MAX,
} ST_ChnType_e;

typedef enum
{
    E_ST_SWITCH_LIVE = 0, /* 实时切屏 */
    E_ST_SWITCH_PLAYBACK, /* 回放切屏 */
    E_ST_SWITCH_MAX,
} ST_SwitchMode_e;

typedef enum
{
    E_ST_CHANNEL_HIDE = 0,
    E_ST_CHANNEL_SHOW,
    E_ST_CHANNEL_MAX,
} ST_ChannelStatus_e;

typedef enum
{
    E_ST_FMT_ARGB1555 = 0,
    E_ST_FMT_ARGB8888,
    E_ST_FMT_YUV422,
    E_ST_FMT_YUV420,
    E_ST_FMT_YUV444,
    E_ST_FMT_MAX,
} ST_ColorFormat_e;

typedef enum
{
    E_ST_TIMING_720P_50 = 0,
    E_ST_TIMING_720P_60,
    E_ST_TIMING_1080P_50,
    E_ST_TIMING_1080P_60,
    E_ST_TIMING_1600x1200_60,
    E_ST_TIMING_1440x900_60,
    E_ST_TIMING_1280x1024_60,
    E_ST_TIMING_1024x768_60,
    E_ST_TIMING_3840x2160_30,
    E_ST_TIMING_3840x2160_60,
    E_ST_TIMING_MAX,
} ST_DispoutTiming_e;

typedef enum
{
    E_ST_VIF_CHN = 0x01,
    E_ST_VDEC_CHN = 0x02,
    E_ST_VIF_VENC_CHN = 0x04,
    E_ST_VIF_RGN_CHN = 0x08,
    E_ST_VDEC_RGN_CHN = 0x10,
    E_ST_VDF_CHN = 0x20,
    E_ST_VDISP_CHN = 0x40,
    E_ST_VIF_CHN_MIPI = 0x80,
    E_ST_CHN_TYPE_MAX,
} ST_VideoChnType_e;

/***************************************************************************************************
 * STRUCTURES
 **************************************************************************************************/

typedef struct ST_Sys_Rect_s
{
    MI_S32 s32X;
    MI_S32 s32Y;
    MI_U16 u16PicW;
    MI_U16 u16PicH;
} ST_Rect_t;

typedef struct ST_Sys_TestDataInfo_s
{
    ST_ColorFormat_e eColorFmt;
    MI_U16 u16PicWidth;
    MI_U16 u16PicHeight;
    MI_U32 u32RunTimes;
    MI_U8 au8FileName[MAX_FILENAME_LEN];
    MI_U8 au8OutputFileName[MAX_FILENAME_LEN]; //port*4-->max
} ST_Sys_TestDataInfo_t; //Yuv test file info


typedef struct ST_VifChnConfig_s
{
    MI_U8 u8ViDev;
    MI_U8 u8ViChn;
    MI_U8 u8ViPort; //main or sub
} ST_VifChnConfig_t;

typedef struct ST_Sys_BindInfo_s
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
} ST_Sys_BindInfo_t;


/***************************************************************************************************
 * FUNC
 **************************************************************************************************/
MI_S32 ST_Sys_Init(void);
MI_S32 ST_Sys_Exit(void);

MI_S32 ST_Sys_Bind(ST_Sys_BindInfo_t *pstBindInfo);
MI_S32 ST_Sys_UnBind(ST_Sys_BindInfo_t *pstBindInfo);
MI_U64 ST_Sys_GetPts(MI_U32 u32FrameRate);

#endif //_ST_COMMON_H
