/*
* LiveJPEGVideoServerMediaSubsession.cpp- Sigmastar
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
#include "LiveJPEGVideoServerMediaSubsession.hh"
#include "LiveStreamSource.hh"
#include "LiveJPEGVideoStreamSource.hh"
#include <JPEGVideoRTPSink.hh>
#include "live555.h"

LiveJPEGVideoServerMediaSubsession*
LiveJPEGVideoServerMediaSubsession::createNew(UsageEnvironment& env,
        const int streamId,
        MI_VideoEncoder* videoEncoder)
{
    return new LiveJPEGVideoServerMediaSubsession(env, streamId, videoEncoder);
}

LiveJPEGVideoServerMediaSubsession
::LiveJPEGVideoServerMediaSubsession(UsageEnvironment& env,
                                     const int streamId, MI_VideoEncoder* videoEncoder)
    : LiveStreamServerMediaSubsession(env, streamId, (void*) videoEncoder, TRUE),
    fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL),fGettingSDP(false)
{
}

LiveJPEGVideoServerMediaSubsession::~LiveJPEGVideoServerMediaSubsession()
{
}

FramedSource* LiveJPEGVideoServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate)
{
    MI_VideoEncoder* videoEncoder = (MI_VideoEncoder*) fStreamEncoder;

    estBitrate = videoEncoder->getBitrate();

    LiveStreamSource* liveVideoSource = LiveStreamSource::createNew(envir(), fOurStreamId,videoEncoder);

    // Create a JPEG stream source (encapsulating the raw JPEG video source):
    if(NULL == liveVideoSource)
    {
        return NULL;
    }

    return LiveJPEGVideoStreamSource::createNew(envir(), liveVideoSource);
}

RTPSink* LiveJPEGVideoServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
                   unsigned char /*rtpPayloadTypeIfDynamic*/,
                   FramedSource* /*inputSource*/)
{
    //setVideoRTPSinkBufferSize();
    return JPEGVideoRTPSink::createNew(envir(), rtpGroupsock);
}

static void afterPlayingDummy(void* clientData)
{
    LiveJPEGVideoServerMediaSubsession* subsess = (LiveJPEGVideoServerMediaSubsession*)clientData;
    subsess->afterPlayingDummy1();
}

void LiveJPEGVideoServerMediaSubsession::afterPlayingDummy1()
{
    // Unschedule any pending 'checking' task:
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    // Signal the event loop that we're done:
    setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData)
{
    LiveJPEGVideoServerMediaSubsession* subsess = (LiveJPEGVideoServerMediaSubsession*)clientData;
    subsess->checkForAuxSDPLine1();
}

void LiveJPEGVideoServerMediaSubsession::checkForAuxSDPLine1()
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


char const* LiveJPEGVideoServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource)
{
    //if(true == fGettingSDP)
    //{
    //    return NULL;
    //}

    fGettingSDP = true;

    //if(fAuxSDPLine != NULL) return fAuxSDPLine;  // it's already been set up (for a previous client)
    if(fAuxSDPLine != NULL)
    {
        delete[] fAuxSDPLine;
        fAuxSDPLine = NULL;
    }

    if(0 == live555::isRunning())
    {
        return NULL;
    }

    if(fDummyRTPSink == NULL)    // we're not already setting it up for another, concurrent stream
    {
        // Note: For JPEG video files, the 'config' information ("x-dimensions=width,height") isn't known
        // until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
        // and we need to start reading data from our file until this changes.
        //envir() << "LiveJPEGVideoServerMediaSubsession::getAuxSDPLine\r\n";
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

//void LiveJPEGVideoServerMediaSubsession::deleteStream(unsigned clientSessionId, void*& streamToken)
//{
//    setDoneFlag();
//    LiveStreamServerMediaSubsession::deleteStream(clientSessionId, streamToken);
//}
