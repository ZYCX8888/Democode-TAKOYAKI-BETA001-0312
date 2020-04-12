/*
* LiveH265VideoServerMediaSubsession.cpp- Sigmastar
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
#include "LiveH265VideoServerMediaSubsession.hh"
#include "H265VideoRTPSink.hh"
#include "LiveH265VideoSource.hh"
#include "H265VideoStreamDiscreteFramer.hh"
#include "live555.h"

LiveH265VideoServerMediaSubsession*
LiveH265VideoServerMediaSubsession::createNew(UsageEnvironment& env,
        const int streamId, MI_VideoEncoder* pVideoEncoder, Boolean needSynchronizedAudio)
{
    return new LiveH265VideoServerMediaSubsession(env, streamId, pVideoEncoder, needSynchronizedAudio);
}

LiveH265VideoServerMediaSubsession::LiveH265VideoServerMediaSubsession(UsageEnvironment& env,
        const int streamId, MI_VideoEncoder* pVideoEncoder, Boolean needSynchronizedAudio)
    : LiveStreamServerMediaSubsession(env, streamId, pVideoEncoder, FALSE),
      fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL), fNeedSynchronizedAudio(needSynchronizedAudio),
      fGettingSDP(false)
{
}

LiveH265VideoServerMediaSubsession::~LiveH265VideoServerMediaSubsession()
{
    delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData)
{
    LiveH265VideoServerMediaSubsession* subsess = (LiveH265VideoServerMediaSubsession*)clientData;
    subsess->afterPlayingDummy1();
}

void LiveH265VideoServerMediaSubsession::afterPlayingDummy1()
{
    // Unschedule any pending 'checking' task:
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    // Signal the event loop that we're done:
    setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData)
{
    LiveH265VideoServerMediaSubsession* subsess = (LiveH265VideoServerMediaSubsession*)clientData;
    subsess->checkForAuxSDPLine1();
}

void LiveH265VideoServerMediaSubsession::checkForAuxSDPLine1()
{
    char const* dasl;

    if(fAuxSDPLine != NULL || 0 == live555::isRunning())
    {
        // Signal the event loop that we're done:
        setDoneFlag();
    }
    else if(fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL)
    {
        fAuxSDPLine = strDup(dasl);
        fDummyRTPSink = NULL;

        // Signal the event loop that we're done:
        setDoneFlag();
    }
    else if(!fDoneFlag)
    {
        // try again after a brief delay:
        int uSecsToDelay = 100000; // 100 ms
        nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
                     (TaskFunc*)checkForAuxSDPLine, this);
    }
}

char const* LiveH265VideoServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource)
{
    //if(true == fGettingSDP)
    //{
    //    return NULL;
    //}

    fGettingSDP = true;
    //if(fAuxSDPLine != NULL) return fAuxSDPLine;  // it's already been set up (for a previous client)
#if 1
    if(fAuxSDPLine != NULL)
    {
        delete[] fAuxSDPLine;
        fAuxSDPLine = NULL;
    }
#endif

    if(0 == live555::isRunning())
    {
        return NULL;
    }

    if(fDummyRTPSink == NULL)    // we're not already setting it up for another, concurrent stream
    {
        // Note: For H265 video files, the 'config' information ("profile-level-id" and "sprop-parameter-sets") isn't known
        // until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
        // and we need to start reading data from our file until this changes.
        //envir() << "LiveH265VideoServerMediaSubsession::getAuxSDPLine\r\n";
        fDummyRTPSink = rtpSink;

        // Start reading the file:
        fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

        // Check whether the sink's 'auxSDPLine()' is ready:
        checkForAuxSDPLine(this);
    }

    envir().taskScheduler().doEventLoop(&fDoneFlag);


    fDoneFlag = 0;
    fGettingSDP = false;

    return fAuxSDPLine;
}

FramedSource* LiveH265VideoServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate)
{
    MI_VideoEncoder* videoEncoder = (MI_VideoEncoder*) fStreamEncoder;

    estBitrate = videoEncoder->getBitrate(); // kbps, estimate

    // Create the video source:
    LiveH265VideoSource* liveVideoSource = NULL;

    if (fAuxSDPLine == NULL)
    {
        liveVideoSource = LiveH265VideoSource::createNew(envir(), fOurStreamId, videoEncoder);
    }
    else
    {
        liveVideoSource = (fNeedSynchronizedAudio)? LiveH265VideoSource::createNew(envir(), fOurStreamId, videoEncoder, True) : LiveH265VideoSource::createNew(envir(), fOurStreamId, videoEncoder);
    }

    if(liveVideoSource == NULL)
    {
        return NULL;
    }

    // Create a framer for the Video Elementary Stream:
    return H265VideoStreamDiscreteFramer::createNew(envir(), liveVideoSource);
}

RTPSink* LiveH265VideoServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
                   unsigned char rtpPayloadTypeIfDynamic,
                   FramedSource* /*inputSource*/)
{
    return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}

//void LiveH265VideoServerMediaSubsession::deleteStream(unsigned clientSessionId, void*& streamToken)
//{
//    setDoneFlag();
//    LiveStreamServerMediaSubsession::deleteStream(clientSessionId, streamToken);
//}
