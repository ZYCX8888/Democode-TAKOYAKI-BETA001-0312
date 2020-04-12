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
#ifndef __MI_VENC_TYPE_H__
#define __MI_VENC_TYPE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "mi_video_type.h"

//Rate control algorithms. It supports constant-quality, constant-bitrate and variable-bitrate, currently.
typedef enum _VideoControlRateType
{
    Video_ControlRateDisable,
    Video_ControlRateVariable,
    Video_ControlRateConstant,
    Video_ControlRateConstantQP,
} VideoControlRateType;

typedef struct _VencH264BitrateControl
{
    VideoControlRateType controlRate;  /* only support Video_ControlRateVariable,
                                                       Video_ControlRateConstant,
                                                       Video_ControlRateConstantQP */
    U32 targetBitrate;  //Depends on image resolution
    U32 refQp;          //Default: 26.  For constant-quality, it is the fixed qp value. For other algorithms, it is leading qp.
    S32 reserve[1];
} VencH264BitrateControl_t;

typedef struct _VencH265BitrateControl
{
    VideoControlRateType controlRate;
    U32 targetBitrate;  //Depends on image resolution
    U32 refQp;          //Default: 26.  For constant-quality, it is the fixed qp value. For other algorithms, it is leading qp.
    S32 reserve[1];
} VencH265BitrateControl_t;

typedef struct _VencRcParam_t
{
    union
    {
        VencH264BitrateControl_t    attrH264RC;   // h264 bitrate control
        VencH265BitrateControl_t    attrH265RC;   // h265 bitrate control
    };

    S32 reserve[1];
} VencRcParam_t;

typedef struct _VencParamCbr_t
{
    U32 gop;
    U32 statTime;
    U32 bitrate;
    U32 fluctuateLevel;
    U32 maxIQp;
    U32 minIQp; // 12 <= minIQp <= maxPQp <= 48
    S32 IPQPDelta; // -12 <= IPQPDelta <= 12
    U32 reserve[4];
} VencParamCbr_t;

typedef struct _VencParamVbr_t
{
    U32 gop;
    U32 statTime;
    U32 maxBitrate;
    U32 maxIQp;
    U32 minIQp; // 12 <= minIQp <= maxIQp <= 48
    U32 maxPQp;
    U32 minPQp; // 12 <= minPQp <= maxPQp <= 48
    S32 IPQPDelta; // -12 <= IPQPDelta <= 12
    S32 changePos;  // [50, 100]
} VencParamVbr_t;

typedef struct _VencParamFixQp
{
    U32 gop;
    U32 IQp;
    U32 PQp;
} VencParamFixQp;

typedef struct _VencRcParamEx_t
{
    VideoControlRateType controlRate;
    union
    {
        VencParamCbr_t h264Cbr;
        VencParamVbr_t h264Vbr;
        VencParamFixQp h264FixQp;
        VencParamCbr_t h265Cbr;
        VencParamVbr_t h265Vbr;
        VencParamFixQp h265FixQp;
    } type;
    U32 rowQpDelta;
    VOID* rcParam;
    U32 rcVersion;
} VencRcParamEx_t;

typedef enum _VideoPlayLoadType_e
{
    VIDEO_PLAYLOAD_H264,        // H.264/AVC
    VIDEO_PLAYLOAD_H265,
    VIDEO_PLAYLOAD_MJPEG,      // Motion JPEG
    VIDEO_PLAYLOAD_JPEG,
    VIDEO_PLAYLOAD_MAX = 0x7FFFFFFF,
} VideoPlayLoadType_e;


typedef enum
{
    PROFILE_BASELINE = 0,
    PROFILE_MAIN     = 1,
    PROFILE_HIGH     = 2,
} VencProfile_e;

typedef struct _VencAttrH264_t
{
    VencProfile_e  profile;          /*0:baseline, 1:MP, 2:HP */
    S32 reserve[1];
} VencAttrH264_t;

typedef struct _VencAttrH265_t
{
    VencProfile_e  profile;          /*0:baseline */
    S32 reserve[1];
} VencAttrH265_t;


typedef struct _VencAttrMjpeg_t
{
    U32  qfactor;
    BOOL bSnap;
} VencAttrMjpeg_t;

typedef struct _VencAttrJpeg_t
{
    U32  qfactor;
    S32 reserve[1];
} VencAttrJpeg_t;


typedef struct _VencAttr_t
{
    VideoPlayLoadType_e  enType;      /*the type of payload*/
    union
    {
        VencAttrH264_t  attrH264;     /*h264*/
        VencAttrH265_t  attrH265;     /*h265*/
        VencAttrMjpeg_t attrMjpeg;     /*mjpeg*/
        VencAttrJpeg_t  attrJpeg;      /*jpeg*/
    };

    U32                 sourceWidth;
    U32                 sourceHeight;
    PixelFormat_e       srcPixFormat;
    U32                 dstFrmRate;           // source frame rate
    ChannelMode_e       mode;
    S32 reserve[1];
} VencAttr_t;


typedef struct _VencChnAttr_t
{
    VencAttr_t      veAttr;        /*the attribute of video encoder*/
    VencRcParamEx_t rcAttr;        /*the attribute of rate  ctrl*/
} VencChnAttr_t;

typedef enum _H264FrameType_e
{
    H264_FRAME_TYPE_P,
    H264_FRAME_TYPE_B,
    H264_FRAME_TYPE_I,
    H264_FRAME_TYPE_LTR_P,
} H264FrameType_e;

typedef enum _H265FrameType_e
{
    H265_FRAME_TYPE_P,
    H265_FRAME_TYPE_B,
    H265_FRAME_TYPE_I,
    H265_FRAME_TYPE_LTR_P,
} H265FrameType_e;


typedef struct _VencPack_t
{
    U8   *addr;            /*the virtual address of stream*/
    U32   len;             /*the length of stream*/
    U64   pts;             /*PTS*/
    union
    {
        H264FrameType_e h264FrameType;
        H265FrameType_e h265FrameType;
    };
    U32 offset;
} VencPack_t;

typedef struct _VencStream_t
{
    VencPack_t          pack;       /*stream pack attribute*/
    U32                 data1;
    U32                 data2;
    U32                 disposeFlag;     /* 1: can  drop,  else    */

    VideoPlayLoadType_e enType;
} VencStream_t;

typedef struct _VencRoi_t
{
    U32  idx;       //h265: 0 or 1, Default:0
    //h264: 0~7, Default:0
    BOOL enable;    //Default: false
    S32  dqp;       //h265: -15~0 (0 means ROI is disabled); Default:4
    //h264: -15~0 (0 means ROI is disabled); Default:3
    U32  mbX;       //nCbX, nCbY是ROI的left-top-position in CB unit. Default: nCbX=0, nCbY=0
    U32  mbY;
    U32  mbW;       //nCbW, nCbH是ROI的width,height in CB unit. Default: nCbW =0, nCbH =0
    U32  mbH;
} VencRoi_t;

typedef struct _VencFramerate_t
{
    U32  dstFrmRate;
    U32  srcFrmRate;
    S32 reserve[1];
} VencFramerate_t;


typedef struct _VencH264Dblk_t
{
    BOOL deblockFiterControl;       //Default: false
    U32  disableDeblockingFilterIdc;//0 or 1 or 2 .Default: 0
    //  0 : enable deblocking.
    //  1 : disable deblocking.
    //  2 : disable deblocking on slice edge.
    S32  sliceAlphaC0Offset;
    S32  sliceBetaOffset;           //-6~6 Default: 0
} VencH264Dblk_t;

typedef struct _VencH264Entropy_t
{
    BOOL entropyCodingCabac;    //Default: false. True: use Cabac; False: use cavlc
} VencH264Entropy_t;

// Enumeration of possible motion vector (MV) types
typedef enum _VideoMotionVectorType
{
    Video_MotionVectorPixel,
    Video_MotionVectorHalfPel,
    Video_MotionVectorQuarterPel,
    Video_MotionVectorEighthPel,
} VideoMotionVectorType;


typedef struct _VencH264InterPred_t
{
    U32  dmvX;                     //(8~16), Default: 16, The motion search range x-direction
    U32  dmvY;                     //(8 or 16), Default: 16, The motion search range y-direction
    VideoMotionVectorType  subpel; //Default: OMX_Video_MotionVectorQuarterPel
    BOOL inter4x4PredEn;
    BOOL inter8x4PredEn;
    BOOL inter4x8PredEn;
    BOOL inter8x8PredEn;
    BOOL inter16x8PredEn;
    BOOL inter8x16PredEn;
    BOOL inter16x16PredEn;
    BOOL interSKIPPredEn;       //Default: True (all enabled)
} VencH264InterPred_t;

typedef struct _VencH264IntraPred_t
{
    BOOL constIpred;    //Default: 0
} VencH264IntraPred_t;

typedef struct _VencH264SliceSplit_t
{
    U32  rows;  //1,2,4 or 8, Default: 0; Row-mode supports 1/2/4/8 Mb-rows per slice , nRows = 0 indicates row-mode is disabled.
    U32  bits;  //0 ~ (2^16 – 1), Default: 0; Bit-mode indicates bits-size of a coded-slice, nBits = 0 indicates bit-mode is disabled.
} VencH264SliceSplit_t;

typedef struct _VencH264Poc_t
{
    U32 pocType;   // 0 or 2    Default: 2
} VencH264Poc_t;

/*
 *    to mark which frame can be disposed
 */
typedef struct _VencDisposable_t
{
    BOOL disposable;        //Default: false
} VencDisposable_t;

typedef struct _VencH265Dblk_t
{
    BOOL deblockingOverrideEnable;              //Default: false
    BOOL sliceDeblockingFilterDisabledFlag;     //Default: false
    S32  sliceTcOffsetDiv2;                     //-6~6  Default: 0
    S32  sliceBetaOffsetDiv2;                   //-6~6  Default: 0
} VencH265Dblk_t;

typedef struct _VencH265Sao_t
{
    BOOL saoEnable; //Default: false
} VencH265Sao_t;

typedef struct _VencH265SliceSplit_t
{
    U32  rows;  // 1 ~ (Image_ height/64)-1  Default: 0
    U32  bits;  //  Default: 0
} VencH265SliceSplit_t;

typedef struct _VencH265PredUnit_t
{
    U32  constrainedIntraPredFlag; //0 or 1 Default: 0
    U32  strongIntraSmoothingEnabledFlag; //0 or 1 Default: 0
} VencH265PredUnit_t;

typedef struct _VencH265Trans_t
{
    S32 cQpOffset;  //-12~12  Default: 0
} VencH265Trans_t;

typedef struct _VencH265Entropy_t
{
    U32 cabacInitFlag; //0 or 1  Default: 0
} VencH265Entropy_t;


typedef struct _VencGop_t
{
    /*
    Used to setup GOP (Group Of Picture)
    U32 nPFrames
    Default value: N =15
    Value range = 0, -1, N
    ‘0’ indicates all I-frames.
    ‘-1’ indicates only 1-st frame is I-frame and others are all P-frames.
    ‘N’ indicates IP-period has length of N+1.
    */
    U32 nPFrames;
} VencGop_t;

typedef struct _VencParamJpeg_t
{
    U32 qfactor;
    U8  YQt[64];
    U8  CQt[64];
    S32 reserve[1];
} VencParamJpeg_t;

typedef struct _VencColor2Grey_t
{
    BOOL color2Grey;
} VencColor2Grey_t;

typedef struct _VencVui_t
{
    BOOL vuiPresentFlag;         // Enable/disable add H264/H265 vui info
    U8   videoFormat;            // Please refer to h.264 / h.265 spec (default:5)
    BOOL videoFullRangeFlag;     // Enable/disable flag
    BOOL timingInfoPresFlag;     // Enable/disable flag
} VencVui_t;

typedef struct _VencParamRefEx_t
{
    U32   base;
    U32   enhance;
    BOOL  enablePred;
    BOOL  virtualIEnable;
    U32   virtualIInterval;
    S32   virtualIQpDelta;     //not support
} VencParamRefEx_t;

typedef struct _VencParamRef_t
{
    U32   base;               //not support
    U32   enhance;            //not support
    U32   virtualIInterval;
    S32   virtualIQpDelta;    //not support
} VencParamRef_t;

typedef struct _VencParamLtr_t
{
    U32   base;
    U32   enhance;
    BOOL  enablePred;
    BOOL  virtualIEnable;
    U32   virtualIInterval;
    S32   virtualIQpDelta;     //not support
    S32   reserve[3];
} VencParamLtr_t;


typedef enum
{
    MI_JPE_BYPASS_MODE_DISABLE = 0,
    MI_JPE_BYPASS_MODE_ENABLE  = 1,
    MI_JPE_BYPASS_MODE_SNAPSHOT = 2
} MI_JPE_BYPASS_MODE;

#ifdef __cplusplus
}
#endif

#endif //__MI_VENC_TYPE_H__
