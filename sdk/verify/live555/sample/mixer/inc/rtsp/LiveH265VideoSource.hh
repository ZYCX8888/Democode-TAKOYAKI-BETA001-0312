#ifndef __MS_H265_VIDEO_SOURCE_HH__
#define __MS_H265_VIDEO_SOURCE_HH__

#ifndef _MEDIA_SOURCE_HH
#include "MediaSource.hh"
#endif

#include "FramedSource.hh"
#include "mid_VideoEncoder.h"

class LiveH265VideoSource : public FramedSource
{
public:
    static LiveH265VideoSource* createNew(UsageEnvironment& env, const int streamId, MI_VideoEncoder *videoEncoder);
    static LiveH265VideoSource* createNew(UsageEnvironment& env, const int streamId, MI_VideoEncoder *videoEncoder, Boolean needSynchronizedAudio);

protected:
    LiveH265VideoSource(UsageEnvironment& env, const int streamId, MI_VideoEncoder *videoEncoder, Boolean needSynchronizedAudio);
    virtual ~LiveH265VideoSource();

protected:

    long fFrameNum;

    void setDoneFlag()
    {
       // fDoneFlag = ~0;
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
   // int fStreamId;
    int fHaveSeenIdrFrame;
    char fDoneFlag;
    Boolean fNeedSynchronizedAudio;
    MI_VideoEncoder *fVideoEncoder;
    unsigned long long fLastFrameTimestamp;
	
	void *m_pToken;  
	FrameInf_t mLastFrame;
	char *fNextFrameToStream;
	unsigned int mLeftData;
	MI_U8  bNeedIFrame; 
	VideoChannelFrame  mLiveChannelFrame;
};

enum HEVCNalUnitType {
    HEVC_NAL_UNIT_CODED_SLICE_TRAIL_N = 0,   // 0
    HEVC_NAL_UNIT_CODED_SLICE_TRAIL_R,       // 1

    HEVC_NAL_UNIT_CODED_SLICE_TSA_N,         // 2
    HEVC_NAL_UNIT_CODED_SLICE_TLA,           // 3    Current name in the spec: TSA_R

    HEVC_NAL_UNIT_CODED_SLICE_STSA_N,        // 4
    HEVC_NAL_UNIT_CODED_SLICE_STSA_R,        // 5

    HEVC_NAL_UNIT_CODED_SLICE_RADL_N,        // 6
    HEVC_NAL_UNIT_CODED_SLICE_DLP,           // 7    Current name in the spec: RADL_R

    HEVC_NAL_UNIT_CODED_SLICE_RASL_N,        // 8
    HEVC_NAL_UNIT_CODED_SLICE_TFD,           // 9    Current name in the spec: RASL_R

    HEVC_NAL_UNIT_CODED_SLICE_BLA = 16,      // 16   Current name in the spec: BLA_W_LP
    HEVC_NAL_UNIT_CODED_SLICE_BLANT,         // 17   Current name in the spec: BLA_W_DLP
    HEVC_NAL_UNIT_CODED_SLICE_BLA_N_LP,      // 18
    HEVC_NAL_UNIT_CODED_SLICE_IDR,           // 19   Current name in the spec: IDR_W_DLP
    HEVC_NAL_UNIT_CODED_SLICE_IDR_N_LP,      // 20
    HEVC_NAL_UNIT_CODED_SLICE_CRA,           // 21
    HEVC_NAL_UNIT_RESERVED_22,
    HEVC_NAL_UNIT_RESERVED_23,

    HEVC_NAL_UNIT_VPS = 32,                  // 32
    HEVC_NAL_UNIT_SPS,                       // 33
    HEVC_NAL_UNIT_PPS,                       // 34
    HEVC_NAL_UNIT_ACCESS_UNIT_DELIMITER,     // 35
    HEVC_NAL_UNIT_EOS,                       // 36
    HEVC_NAL_UNIT_EOB,                       // 37
    HEVC_NAL_UNIT_FILLER_DATA,               // 38
    HEVC_NAL_UNIT_SEI,                       // 39   Prefix SEI
    HEVC_NAL_UNIT_SEI_SUFFIX,                // 40   Suffix SEI
};

#endif
