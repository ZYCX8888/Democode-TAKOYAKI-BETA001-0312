/*
* mid_VideoEncoder.h- Sigmastar
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
#ifndef _MI_VIDEO_ENCODER_H_
#define _MI_VIDEO_ENCODER_H_
#pragma once

#include <pthread.h>
#include <errno.h>

#include "List.h"
#include "mid_common.h"
#include "mid_utils.h"
#include "mi_venc.h"
#include "mi_sys_datatype.h"
#include "module_common.h"
#include "mid_venc.h"
#include "mid_vpe.h"
#include "mid_divp.h"
#include "mi_venc_datatype.h"
//#include "ms_notify.h"
#include "VideoChannelFrame.h"

#if TARGET_CHIP_I6E
#include "mi_eptz.h"
#include "mi_ldc.h"
#endif
typedef struct _VideoConsumer
{
    struct list_head ConsumerList;
    VideoChannelFrame *tChannel;
}VideoConsumer;

class MI_VideoConsumer
{
public:
    MI_VideoConsumer();
    ~MI_VideoConsumer();

    MI_BOOL AddConsumer(VideoChannelFrame &tframe);
    MI_BOOL DelConsumer(VideoChannelFrame &tframe);
    MI_U32 GetConsumerCount();
    MI_U32 Consumer(const FrameInf_t &pFrameInf);

private:

    MI_U32 ConsumerCount;
    MyMutex        m_Mutex;
    struct list_head mConsumerList;
    //deoConsumer mTmpVideoConsumerList;
};

class MI_VideoEncoder
{
public:
    static MI_VideoEncoder* createNew(const int streamId);

    ~MI_VideoEncoder(void);

    static MI_U64 fStreamTimestamp;
    static MI_VPE_HDRType_e    eHDRType;
    static MI_VPE_3DNR_Level_e e3DNRLevel;
    static MI_U32              vifframeRate;
    static MI_U32              vpeframeRate;
    static MI_SYS_ChnPort_t    VifChnPort;
    static MI_SYS_ChnPort_t    VpeChnPortTop;
    static MI_SYS_ChnPort_t    VpeChnPortBottom;
#if TARGET_CHIP_I6B0 || TARGET_CHIP_I6 || TARGET_CHIP_I6E
    static BOOL bVpePort2share;        //multiple chn connect after vpe port2,it is unneeded to change port 2 resolution when change one resolution of them.
#endif
#if TARGET_CHIP_I6B0
    static MI_U32 u32RealDivpChn;    //0xff means had not created.
    static BOOL  bRealDivpInit;
#endif
#if TARGET_CHIP_I6E
    static MI_S32 initLdc(MI_VPE_CHANNEL VpeChannel, char *pLdcBinPath);
#endif

private:
    MI_VideoEncoder(const int streamId);

public:
    MI_S32 startVideoEncoder(void);
    MI_S32 stopVideoEncoder();
    MI_S32 initVideoEncoder();
    MI_S32 initDivpAndVdisp(MI_S32 s32Mode);
    MI_S32 initVideoInput(Mixer_VPE_PortInfo_T *pstMixerVpePortInfo);

    MI_S32 uninitVideo(MI_U32 s32Mode);
    MI_S32 uninitVideoV2(MI_U32 s32Mode);

    MI_S32 setResolution(MI_U32 width, MI_U32 height);
    MI_S32 _setResolution(MI_U32 width, MI_U32 height);
    MI_S32 setRotate(MI_SYS_Rotate_e eSetType);
    MI_S32 setRotateV2(MI_SYS_Rotate_e eSetType);
    MI_S32 setFrameRate(MI_U32 frameRate,MI_U32 bChangeBitrate = 0);
    MI_S32 changeCodec(MI_S32 * param);
    VOID setGop(MI_U32 gop);
    VOID setVirGop(MI_U32 enhance, MI_U32 virtualIEnable);
    MI_S32 SetRoiCfg(MI_VENC_RoiCfg_t *pstRoiCfg);
    MI_S32 setRcParamEx(MI_S32* pRcParam);

    void requestIDR();

#if TARGET_CHIP_I5 || TARGET_CHIP_I6 || TARGET_CHIP_I6B0 || TARGET_CHIP_I6E
    MI_S32 setSuperFrm(MI_U32 superIFrmBitsThr, MI_U32 superPFrmBitsThr);
#endif

    void startGetFrame();
    void stopGetFrame();
    BOOL canGetFrame();
    Mixer_EncoderType_e getCodec()
    {
        return m_encoderType;
    };

    unsigned int getBitrate() {return m_bitRate;};

    void startGetStreamThread();
    void stopGetStreamThread();

    MI_VideoConsumer &GetLiveChannelConsumer(){ return mLiveChannelConsumer;}

    pthread_t &GetThreadState(){return m_getStreamThread; }
    void ClearThreadState(){ m_getStreamThread =0x0;}

    BOOL m_thread_exit;

    Mixer_EncoderType_e   m_encoderType;
    MI_SYS_ChnPort_t      m_VpeChnPort;
    MI_SYS_ChnPort_t      m_VencChnPort;
    MI_SYS_ChnPort_t      m_DivpChnPort;
    BOOL                  m_bDivpInit;
    BOOL                  m_bDivpFixed;

    MI_BOOL m_bUseDivp;

    //_VPE_RunningMode_e  m_eRunningMode;
    MI_SYS_PixelFormat_e  m_eVpeOutportPixelFormat;

    VENC_CHN              m_veChn;  //StreamIndex
    MI_U32                m_width;
    MI_U32                m_height;
    MI_U32                m_widthmax;
    MI_U32                m_heightmax;
    MI_U16                m_VpeOutputWidth;    // Width  of VPE output port
    MI_U16                m_VpeOutputHeight;   // Height of VPE output port

    MI_U32                m_vencframeRate;
    MI_U32                m_vpeBufUsrDepth;
    MI_U32                m_vpeBufCntQuota;
    MI_U32                m_vencBufUsrDepth;
    MI_U32                m_vencBufCntQuota;
    MI_U32                m_divpBufUsrDepth;
    MI_U32                m_divpBufCntQuota;
    ViChnStatus           m_viChnStatus;
    MI_SYS_Rotate_e       m_initRotate;


    MI_U32                m_pipCfg;
    MI_DIVP_CHN m_DivpSclUpChnId;
    MI_DIVP_CHN m_DivpSclDownChnId;
    MI_U16                m_pipRectX;
    MI_U16                m_pipRectY;
    MI_U16                m_pipRectW;
    MI_U16                m_pipRectH;

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    Mixer_Venc_Bind_Mode_E m_bindMode;
#endif

// for changeCodec : Mjpeg -> H264/H265
    MI_S32                m_qfactor;
    MI_U32                m_MaxQfactor;
    MI_U32                m_MinQfactor;

// for jepg/Mjpeg dynamically snap
    MI_U32                m_snapLastTimeStamp;
    MI_U32                m_snapInterval;
    MI_U32                m_snapNumber;
    BOOL                  m_snapDynamicallyMode;

    // for -w start at PPS/SPS
    pthread_mutex_t m_stopMutex = PTHREAD_MUTEX_INITIALIZER;

    MI_U32          m_VideoProfile;
    MI_U32                 m_bitRate;
    BOOL                   m_disposable;
    MI_U32                 m_rateCtlType;
    MI_U32                 m_gop;
    MI_U32                 m_maxIQp;
    MI_U32                 m_minIQp;
    MI_U32                 m_maxPQp;
    MI_U32                 m_minPQp;
    MI_S32                 m_IPQPDelta;
    MI_U32                 m_64mMemoryMode;
    VideoInfoCalculator   *m_videoInfoCaculator;
    MI_U32                 m_virtualIInterval;
    BOOL                   m_virtualIEnable;
    BOOL                   m_vencGetStreamStatus;
    MI_BOOL m_bChangeRes; //venc channel need to change resolution or not.

protected:
    pthread_t m_getStreamThread;

    MI_U32 m_videoBufferMaxCount;

    BOOL   m_startGetFrame;

    //VideoChannelFrame mLiveChannelFrame1;
    MI_VideoConsumer mLiveChannelConsumer;
};

#endif //_MI_VIDEO_ENCODER_H_

