/*
* live555.cpp- Sigmastar
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
#include <BasicUsageEnvironment.hh>
#include "liveMedia.hh"
#include <pthread.h>
#include "live555.h"

#include "LiveVideoServerMediaSubsession.hh"
#include "LiveH264VideoServerMediaSubsession.hh"
#include "LiveH265VideoServerMediaSubsession.hh"
#include "LiveJPEGVideoServerMediaSubsession.hh"
#include "LivePCMAAudioServerMediaSubsession.hh"

#define LIVE_STATE_INVALID     0
#define LIVE_STATE_PREPARE_RUN 1
#define LIVE_STATE_RUNNING     2
#define LIVE_STATE_STOPING     3


void* live555_Task(void * argv)
{
    // Add live stream
    live555 *liveServer = (live555*) argv;

    for(int i = 0 ; i < liveServer->m_smsCount; i++)
    {
        liveServer->m_rtspServer->addServerMediaSession(liveServer->m_sms[i]);
        char *url = liveServer->m_rtspServer->rtspURL(liveServer->m_sms[i]);

        printf("using url %s\n", url);
        delete[] url;
    }

    if(liveServer->m_liveState == LIVE_STATE_STOPING)
    {
        liveServer->m_liveState = LIVE_STATE_INVALID;
        printf("[live555] live555_Task not loop, exit\n");
        return NULL;
    }

    printf("[live555] live555_Task loop\n");

    // Run loop
    liveServer->m_watchVariable = 0;
    liveServer->m_liveState = LIVE_STATE_RUNNING;
    liveServer->m_env->taskScheduler().doEventLoop(&liveServer->m_watchVariable);

    printf("[live555] live555_Task exit\n");

    liveServer->m_liveState = LIVE_STATE_INVALID;

    return NULL;
}

int live555::fRtspServerPortNum = 554;
int live555::fIsRunning = 0;

live555* live555::createNew()
{
    live555 *live = NULL;

    UsageEnvironment *env = NULL;
    TaskScheduler *scheduler = NULL;
    RTSPServer *rtspServer = NULL;
    UserAuthenticationDatabase *authDB = NULL;

    do
    {
        live555::fRtspServerPortNum = 554;

        if(NULL == (scheduler = BasicTaskScheduler::createNew()))
        {
            break;
        }

        if(NULL == (env = BasicUsageEnvironment::createNew(*scheduler)))
        {
            break;
        }

#ifdef ACCESS_CONTROL
        // To implement client access control to the RTSP server, do the following:
        authDB = new UserAuthenticationDatabase;
        authDB->addUserRecord("username1", "password1");
        // Repeat the above with each <username>, <password> that you wish to allow
        // access to the server.
#endif
        while(NULL == rtspServer && live555::fRtspServerPortNum < 65535)
        {
            if(NULL != (rtspServer = RTSPServer::createNew(*env, fRtspServerPortNum, authDB)))
            {
                break;
            }
            live555::fRtspServerPortNum++;
        }
        if(NULL == rtspServer)
        {
            if(env)
            {
                env->reclaim();
            }

            if(scheduler)
            {
                delete scheduler;
            }
            break;
        }

        live = new live555();
        live->m_scheduler = scheduler;
        live->m_env = env;
        live->m_authDB = authDB;
        live->m_rtspServer = rtspServer;

        return live;
    }while(0);

    return NULL;
}


live555::live555(): m_smsCount(0)
{
    OutPacketBuffer::maxSize = 2000000;
    fIsRunning = FALSE;
    fRtspServerPortNum = 555;
    m_smsCount = 0;
    m_watchVariable = 0x00;
    m_liveState = 0x00;
    m_authDB = NULL;
    m_rtspServer = NULL;
    m_env = NULL;
    m_scheduler = NULL;
    for(int i = 0 ; i < MAX_SERRVER_MEDIA_SESSION ; i ++)
    {
        m_pAudioEncode[i] = NULL;
    }
}

live555::~live555()
{
    printf("[live555] ~live555\n");

    if(m_rtspServer)
    {
        for(int i = 0; i < m_smsCount; i++)
        {
            if(m_sms[i])
            {
                m_rtspServer->closeAllClientSessionsForServerMediaSession(m_sms[i]);
                m_rtspServer->removeServerMediaSession(m_sms[i]);
            }
        }

        Medium::close(m_rtspServer);
    }

    if(m_authDB)
    {
        delete m_authDB;
    }

    if(m_env)
    {
        m_env->reclaim();
    }

    if(m_scheduler)
    {
        delete m_scheduler;
    }

}

int live555::stopVideoSubMediaSession(int streamId, MI_VideoEncoder *videoEncoder)
{
    if(NULL == m_rtspServer)
    {
        printf("stopVideoSubMediaSession failed, live555 server not create!\n");
        return -1;
    }
    if(streamId < m_smsCount)
    {
        if(m_sms[streamId])
        {
            m_rtspServer->deleteServerMediaSession(m_sms[streamId]);
            m_sms[streamId]->deleteAllSubsessions();
        }
        printf("live555 stopVideoSubMediaSession ok!\n");
    }
    else
    {
        printf("live555 stopVideoSubMediaSession failed, restart num : %d, SubMediaSession count : %d\n", streamId, m_smsCount);
        return -1;
    }
    return 0;
}

int live555::RestartVideoSubMediaSession(int streamId, MI_VideoEncoder *videoEncoder,MI_AudioEncoder *audioEncoder)
{
    ServerMediaSession *sms = NULL;
    char streamName[8];

    if(m_smsCount >= MAX_SERRVER_MEDIA_SESSION)
    {
        printf("live555 createServerMediaSession failed,sms is full\n");
        return -1;
    }
    if(NULL == m_rtspServer)
    {
        printf("RestartVideoSubMediaSession failed, live555 server not create!\n");
        return -1;
    }

    sprintf(streamName, "video%d", streamId);

    sms = ServerMediaSession::createNew(*m_env, streamName, 0, "ww live test");
    m_sms[streamId] = sms;
    m_pAudioEncode[streamId] = audioEncoder;

    if(streamId < m_smsCount)
    {
        if(m_sms[streamId])
        {
            CreateAudioEncode(streamId, videoEncoder);
            m_sms[streamId]->addSubsession(LiveVideoServerMediaSubsession::createNew(*m_env, streamId, videoEncoder, False));

            m_rtspServer->addServerMediaSession(m_sms[streamId]);
            char *url = m_rtspServer->rtspURL(m_sms[streamId]);
            printf("using url %s\n", url);
            delete[] url;
        }
        printf("live555 RestartVideoSubMediaSession ok!\n");
    }
    else
    {
        printf("live555 RestartVideoSubMediaSession failed, restart num : %d, SubMediaSession count : %d\n", streamId, m_smsCount);
        return -1;
    }
    return 0;
}

int live555::CreateAudioEncode(int streamId, MI_VideoEncoder *videoEncoder)
{
    int iRet = 0;
    if(streamId < m_smsCount && NULL != m_pAudioEncode[streamId] && NULL != m_sms[streamId])
    {
        printf("CreateAudioEncode ve=%d ae=%d\n", videoEncoder->m_encoderType, m_pAudioEncode[streamId]->m_AiMediaType);

        switch(m_pAudioEncode[streamId]->m_AiMediaType)
        {
            case MT_PCM:
                m_sms[streamId]->addSubsession(LivePCMAAudioServerMediaSubsession::createNew(*m_env, streamId, m_pAudioEncode[streamId]));
                printf("live555::CreateAudioEncode(LivePCM Audio), streamId<%d>\r\n", streamId);
                break;

            case MT_G711A:
                m_sms[streamId]->addSubsession(LivePCMAAudioServerMediaSubsession::createNew(*m_env, streamId, m_pAudioEncode[streamId]));
                printf("live555::CreateAudioEncode(LiveG711A Audio), streamId<%d>\r\n", streamId);
                break;

            case MT_G711U:
                m_sms[streamId]->addSubsession(LivePCMAAudioServerMediaSubsession::createNew(*m_env, streamId, m_pAudioEncode[streamId]));
                printf("live555::CreateAudioEncode(LiveG711U Audio), streamId<%d>\r\n", streamId);
                break;

            case MT_G726:
                m_sms[streamId]->addSubsession(LivePCMAAudioServerMediaSubsession::createNew(*m_env, streamId, m_pAudioEncode[streamId]));
                printf("live555::CreateAudioEncode(LiveG726_16 Audio), streamId<%d>\r\n", streamId);
                break;

            default:
                m_sms[streamId]->addSubsession(LivePCMAAudioServerMediaSubsession::createNew(*m_env, streamId, m_pAudioEncode[streamId]));
                printf("live555::CreateAudioEncode(LivePCMA Audio), streamId<%d>\r\n", streamId);
        }
    }
    else
    {
        printf("CreateAudioEncode ve=%d without audio\n", videoEncoder->m_encoderType);
        iRet = -1;
    }
    return iRet;
}

int live555::createServerMediaSession(MI_VideoEncoder *videoEncoder,  MI_AudioEncoder *audioEncoder, char *streamName, const int streamId)
{
    ServerMediaSession *sms = NULL;

    if(m_smsCount >= MAX_SERRVER_MEDIA_SESSION)
    {
        printf("live555 createServerMediaSession failed,sms is full\n");
        return -1;
    }

    sms = ServerMediaSession::createNew(*m_env, streamName, 0, "ww live test");
    m_sms[m_smsCount] = sms;
    m_smsCount++;
    m_pAudioEncode[streamId] = audioEncoder;

    CreateAudioEncode(streamId, videoEncoder);

    //sms->addSubsession(LiveVideoServerMediaSubsession::createNew(*m_env, streamId, videoEncoder, (audioEncoder)? True : False));
    sms->addSubsession(LiveVideoServerMediaSubsession::createNew(*m_env, streamId, videoEncoder,  False));
#if 0
    switch(videoEncoder->m_encoderType)
    {
        case VE_AVC:
            sms->addSubsession(LiveH264VideoServerMediaSubsession::createNew(*m_env, streamId, videoEncoder, (audioEncoder)? True : False));
            printf("live555::createServerMediaSession(LiveH264Video), streamId<%d>\r\n", streamId);
            break;

        case VE_H265:
            sms->addSubsession(LiveH265VideoServerMediaSubsession::createNew(*m_env, streamId, videoEncoder, (audioEncoder)? True : False));
            printf("live555::createServerMediaSession(LiveH265Video), streamId<%d>\r\n", streamId);
            break;

        case VE_MJPEG:
        case VE_JPG:
            sms->addSubsession(LiveJPEGVideoServerMediaSubsession::createNew(*m_env, streamId, videoEncoder));
            printf("live555::createServerMediaSession(LiveJPEGVideo), streamId<%d>\r\n", streamId);
            break;

        default:
            sms->addSubsession(LiveH264VideoServerMediaSubsession::createNew(*m_env, streamId, videoEncoder, (audioEncoder)? True : False));
            printf("live555::createServerMediaSession(LiveH264Video), streamId<%d>\r\n", streamId);
    }
#endif
    printf("live555 createServerMediaSession suc, count=%d\n", m_smsCount);

    return 0;
}

void live555::setFrameRate(int framerate)
{
    H264or5VideoStreamFramer::defaultFrameRate = (double)framerate;
}

static pthread_t live555_thread;
int live555::startLive555Server()
{
    if(m_rtspServer == NULL)
    {
        printf("live555 startLive555Server failled, m_rtspServer is null\n");
        return -1;
    }

    m_watchVariable = 0;

    m_liveState = LIVE_STATE_PREPARE_RUN;
    pthread_create(&live555_thread, NULL, live555_Task, this);
    pthread_setname_np(live555_thread , "live555_Task");
    live555::setRunning(1);
    return 0;
}

int live555::stopLive555Server()
{
    printf("[live555] stopLive555Server\n");
    m_watchVariable = 1;
    m_liveState = LIVE_STATE_STOPING;
    live555::setRunning(0);

    do
    {
        usleep(10000);
        printf("[live555] stopLive555Server: wait thread exit %d\n", m_liveState);
        printf("m_watchVariable = %d\n", m_watchVariable);
    }while(LIVE_STATE_INVALID != m_liveState);

    printf("[%s] join+\n", __FUNCTION__);
    pthread_join(live555_thread, NULL);
    printf("[%s] join-\n", __FUNCTION__);

    return -1;
}
