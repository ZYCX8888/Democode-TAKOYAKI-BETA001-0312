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

#include "LiveVideoServerMediaSubsession.hh"
#include "H264VideoRTPSink.hh"
#include "LiveH264VideoSource.hh"
#include "H264VideoStreamDiscreteFramer.hh"

#include "LiveH265VideoServerMediaSubsession.hh"
#include "H265VideoRTPSink.hh"
#include "LiveH265VideoSource.hh"
#include "H265VideoStreamDiscreteFramer.hh"

#include "LiveJPEGVideoServerMediaSubsession.hh"
#include "LiveStreamSource.hh"
#include "LiveJPEGVideoStreamSource.hh"
#include <JPEGVideoRTPSink.hh>

#include "live555.h"

LiveVideoServerMediaSubsession*
LiveVideoServerMediaSubsession::createNew(UsageEnvironment& env,
        const int streamId,
        MI_VideoEncoder* pVideoEncoder, Boolean needSynchronizedAudio)
{
    return new LiveVideoServerMediaSubsession(env, streamId, pVideoEncoder, needSynchronizedAudio);
}

LiveVideoServerMediaSubsession::LiveVideoServerMediaSubsession(UsageEnvironment& env,
        const int streamId, MI_VideoEncoder* pVideoEncoder, Boolean needSynchronizedAudio)
    : LiveStreamServerMediaSubsession(env, streamId, (void*) pVideoEncoder, FALSE),
      fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL), fNeedSynchronizedAudio(needSynchronizedAudio),
      fGettingSDP(false)
{
}

LiveVideoServerMediaSubsession::~LiveVideoServerMediaSubsession()
{
    delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData)
{
    LiveVideoServerMediaSubsession* subsess = (LiveVideoServerMediaSubsession*)clientData;
    subsess->afterPlayingDummy1();
}

void LiveVideoServerMediaSubsession::afterPlayingDummy1()
{
    // Unschedule any pending 'checking' task:
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    // Signal the event loop that we're done:
    setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData)
{
    LiveVideoServerMediaSubsession* subsess = (LiveVideoServerMediaSubsession*)clientData;
    subsess->checkForAuxSDPLine1();
}

void LiveVideoServerMediaSubsession::checkForAuxSDPLine1()
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

char const* LiveVideoServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource)
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
        //envir() << "LiveVideoServerMediaSubsession::getAuxSDPLine\r\n";
        fDummyRTPSink = rtpSink;

        // Start reading the file:
        fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

        // Check whether the sink's 'auxSDPLine()' is ready:
        checkForAuxSDPLine(this);
    }


    envir().taskScheduler().doEventLoop(&fDoneFlag);
    fDoneFlag = 0;
    //fDummyRTPSink->stopPlaying();

    fGettingSDP = false;

    return fAuxSDPLine;
}

FramedSource* LiveVideoServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate)
{
    MI_VideoEncoder* videoEncoder = (MI_VideoEncoder*) fStreamEncoder;

    estBitrate = videoEncoder->getBitrate(); // kbps, estimate

    int encType = videoEncoder->getCodec();

    if (VE_AVC == encType)
    {
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
    else if (VE_H265 == encType)
    {
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
    else if (VE_MJPEG == encType || VE_JPG == encType)
    {
        LiveStreamSource* liveVideoSource = LiveStreamSource::createNew(envir(), fOurStreamId,videoEncoder);

        // Create a JPEG stream source (encapsulating the raw JPEG video source):
        if(NULL == liveVideoSource)
        {
            return NULL;
        }

        return LiveJPEGVideoStreamSource::createNew(envir(), liveVideoSource);
    }

    return NULL;
}

RTPSink* LiveVideoServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
                   unsigned char rtpPayloadTypeIfDynamic,
                   FramedSource* /*inputSource*/)
{
    MI_VideoEncoder* videoEncoder = (MI_VideoEncoder*) fStreamEncoder;
    int encType = videoEncoder->getCodec();

    if (VE_AVC == encType)
    {
        return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
    }
    else if (VE_H265 == encType)
    {
        return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
    }
    else if (VE_MJPEG == encType || VE_JPG == encType)
    {
        return JPEGVideoRTPSink::createNew(envir(), rtpGroupsock);
    }

    return NULL;
}

//void LiveVideoServerMediaSubsession::deleteStream(unsigned clientSessionId, void*& streamToken)
//{
//    setDoneFlag();
//    LiveStreamServerMediaSubsession::deleteStream(clientSessionId, streamToken);
//}
