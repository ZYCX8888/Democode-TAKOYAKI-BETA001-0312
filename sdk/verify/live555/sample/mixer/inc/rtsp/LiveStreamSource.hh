#ifndef __Live_STREAM_SOURCE_HH__
#define __Live_STREAM_SOURCE_HH__

#ifndef _MEDIA_SOURCE_HH
#include "MediaSource.hh"
#endif

#include "FramedSource.hh"
#include "mid_VideoEncoder.h"
#include "ms_notify.h"

class LiveStreamSource : public FramedSource
{
public:
    static LiveStreamSource* createNew(UsageEnvironment& env, const int streamId, MI_VideoEncoder* videoEncoder);
    static LiveStreamSource* createNew(UsageEnvironment& env, const int streamId, Boolean isAudioSource);
    int receiveData();
private:
   // LiveStreamSource(UsageEnvironment& env, const int streamId, Boolean isAudioSource);
    LiveStreamSource(UsageEnvironment& env, const int streamId, MI_VideoEncoder* videoEncoder);
    virtual ~LiveStreamSource();
    virtual void doGetNextFrame();
    static void incomingDataHandler(void* clientData);
    void incomingDataHandler1();

private:
	#if 0
    int fNotifyFd;
	#endif
	void *m_pToken;  
    int fStreamId;
    mediaType_t fMediaType;
    unsigned long long fLastFrameTimestamp;
    MI_VideoEncoder *fVideoEncoder;

	VideoChannelFrame  mLiveChannelFrame;
};

#endif
