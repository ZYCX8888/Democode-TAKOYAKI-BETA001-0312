#ifndef _MY_MUTEX_H_
#define _MY_MUTEX_H_

#include <pthread.h>
#include "mi_common_datatype.h"

class MyMutex
{
public:
    MyMutex();
    ~MyMutex();

public:
    void OnLock();
    void OnUnLock();
    pthread_mutex_t & GetMutex();
private:
    pthread_mutex_t m_ObjMutex;
};

class MyMutexCond:public MyMutex
{
public:
    MyMutexCond();
    ~MyMutexCond();

public:
    //MI_S32 OnWaitCond();
    MI_S32 OnSignalCond(MI_BOOL  sync = 0x0);
    MI_S32 OnWaitCondTimeout(MI_U32 MsTime = 2000);
private:
    volatile MI_BOOL mStatue;
    pthread_cond_t m_ObjReady;
};

#endif //_MY_MUTEX_H_