/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2016 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a H264 video file.
// Implementation

#include "LiveH264VideoServerMediaSubsession.hh"
#include "H264VideoRTPSink.hh"
#include "LiveH264VideoSource.hh"
#include "H264VideoStreamDiscreteFramer.hh"
#include "live555.h"

LiveH264VideoServerMediaSubsession*
LiveH264VideoServerMediaSubsession::createNew(UsageEnvironment& env,
        const int streamId,
        MI_VideoEncoder* pVideoEncoder, Boolean needSynchronizedAudio)
{
    return new LiveH264VideoServerMediaSubsession(env, streamId, pVideoEncoder, needSynchronizedAudio);
}

LiveH264VideoServerMediaSubsession::LiveH264VideoServerMediaSubsession(UsageEnvironment& env,
        const int streamId, MI_VideoEncoder* pVideoEncoder, Boolean needSynchronizedAudio)
    : LiveStreamServerMediaSubsession(env, streamId, (void*) pVideoEncoder, FALSE),
      fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL), fNeedSynchronizedAudio(needSynchronizedAudio),
      fGettingSDP(false)
{
}

LiveH264VideoServerMediaSubsession::~LiveH264VideoServerMediaSubsession()
{
    delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData)
{
    LiveH264VideoServerMediaSubsession* subsess = (LiveH264VideoServerMediaSubsession*)clientData;
    subsess->afterPlayingDummy1();
}

void LiveH264VideoServerMediaSubsession::afterPlayingDummy1()
{
    // Unschedule any pending 'checking' task:
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    // Signal the event loop that we're done:
    setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData)
{
    LiveH264VideoServerMediaSubsession* subsess = (LiveH264VideoServerMediaSubsession*)clientData;
    subsess->checkForAuxSDPLine1();
}

void LiveH264VideoServerMediaSubsession::checkForAuxSDPLine1()
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

char const* LiveH264VideoServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource)
{
    //if(true == fGettingSDP)
   // {
   //     return NULL;
   // }

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
        // Note: For H264 video files, the 'config' information ("profile-level-id" and "sprop-parameter-sets") isn't known
        // until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
        // and we need to start reading data from our file until this changes.
        //envir() << "LiveH264VideoServerMediaSubsession::getAuxSDPLine\r\n";
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

FramedSource* LiveH264VideoServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate)
{
    MI_VideoEncoder* videoEncoder = (MI_VideoEncoder*) fStreamEncoder;

    estBitrate = videoEncoder->getBitrate(); // kbps, estimate

    // Create the video source:
    LiveH264VideoSource* liveVideoSource = NULL;

    if (fAuxSDPLine == NULL)
    {
        liveVideoSource = LiveH264VideoSource::createNew(envir(), fOurStreamId, videoEncoder);
    }
    else
    {
        liveVideoSource = (fNeedSynchronizedAudio)? LiveH264VideoSource::createNew(envir(), fOurStreamId, videoEncoder, True) : LiveH264VideoSource::createNew(envir(), fOurStreamId, videoEncoder);
        //LiveH264VideoSource::exportStreamToFile((void*) liveVideoSource);
    }

    if(liveVideoSource == NULL)
    {
        return NULL;
    }

    // Create a framer for the Video Elementary Stream:
    return H264VideoStreamDiscreteFramer::createNew(envir(), liveVideoSource);
}

RTPSink* LiveH264VideoServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
                   unsigned char rtpPayloadTypeIfDynamic,
                   FramedSource* /*inputSource*/)
{
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}

//void LiveH264VideoServerMediaSubsession::deleteStream(unsigned clientSessionId, void*& streamToken)
//{
//    setDoneFlag();
//    LiveStreamServerMediaSubsession::deleteStream(clientSessionId, streamToken);
//}
