#ifndef _LIVE_H265_VIDEO_SERVER_MEDIA_SUBSESSION_HH
#define _LIVE_H265_VIDEO_SERVER_MEDIA_SUBSESSION_HH

#ifndef _LIVE_STREAM_SERVER_MEDIA_SUBSESSION_HH
#include "LiveStreamServerMediaSubsession.hh"
#endif

class LiveH265VideoServerMediaSubsession: public LiveStreamServerMediaSubsession
{
public:
    static LiveH265VideoServerMediaSubsession*
    createNew(UsageEnvironment& env, const int streamId, MI_VideoEncoder *videoEncoder, Boolean needSynchronizedAudio);

    // Used to implement "getAuxSDPLine()":
    void checkForAuxSDPLine1();
    void afterPlayingDummy1();

protected:
    LiveH265VideoServerMediaSubsession(UsageEnvironment& env,
                                       const int streamId, MI_VideoEncoder *videoEncoder, Boolean needSynchronizedAudio);
    // called only by createNew();
    virtual ~LiveH265VideoServerMediaSubsession();

    void setDoneFlag()
    {
        fDoneFlag = ~0;
    }

protected: // redefined virtual functions
    virtual char const* getAuxSDPLine(RTPSink* rtpSink,
                                      FramedSource* inputSource);
    virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
            unsigned& estBitrate);
    virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                      unsigned char rtpPayloadTypeIfDynamic,
                                      FramedSource* inputSource);
    //virtual void deleteStream(unsigned clientSessionId, void*& streamToken);

private:
    char* fAuxSDPLine;
    char fDoneFlag; // used when setting up "fAuxSDPLine"
    RTPSink* fDummyRTPSink; // ditto
    Boolean fNeedSynchronizedAudio;
    Boolean fGettingSDP;
};

#endif
