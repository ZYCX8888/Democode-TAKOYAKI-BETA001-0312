#ifndef __TP_VIDEO_API_H__
#define __TP_VIDEO_API_H__

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"
#include "mi_vif.h"
#include "mi_vpe.h"
#include "mi_venc.h"
#include "mi_sensor.h"
#include "mi_divp.h"

/****************************** common ******************************/
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
        s32Ret = _func_; \
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

#define ST_DBG(fmt, args...) \
    do { \
        printf("[DBG]:%s[%d]: " , __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define ST_WARN(fmt, args...) \
    do { \
        printf("[WARN]:%s[%d]: " , __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define ST_INFO(fmt, args...) \
    do { \
        printf("[INFO]:%s[%d]: \n", __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define ST_ERR(fmt, args...) \
    do { \
        printf("[ERR]:%s[%d]: " , __FUNCTION__,__LINE__); \
        printf(fmt, ##args); \
    }while(0)

#define DIVP_CHN_FOR_SCALE 0

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

typedef struct ST_Sys_BindInfo_s
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
#ifdef TARGET_CHIP_I6
    MI_SYS_BindType_e eBindType;
    MI_U32 u32BindParam;
#endif
} ST_Sys_BindInfo_T;

typedef struct ST_Sys_Rect_s
{
    MI_U32 u32X;
    MI_U32 u32Y;
    MI_U16 u16PicW;
    MI_U16 u16PicH;
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

typedef enum
{
    STRM_ID_MAIN = 0,
    STRM_ID_MINOR = 1,
    STRM_ID_THIRD,
    STRM_ID_VIDEO_LAST,
}TP_STRM_ID_E;

typedef enum
{
    TP_ENCODE_TYPE_VIDEO_H264,
    TP_ENCODE_TYPE_VIDEO_H265,
    TP_ENCODE_TYPE_SNAPSHOT_JPEG,
}TP_ENCODE_TYPE_E;

typedef enum
{
    TP_FRAME_TYPE_I,
    TP_FRAME_TYPE_P,
    TP_FRAME_TYPE_VIRTUAL_I,
}TP_VIDEO_FRAME_TYPE_E;

typedef enum
{
    TP_BITRATE_TYPE_CBR,
    TP_BITRATE_TYPE_VBR,
    TP_BITRATE_TYPE_AVBR,
}TP_BITRATE_TYPE;

typedef enum
{
    TP_SMART_CODEC_CBR,
    TP_SMART_CODEC_VBR,
}TP_SMART_CODEC_TYPE;


typedef struct ST_Stream_Attr_T
{
    ST_Sys_Input_E enInput;
    MI_U32     u32InputChn;
    MI_U32     u32InputPort;
    MI_VENC_CHN vencChn;
    MI_VENC_ModType_e eType;
    MI_U32    u32Width;
    MI_U32     u32Height;
    MI_U32 enFunc;
    const char    *pszStreamName;
} ST_Stream_Attr_T;

#define PACK_MAX    2
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;
typedef unsigned char       uint8_t;

typedef struct _tp_venc_strm_pack_s
{
    uint8_t *addr;
    uint32_t len;
}TP_VENC_STRM_PACK_S;

typedef struct _tp_venc_strm_s
{
    TP_VENC_STRM_PACK_S pack[PACK_MAX];
    uint32_t    pack_cnt;
    uint32_t    len;
    uint64_t    pts;
    TP_VIDEO_FRAME_TYPE_E frame_type;
#ifdef VIDEO_AVBR_ENABLE
    uint8_t is_smart_codec;
#endif
}TP_VENC_STRM_S;

typedef struct _strm_res_s
{
    uint32_t    res_w;
    uint32_t    res_h;
}STRM_RES_S;

typedef struct _tp_stream_attr_s
{
    STRM_RES_S  strm_max_res;
    STRM_RES_S  strm_res;
    uint32_t    framerate;
    uint32_t    bitrate;
    uint32_t    gopfactor;
    TP_ENCODE_TYPE_E    encode_type;
    TP_BITRATE_TYPE bitrate_type;
    TP_SMART_CODEC_TYPE smart_codec_type;
}TP_STREAM_ATTR_S;


MI_S32 ST_Sys_Init(void);
MI_S32 ST_Sys_Exit(void);

MI_S32 ST_Sys_Bind(ST_Sys_BindInfo_T *pstBindInfo);
MI_S32 ST_Sys_UnBind(ST_Sys_BindInfo_T *pstBindInfo);

MI_U64 ST_Sys_GetPts(MI_U32 u32FrameRate);

/****************************** VIF ******************************/
typedef struct ST_VIF_PortInfo_s
{
	MI_U32 u32RectX;
	MI_U32 u32RectY;
	MI_U32 u32RectWidth;
	MI_U32 u32RectHeight;
	MI_U32 u32DestWidth;
	MI_U32 u32DestHeight;
    MI_U32 u32IsInterlace;
    MI_VIF_FrameRate_e eFrameRate;
    MI_SYS_PixelFormat_e ePixFormat;
} ST_VIF_PortInfo_T;

MI_S32 ST_Vif_EnableDev(MI_VIF_DEV VifDev,MI_VIF_HDRType_e eHdrType, MI_SNR_PADInfo_t *pstSnrPadInfo);
MI_S32 ST_Vif_DisableDev(MI_VIF_DEV VifDev);

MI_S32 ST_Vif_CreatePort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort, ST_VIF_PortInfo_T *pstPortInfoInfo);

MI_S32 ST_Vif_StartPort(MI_VIF_DEV VifDev, MI_VIF_CHN VifChn, MI_VIF_PORT VifPort);

MI_S32 ST_Vif_StopPort(MI_VIF_CHN VifChn, MI_VIF_PORT VifPort);


/****************************** VPE ******************************/
typedef struct ST_VPE_ChannelInfo_s
{
    MI_U16 u16VpeMaxW;
    MI_U16 u16VpeMaxH;
    MI_U16 u16VpeCropW;
    MI_U16 u16VpeCropH;
    MI_S32 u32X;
    MI_S32 u32Y;
    MI_SYS_PixelFormat_e eFormat;
    MI_VPE_RunningMode_e eRunningMode;
    MI_VPE_HDRType_e eHDRtype;
    MI_BOOL bRotation;
} ST_VPE_ChannelInfo_T;

typedef struct ST_VPE_PortInfo_s
{
    MI_VPE_CHANNEL DepVpeChannel;
    MI_U16 u16OutputWidth;                         // Width of target image
    MI_U16 u16OutputHeight;                        // Height of target image
    MI_SYS_PixelFormat_e  ePixelFormat;      // Pixel format of target image
    MI_SYS_CompressMode_e eCompressMode;     // Compression mode of the output
} ST_VPE_PortInfo_T;

//vpe channel
MI_S32 ST_Vpe_CreateChannel(MI_VPE_CHANNEL VpeChannel, ST_VPE_ChannelInfo_T *pstChannelInfo);
MI_S32 ST_Vpe_DestroyChannel(MI_VPE_CHANNEL VpeChannel);

MI_S32 ST_Vpe_StartChannel(MI_VPE_CHANNEL VpeChannel);
MI_S32 ST_Vpe_StopChannel(MI_VPE_CHANNEL VpeChannel);

//vpe port
MI_S32 ST_Vpe_StartPort(MI_VPE_PORT VpePort, ST_VPE_PortInfo_T *pstPortInfo, MI_U32 u32Depth);
MI_S32 ST_Vpe_StopPort(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort);


/****************************** VENC ******************************/
MI_S32 ST_Venc_CreateChannel(MI_VENC_CHN VencChn, MI_VENC_ChnAttr_t *pstAttr);
MI_S32 ST_Venc_DestoryChannel(MI_VENC_CHN VencChn);

MI_S32 ST_Venc_StartChannel(MI_VENC_CHN VencChn);
MI_S32 ST_Venc_StopChannel(MI_VENC_CHN VencChn);



#ifdef __cplusplus
}
#endif	// __cplusplus
#endif
