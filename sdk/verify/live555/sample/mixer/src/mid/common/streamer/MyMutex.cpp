/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#include "MyMutex.h"
#include <sys/time.h>
#include "mid_utils.h"

MyMutex::MyMutex()
{
    pthread_mutex_init(&m_ObjMutex, NULL);
}

MyMutex::~MyMutex()
{
    pthread_mutex_destroy(&m_ObjMutex);
}

void MyMutex::OnLock()
{
    pthread_mutex_lock(&m_ObjMutex);
}

void MyMutex::OnUnLock()
{
    pthread_mutex_unlock(&m_ObjMutex);
}

pthread_mutex_t & MyMutex::GetMutex()
{
    return m_ObjMutex;
}


MyMutexCond::MyMutexCond()
{
    mStatue = 0x0;
    pthread_cond_init(&m_ObjReady, NULL);
}

MyMutexCond::~MyMutexCond()
{
    pthread_cond_destroy(&m_ObjReady);
}

#if 0
MI_S32 MyMutexCond::OnWaitCond()
{
    OnLock();
    mStatue = 0x1;
        pthread_cond_wait(&m_ObjReady, &GetMutex());
    mStatue = 0x0;
    OnUnLock();

    return 0;
}
#endif

MI_S32 MyMutexCond::OnWaitCondTimeout(MI_U32 MsTime /* = 2000*/)
{
    MI_S32 ret = 0x0;

    struct timeval now;
    struct timespec outtime;

    gettimeofday(&now, NULL);
    outtime.tv_sec = now.tv_sec + MsTime/1000;
    outtime.tv_nsec = now.tv_usec * (MsTime%1000)*1000;

    OnLock();
    mStatue = 0x1;
        ret = pthread_cond_timedwait(&m_ObjReady, &GetMutex(), &outtime);
    mStatue = 0x0;
    OnUnLock();

    return ret;
}

MI_S32 MyMutexCond::OnSignalCond(MI_BOOL sync /* = 0x0*/)
{
    volatile MI_U16 count = 0x0;


    //if(0x1 == mStatue)
    if(!!sync)
    {
        while(0x01 != mStatue && count < 100) //it must wait for pthread_cond has being waiting, or it will lost signal
        {
            MySystemDelay(20);
            count++;
        }
        if(count >= 100)
        {
            MIXER_WARN("wait for mStatue==1 timeout. it will lost signal.\n");
        }
    }
    OnLock();
    pthread_cond_signal(&m_ObjReady);
    OnUnLock();

    return 0;
}


