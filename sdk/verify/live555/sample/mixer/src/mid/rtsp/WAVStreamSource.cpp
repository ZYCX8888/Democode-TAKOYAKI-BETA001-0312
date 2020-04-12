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
// Copyright (c) 1996-2015 Live Networks, Inc.  All rights reserved.
// A WAV audio file source
// Implementation

#include "WAVStreamSource.h"
#include "InputFile.hh"
#include "GroupsockHelper.hh"
#include "mid_AudioEncoder.h"
#include "mid_sys.h"

extern int *gDebug_streamLog;
extern int g_bNotUseTimestamp;

WAVStreamSource*
WAVStreamSource::createNew(UsageEnvironment& env, MI_AudioEncoder *audioEncoder)
{
    do
    {
        WAVStreamSource* newSource = new WAVStreamSource(env, audioEncoder);

        if(newSource != NULL && newSource->bitsPerSample() == 0)
        {
            // The WAV file header was apparently invalid.
            Medium::close(newSource);
            break;
        }

        return newSource;
    }while(0);

    return NULL;
}

unsigned char WAVStreamSource::getAudioFormat()
{
    return fAudioFormat;
}


WAVStreamSource::WAVStreamSource(UsageEnvironment& env , MI_AudioEncoder *audioEncoder)
    : FramedSource(env), fFidIsSeekable(False), fLastPlayTime(0), fHaveStartedReading(False),
      fLimitNumBytesToStream(False), fNumBytesToStream(0), fAudioFormat(WA_UNKNOWN), mLiveChannelFrame(0)
{
    m_audioEncoder = audioEncoder;

    if(m_audioEncoder)
    {
        switch(m_audioEncoder->m_AiMediaType)
        {
            case  MT_G711A:
                fAudioFormat = WA_PCMA;
                break;
            case  MT_G711U:
                fAudioFormat = WA_PCMU;
                break;
            case  MT_PCM:
                fAudioFormat = WA_PCM;
                break;
            case  MT_G726:
                fAudioFormat = 0x45;
                break;
            default:
                fAudioFormat = WA_UNKNOWN;
                printf("can not support the audio format %d\n", m_audioEncoder->m_AiMediaType);
                break;
        }
    }

    fNumChannels = m_audioEncoder->fNumChannels;
    fSamplingFrequency = m_audioEncoder->fSampleFrequency;
    fBitsPerSample = m_audioEncoder->fBitsPerSample;
    fPlayTimePerSample = 1e6 / (double)fSamplingFrequency;

    unsigned maxSamplesPerFrame = (1400 * 8) / (fNumChannels * fBitsPerSample);
    unsigned desiredSamplesPerFrame = (unsigned)(0.02 * fSamplingFrequency);
    unsigned samplesPerFrame = MIN(desiredSamplesPerFrame, maxSamplesPerFrame);
    fPreferredFrameSize = 2 * (samplesPerFrame * fNumChannels * fBitsPerSample) / 8;

    lastPcmLen = 0;
    mLiveChannelFrame.Open();
    if(NULL != m_audioEncoder)
        m_audioEncoder->GetLiveChannelConsumer().AddConsumer(mLiveChannelFrame);    //register
}

WAVStreamSource::~WAVStreamSource()
{
    if(NULL != m_audioEncoder)
        m_audioEncoder->GetLiveChannelConsumer().DelConsumer(mLiveChannelFrame); //unregister

    mLiveChannelFrame.Close();
}

void WAVStreamSource::doGetNextFrame()
{
    if(fLimitNumBytesToStream && fNumBytesToStream == 0)
    {
        handleClosure();
        return;
    }

    fFrameSize = 0; // until it's set later

    doReadFromFile();

}

void WAVStreamSource::doStopGettingFrames()
{
    printf("doStopGettingFrames\n");
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
}

void WAVStreamSource::getNextFrame(void *ptr)
{
    ((WAVStreamSource *)ptr)->doReadFromFile();
}


void WAVStreamSource::fileReadableHandler(WAVStreamSource* source, int /*mask*/)
{
    if(!source->isCurrentlyAwaitingData())
    {
        source->doStopGettingFrames(); // we're not ready for the data yet
        return;
    }

    source->doReadFromFile();
}

void WAVStreamSource::doReadFromFile()
{
    FrameInf_t objFrame;

    fFrameSize = 0;

    // Try to read as many bytes as will fit in the buffer provided (or "fPreferredFrameSize" if less)
    if(fLimitNumBytesToStream && fNumBytesToStream < fMaxSize)
    {
        fMaxSize = fNumBytesToStream;
    }

    if(fPreferredFrameSize < fMaxSize)
    {
        fMaxSize = fPreferredFrameSize;
    }

    unsigned bytesPerSample = (fNumChannels * fBitsPerSample) / 8;

    if(bytesPerSample == 0) bytesPerSample = 1;  // because we can't read less than a byte at a time


    //MIXER_DBG("MaxSize(%d), bytesToRead(%d), bytesPerSample(%d)\n", fMaxSize, bytesToRead, bytesPerSample);
    {
        memset(&objFrame, 0x0, sizeof(objFrame));

        if(m_audioEncoder)
        {
            if(0x0 == mLiveChannelFrame.GetOneFrame(objFrame))
            {
                nextTask() = envir().taskScheduler().scheduleDelayedTask(15000, (TaskFunc*)getNextFrame, this);
                return;
            }
            else
            {
                CPacket *tmp = (CPacket *)objFrame.pPacketAddr;
                memcpy(fTo, tmp->GetBuffer(), objFrame.nLen);
                fFrameSize = objFrame.nLen;

                mLiveChannelFrame.ReleaseOneFrame(objFrame);

                if(16 == fBitsPerSample)    //16bit
                {
                    if(m_audioEncoder->m_AiMediaType == MT_PCM)
                    {
                        unsigned numValues = fFrameSize/2;
                        unsigned short* value = (unsigned short*)fTo;
                        for (unsigned i = 0; i < numValues; ++i)
                        {
                            unsigned short const orig = value[i];
                            value[i] = ((orig&0xFF)<<8) | ((orig&0xFF00)>>8);
                        }
                    }
                }
                else if(24 == fBitsPerSample)
                {
                    if(m_audioEncoder->m_AiMediaType == MT_PCM)
                    {
                        unsigned const numValues = fFrameSize/3;
                        unsigned char * p = fTo;
                        for (unsigned i = 0; i < numValues; ++i)
                        {
                            unsigned char  tmp = p[0];
                            p[0] = p[2];
                            p[2] = tmp;
                            p += 3;
                        }
                    }
                }

                // Remember the play time of this data:
                fDurationInMicroseconds = fLastPlayTime = (unsigned)((fPlayTimePerSample * fFrameSize) / bytesPerSample);
                //MIXER_DBG("WAVStreamSource::doReadFromFile:read %d\n", fFrameSize);
            }
        }
    }

    //if(g_bNotUseTimestamp)
    if(1)
    {
        gettimeofday(&fPresentationTime, NULL);
    }
    else
    {
       //fPresentationTime.tv_sec = timestame.tv_sec;
       // fPresentationTime.tv_usec = timestame.tv_usec;
    }

    afterGetting(this);
}

Boolean WAVStreamSource::setInputPort(int /*portIndex*/)
{
    return True;
}

double WAVStreamSource::getAverageLevel() const
{
    return 0.0;//##### fix this later
}
