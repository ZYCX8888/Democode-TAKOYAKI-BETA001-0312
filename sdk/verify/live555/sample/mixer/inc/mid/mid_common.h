/*
* mid_common.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef _MID_COMMON_H_
#define _MID_COMMON_H_

#ifdef __cplusplus
extern "C"{
#endif    // __cplusplus

#include <stdio.h>
#include <fcntl.h>
#include <error.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>


#include "mi_sys.h"
#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_aio_datatype.h"
#include "mi_rgn_datatype.h"
#include "mi_vif_datatype.h"
#include "mi_vpe_datatype.h"
#include "mi_venc_datatype.h"


extern MI_S32 g_dbglevel;


#if    TARGET_CHIP_I5

#define MAX_VIF_DEV_NUMBER        2
#define MAX_VPE_PORT_NUMBER       4
#define MAX_VIDEO_STREAM          6
#define MAX_VIDEO_NUMBER          MAX_VIDEO_STREAM

#define MIXER_MAIN_STREAM         0
#define VPE_SUB_PORT              1
#define VPE_REALMODE_SUB_PORT     (-1)

#define VENC_ROTA0_MAX_W               3840
#define VENC_ROTA0_MAX_H               2160

#elif   TARGET_CHIP_I6

#define MAX_VIF_DEV_NUMBER        1
#define MAX_VPE_PORT_NUMBER       3
#define MAX_VIDEO_STREAM          6
#define MAX_VIDEO_NUMBER          MAX_VIDEO_STREAM

#define MIXER_MAIN_STREAM         1
#define VPE_SUB_PORT              2
#define MAIN_STREAM_CHNUMBER      0
#define VPE_REALMODE_SUB_PORT     (-1)
#define VENC_ROTA0_MAX_W               3840
#define VENC_ROTA0_MAX_H               2160


#elif     TARGET_CHIP_I6E

#define MAX_VIF_DEV_NUMBER        1
#define MAX_VPE_PORT_NUMBER       3
#define MAX_VIDEO_STREAM          6
#define MAX_VIDEO_NUMBER          MAX_VIDEO_STREAM

#define MIXER_MAIN_STREAM         1
#define VPE_SUB_PORT              2
#define MAIN_STREAM_CHNUMBER      0
#define VPE_REALMODE_SUB_PORT     (-1)
#define VENC_ROTA0_MAX_W               3840
#define VENC_ROTA0_MAX_H               2160

#elif     TARGET_CHIP_I6B0

#define MAX_VIF_DEV_NUMBER        1
#define MAX_VPE_PORT_NUMBER       4  //port 3 only for real DIVP
#define MAX_VIDEO_STREAM          6
#define MAX_VIDEO_NUMBER          MAX_VIDEO_STREAM

#define MIXER_MAIN_STREAM         1
#define VPE_SUB_PORT              2
#define VPE_REALMODE_SUB_PORT     3
#define MAIN_STREAM_CHNUMBER      0

#define VENC_ROTA0_MAX_W               3840
#define VENC_ROTA0_MAX_H               2160

#endif

#define MIXER_INVALID_CHANNEL_ID (-1)

#define typeof(x)   __typeof__(x)

#define MIXER_MAX_FPS             60
#define MIXER_DEFAULT_FPS         30

#define MIXER_MAX_ROI             8

#define MIXER_DIVP_BUFUSRDEPTH_DEF 0
#define MIXER_DIVP_BUFCNTQUOTA_DEF 3

#ifndef BOOL
#define BOOL        unsigned int
#endif
//typedef int BOOL;

#ifndef VOID
#define VOID        void
#endif
//typedef void    VOID;


#ifndef HANDLE
#define HANDLE      void*
#endif


typedef struct _Point_t
{
    MI_S32 x;
    MI_S32 y;
} Point_t;

typedef struct _Size_t
{
    MI_U32 width;
    MI_U32 height;
} Size_t;

typedef struct _Rect_s
{
    MI_U16 x;
    MI_U16 y;
    MI_U16 width;
    MI_U16 height;
}Rect_t;

typedef struct _YUVColor_t
{
    MI_U8  y;
    MI_U8  u;
    MI_U8  v;
    MI_U8  transparent;
} YUVColor_t;

typedef enum _Rotate_e
{
    ROTATE_0    = 0,
    ROTATE_90   = 1,
    ROTATE_180  = 2,
    ROTATE_270  = 3,
} Rotate_e;

typedef enum _RateCtlType_e
{
    RATECTLTYPE_CBR   = 0,
    RATECTLTYPE_VBR   = 1,
    RATECTLTYPE_FIXQP = 2,
    RATECTLTYPE_ABR   = 3,
    RATECTLTYPE_MAX   = 4,
} RateCtlType_e;

typedef struct _Color_t
{
    MI_U8  a;
    MI_U8  r;
    MI_U8  g;
    MI_U8  b;
} Color_t;

typedef struct OsdInvertColor_s
{
    MI_U32  nLumThresh;                 //The threshold to decide whether invert the OSD's color or not.
    BOOL    bInvColEn;                  //The switch of inverting color.
} OsdInvertColor_t;


#ifndef MALLOC
#define MALLOC(s) malloc(s)
#endif

#ifndef FREEIF
#define FREEIF(m) if(m!=0) {free(m);m=NULL;}
#endif

#ifndef MI_SYS_Malloc
#define MI_SYS_Malloc(size) malloc(size)
#endif

#ifndef MI_SYS_Realloc
#define MI_SYS_Realloc(ptr, size) realloc(ptr, size)
#endif

#ifndef MI_SYS_Free
#define MI_SYS_Free(pData) { if(pData != NULL) free(pData); pData = NULL; }
#endif

#ifndef MIXER_ALIGN_UP
#define MIXER_ALIGN_UP(value, align)    ((value+align-1) / align * align)
#endif

#ifndef MIXER_ALIGN_DOWN
#define MIXER_ALIGN_DOWN(value, align)  (value / align * align)
#endif


static inline void* MI_SYS_Memset(void* pDest, MI_U32 cChar, MI_U32 nCount)
{
    return memset(pDest, cChar, nCount);
}

static inline void* MI_SYS_Memcpy(void* pDest, void* pSrc, MI_U32 nCount)
{
    return memcpy(pDest, pSrc, nCount);
}


#ifndef ExecFunc
#define ExecFunc(_func_, _ret_) \
    do{ \
        MI_S32 s32Ret = MI_SUCCESS; \
        s32Ret = _func_; \
        if (s32Ret != _ret_) \
        { \
            printf("[%s %d] exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
            return s32Ret; \
        } \
        else \
        { \
            printf("[%s %d] exec function pass\n", __func__, __LINE__); \
        } \
    } while(0)
#endif

#ifndef ExecFuncWARNING
#define ExecFuncWARNING(_func_, _ret_) \
    do{ \
        MI_S32 s32Ret = MI_SUCCESS; \
        s32Ret = _func_; \
        if (s32Ret != _ret_) \
        { \
            printf("[%s %d] exec function failed, warning:%x\n", __func__, __LINE__, s32Ret); \
        } \
        else \
        { \
            printf("[%s %d] exec function pass\n", __func__, __LINE__); \
        } \
    } while(0)
#endif

#ifndef ExecFuncNoReturn
#define ExecFuncReturnNothing(_func_, _ret_) \
    do{ \
        MI_S32 s32Ret = MI_SUCCESS; \
        s32Ret = _func_; \
        if (s32Ret != _ret_) \
        { \
            printf("[%s %d] exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
            return;\
        } \
        else \
        { \
            printf("[%s %d] exec function pass\n", __func__, __LINE__); \
        } \
    } while(0)
#endif

#ifndef MIXERCHECKRESULT
#define MIXERCHECKRESULT(_func_)\
    do{ \
        MI_S32 s32Ret = MI_SUCCESS; \
        s32Ret = _func_; \
        if (s32Ret != MI_SUCCESS)\
        { \
            printf("[%s %d] exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
            return s32Ret; \
        } \
        else \
        { \
            printf("(%s %d) exec function pass\n", __FUNCTION__,__LINE__); \
        } \
    } while(0)
#endif

#ifndef Mixer_API_ISVALID_POINT
#define Mixer_API_ISVALID_POINT(X)  \
    {   \
        if( X == NULL)  \
        {   \
            printf("mixer input point param is null!\n");  \
            return MI_SUCCESS;   \
        }   \
    }
#endif

#define MIXERDBG_ENTER() \
    printf("\n"); \
    printf("[IN] [%s:%s:%d] \n", __FILE__, __FUNCTION__, __LINE__); \
    printf("\n"); \

#define MIXERDBG_LEAVE() \
    printf("\n"); \
    printf("[OUT] [%s:%s:%d] \n", __FILE__, __FUNCTION__, __LINE__); \
    printf("\n"); \

#define MIXER_RUN() \
    printf("\n"); \
    printf("[RUN] ok [%s:%s:%d] \n", __FILE__, __FUNCTION__, __LINE__); \
    printf("\n"); \

#define MIXER_DBGLV_NONE           0   //disable all the debug message
#define MIXER_DBGLV_INFO           1   //information
#define MIXER_DBGLV_NOTICE         2   //normal but significant condition
#define MIXER_DBGLV_DEBUG          3   //debug-level messages
#define MIXER_DBGLV_WARNING        4   //warning conditions
#define MIXER_DBGLV_ERR            5   //error conditions
#define MIXER_DBGLV_CRIT           6   //critical conditions
#define MIXER_DBGLV_ALERT          7   //action must be taken immediately
#define MIXER_DBGLV_EMERG          8   //system is unusable

#define COLOR_NONE                 "\033[0m"
#define COLOR_BLACK                "\033[0;30m"
#define COLOR_BLUE                 "\033[0;34m"
#define COLOR_GREEN                "\033[0;32m"
#define COLOR_CYAN                 "\033[0;36m"
#define COLOR_RED                  "\033[0;31m"
#define COLOR_YELLOW               "\033[1;33m"
#define COLOR_WHITE                "\033[1;37m"

#define MIXER_NOP(fmt, args...)
#define MIXER_DBG(fmt, args...) \
    if(g_dbglevel <= MIXER_DBGLV_DEBUG)  \
        do { \
            printf(COLOR_GREEN "[DBG]:%s[%d]:  " COLOR_NONE, __FUNCTION__,__LINE__); \
            printf(fmt, ##args); \
        }while(0)

#define MIXER_WARN(fmt, args...) \
    if(g_dbglevel <= MIXER_DBGLV_WARNING)  \
        do { \
            printf(COLOR_YELLOW "[WARN]:%s[%d]: " COLOR_NONE, __FUNCTION__,__LINE__); \
            printf(fmt, ##args); \
        }while(0)

#define MIXER_INFO(fmt, args...) \
    if(g_dbglevel <= MIXER_DBGLV_INFO)  \
        do { \
            printf("[INFO]:%s[%d]: \n", __FUNCTION__,__LINE__); \
            printf(fmt, ##args); \
        }while(0)

#define MIXER_ERR(fmt, args...) \
    if(g_dbglevel <= MIXER_DBGLV_ERR)  \
        do { \
            printf(COLOR_RED "[ERR]:%s[%d]: " COLOR_NONE, __FUNCTION__,__LINE__); \
            printf(fmt, ##args); \
        }while(0)


#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

#define fifo_name       "/tmp/my_fifo"
const int open_mode_r = O_RDONLY;
const MI_S32 open_mode_w = O_WRONLY | O_NONBLOCK;

typedef enum _ViChnStatus
{
    VI_ILLEGAL = -1,
    VI_DISABLE,
    VI_DISABLE_DEPTHLY,
    VI_ENABLE,
} ViChnStatus;

typedef enum _EncoderType_e
{
    VE_AVC,
    VE_H265,
    VE_MJPEG,
    VE_YUV420,
    VE_JPG,
    VE_JPG_YUV422,
    VE_TYPE_MAX,
} Mixer_EncoderType_e;

typedef enum _Mixer_Sys_Input_e
{
    MIXER_SYS_INPUT_VPE = 0,
#if TARGET_CHIP_I5
    MIXER_SYS_INPUT_MAX,
#elif TARGET_CHIP_I6
    MIXER_SYS_INPUT_BUTT,
#elif TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    MIXER_SYS_INPUT_BUTT,
#endif
} Mixer_Sys_Input_E;

typedef struct Mixer_Sys_BindInfo_s
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
#if (TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0)
    MI_SYS_BindType_e eBindType;
    MI_U32 u32BindParam;
#endif
} Mixer_Sys_BindInfo_T;

struct Mixer_Stream_Attr_T
{
    Mixer_Sys_Input_E enInput;
    MI_U32     u32InputChn;
    MI_U32     u32InputPort;
    MI_VENC_CHN vencChn;
    MI_VENC_ModType_e eType;
    MI_U32    u32Width;
    MI_U32     u32Height;
    const char*    pszStreamName;
};

typedef struct
{
    MI_VENC_CHN vencChn;
    MI_VENC_ModType_e enType;
    char szStreamName[64];

    MI_BOOL bWriteFile;
    MI_S32 fd;
    char szDebugFile[128];
} Mixer_StreamInfo_T;

typedef struct _IeParamStruct
{
    MI_S32 box_id;
    MI_S8 NewAddName[128];
    MI_S8 NewDelName[128];
}IeParamInfo;

typedef enum
{
    Model_Type_None = -1,
    Model_Type_Classify,
    Model_Type_Detect,
    Model_Type_FaceReg,
    Model_Type_Hc,
    Model_Type_Debug,
    Model_Type_Butt,
} IPU_Model_Type_E;

typedef struct
{
    IPU_Model_Type_E enModelType;
    char szIPUfirmware[128];
    char szModelFile[128];
    union _u
    {
        struct _ExtendInfo1
        {
            char szLabelFile[128];
        } ExtendInfo1;
        struct _ExtendInfo2
        {
            char szModelFile1[128];
            char szFaceDBFile[128];
            char szNameListFile[128];
        } ExtendInfo2;
    }u;
} IPU_InitInfo_S;

#define SUPPORT_RGN_OSD_TIME    0
#define RGN_OSD_MAX_NUM         4
#define RGN_OSD_TIME_WIDTH        200
#define RGN_OSD_TIME_HEIGHT        200

typedef struct
{
    MI_RGN_HANDLE hHandle[RGN_OSD_MAX_NUM];
    MI_RGN_CanvasInfo_t stCanvasInfo[RGN_OSD_MAX_NUM];

    pthread_t pt;
    MI_BOOL bRun;
} Mixer_RGN_Osd_T;

typedef enum _PlatformType_e
{
    PLATFORM_TYPE_313E,
    PLATFORM_TYPE_318,
    PLATFORM_TYPE_MAX,
} PlatformType_e;

#if TARGET_CHIP_I6 || TARGET_CHIP_I5 || TARGET_CHIP_I6E
typedef enum Mixer_HDR_Mode_e
{
    Mixer_HDR_TYPE_OFF=0,
    Mixer_HDR_TYPE_VC,                 //virtual channel mode HDR,vc0->long, vc1->short
    Mixer_HDR_TYPE_DOL,
    Mixer_HDR_TYPE_EMBEDDED,              //compressed HDR mode
    Mixer_HDR_TYPE_LI,                //Line interlace HDR
    Mixer_HDR_Mode_NUM,
} Mixer_HDR_Mode_E;

typedef enum Mixer_Venc_Bind_Mode_e
{
    Mixer_Venc_Bind_Mode_FRAME = 0,
    Mixer_Venc_Bind_Mode_REALTIME,
    Mixer_Venc_Bind_Mode_HW_RING,
    Mixer_Venc_Bind_Mode_NUM,
} Mixer_Venc_Bind_Mode_E;
#elif TARGET_CHIP_I6B0
typedef enum Mixer_HDR_Mode_e
{
    Mixer_HDR_TYPE_OFF=0,
    Mixer_HDR_TYPE_VC,                 //virtual channel mode HDR,vc0->long, vc1->short
    Mixer_HDR_TYPE_DOL,
    Mixer_HDR_TYPE_EMBEDDED,              //compressed HDR mode
    Mixer_HDR_TYPE_LI,                //Line interlace HDR
    Mixer_HDR_Mode_NUM,
} Mixer_HDR_Mode_E;

typedef enum Mixer_Venc_Bind_Mode_e
{
    Mixer_Venc_Bind_Mode_FRAME = 0,
    Mixer_Venc_Bind_Mode_REALTIME,
    Mixer_Venc_Bind_Mode_HW_RING,
    Mixer_Venc_Bind_Mode_HW_HALF_RING,
    Mixer_Venc_Bind_Mode_NUM,
} Mixer_Venc_Bind_Mode_E;
#endif

/// 系统时间结构
typedef struct SYSTEM_TIME{
    MI_S32  year;///< 年。
    MI_S32  month;///< 月，January = 1, February = 2, and so on.
    MI_S32  day;///< 日。
    MI_S32  wday;///< 星期，Sunday = 0, Monday = 1, and so on
    MI_S32  hour;///< 时。
    MI_S32  minute;///< 分。
    MI_S32  second;///< 秒。
    MI_S32  isdst;///< 夏令时标识。
}SYSTEM_TIME;

PlatformType_e mixerGetPlatformType();
int mixerStr2Int(char *strNum);


#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])

#define MAKE_YUYV_VALUE(y,u,v)  ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK              MAKE_YUYV_VALUE(0,128,128)
#define YUYV_WHITE              MAKE_YUYV_VALUE(255,128,128)
#define YUYV_RED                MAKE_YUYV_VALUE(76,84,255)
#define YUYV_GREEN              MAKE_YUYV_VALUE(149,43,21)
#define YUYV_BLUE               MAKE_YUYV_VALUE(29,225,107)

#define SENSOR_FPS_UP2_INT(fps)   (((fps)*100+50)/100)

#define ALIGN_UP(x, align)      (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_BACK(x, a)        (((x) / (a)) * (a))

#define MAX_VDF_NUM_PER_CHN     16

void watchDogInit();
void watchDogUninit();
void segfault_sigaction(int signo, siginfo_t *si, void *arg);


MI_S32 Mixer_Sys_Init(void);
MI_S32 Mixer_Sys_Exit(void);

MI_S32 Mixer_Sys_Bind(Mixer_Sys_BindInfo_T *pstBindInfo);
MI_S32 Mixer_Sys_UnBind(Mixer_Sys_BindInfo_T *pstBindInfo);

MI_U64 Mixer_Sys_GetPts(MI_U32 u32FrameRate);

int mixer_property_set(const char *key, const char *value);
int mixer_property_get(const char *key, char *value, const char *default_value);
int mixer_property_init(void);
int mixer_property_uninit(void);

 void mixer_SetRotaValue(MI_U32 value);
 MI_U32 mixer_GetRotaValue(void);

 void mixer_setHdrValue(BOOL value);
 BOOL mixer_GetHdrValue(void);

 void Mixer_SetShowFrameIntervalState(BOOL state);
 BOOL Mixer_GetShowFrameIntervalState(void);

 void Mixer_SetFdState(BOOL state);
 BOOL Mixer_GetFdState(void);

void Mixer_SetHdState(BOOL state);
BOOL Mixer_GetHdState(void);

 void Mixer_SetHcState(BOOL state);
 BOOL Mixer_GetHcState(void);

 void Mixer_SetMdState(BOOL state);
 BOOL Mixer_GetOdState(void);

 void Mixer_SetIeLogState(BOOL state);
 BOOL Mixer_GetIeLogState(void);

 void Mixer_SetVgState(BOOL state);
 BOOL Mixer_GetVgState(void);


 void Mixer_SetDlaState(BOOL state);
 BOOL Mixer_GetDlaState(void);

 void Mixer_SetOsdState(BOOL state);
 BOOL Mixer_GetOsdState(void);

 void Mixer_SetVideoNum(MI_U16 num);
 MI_U16 Mixer_GetVideoNum(void);

 MI_U16 CheckAlign(MI_U16 mb_size, MI_U32 stride);

 MI_S32 Mixer_get_thread_policy(pthread_attr_t *attr);
 void Mixer_show_thread_priority(pthread_attr_t *attr,int policy);
 MI_S32 Mixer_get_thread_priority(pthread_attr_t *attr);
 void Mixer_set_thread_policy(pthread_attr_t *attr,int policy);

#if TARGET_CHIP_I6

int Mixer_coverVi2Vpe(int viChn);
MI_U32 Mixer_getIntFramerate(int u32FrameRate);
MI_SYS_BindType_e coverMixerBindMode(Mixer_Venc_Bind_Mode_E eMixerBindMode);

#elif TARGET_CHIP_I6E || TARGET_CHIP_I6B0

int Mixer_coverVi2Vpe(int viChn);
MI_U32 Mixer_getIntFramerate(int u32FrameRate);
MI_SYS_BindType_e coverMixerBindMode(Mixer_Venc_Bind_Mode_E eMixerBindMode);
#endif

#ifdef __cplusplus
}
#endif //__cplusplus


#endif //#define _MID_COMMON_H_
