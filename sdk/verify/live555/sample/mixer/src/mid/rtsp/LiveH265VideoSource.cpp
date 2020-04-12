/*
* LiveH265VideoSource.cpp- Sigmastar
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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <netinet/in.h>
#include <malloc.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>

#include "LiveH265VideoSource.hh"
#include "ms_notify.h"


LiveH265VideoSource* LiveH265VideoSource::
createNew(UsageEnvironment& env, int streamId, MI_VideoEncoder *videoEncoder)
{
    LiveH265VideoSource *liveVideoSource = NULL;

    do
    {
        liveVideoSource = new LiveH265VideoSource(env, streamId, videoEncoder, False);

    return liveVideoSource;
    }while(0);

    return NULL;
}

LiveH265VideoSource* LiveH265VideoSource::
createNew(UsageEnvironment& env, int streamId, MI_VideoEncoder *videoEncoder, Boolean needSynchronizedAudio)
{
    LiveH265VideoSource *liveVideoSource = NULL;

    do
    {
        liveVideoSource = new LiveH265VideoSource(env, streamId, videoEncoder, needSynchronizedAudio);

        return liveVideoSource;
    }while(0);

    return NULL;
}

LiveH265VideoSource::
LiveH265VideoSource(UsageEnvironment &env, const int streamId, MI_VideoEncoder *videoEncoder, Boolean needSynchronizedAudio)
    : FramedSource(env), \
      fNeedSynchronizedAudio(needSynchronizedAudio), \
      fVideoEncoder(videoEncoder),\
      fLastFrameTimestamp(0),\
      m_pToken(NULL),\
      mLiveChannelFrame(0)
{
       fDurationInMicroseconds = 0;

    fNumTruncatedBytes = 0x0;
    mLeftData = 0x0;
    bNeedIFrame = TRUE;
    fNextFrameToStream =NULL;
    memset(&mLastFrame, 0x0, sizeof(mLastFrame));

    MI_BOOL bRet = FALSE;

    if(NULL != fVideoEncoder)
    {
        bRet = fVideoEncoder->GetLiveChannelConsumer().AddConsumer(mLiveChannelFrame);    //register
        if(!bRet)
        {
            MIXER_ERR("AddConsumer failed! return!\n");
            return ;
        }
    }

    mLiveChannelFrame.Open(streamId);

    if (!fNeedSynchronizedAudio && NULL != fVideoEncoder)
        fVideoEncoder->requestIDR();
}

LiveH265VideoSource::
~LiveH265VideoSource()
{
    if(NULL != fVideoEncoder)
        fVideoEncoder->GetLiveChannelConsumer().DelConsumer(mLiveChannelFrame);    //unregister

    mLiveChannelFrame.Close();

    if(NULL != mLastFrame.pPacketAddr && NULL != fVideoEncoder )
    {
        mLiveChannelFrame.ReleaseOneFrame(mLastFrame);
    }

    envir().taskScheduler().unscheduleDelayedTask(m_pToken);

    envir() << "~LiveH265VideoSource()\r\n";
}

void LiveH265VideoSource::
checkForSynchronizedAudio(void *clientData)
{
    LiveH265VideoSource *source = (LiveH265VideoSource*) clientData;

    source->checkForSynchronizedAudio1();
}

void LiveH265VideoSource::
checkForSynchronizedAudio1()
{

    do
    {

        fNeedSynchronizedAudio = False;

        fVideoEncoder->requestIDR();

        setDoneFlag();

        return;
    } while (0);

    //envir().taskScheduler().turnOnBackgroundReadHandling(fNotifyFd, (TaskScheduler::BackgroundHandlerProc*) LiveH265VideoSource::checkForSynchronizedAudio, this);
}

void LiveH265VideoSource::
waitForSynchronizedAudio()
{
    //envir().taskScheduler().turnOnBackgroundReadHandling(fNotifyFd, (TaskScheduler::BackgroundHandlerProc*) LiveH265VideoSource::checkForSynchronizedAudio, this);
    //envir().taskScheduler().doEventLoop(&fDoneFlag);
    envir() << "out LiveH265VideoSource::waitForSynchronizedAudio()\r\n";
}

void LiveH265VideoSource::
doGetNextFrame()
{
    if (fNeedSynchronizedAudio)
    {
        waitForSynchronizedAudio();
    }

    {
        incomingDataHandler(this);
    }

}

void LiveH265VideoSource::
incomingDataHandler(void *clientData)
{
    LiveH265VideoSource *source = (LiveH265VideoSource*) clientData;

    source->incomingDataHandler1();
}

void LiveH265VideoSource::
incomingDataHandler1()
{
    unsigned long long microDeltaTime = 0LL;
    FrameInf_t  tFrame;

        char* pFrame = NULL;

    fFrameSize  = 0x0;
    fNumTruncatedBytes = 0x0;
    //MIXER_ERR("fNumTruncatedByte(%d)\n", fNumTruncatedBytes);
    do{
        if(0x0 == mLeftData)
        {
            if(0x0 == mLiveChannelFrame.GetOneFrame(tFrame))
            {
                //envir() << "LiveH265VideoSource, get frame err\r\n";
                break;
            }

                if(VIDEO_YUV_STREAM < tFrame.StreamType)
                {
                envir() << "LiveH265VideoSource:: stream type err\r\n";
                break;
                }
                if(TRUE == bNeedIFrame)
                {
                if(VIDEO_FRAME_TYPE_I == tFrame.nFrameType)
                {
                    bNeedIFrame = FALSE;
                }
                else
                {
                    //fVideoEncoder->GetLiveChannelFrameNode().ReleaseOneFrame(tFrame);
                    mLiveChannelFrame.ReleaseOneFrame(tFrame);
                    MIXER_DBG("LiveH265VideoSource:: it is not I frame, ch(%d), %d\r\n", \
                                        fVideoEncoder->m_veChn, tFrame.Ch);
                    break;
                }
            }

            if(0x0 == tFrame.nLen)
            {
                envir() << "LiveH265VideoSource:: frame len == 0\r\n";
                break;
            }

            //拆分pps sps vps 等单元
            findNalUnit((const char*) (((CPacket *)tFrame.pPacketAddr)->GetBuffer()), \
                        tFrame.nLen,\
                        &pFrame,\
                        &fFrameSize);

            if (0 == fHaveSeenIdrFrame || 0 == fFrameSize)
                {
                    MIXER_ERR("fHaveSeenIdrFrame:%d, fFrameSize:%d\r\n", \
                            fHaveSeenIdrFrame,\
                            fFrameSize);

                mLiveChannelFrame.ReleaseOneFrame(tFrame);
                    break;
                }

            if (0 == fLastFrameTimestamp)
            {
                gettimeofday(&fPresentationTime, NULL);
                /*if(0 != fLastFrameTimestamp)
                {
                    MIXER_ERR("time:%lld\n", (fPresentationTime.tv_sec * 1000L + fPresentationTime.tv_usec /1000L) - fLastFrameTimestamp);
                }

                fLastFrameTimestamp =  (fPresentationTime.tv_sec * 1000L + fPresentationTime.tv_usec /1000L);*/
            }
            else
            {
                microDeltaTime = tFrame.nPts - fLastFrameTimestamp;
                fPresentationTime.tv_usec += microDeltaTime;
                fPresentationTime.tv_sec += (fPresentationTime.tv_usec / 1000000L);
                fPresentationTime.tv_usec %= 1000000L;
            }

            fLastFrameTimestamp = tFrame.nPts;

            /*MIXER_ERR("len is %d, fMaxSize:%d, %p, %p, %d, %d\n", tFrame.nLen, \
                                            fMaxSize,\
                                            (char*) (((CPacket *)tFrame.pPacketAddr)->GetBuffer()),\
                                            pFrame,\
                                            fFrameSize,\
                                            fHaveSeenIdrFrame);*/

             if(fFrameSize > fMaxSize)
                {
                fNumTruncatedBytes = fFrameSize - fMaxSize;
                fFrameSize = fMaxSize;
                }

            fNextFrameToStream = pFrame + fFrameSize;
            mLeftData =(char *)( ((CPacket *)tFrame.pPacketAddr)->GetBuffer() + tFrame.nLen) - fNextFrameToStream;
            mLastFrame = tFrame;
            memcpy(fTo, pFrame, fFrameSize);

            if(0x0 == mLeftData)
            {
                mLiveChannelFrame.ReleaseOneFrame(tFrame);
            }

        }
        else
        {
            //MIXER_ERR("mLeftData(%d)\n", mLeftData);

            if(NULL != mLastFrame.pPacketAddr)
            {
                if(NULL != fNextFrameToStream)
                {
                    findNalUnit( fNextFrameToStream, \
                                mLeftData,\
                                &pFrame,\
                                &fFrameSize);
                    if (0 == fHaveSeenIdrFrame || 0 == fFrameSize)
                       {
                           if(NULL != fVideoEncoder)
                           {
                            mLiveChannelFrame.ReleaseOneFrame(mLastFrame);
                        }
                        MIXER_ERR("fHaveSeenIdrFrame:%d, fFrameSize:%d\r\n", \
                            fHaveSeenIdrFrame,\
                            fFrameSize);
                            break;
                       }
                    if(fFrameSize > fMaxSize)    // it won't be happened
                        {
                            MIXER_ERR("it won't be happened, please check, fMaxSize(%d), fFrameSize(%d)\n",\
                                    fMaxSize, fFrameSize);
                        fNumTruncatedBytes = fFrameSize - fMaxSize;
                        fFrameSize = fMaxSize;

                        }

                    fNextFrameToStream = pFrame + fFrameSize;
                    mLeftData =  (char*)(((CPacket *)mLastFrame.pPacketAddr)->GetBuffer() + mLastFrame.nLen) - fNextFrameToStream;
                    memcpy(fTo, pFrame, fFrameSize);

                    if(0x0 == mLeftData)
                    {
                        mLiveChannelFrame.ReleaseOneFrame(mLastFrame);
                    }
                }
                else
                {
                    mLiveChannelFrame.ReleaseOneFrame(mLastFrame);
                    break;
                }
            }
            else
            {
                mLeftData = 0x0;
                break;
            }
        }

        afterGetting(this);
        return;
    }while(0);

    mLeftData = 0;
    m_pToken = envir().taskScheduler().scheduleDelayedTask(5000, incomingDataHandler, this);

    return;
}


#if 1
int LiveH265VideoSource::findNalUnit(const char *frameToStream, const int frameBytesToStream, char** naluToStream, unsigned int* naluBytesToStream)
{
    char *bs = (char*) frameToStream;
    const char *ls = (frameToStream + frameBytesToStream) - 1;

    char naluType = 0;
    int state = 7;

    //printf("%s[+]\r\n", __func__);

    do
    {
        *naluToStream = NULL;
        *naluBytesToStream = 0;

        while(bs < ls)
        {
            if(7 == state)
            {
                if(0x00 == *bs)
                {
                    state = 2;
                }
            }
            else if(2 >= state)
            {
                if(0x01 == *bs)
                {
                    state ^= 5;
                }
                else if(*bs)
                {
                    state = 7;
                }
                else
                {
                    state >>= 1;
                }
            }
            else if(5 >= state)
            {
                naluType = (*bs >> 1) & 0x3f;

                if(NULL == *naluToStream)
                {
                    *naluToStream = (char*) bs;

                    fHaveSeenIdrFrame = (HEVC_NAL_UNIT_VPS == naluType)? 1 : fHaveSeenIdrFrame;

                    //printf("_findNalUnit(), 0x%08x, naluType = %d\n", (unsigned int) this, (int) naluType);
                }
                else
                {
                    *naluBytesToStream += (bs - *naluToStream);
                    *naluBytesToStream -= (state - 1);

                    break;
                }

                if(naluType == HEVC_NAL_UNIT_CODED_SLICE_TRAIL_R || naluType == HEVC_NAL_UNIT_CODED_SLICE_IDR)
                {
                    *naluBytesToStream = ls - *naluToStream + 1;
                    break;
                }

                state = 7;
            }

            bs++;
        }

        //printf("_findNalUnit(), bs = 0x%x, ls = 0x%x\n", bs, ls);

        if(ls == bs)
        {
            if(NULL == *naluToStream)
            {
                //printf("_findNalUnit(), no find start code\n");
                break;
            }

            *naluBytesToStream += (ls - *naluToStream) + 1;

            //printf("_findNalUnit(), find nul unit, naluType = %d, naluBytesToStream = %d\n", naluType, *naluBytesToStream);
        }

        //printf("%s[-]\r\n", __func__);
        return 0;
    }while(0);

    return -1;
}
#endif
