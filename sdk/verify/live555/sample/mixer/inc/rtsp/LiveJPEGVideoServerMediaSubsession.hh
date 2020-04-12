#ifndef _JPEG_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH
#define _JPEG_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH

#ifndef _LIVE_STREAM_SERVER_MEDIA_SUBSESSION_HH
#include "LiveStreamServerMediaSubsession.hh"
#endif

class LiveJPEGVideoServerMediaSubsession: public LiveStreamServerMediaSubsession
{
public:
    static LiveJPEGVideoServerMediaSubsession*
    createNew(UsageEnvironment& env, const int streamId, MI_VideoEncoder *videoEncoder);

    // Used to implement "getAuxSDPLine()":
    void checkForAuxSDPLine1();
    void afterPlayingDummy1();

protected:
    LiveJPEGVideoServerMediaSubsession(UsageEnvironment& env,
                                       const int streamId, MI_VideoEncoder *videoEncoder);
    // called only by createNew();
    virtual ~LiveJPEGVideoServerMediaSubsession();

    void setDoneFlag()
    {
        fDoneFlag = ~0;
    }

protected: // redefined virtual functions
    virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
            unsigned& estBitrate);
    virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                      unsigned char rtpPayloadTypeIfDynamic,
                                      FramedSource* inputSource);
    virtual char const* getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource);
    //virtual void deleteStream(unsigned clientSessionId, void*& streamToken);
    char* fAuxSDPLine;
    char fDoneFlag; // used when setting up "fAuxSDPLine"
    RTPSink* fDummyRTPSink; // ditto
    Boolean fGettingSDP;
};

#endif
