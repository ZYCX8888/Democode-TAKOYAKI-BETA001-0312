#include "CUvc.h"

CUvc::CUvc(MI_U32 ch, MI_VideoEncoder *fpVideoEncoder):
    m_bNeedIFrame(FALSE),
    mpVideoEncoder(fpVideoEncoder),
    mUvcChannelFrame(ch)
{
    MI_U8 Stype = VIDEO_MAIN_STREAM, len = FIFO_LEN_DEF;
    Mixer_EncoderType_e eType;

    if(NULL != fpVideoEncoder)
    {
        eType = fpVideoEncoder->getCodec();
        if(VE_AVC == eType || VE_H265 == eType)
        {
            Stype = VIDEO_MAIN_STREAM;
            len = FIFO_LEN_DEF;
        }
        else if(VE_MJPEG == eType || VE_JPG == eType || VE_JPG_YUV422 == eType)
        {
            Stype = VIDEO_MAIN_STREAM;
            len = 2;
        }
        else if(VE_YUV420 == eType)
        {
            Stype = VIDEO_YUV_STREAM;
            len = 4;
        }

    }
    mUvcChannelFrame.Open(Stype, len);
}

CUvc::~CUvc()
{

}

MI_U32 CUvc::RegisterVideoEncoder()
{
    if(NULL != mpVideoEncoder)
    {
        mpVideoEncoder->GetLiveChannelConsumer().AddConsumer(mUvcChannelFrame);
    }

    return TRUE;
}

MI_U32 CUvc::UnRegisterVideoEncoder()
{
    if(NULL != mpVideoEncoder)
    {
        mpVideoEncoder->GetLiveChannelConsumer().DelConsumer(mUvcChannelFrame);
    }

    return TRUE;
}


MI_U32 CUvc::RequestIDR()
{
    if(NULL != mpVideoEncoder)
    {
        mpVideoEncoder->requestIDR();
    }

    return TRUE;
}

MI_U32 CUvc::GetFrameData(void *buf)
{
    FrameInf_t stFrameInf;
    memset(&stFrameInf, 0x0, sizeof(stFrameInf));

    do{
        if(0x0 == mUvcChannelFrame.GetOneFrame(stFrameInf))
        {
            //venc framerate is low, write faster than getstream

            return 0;
        }

        if(NULL == stFrameInf.pPacketAddr)
        {
            MIXER_ERR("pPacketAddr is null!\n");
            return 0;
        }

        //DO_CHECK_FRAME:
        if(m_bNeedIFrame)
        {
            if((VIDEO_FRAME_TYPE_I != stFrameInf.nFrameType) && (VIDEO_FRAME_TYPE_YUV != stFrameInf.nFrameType) && (VIDEO_FRAME_TYPE_JPEG) != stFrameInf.nFrameType)
            {
                mUvcChannelFrame.ReleaseOneFrame(stFrameInf);
                continue;
            }
            else
            {
                m_bNeedIFrame = FALSE;
                break;
            }
        }
    }while(m_bNeedIFrame);

    if(buf)
    {
        memcpy(buf, ((CPacket*)stFrameInf.pPacketAddr)->GetBuffer(),  stFrameInf.nLen);
    }

    mUvcChannelFrame.ReleaseOneFrame(stFrameInf);

    return stFrameInf.nLen;
}

MI_U32 CUvc::FlushVideoBuff()
{
    MI_S32 ret = 0x0;

    do{
        ret = GetFrameData(NULL);
    }while(ret != 0);

    return TRUE;
}

