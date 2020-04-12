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
#include "AudioChannelFrame.h"
#include "ChannelFrame.h"
#include "mid_common.h"
#include "mid_sys.h"
#include "assert.h"

AudioChannelFrame::AudioChannelFrame(MI_U32 length /* = AUDIO_FIFO_LEN_DEF*/, MI_U8 m_nChnId)
{
    m_nId = m_nChnId;
    m_CChannelFrame = NULL;
}

AudioChannelFrame::~AudioChannelFrame()
{
    Close();
    FrameInf_t FrameInf;
    while ( GetOneFrame(FrameInf)>0 ) {
        ReleaseOneFrame(FrameInf);
    }
}

MI_S32 AudioChannelFrame::Open()
{
    if(NULL != m_CChannelFrame)
    {
        MIXER_ERR("m_CChannelFrame must close first\n");
        return -1;
    }
    m_CChannelFrame = new CChannelFrame((MI_U32)AUDIO_FIFO_LEN_DEF);

    assert(NULL != m_CChannelFrame);
    return 0x0;
}

MI_S32 AudioChannelFrame::Close()
{
    FrameInf_t FrameInf;
    while(GetOneFrame(FrameInf) > 0)
    {
        ReleaseOneFrame(FrameInf);
    }
    if(NULL != m_CChannelFrame)
    {
        delete m_CChannelFrame;
        m_CChannelFrame =NULL;
    }
    return 0x0;
}

bool AudioChannelFrame::State()
{
    return 0x0;
}

MI_S32 AudioChannelFrame::GetOneFrame(FrameInf_t &pFrameInf)
{
    if(NULL == m_CChannelFrame)
    {
        return 0;
    }
    return m_CChannelFrame->GetFrameFromFifo(pFrameInf);
}

MI_S32 AudioChannelFrame::ReleaseOneFrame(FrameInf_t &pFrameInf)
{
#if 0
    CPacket *tmp;
    if(NULL == m_CChannelFrame)
    {
        return 0;
    }
    tmp = (CPacket *)pFrameInf.pPacketAddr;
    return tmp->Release();
#else
    if(NULL == m_CChannelFrame)
    {
        return 0;
    }
    return m_CChannelFrame->ReleaseFrame(pFrameInf);
#endif
}

MI_S32 AudioChannelFrame::OnData(const FrameInf_t &pFrameInf)
{
    FrameInf_t objStreamData = pFrameInf;
    MI_S32 _tmp = 0x0;

    //printf("addr(%p), len(%d), StreamType(%d)\n", objStreamData.pPacketAddr, pFrameInf.nLen, pFrameInf.StreamType);
    CPacket *tmp = (CPacket *)objStreamData.pPacketAddr;
    tmp->AddRef();

    if(m_nId == (objStreamData.StreamType - 5)) //audio
    {
        do {
            _tmp = m_CChannelFrame->PutFrame2Fifo(objStreamData);
            if (0x0 == _tmp)
            {
                //DoClean();
                tmp->Release();
                continue;
            }

        } while (0x0 == _tmp);
    }
    else
    {
        tmp->Release();
        return -1;
    }

    return _tmp;
}

