/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  module_record.h
* Author:     fisher.yang@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2019/8/2
* Description: mixer record source file
*
*
*
* History:
*
*    1. Date  :        2019/8/2
*       Author:        fisher.yang@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/

#ifndef _MODULE_RECORD_H_
#define _MODULE_RECORD_H_

#include "Record.h"
#include "mi_common_datatype.h"

#define REC_MAX_CHN_NUM (MAX_VIDEO_NUMBER)

#define DISK_EVENT_INIT      (0)
#define DISK_EVENT_NORMAL   DISK_EVENT_INIT
#define DISK_EVENT_ABNORMAL (0x01 << 15)
#define DISK_EVENT_NO_SPACE   (0x01 << 14)

typedef enum{
    StorageNull = 0x0,
    StorageSD,
    StorageUSB,
}DevStorage;

class  CRecordManager
{
public:
    static CRecordManager * instance();

    CRecordManager();
    ~CRecordManager();

    MI_S32 Start();
    MI_S32 Stop();
    MI_S32 Pause(MI_U32 ch);
    MI_S32 Resume(MI_U32 ch);

    MI_BOOL GetThreadOpenFlag(void);

    pthread_t m_RecordFrameThread;
    volatile MI_BOOL bThreadOpen;

    //pthread_cond_t m_cond;
    //pthread_mutex_t m_mutex;

    CRecord *pRecord[REC_MAX_CHN_NUM];
    MI_BOOL bRecOpen[REC_MAX_CHN_NUM];    //only control by CMD_RECORD_SETMODE

    MI_U32 m_nChannelNum;
    volatile MI_U32 m_nRecord_event;
    MI_U32 GetStorageState();
private:

    MI_U8 path[128];
    MI_U8 storage;
};

#define g_RecordManager (CRecordManager::instance())
#endif

