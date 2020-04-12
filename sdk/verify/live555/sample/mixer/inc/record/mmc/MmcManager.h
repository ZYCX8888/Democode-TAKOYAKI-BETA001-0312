/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  MmcManager.h
* Author:     fisher.yang@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2019/8/3
* Description: mixer record source file
*
*
*
* History:
*
*    1. Date  :        2019/8/3
*       Author:        fisher.yang@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/

#ifndef _MMC_MANAGER_H_
#define _MMC_MANAGER_H_

#include "mi_common_datatype.h"
#include "object.h"

#include <pthread.h>

#define DRIVER_TYPE_NR            5

#define MMC_TYPE 2
#define PROC_PART_INFO    "/proc/partitions"
#define MMC_MOUNT_PATH "/tmp/mnt"

typedef struct tagMMCInfo
{
    MI_S32    mmcn;
    MI_S32    part;
    MI_S32     major;
    MI_S32     minor;
    MI_S32    blocks;
    char name[64];
}MMCInfo;

typedef struct tagMmcDev
{
    MI_S32 scsin;
    MI_S32 sdx;
    MI_S32 type;
    MI_S32 blocks;
    char name[64];
}MmcDev;

typedef struct __card_storage_device_info
{
    MI_U8 Version;
    MI_U8 Reserved[3];
    MI_U32 DriverType;
    MI_U32 TotalSpace;
    MI_U32 RemainSpace;
    MI_U8  DriverName[64];
}CARD_STORAGE_DEVICE_INFO;

class CMmcManager : public CObject
{
public:
    static CMmcManager *instance();
    MI_BOOL bRecInMMC;
    CMmcManager();
    virtual ~CMmcManager();

    void Init();

    MI_BOOL Start();
    MI_BOOL Stop();

    MI_BOOL GetStat(MI_S32 &stat, MI_U32 driver_type = 0);
    MI_BOOL GetCardDevInfo(CARD_STORAGE_DEVICE_INFO &cardDevInfo);
    MI_BOOL FormatDisk(MI_U32 disk_no = 0);


    MI_BOOL ReMountDisk();
    static MI_BOOL CUpdateState();
private:
    MI_BOOL InitializeFS();
    MI_BOOL UpdateState();
    static void onTimerCallBack(int);

    MI_S32 TimerCreate(void(*pCallBack)(int), MI_U32 dwMilliSeconds);
    MI_S32 TimerDestroy();


    //pthread_cond_t m_cond;
    pthread_mutex_t m_mutex;

    MI_S32 m_dwFSError;    //useless
    MI_S32  m_dwStat[5];
    MI_U64 m_dwCapability;
    volatile MI_U64  m_dwRemain;

};

#define g_MmcManager (CMmcManager::instance())


#endif

