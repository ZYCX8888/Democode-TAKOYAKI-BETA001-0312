#ifndef _ST_VENC_H
#define _ST_VENC_H
#include "mi_sys.h"

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

#define VENC_H264_ALIGN_W   32
#define VENC_H264_ALIGN_H   8
#define VENC_H265_ALIGN_W   16
#define VENC_H265_ALIGN_H   8
#define VENC_JPEG_ALIGN_W   32
#define VENC_JPEG_ALIGN_H   8

#define OUT_DEPTH_VENC 3

#define MAIN_STREAM_VENC 0
#define SNAPFACE_CHN 4

typedef enum
{
    E_ST_JPEG = 1,
    E_ST_H264,
    E_ST_H265,
    E_ST_MAX
} ST_VENC_Type_e;

typedef struct st_VencRunParam_s
{
    MI_S32 s32VencChn;
    MI_S32 s32Socket;
    MI_S32 s32SaveFileFlag;
    unsigned long IPaddr;
} VencRunParam_T;

MI_S32 ST_VencCreateChannel(MI_S32 s32VencChn, MI_S32 s32VencType, MI_U16 u16Width, MI_U16 u16Height, MI_S32 s32FrameRate);
MI_S32 ST_VencDestroyChannel(MI_S32 s32VencChn);
MI_S32 ST_VencStartGetStream(VencRunParam_T *pstVencRunParam);
MI_S32 ST_VencStopGetStream();

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif
