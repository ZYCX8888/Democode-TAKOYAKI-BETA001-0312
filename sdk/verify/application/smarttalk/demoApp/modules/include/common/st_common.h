#ifndef _ST_COMMON_H_
#define _ST_COMMON_H_

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

#include <stdio.h>
#include <unistd.h>
#include <string.h>


#include "mi_sys.h"
#include "mi_hdmi_datatype.h"
#include "mi_disp_datatype.h"

#define TALK_VDEC_CHN 0
#define UVC_VDEC_CHN 3

#define UVC_DIVP_CHN 0
#define MAINVENC_DIVP_CHN 1
#define LOCAL_DISP_DIVP_CHN 2
#define FD_DIVP_CHN 3
#define TALK_DIVP_CHN 4
#define JPEG_DIVP_CHN 5


#define VIDEO_DISP_X 64
#define VIDEO_DISP_Y 64
#define VIDEO_DISP_W 640
#define VIDEO_DISP_H 360

#define VIDEO_DISP_1024_X 100
#define VIDEO_DISP_1024_Y 60
#define VIDEO_DISP_1024_W 704
#define VIDEO_DISP_1024_H 480

#define VIDEO_STREAM_W 704
#define VIDEO_STREAM_H 480

#ifndef ExecFunc
#define ExecFunc(_func_, _ret_) \
    do{ \
        MI_S32 s32Ret = MI_SUCCESS; \
        s32Ret = _func_; \
        if (s32Ret != _ret_) \
        { \
            printf("[%s %d]exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
            return s32Ret; \
        } \
        else \
        { \
            printf("[%s %d]exec function pass\n", __func__, __LINE__); \
        } \
    } while(0)
#endif

#ifndef STCHECKRESULT
#define STCHECKRESULT(_func_)\
    do{ \
        MI_S32 s32Ret = MI_SUCCESS; \
        s32Ret = (MI_S32)_func_; \
        if (s32Ret != MI_SUCCESS)\
        { \
            printf("[%s %d]exec function failed, error:%x\n", __func__, __LINE__, s32Ret); \
            return s32Ret; \
        } \
        else \
        { \
            printf("(%s %d)exec function pass\n", __FUNCTION__,__LINE__); \
        } \
    } while(0)
#endif

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
#define ST_DBG(fmt, args...) \
    do { \
        printf(COLOR_GREEN "[DBG]:%s[%d]: " COLOR_NONE, __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define ST_WARN(fmt, args...) \
    do { \
        printf(COLOR_YELLOW "[WARN]:%s[%d]: " COLOR_NONE, __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define ST_INFO(fmt, args...) \
    do { \
        printf("[INFO]:%s[%d]: \n", __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define ST_ERR(fmt, args...) \
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

#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])

#define MAKE_YUYV_VALUE(y,u,v)  ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK              MAKE_YUYV_VALUE(0,128,128)
#define YUYV_WHITE              MAKE_YUYV_VALUE(255,128,128)
#define YUYV_RED                MAKE_YUYV_VALUE(76,84,255)
#define YUYV_GREEN              MAKE_YUYV_VALUE(149,43,21)
#define YUYV_BLUE               MAKE_YUYV_VALUE(29,225,107)

#define ALIGN_UP(x, align)      (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_BACK(x, a)        (((x) / (a)) * (a))

#define MAX_VDF_NUM_PER_CHN     16

#define MAX_CHANNEL_NUM 16

typedef enum
{
    ST_Sys_Input_VPE = 0,
    ST_Sys_Input_DIVP = 1,

    ST_Sys_Input_BUTT,
} ST_Sys_Input_E;

typedef enum
{
    ST_Sys_Func_RTSP = 0x01,
    ST_Sys_Func_CAPTURE = 0x02,
    ST_Sys_Func_DISP = 0x04,

    ST_Sys_Func_BUTT,
} ST_Sys_Func_E;

typedef enum
{
    E_ST_TIMING_720P_50 = 1,
    E_ST_TIMING_720P_60,
    E_ST_TIMING_1080P_50,
    E_ST_TIMING_1080P_60,
    E_ST_TIMING_1600x1200_60,
    E_ST_TIMING_1440x900_60,
    E_ST_TIMING_1280x1024_60,
    E_ST_TIMING_1024x768_60,
    E_ST_TIMING_2560x1440_30,
    E_ST_TIMING_3840x2160_30,
    E_ST_TIMING_3840x2160_60,

    E_ST_TIMING_USER_CUSTOM,

    E_ST_TIMING_MAX,
} ST_DispoutTiming_e;


//sync with app_config.h
#define CALL_ID_LEN 8 //Çø+¶°+Â¥+·¿ 2+2+2+2 "01021102" "Room 02, 11th Floor, Uint 2, Group 1"
#define CALL_ID_MAX_LEN 16
#define TELPHONE_LEN 12
#define DEVICE_ADDR_MAX_LEN 64
#define IPADDR_MAX_LEN 16

typedef struct stMachinePostion_Desc_s
{
    MI_S32 s32MainID;
    MI_S32 s32SubID;
    MI_S32 s32DeviceType;// ST_DeviceType_E
    MI_U8 u8LocalID[CALL_ID_MAX_LEN];
    MI_U8 u8IPaddr[IPADDR_MAX_LEN]; //device ipaddr
    MI_U8 u8Telphone[TELPHONE_LEN];
} SMART_BD_Machine_CFG_T; //build device 

typedef struct st_CmdPack_s
{
    MI_U32 u32HeaderMagicNum; //default 0x66668888
    MI_S32 s32PackLen;
    MI_U16 u16Cmd;
    MI_S16 s16DeviceType;
    MI_U8 u8SrcID[CALL_ID_LEN];
    MI_U8 u8DstID[CALL_ID_LEN];
} st_CmdPack_T;

typedef struct ST_Sys_BindInfo_s
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
} ST_Sys_BindInfo_T;

typedef struct ST_Sys_Rect_s
{
    MI_S32 s32X;
    MI_S32 s32Y;
    MI_S16 s16PicW;
    MI_S16 s16PicH;
} ST_Rect_T;

typedef struct
{
    MI_U32 u32X;
    MI_U32 u32Y;
} ST_Point_T;

typedef enum
{
    ST_Sensor_Type_IMX291 = 0,
    ST_Sensor_Type_IMX307,
    ST_Sensor_Type_AR0237,
    ST_Sensor_Type_os08a10,
    ST_Sensor_Type_IMX274,
    ST_Sensor_Type_BUTT,
} ST_Sensor_Type_T;

MI_S32 ST_Sys_Init(void);
MI_S32 ST_Sys_Exit(void);
MI_U64 ST_Sys_GetCurTimeU64();

MI_S32 ST_ModuleBind(MI_S32 s32SrcMod, MI_S32 s32SrcDev, MI_S32 s32SrcChn,MI_S32 s32SrcPort,
    MI_S32 s32DstMod,MI_S32 s32DstDev,MI_S32 s32DstChn,MI_S32 s32DstPort);

MI_S32 ST_ModuleUnBind(MI_S32 s32SrcMod, MI_S32 s32SrcDev, MI_S32 s32SrcChn,MI_S32 s32SrcPort,
    MI_S32 s32DstMod,MI_S32 s32DstDev,MI_S32 s32DstChn,MI_S32 s32DstPort);

MI_U64 ST_Sys_GetPts(MI_U32 u32FrameRate);
MI_S32 ST_GetTimingInfo(MI_S32 s32ApTiming, MI_S32 *ps32HdmiTiming, MI_S32 *ps32DispTiming, MI_U16 *pu16DispW, MI_U16 *pu16DispH);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif //_ST_COMMON_H_

