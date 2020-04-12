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
#ifndef __MI_VI_TYPE_H__
#define __MI_VI_TYPE_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "mi_video_type.h"

typedef enum _ScaSrcType_e
{
    SCA_SRC_VGA,
    SCA_SRC_YPBPR,
    SCA_SRC_CVBS,
    SCA_SRC_CVBS2,
    SCA_SRC_CVBS3,
    SCA_SRC_SVIDEO,
    SCA_SRC_DTV,
    SCA_SRC_SC0_VOP,
    SCA_SRC_SC1_VOP,
    SCA_SRC_SC2_VOP,
    SCA_SRC_BT656,
    SCA_SRC_BT656_1,
    SCA_SRC_CAMERA,       // support in I1
    SCA_SRC_HVSP,         // support in I1
    SCA_SRC_NUM,
} ScaSrcType_e;

typedef struct _ViCrop_t
{
    BOOL enable;
    U32  x;
    U32  y;
    U32  w;
    U32  h;
} ViCrop_t;


/* the attributes of dev */
typedef struct _ViDevAtrr_t
{
    ScaSrcType_e scaSrcType;
    F32          sensorFrameRate;
    ViCrop_t     crop;
    S32          reserve[1];
} ViDevAtrr_t;

/* the attributes of a VI channel */
typedef struct _ViChnAttr_t
{
    Size_t             destSize;

    PixelFormat_e      pixFormat;

    ChannelMode_e      mode;
    VOID               *privateData;
    S32                reserve[1];
} ViChnAttr_t;




typedef S32(*bufferAvailableCallBack)(PixelFormat_e format, U8* yBuffer, U8* uvBuffer, U32 width, U32 height, void *userData);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__MI_VI_TYPE_H__*/
