/*
* live555.h- Sigmastar
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
#ifndef _LIVE555_H__
#define _LIVE555_H__
#pragma once
#include <BasicUsageEnvironment.hh>
#include "liveMedia.hh"
#include "mid_VideoEncoder.h"
#include "mid_AudioEncoder.h"
#define MAX_SERRVER_MEDIA_SESSION 10
class live555
{
public:
    static live555* createNew();
    ~live555();
    int startLive555Server();
    int stopLive555Server();
    int stopVideoSubMediaSession(int streamId, MI_VideoEncoder *videoEncoder);
    int createServerMediaSession(MI_VideoEncoder *videoEncoder,  MI_AudioEncoder *audioEncoder, char *streamName, int streamId);
    int RestartVideoSubMediaSession(int streamId, MI_VideoEncoder *videoEncoder,MI_AudioEncoder *audioEncoder);
    void setFrameRate(int framerate);
    static int isRunning(){return fIsRunning;};
    static void setRunning(int isRunning){fIsRunning = isRunning;};
    char                            m_watchVariable;
    int                             m_liveState;

    TaskScheduler*                  m_scheduler;
    UsageEnvironment*               m_env;
    UserAuthenticationDatabase*     m_authDB;
    RTSPServer*                     m_rtspServer;

    ServerMediaSession              *m_sms[MAX_SERRVER_MEDIA_SESSION];
    int                             m_smsCount;

    static int fRtspServerPortNum;
    static int fIsRunning;

private:
    int CreateAudioEncode(int streamId, MI_VideoEncoder *videoEncoder);
    live555();

private:
    MI_AudioEncoder                 *m_pAudioEncode[MAX_SERRVER_MEDIA_SESSION];
};

#endif //_LIVE555_H__
