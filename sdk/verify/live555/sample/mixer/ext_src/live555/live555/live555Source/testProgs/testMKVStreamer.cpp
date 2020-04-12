#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include "H264VideoFileServerMediaSubsession.hh"

int main(int argc, char** argv)
{
    UsageEnvironment* env;
    RTSPServer* rtspServer;
    ServerMediaSession* sms;

    // Begin by setting up our usage environment:
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

    // Create our RTSP server.  (Receivers will need to use RTSP to access the stream.)
    rtspServer = RTSPServer::createNew(*env, 554);

    if (rtspServer == NULL)
    {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
        exit(1);
    }

    sms = ServerMediaSession::createNew(*env, "testStream", "testMKVStreamer",
                                        "Session streamed by \"testMKVStreamer\"",
                                        True /*SSM*/);
    rtspServer->addServerMediaSession(sms);
    sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(*env, argv[1], False));
    OutPacketBuffer::maxSize = 2000000;
    env->taskScheduler().doEventLoop(); // does not return

    return 0; // only to prevent compiler warning
}
