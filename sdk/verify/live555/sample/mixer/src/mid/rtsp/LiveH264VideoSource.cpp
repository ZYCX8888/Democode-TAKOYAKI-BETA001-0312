/*
* LiveH264VideoSource.cpp- Sigmastar
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <netinet/in.h>
#include <malloc.h>
#include <assert.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>

#include "LiveH264VideoSource.hh"
#include "ms_notify.h"

FILE *live555StreamFile = NULL;

LiveH264VideoSource* LiveH264VideoSource::
createNew(UsageEnvironment& env, int streamId, MI_VideoEncoder *videoEncoder)
{
    LiveH264VideoSource *liveVideoSource = NULL;

    do
    {
        liveVideoSource = new LiveH264VideoSource(env, streamId, videoEncoder, False);

        return liveVideoSource;
    }while(0);

    return NULL;
}

LiveH264VideoSource* LiveH264VideoSource::
createNew(UsageEnvironment& env, int streamId, MI_VideoEncoder *videoEncoder, Boolean needSynchronizedAudio)
{
    LiveH264VideoSource *liveVideoSource = NULL;

    do
    {
        liveVideoSource = new LiveH264VideoSource(env, streamId, videoEncoder, needSynchronizedAudio);

        return liveVideoSource;
    }    while(0);

    return NULL;
}

LiveH264VideoSource::
LiveH264VideoSource(UsageEnvironment &env, const int streamId, MI_VideoEncoder *videoEncoder, Boolean needSynchronizedAudio)
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
    fNextFrameToStream = NULL;
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

    /*if (live555StreamFile == NULL)
            live555StreamFile = fopen("live555_stream.h264", "wb");*/
}

LiveH264VideoSource::
~LiveH264VideoSource()
{
    if(NULL != fVideoEncoder)
        fVideoEncoder->GetLiveChannelConsumer().DelConsumer(mLiveChannelFrame);    //unregister

    mLiveChannelFrame.Close();

    if(NULL != mLastFrame.pPacketAddr && NULL != fVideoEncoder )
    {
        MIXER_DBG("~~~~~~~~~~~~~~~~~\n");

        mLiveChannelFrame.ReleaseOneFrame(mLastFrame);
    }

    envir().taskScheduler().unscheduleDelayedTask(m_pToken);

    /*if (NULL != live555StreamFile)
    {
         fflush(live555StreamFile);
        fclose(live555StreamFile);
        live555StreamFile = NULL;
    }*/
    envir() << "~LiveH264VideoSource()\r\n";
}

void LiveH264VideoSource::
exportStreamToFile(void *clientData)
{
#if 0
    struct timeval tv;

    char fileName[128] = {0};

    LiveH264VideoSource *source = (LiveH264VideoSource*) clientData;

    gettimeofday(&tv, NULL);

    getcwd(fileName, sizeof(fileName));
    snprintf(fileName + strlen(fileName), sizeof(fileName) - strlen(fileName), "/%lu%03ld.h264", tv.tv_sec, tv.tv_usec / 1000L);

    if (NULL == (source->fExportStream = fopen(fileName, "wb")))
    {
        printf("%lu.%03ld, LiveH264VideoSource::exportStreamToFile, fopen(%s) err!\r\n", tv.tv_sec, (tv.tv_usec / 1000L), fileName);
    }
#endif
}

void LiveH264VideoSource::
checkForSynchronizedAudio(void *clientData)
{
    LiveH264VideoSource *source = (LiveH264VideoSource*) clientData;

    source->checkForSynchronizedAudio1();
}

void LiveH264VideoSource::
checkForSynchronizedAudio1()
{
#if 1
    static int audio = 0;
    do
    {

        audio++;

        if (2 == audio)
        {
            fNeedSynchronizedAudio = False;

            fVideoEncoder->requestIDR();

            setDoneFlag();

            audio = 0;
            return;
        }
    } while (0);
#endif

  //  envir().taskScheduler().turnOnBackgroundReadHandling(fNotifyFd, (TaskScheduler::BackgroundHandlerProc*) LiveH264VideoSource::checkForSynchronizedAudio, this);

}

void LiveH264VideoSource::
waitForSynchronizedAudio()
{
    //envir().taskScheduler().turnOnBackgroundReadHandling(fNotifyFd, (TaskScheduler::BackgroundHandlerProc*) LiveH264VideoSource::checkForSynchronizedAudio, this);
   // envir().taskScheduler().doEventLoop(&fDoneFlag);
    envir() << "out LiveH264VideoSource::waitForSynchronizedAudio()\r\n";
}

void LiveH264VideoSource::
doGetNextFrame()
{
    if (fNeedSynchronizedAudio)
    {
        waitForSynchronizedAudio();
    }

    {
        incomingDataHandler1();
    }

}

void LiveH264VideoSource::
incomingDataHandler(void *clientData)
{
    LiveH264VideoSource *source = (LiveH264VideoSource*) clientData;

    source->incomingDataHandler1();
}

void LiveH264VideoSource::
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
                //envir() << "LiveH264VideoSource, get frame err\r\n";
                break;
            }

                if(VIDEO_YUV_STREAM < tFrame.StreamType)
                {
                envir() << "LiveH264VideoSource:: stream type err\r\n";
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

                    mLiveChannelFrame.ReleaseOneFrame(tFrame);
                    MIXER_DBG("LiveH264VideoSource:: it is not I frame, ch(%d), %d\r\n", \
                                                    fVideoEncoder->m_veChn,\
                                                    tFrame.Ch);
                    break;
                }
            }

            if(0x0 == tFrame.nLen)
            {
                envir() << "LiveH264VideoSource:: frame len == 0\r\n";
                break;
            }

            //拆分nal  等单元
            findNalUnit((const char*) (((CPacket *)tFrame.pPacketAddr)->GetBuffer()), \
                        tFrame.nLen,\
                        &pFrame,\
                        &fFrameSize);

            if (0 == fHaveSeenIdrFrame || 0 == fFrameSize)
               {
                   /* MIXER_ERR("LiveH264VideoSource:: fHaveSeenIdrFrame:%d, fFrameSize:%d\r\n", \
                            fHaveSeenIdrFrame,\
                            fFrameSize);*/

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

            /*MIXER_ERR("len is %d, fMaxSize:%d, %p\n", tFrame.nLen, \
                                            fMaxSize,\
                                            (char*) (((CPacket *)tFrame.pPacketAddr)->GetBuffer()));*/

             if(fFrameSize > fMaxSize)
                {
                fNumTruncatedBytes = fFrameSize - fMaxSize;
                fFrameSize = fMaxSize;
                }

            fNextFrameToStream = pFrame + fFrameSize;

            mLeftData = (char *)(((CPacket *)tFrame.pPacketAddr)->GetBuffer() + tFrame.nLen) - fNextFrameToStream;
            mLastFrame = tFrame;
            memcpy(fTo, pFrame, fFrameSize);

            if(0x0 == mLeftData)
            {
                mLiveChannelFrame.ReleaseOneFrame(tFrame);
            }

        }
        else
        {
            //MIXER_ERR("mLeftData(%d), addr(%p)\n", mLeftData, fNextFrameToStream);
            if(NULL != mLastFrame.pPacketAddr )
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
                    mLeftData = (char*)(((CPacket *)mLastFrame.pPacketAddr)->GetBuffer() + mLastFrame.nLen) - fNextFrameToStream;
                    memcpy(fTo, pFrame, fFrameSize);

                    if(0x0 == mLeftData)
                    {
                        mLiveChannelFrame.ReleaseOneFrame(mLastFrame);
                    }
                }
                else
                {
                    mLiveChannelFrame.ReleaseOneFrame(mLastFrame);
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
int LiveH264VideoSource::findNalUnit(const char* frameToStream, const int frameBytesToStream, char** naluToStream, unsigned int* naluBytesToStream)
{
    const char* bs = frameToStream;
    const char* ls = (frameToStream + frameBytesToStream) - 1;

    int state = 7;

    unsigned char naluType = 0;

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
                if(AVC_NAL_UNIT_SEI != (*bs & 0x1F) && AVC_NAL_UNIT_AUD != (*bs & 0x1F))
                {
                    if(NULL == *naluToStream)
                    {
                        naluType = *bs & 0x1F;

                        *naluToStream = (char*) bs;

                        fHaveSeenIdrFrame = (AVC_NAL_UNIT_SPS == naluType)? 1 : fHaveSeenIdrFrame;
                    }
                    else
                    {
                        *naluBytesToStream += (bs - *naluToStream);
                        *naluBytesToStream -= (state - 1);
                        break;
                    }
                }

                if(AVC_NAL_UNIT_IDR == naluType || AVC_NAL_UNIT_NONIDR == naluType)
                {
                    *naluBytesToStream = ls - *naluToStream + 1;
                    break;
                }

                state = 7;
            }

            bs++;
        }

        if(ls == bs)
        {
            break;
        }
#if 0
        if (fHaveSeenIdrFrame && *naluToStream)
        {
            printf("h264FindNalUnit(), 0x%08x, naluType = 0x%02x, naluBytesToStream = %d\n", (unsigned int) this, naluType, *naluBytesToStream);
        }
#endif
        //printf("%s[-]\r\n", __func__);
        return 0;
    }
    while(0);

    return -1;
}
#endif



