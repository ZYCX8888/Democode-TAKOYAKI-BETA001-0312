/*
* LiveStreamSource.cpp- Sigmastar
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

#include "LiveStreamSource.hh"
#include "mid_sys.h"
LiveStreamSource* LiveStreamSource::
createNew(UsageEnvironment& env, int streamId,MI_VideoEncoder* videoEncoder)
{
    LiveStreamSource *liveVideoSource = NULL;

    do
    {

        liveVideoSource = new LiveStreamSource(env, streamId,videoEncoder);

        return liveVideoSource;
    }while(0);

    return NULL;
}

LiveStreamSource::LiveStreamSource(UsageEnvironment& env, const int streamId, MI_VideoEncoder* videoEncoder)
  : FramedSource(env), \
      m_pToken(NULL),\
      fStreamId(streamId), \
      fLastFrameTimestamp(0),\
      fVideoEncoder(videoEncoder),\
      mLiveChannelFrame(0)
{
    fDurationInMicroseconds = 0;

    MI_BOOL bRet = FALSE;

    if(NULL != fVideoEncoder)
    {
        bRet = fVideoEncoder->GetLiveChannelConsumer().AddConsumer(mLiveChannelFrame);
        if(!bRet)
        {
            MIXER_ERR("AddConsumer failed! return!\n");
            return ;
        }
    }

    mLiveChannelFrame.Open(streamId,2);
}

LiveStreamSource::~LiveStreamSource()
{
    if(NULL != fVideoEncoder)
        fVideoEncoder->GetLiveChannelConsumer().DelConsumer(mLiveChannelFrame);

    mLiveChannelFrame.Close();

    envir().taskScheduler().unscheduleDelayedTask(m_pToken);

    envir() << "~LiveStreamSource()\r\n";
}


void LiveStreamSource::
doGetNextFrame()
{
    incomingDataHandler(this);
#if 0
    struct timeval timestamp;
    gettimeofday(&timestamp, NULL);
    printf("%lu.%03ld, LiveStreamSource::doGetNextFrame()\r\n", timestamp.tv_sec, (timestamp.tv_usec / 1000L));
#endif
}

void LiveStreamSource::
incomingDataHandler(void *clientData)
{
    LiveStreamSource *source = (LiveStreamSource*) clientData;

    source->incomingDataHandler1();
}

void LiveStreamSource::
incomingDataHandler1()
{
    //struct timeval nowTime;
   FrameInf_t  tFrame;
   unsigned long long microDeltaTime = 0LL;
   //memset(&tFrame, 0x0 , sizeof(FrameInf_t));
   fFrameSize  = 0x0;
   fNumTruncatedBytes = 0x0;
  do{
        if(0x0 == mLiveChannelFrame.GetOneFrame(tFrame))
        {
            //envir() << "LiveStreamSource, get frame err\r\n";
            break;
        }
        if(VIDEO_YUV_STREAM < tFrame.StreamType)
        {
            envir() << "LiveStreamSource:: stream type err\r\n";
            break;
        }
        if(0x0 == tFrame.nLen)
        {
            envir() << "LiveH264VideoSource:: frame len == 0\r\n";
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
        //MIXER_ERR("time:%lld\n", (fPresentationTime.tv_sec * 1000L + fPresentationTime.tv_usec /1000L) - fLastFrameTimestamp);
        fLastFrameTimestamp = tFrame.nPts;
        fFrameSize = tFrame.nLen;
        if(fFrameSize > fMaxSize)
            {
                fNumTruncatedBytes = fFrameSize - fMaxSize;
                fFrameSize = fMaxSize;
            }
        memcpy(fTo,((CPacket *)tFrame.pPacketAddr)->GetBuffer(), fFrameSize);

        mLiveChannelFrame.ReleaseOneFrame(tFrame);
        afterGetting(this);
        return;
      }while(0);


    m_pToken = envir().taskScheduler().scheduleDelayedTask(30000, incomingDataHandler, this);
    return;

}
