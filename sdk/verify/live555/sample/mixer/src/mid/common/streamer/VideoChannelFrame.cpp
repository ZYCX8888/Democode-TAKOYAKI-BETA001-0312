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
#include "VideoChannelFrame.h"
#include "mid_common.h"
#include "mid_sys.h"
#include "assert.h"

VideoChannelFrame::VideoChannelFrame(MI_U8 ch)
{

    NeedIFrame = true;
    m_Channel = ch;
    m_CChannelFrame = NULL;
    m_StreamType = 0x00;
    //pthread_mutex_init(&m_mutex, NULL);
}

VideoChannelFrame::~VideoChannelFrame()
{
    Close();

    //pthread_mutex_destroy(&m_mutex);

}

MI_S32 VideoChannelFrame::Open(MI_U8 tStreamType, MI_U32 length  /* = FIFO_LEN_DEF*/)
{
    
    if(NULL != m_CChannelFrame)
    {
        MIXER_ERR("m_CChannelFrame must close first\n");
        return -1;
    }
      m_StreamType = tStreamType;
    if(VIDEO_YUV_STREAM > m_StreamType)//h264/h265/jpeg
    {
        if(length > FIFO_LEN_DEF)
            length = FIFO_LEN_DEF;

       m_CChannelFrame = new CChannelFrame((MI_U32)length);
    }
    else if(VIDEO_YUV_STREAM == m_StreamType)//yuv
    {
        if(length > FIFO_YUV_LEN_DEF)
            length = FIFO_YUV_LEN_DEF;
       m_CChannelFrame = new CChannelFrame((MI_U32)length);
    }
    else //other
    {
        if(length > FIFO_LEN_DEF)
            length = FIFO_LEN_DEF;
       m_CChannelFrame = new CChannelFrame((MI_U32)length);
    }

    assert(NULL != m_CChannelFrame);
	MIXER_DBG("streamType:%d, length:%d\n", tStreamType, length);

    return 0x0;
}

MI_S32 VideoChannelFrame::Close()
{
    FrameInf_t FrameInf;
    while ( GetOneFrame(FrameInf)>0 )
    {
        ReleaseOneFrame(FrameInf);
    }

    if(NULL != m_CChannelFrame)
    {
        delete m_CChannelFrame;
        m_CChannelFrame = NULL;
    }
    return 0x0;
}

bool VideoChannelFrame::State()
{
    return 0x0;
}

MI_S32 VideoChannelFrame::GetOneFrame(FrameInf_t &pFrameInf)
{
    if(NULL == m_CChannelFrame)
    {
        return 0;
    }
        return m_CChannelFrame->GetFrameFromFifo(pFrameInf);
}

MI_S32 VideoChannelFrame::ReleaseOneFrame(FrameInf_t &pFrameInf)
{
#if 0
    CPacket *tmp;
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

MI_S32 VideoChannelFrame::SetbFlagNeedIFrame()
{
    NeedIFrame = TRUE;

    return 0;
}

MI_S32 VideoChannelFrame::CleanFrameBuf()
{
    /*pthread_mutex_lock(&m_mutex);
    m_CChannelFrame->DoClean();
    pthread_mutex_unlock(&m_mutex);*/
    return 0;
}

MI_S32 VideoChannelFrame::OnData(const FrameInf_t &pFrameInf)
{
    MI_S32 _tmp = 0x0;
    FrameInf_t objStreamData = pFrameInf;
    CPacket *tmp = (CPacket *)objStreamData.pPacketAddr;

    //pthread_mutex_lock(&m_mutex);
    tmp->AddRef();

    if (objStreamData.StreamType < AV_STREAM_MAX)    //video
    {
        do {
            if (TRUE == NeedIFrame && \
                VIDEO_FRAME_TYPE_I != objStreamData.nFrameType && \
                VIDEO_FRAME_TYPE_YUV != objStreamData.nFrameType && \
                VIDEO_FRAME_TYPE_JPEG != objStreamData.nFrameType)
            {
                tmp->Release();
                //pthread_mutex_unlock(&m_mutex);
                return -1;
            }

            NeedIFrame = FALSE;
            _tmp = m_CChannelFrame->PutFrame2Fifo(objStreamData);
            if (0x0 == _tmp)
            {
                MIXER_INFO("fifo is full, clean it, owner:%p\n", this);
                //DoClean();
                tmp->Release();

                NeedIFrame = TRUE;
                //continue;
                //pthread_mutex_unlock(&m_mutex);
                return -1;
            }
        } while (0x0 == _tmp);
    }
    else
    {
        tmp->Release();
        //pthread_mutex_unlock(&m_mutex);
        return -1;
    }
    //pthread_mutex_unlock(&m_mutex);
    return _tmp;
}
