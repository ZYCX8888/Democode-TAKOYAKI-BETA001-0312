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
// NOTE: Samples are returned in little-endian order (the same order in which
// they were stored in the file).
// C++ header

#ifndef _WAV_AUDIO_FILE_SOURCE1_HH
#define _WAV_AUDIO_FILE_SOURCE1_HH

#ifndef _AUDIO_INPUT_DEVICE1_HH
#include "AudioInputDevice.hh"
#endif

#include "WAVAudioFileSource.hh"
#include "mid_AudioEncoder.h"
#include "AudioChannelFrame.h"
/*typedef enum {
  WA_PCM = 0x01,
  WA_PCMA = 0x06,
  WA_PCMU = 0x07,
  WA_IMA_ADPCM = 0x11,
  WA_UNKNOWN
} WAV_AUDIO_FORMAT;*/


class WAVStreamSource: public FramedSource
{
public:

    static WAVStreamSource* createNew(UsageEnvironment& env, MI_AudioEncoder *audioEncoder);

    //unsigned numPCMBytes() const;
    //void setScaleFactor(int scale);
    //void seekToPCMByte(unsigned byteNumber);
    //void limitNumBytesToStream(unsigned numBytesToStream);
    // if "numBytesToStream" is >0, then we limit the stream to that number of bytes, before treating it as EOF

    unsigned char getAudioFormat();

    unsigned char bitsPerSample() const
    {
        return fBitsPerSample;
    }
    unsigned char numChannels() const
    {
        return fNumChannels;
    }
    unsigned samplingFrequency() const
    {
        return fSamplingFrequency;
    }

protected:
    WAVStreamSource(UsageEnvironment& env, MI_AudioEncoder *audioEncoder);
    // called only by createNew()

    virtual ~WAVStreamSource();

    static void fileReadableHandler(WAVStreamSource* source, int mask);
    void doReadFromFile();
    static void getNextFrame(void *ptr);

private:
    // redefined virtual functions:
    virtual void doGetNextFrame();
    virtual void doStopGettingFrames();
    virtual Boolean setInputPort(int portIndex);
    virtual double getAverageLevel() const;



protected:
    unsigned fPreferredFrameSize;

private:
    double fPlayTimePerSample; // useconds
    Boolean fFidIsSeekable;
    unsigned fLastPlayTime; // useconds
    Boolean fHaveStartedReading;
    Boolean fLimitNumBytesToStream;
    unsigned fNumBytesToStream; // used iff "fLimitNumBytesToStream" is True
    unsigned char fAudioFormat;

    unsigned char fBitsPerSample, fNumChannels;
    unsigned fSamplingFrequency;
    unsigned fGranularityInMS;

    unsigned char lastPcm[1024];
    int lastPcmLen;

    MI_AudioEncoder *m_audioEncoder;
    AudioChannelFrame mLiveChannelFrame;
};

#endif
