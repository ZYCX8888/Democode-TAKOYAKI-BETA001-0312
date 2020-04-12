/*
* LivePCMAAudioServerMediaSubsession.cpp- Sigmastar
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
#include "LivePCMAAudioServerMediaSubsession.hh"
#include "WAVStreamSource.h"
#include "SimpleRTPSink.hh"
#include "mid_AudioEncoder.h"

LivePCMAAudioServerMediaSubsession* LivePCMAAudioServerMediaSubsession
::createNew(UsageEnvironment& env, const int streamId, MI_AudioEncoder* pAudioEncoder) {
    return new LivePCMAAudioServerMediaSubsession(env, streamId, pAudioEncoder);
}

LivePCMAAudioServerMediaSubsession
::LivePCMAAudioServerMediaSubsession(UsageEnvironment& env, const int streamId, MI_AudioEncoder* pAudioEncoder)
    : LiveStreamServerMediaSubsession(env, streamId, (void*) pAudioEncoder, TRUE),fAudioFormat(0) {
}

LivePCMAAudioServerMediaSubsession::~LivePCMAAudioServerMediaSubsession() {
}

FramedSource* LivePCMAAudioServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
    estBitrate = MI_AudioEncoder::fSampleFrequency * ((MI_AudioEncoder::fBitsPerSample + 7) / 8) * MI_AudioEncoder::fNumChannels; // kbps, estimate

    FramedSource* resultSource = NULL;

    do{
        WAVStreamSource* wavSource = WAVStreamSource::createNew(envir(),  (MI_AudioEncoder *)fStreamEncoder);
        if(NULL == wavSource)
            break;

        fBitsPerSample = wavSource->bitsPerSample();
        fAudioFormat = wavSource->getAudioFormat();
        if (fAudioFormat == WA_PCM)
        {
            if (fBitsPerSample == 16)
            {
                {
                  // Add a filter that converts from little-endian to network (big-endian) order:
                  //resultSource = EndianSwap16::createNew(envir(), wavSource);
                }
              }
            else if (fBitsPerSample == 20 || fBitsPerSample == 24)
            {
                //resultSource = EndianSwap24::createNew(envir(), wavSource);
              }
        }
        resultSource = wavSource;
        return resultSource;
    }while(0);
    Medium::close(resultSource);
     return NULL;
}

RTPSink* LivePCMAAudioServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic,
        FramedSource* /*inputSource*/) {

    unsigned int fSamplingFrequency = MI_AudioEncoder::fSampleFrequency;
    unsigned int fNumChannels  = MI_AudioEncoder::fNumChannels;

    do {
        char const* mimeType;
        unsigned char payloadFormatCode = rtpPayloadTypeIfDynamic; // by default, unless a static RTP payload type can be used
        if (fAudioFormat == WA_PCM)
        {
            if (fBitsPerSample == 16)
            {
                {
                    mimeType = "L16";
                    if (fSamplingFrequency == 44100 && fNumChannels == 2)
                    {
                        payloadFormatCode = 10; // a static RTP payload type
                    }
                    else if (fSamplingFrequency == 44100 && fNumChannels == 1)
                    {
                        payloadFormatCode = 11; // a static RTP payload type
                    }
                }
            }
            else if (fBitsPerSample == 20)
            {
                mimeType = "L20";
            }
            else if (fBitsPerSample == 24)
            {
                mimeType = "L24";
            }
            else
            { // fBitsPerSample == 8 (we assume that fBitsPerSample == 4 is only for WA_IMA_ADPCM)
                mimeType = "L8";
            }
        }
        else if (fAudioFormat == WA_PCMU)
        {
            mimeType = "PCMU";
            if (fSamplingFrequency == 8000 && fNumChannels == 1)
            {
                payloadFormatCode = 0; // a static RTP payload type
            }
        }
        else if (fAudioFormat == WA_PCMA)
        {
            mimeType = "PCMA";
            if (fSamplingFrequency == 8000 && fNumChannels == 1)
            {
                payloadFormatCode = 8; // a static RTP payload type
            }
        }
        else if (fAudioFormat == WA_IMA_ADPCM)
        {
            mimeType = "DVI4";
            // Use a static payload type, if one is defined:
            if (fNumChannels == 1)
            {
                if (fSamplingFrequency == 8000)
                {
                    payloadFormatCode = 5; // a static RTP payload type
                }
                else if (fSamplingFrequency == 16000)
                {
                    payloadFormatCode = 6; // a static RTP payload type
                }
                else if (fSamplingFrequency == 11025)
                {
                    payloadFormatCode = 16; // a static RTP payload type
                }
                else if (fSamplingFrequency == 22050)
                {
                    payloadFormatCode = 17; // a static RTP payload type
                }
            }
        }
        else
        { //unknown format
            break;
        }

        printf("payload(%d), format(%d), payloadcode(%d)\n", rtpPayloadTypeIfDynamic, fAudioFormat, payloadFormatCode);
        return SimpleRTPSink::createNew(envir(), rtpGroupsock, payloadFormatCode,
                MI_AudioEncoder::fSampleFrequency, "audio",
                mimeType, MI_AudioEncoder::fNumChannels);
    }while(0);

    return NULL;

}
