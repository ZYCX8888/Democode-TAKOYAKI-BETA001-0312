#ifndef __MS_H264_VIDEO_SOURCE_HH__
#define __MS_H264_VIDEO_SOURCE_HH__

#ifndef _MEDIA_SOURCE_HH
#include "MediaSource.hh"
#endif

#include "FramedSource.hh"
#include "mid_VideoEncoder.h"

class LiveH264VideoSource : public FramedSource
{
public:
    static LiveH264VideoSource* createNew(UsageEnvironment& env, const int streamId, MI_VideoEncoder *videoEncoder);
    static LiveH264VideoSource* createNew(UsageEnvironment& env, const int streamId, MI_VideoEncoder *videoEncoder, Boolean needSynchronizedAudio);
    static void exportStreamToFile(void* clientData);

protected:
    LiveH264VideoSource(UsageEnvironment& env, const int streamId, MI_VideoEncoder *videoEncoder, Boolean needSynchronizedAudio);
    virtual ~LiveH264VideoSource();

protected:
 /*   int fNotifyFd;
    char *fIpcMem;
    char *fFrameEnd;
    char *fNextFrameToStream;

    long fFrameBytesToStream;*/
   // long fFrameNum;

    void setDoneFlag()
    {
        //fDoneFlag = ~0;
    }

private:
    virtual void doGetNextFrame();
    static void incomingDataHandler(void* clientData);
    void incomingDataHandler1();
    int findNalUnit(const char* frameToStream, const int frameBytesToStream, char **naluToStream, unsigned int *naluBytesToStream);

    int receiveData();
    static void checkForSynchronizedAudio(void *clientData);
    void checkForSynchronizedAudio1();
    void waitForSynchronizedAudio();
private:
	
  	int fHaveSeenIdrFrame;
    	char fDoneFlag;
	Boolean fNeedSynchronizedAudio;
	MI_VideoEncoder *fVideoEncoder;
	MI_U64 fLastFrameTimestamp;

	void *m_pToken;  
	FrameInf_t mLastFrame;
	char *fNextFrameToStream;
	unsigned int mLeftData;
	MI_U8  bNeedIFrame; 
	
	VideoChannelFrame  mLiveChannelFrame;
};

enum AVCNalUnitType {
    AVC_NAL_UNIT_NONIDR = 1,
    AVC_NAL_UNIT_IDR    = 5,
    AVC_NAL_UNIT_SEI,   //6
    AVC_NAL_UNIT_SPS,   //7
    AVC_NAL_UNIT_PPS,   //8
    AVC_NAL_UNIT_AUD,   //9
};

#endif
