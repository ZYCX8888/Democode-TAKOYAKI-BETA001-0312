/*************************************************
 *
 * Copyright (c) 2018-2019 SigmaStar Technology Inc.
 * All rights reserved.
 *
 **************************************************
 * File name:  Record.cpp
 * Author:     fisher.yang@sigmastar.com.cn
 * Version:    Initial Draft
 * Date:       2019/8/2
 * Description: mixer record source file
 *
 *
 *
 * History:
 *
 *    1. Date  :        2019/8/3
 *       Author:        fisher.yang@sigmastar.com.cn
 *       Modification:  Created file
 *
 **************************************************/
#include <assert.h>
#include "Record.h"
#include "mid_common.h"
#include "SStarFs.h"


#define CheckTimeChange(t1, t2)    (\
        t1.second != t2.second || t2.minute != t2.minute || t1.hour != t2.hour ||\
        t1.day != t2.day || t1.month != t2.month || t1.year != t2.year \
        )\

CRecord::CRecord(MI_U32 ch, MI_VideoEncoder *fpVideoEncoder, const MI_U8 *path):
    m_nChannel (ch),
    m_nMode(REC_MODE_NONE),
    m_bNeedIFrame(FALSE),
    pVideoEncoder(fpVideoEncoder),
    mRecordChannelFrame(ch)
{
    LastTime = 0x0;
    pthread_mutex_init(&m_mutexCtrl, NULL);
    pthread_cond_init(&m_condCtrl, NULL);

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
    mRecordChannelFrame.Open(Stype, len);
    m_pRecordStorage = new CRecordStorage(m_nChannel, path);
    assert(m_pRecordStorage);

}

CRecord::~CRecord()
{
    if(m_pRecordStorage)
    {
        delete m_pRecordStorage;
        m_pRecordStorage = NULL;
    }

    pthread_mutex_destroy(&m_mutexCtrl);
    pthread_cond_destroy(&m_condCtrl);
}

MI_S32 CRecord::Init( )
{
    pthread_mutex_lock(&m_mutexCtrl);

    pthread_mutex_unlock(&m_mutexCtrl);

    return 0;
}

MI_S32 CRecord::UnInit()
{
    MI_S32 n32Ret = 0x0;
    FrameInf_t stFrameInf;

    pthread_mutex_lock(&m_mutexCtrl);

    do{

        n32Ret = mRecordChannelFrame.GetOneFrame(stFrameInf);
        if(0x0 != n32Ret)
            mRecordChannelFrame.ReleaseOneFrame(stFrameInf);

    }while(0x0 != n32Ret);

    pthread_mutex_unlock(&m_mutexCtrl);

    return MI_SUCCESS;
}

MI_S32 CRecord::Start(MI_U32 mode)
{
    pthread_mutex_lock(&m_mutexCtrl);

    SetMode(mode);

    MI_U32 m_nCurMode = GetCurMode();

    if(mode < m_nCurMode)
    {
        MIXER_ERR("Record ch%d start mode(0x%x) priority lower than current mode(0x%x).\n", m_nChannel, mode, GetMode());
        MIXER_ERR("Maybe not sdcard or sdcard is covering!\n");
    }
    else
    {
        //creat storage
        if(FALSE == m_pRecordStorage->CreateStorage(GetCodecFormat()))
        {
            MIXER_ERR("can not create storage\n");
            ClearMode(mode);
            pthread_mutex_unlock(&m_mutexCtrl);

            return -1;
        }
        //register
        RegisterVideoEncoder();
        RequestIDR();
        LastTime = 0x0;
    }

    m_bNeedIFrame = TRUE;

    pthread_mutex_unlock(&m_mutexCtrl);

    return 0;
}

MI_S32 CRecord::Stop(MI_U32 mode)
{
    pthread_mutex_lock(&m_mutexCtrl);

    MI_U32 m_nCurMode = GetCurMode();

    ClearMode(mode);

    if(mode != m_nCurMode)
    {
        pthread_mutex_unlock(&m_mutexCtrl);
        return 0;
    }
    else
    {
        //unregister
        UnRegisterVideoEncoder();
        FlushVideoBuff();
        //close storage
        m_pRecordStorage->CloseStorage();
    }

    pthread_mutex_unlock(&m_mutexCtrl);

    return 0;
}

MI_S32 CRecord::Pause()
{
    pthread_mutex_lock(&m_mutexCtrl);
    SetMode(REC_MODE_CLS);

    MI_U32 m_nCurMode = GetCurMode();

    if(m_pRecordStorage->IsOpened())
    {
        UnRegisterVideoEncoder();
        FlushVideoBuff();
        if(FALSE == m_pRecordStorage->CloseStorage())
        {
            MIXER_ERR("Record ch%d do stop failed! Close storage failed\n", m_nChannel);
        }


        MIXER_DBG("CreateStorage Mode is :%d \n", m_nCurMode);

        m_bNeedIFrame = TRUE;
    }
    pthread_mutex_unlock(&m_mutexCtrl);

    return 0;
}

MI_S32 CRecord::Resume()
{
    pthread_mutex_lock(&m_mutexCtrl);
    ClearMode(REC_MODE_CLS);
    m_bNeedIFrame = TRUE;
    MI_U32 m_nCurMode = GetCurMode();

    //protect from Resume() called double time:during sdcard recovering ,the venc call stopVideoEncoder and then startVideoEncoder
    if(REC_MODE_NONE != m_nCurMode && REC_MODE_CLS != m_nCurMode && m_pRecordStorage->IsOpened()==FALSE)
    {
        //DBG_RECORD("Check pack: it is time to pack\n");
        if(FALSE == m_pRecordStorage->CreateStorage(GetCodecFormat()))
        {
            MIXER_ERR("######ch%d CreateStorage failed\n", m_nChannel);

            pthread_mutex_unlock(&m_mutexCtrl);
            return -1;
        }
        RegisterVideoEncoder();
        RequestIDR();
        LastTime = 0x0;

    }
    /*else if(REC_MODE_NONE != m_nCurMode && REC_MODE_CLS != m_nCurMode && m_pRecordStorage->IsOpened()==TRUE)
    {
        char lastFilename[64];
        MI_U32 offset=0;
        strcpy(lastFilename,m_pRecordStorage->GetLastFileName());
        offset = strstr(lastFilename,".");
        if(strncmp(lastFilename[offset+1],GetCodecFormat(),strlen(GetCodecFormat()) ) !=0 )
        {
            MIXER_ERR("call Record::Resume DOUBLE time\nmaybe during sdcard recovering ,the venc call stopVideoEncoder and then startVideoEncoder \n ")

        }

    }*/
    pthread_mutex_unlock(&m_mutexCtrl);

    return 0;
}

MI_S32 CRecord::DoRecord()
{
    MI_U64 CurTime = 0x0;
    SYSTEM_TIME stCurTime = {0x0,0x0,};
    MI_U32 m_nCurMode = 0x0;
    Mixer_EncoderType_e codec = VE_TYPE_MAX;
    MI_S32 ret = 0x00;
    if(!m_pRecordStorage)
    {
        MIXER_ERR("ch %d record failed! para is null!\n", m_nChannel);
        return -1;
    }

    if(NULL != pVideoEncoder)
    {
        codec = pVideoEncoder->getCodec();
    }

    pthread_mutex_lock(&m_mutexCtrl);
    m_nCurMode = GetCurMode();

    if(codec <= VE_YUV420)    //h264/h265/mjpeg/yuv
    {
        SystemGetCurrentTime(&stCurTime);
        if(CheckTimeChange(stCurTime, m_stSystemTime))    //这里保证每second 都检测
        {
            if(MI_SUCCESS == MI_SYS_GetCurPts(&CurTime))    //这里保证3分钟打包一个文件
            {
                if(((CurTime - LastTime)/1000000) >= (FILE_LAST_TIME))
                {
                    if(0 != LastTime )
                    {
                        if(m_pRecordStorage->IsOpened())
                        {
                            UnRegisterVideoEncoder();
                            FlushVideoBuff();
                            if(FALSE == m_pRecordStorage->CloseStorage())
                            {
                                MIXER_ERR("Record ch%d do stop failed! Close storage failed\n", m_nChannel);
                            }

                            if(TRUE == m_pRecordStorage->IsDiskFull())
                            {
                                MIXER_ERR("disk is already full\n");
                                pthread_mutex_unlock(&m_mutexCtrl);
                                return REC_ERR_DISKFULL;
                            }

                            MIXER_DBG("CreateStorage Mode is :%d \n", m_nCurMode);
                            if(FALSE == m_pRecordStorage->CreateStorage(GetCodecFormat()))
                            {
                                MIXER_ERR("######ch%d CreateStorage failed\n", m_nChannel);

                                pthread_mutex_unlock(&m_mutexCtrl);
                                return REC_ERR_OPEN;
                            }
                            RegisterVideoEncoder();
                            RequestIDR();
                            m_bNeedIFrame = TRUE;

                        }
                    }

                    LastTime = CurTime;
                }
            }

            m_stSystemTime = stCurTime;
        }

        if((REC_MODE_NONE == m_nCurMode) || (REC_MODE_CLS == m_nCurMode))
        {
            pthread_mutex_unlock(&m_mutexCtrl);
            return REC_ERR_CLSMODE;
        }

        if((ret = DoRecordOneFrame()) != 0)
        {
            pthread_mutex_unlock(&m_mutexCtrl);
            return ret;
        }
    }
    else if(codec < VE_TYPE_MAX) //jpg
    {
        if((REC_MODE_NONE == m_nCurMode) || (REC_MODE_CLS == m_nCurMode))
        {
            pthread_mutex_unlock(&m_mutexCtrl);
            return REC_ERR_CLSMODE;
        }

        if(TRUE == m_pRecordStorage->IsOpened())
        {
            if((ret = DoRecordOneFrame()) != 0)
            {
                pthread_mutex_unlock(&m_mutexCtrl);
                return ret;
            }
            else        //make packet for jpg when every frame come
            {
                UnRegisterVideoEncoder();
                FlushVideoBuff();
                if(FALSE == m_pRecordStorage->CloseStorage())
                {
                    MIXER_ERR("Record ch%d do stop failed! Close storage failed\n", m_nChannel);
                }

                if(TRUE == m_pRecordStorage->IsDiskFull())
                {
                    MIXER_ERR("disk full\n");
                    pthread_mutex_unlock(&m_mutexCtrl);
                    return REC_ERR_DISKFULL;
                }

                if(FALSE == m_pRecordStorage->CreateStorage(GetCodecFormat()))
                {
                    MIXER_ERR("##ch%d Create err\n", m_nChannel);

                    pthread_mutex_unlock(&m_mutexCtrl);
                    return REC_ERR_OPEN;
                }
                RegisterVideoEncoder();
            }
        }
        else
        {

        }
    }
    else
    {

    }



    pthread_mutex_unlock(&m_mutexCtrl);
    return 0;
}

//must notice :: retrun RECORD_RET enum
MI_S32 CRecord::DoRecordOneFrame()
{
#if 1
    FrameInf_t stFrameInf;
    MI_S32 ret = 0x0;

    memset(&stFrameInf, 0x0, sizeof(stFrameInf));

    do{
        ret = 0x0;
        if(0x0 == mRecordChannelFrame.GetOneFrame(stFrameInf))
        {
            //venc framerate is low, write faster than getstream
            return REC_ERR_GETFRAME;
        }

        if(NULL == stFrameInf.pPacketAddr)
        {
            MIXER_ERR("pPacketAddr is null!\n");
            return REC_ERR_GETFRAME;
        }

        //DO_CHECK_FRAME:
        if(m_bNeedIFrame)
        {
            if((VIDEO_FRAME_TYPE_I != stFrameInf.nFrameType) && (VIDEO_FRAME_TYPE_YUV != stFrameInf.nFrameType) && (VIDEO_FRAME_TYPE_JPEG) != stFrameInf.nFrameType)
            {
                mRecordChannelFrame.ReleaseOneFrame(stFrameInf);
                ret = REC_ERR_NEEDIFRAME;
                continue;
            }
            else
            {
                m_bNeedIFrame = FALSE;
                break;
            }
        }
    }while(0 != ret);

    //DO_WRITE_STORAGE:
    if(FALSE == m_pRecordStorage->WriteStorage(((CPacket*)stFrameInf.pPacketAddr)->GetBuffer(), \
                stFrameInf.StreamType, \
                stFrameInf.nFrameType,\
                stFrameInf.nLen))
    {
        MIXER_ERR("ch%d record WriteStorage failed!, %d\n", m_nChannel, stFrameInf.Ch);

        mRecordChannelFrame.ReleaseOneFrame(stFrameInf);
        return REC_ERR_WRITE;
    }

    //DO_RELEASE:
    mRecordChannelFrame.ReleaseOneFrame(stFrameInf);
#endif

    return 0;
}

MI_BOOL CRecord::CheckIsDiskFull()
{
    if(!m_pRecordStorage)
    {
        return FALSE;
    }
    pthread_mutex_lock(&m_mutexCtrl);
    MI_BOOL bRet = m_pRecordStorage->IsDiskFull();
    pthread_mutex_unlock(&m_mutexCtrl);
    return bRet;
}

MI_BOOL CRecord::RecRun()
{
    if(!m_pRecordStorage)
    {
        return FALSE;
    }
    MI_BOOL bRet = m_pRecordStorage->IsOpened();
    return bRet;
}

MI_U32 CRecord::GetCurMode()
{
    if(m_nMode & REC_MODE_CLS)
    {
        return REC_MODE_CLS;
    }
    else if(m_nMode & REC_MODE_MAN)
    {
        return REC_MODE_MAN;
    }
    else if(m_nMode & REC_MODE_TIM)
    {
        return REC_MODE_TIM;
    }
    else
    {
        return REC_MODE_NONE;
    }
}

MI_U32 CRecord::GetMode()
{
    return m_nMode;
}

MI_U32 CRecord::SetMode(MI_U32 mode)
{
    m_nMode |= mode;
    return m_nMode;
}

MI_U32 CRecord::ClearMode(MI_U32 mode)
{
    m_nMode &= ~mode;
    return m_nMode;
}

MI_U32 CRecord::RegisterVideoEncoder()
{
    if(NULL != pVideoEncoder)
    {
        pVideoEncoder->GetLiveChannelConsumer().AddConsumer(mRecordChannelFrame);
    }

    return TRUE;
}

MI_U32 CRecord::UnRegisterVideoEncoder()
{
    if(NULL != pVideoEncoder)
    {
        pVideoEncoder->GetLiveChannelConsumer().DelConsumer(mRecordChannelFrame);
    }

    return TRUE;
}


MI_U32 CRecord::FlushVideoBuff()
{
    MI_S32 ret = 0x0;

    do{
        ret = DoRecordOneFrame();
    }while(ret == 0);

    return TRUE;
}

MI_U32 CRecord::RequestIDR()
{
    if(NULL != pVideoEncoder)
    {
        pVideoEncoder->requestIDR();
    }

    return TRUE;
}

const MI_U8 * CRecord::GetCodecFormat()
{
    Mixer_EncoderType_e codec = VE_TYPE_MAX;
    const MI_U8* pname = NULL;

    if(NULL != pVideoEncoder)
    {
        codec = pVideoEncoder->getCodec();
    }

    switch(codec)
    {
        case VE_AVC:
            pname = (const MI_U8* )"h264";
            break;

        case VE_H265:
            pname = (const MI_U8* )"h265";
            break;

        case VE_MJPEG:
            pname = (const MI_U8* )"mjpeg";
            break;

         case VE_YUV420:
            pname = (const MI_U8* )"yuv420";
            break;

        case VE_JPG:
            pname = (const MI_U8* )"jpg";
            break;

        case VE_JPG_YUV422:
            pname = (const MI_U8* )"yuv422";
            break;

        default:
            pname = (const MI_U8* )"null";
            break;
    }

    return pname;
}
