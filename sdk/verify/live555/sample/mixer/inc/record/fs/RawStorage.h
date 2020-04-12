/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  RawStorage.h
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

#ifndef _RawStorage_H_
#define _RawStorage_H_

#include "mid_common.h"
#include "IStorage.h"
#include "RawFile.h"

//#define STORAGE_PATH "/tmp/sd/"
#define STORAGE_PATH "./"

class CRawStorage : public CIStorage
{
public:
    CRawStorage(MI_U8 ch, const MI_U8* path);
    ~CRawStorage();

    MI_BOOL CreateStorage(const MI_U8* name);
    MI_BOOL CloseStorage();
    MI_BOOL WriteStorage(void* pBuffer, MI_U8 StreamType, MI_U8 Frametype, MI_U32 dwCount);
    MI_U32 GetLength();
    MI_BOOL IsOpened();
    MI_BOOL IsDiskFull();

    const char * GetLastFileName(void);
    void GetRecStartTime(SYSTEM_TIME & time);
    MI_U32    nChannel;
    CRawFile mRawFile;
    char mfilename[256];
    char mLastFileName[64];
    char mStoragePathName[64];

private:
    SYSTEM_TIME mStarttime;
    char path[128];
    volatile MI_BOOL mbOpened;

};

#endif
