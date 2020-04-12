/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  RecordStorage.h
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


#ifndef __RecordStorage_H_
#define __RecordStorage_H_

#include "mid_common.h"
#include "IStorage.h"

class CRecordStorage //: public IStorage
{
public:
    const char * GetLastFileName(void);
    MI_BOOL GetRecStartTime(SYSTEM_TIME & time);
private:
    CRecordStorage(MI_U8 ch, const MI_U8* path);
    ~CRecordStorage();

    MI_BOOL CreateStorage(const MI_U8* name);
    MI_BOOL CloseStorage();
    MI_BOOL WriteStorage(void *pBuffer, MI_U8 StreamType, MI_U8 Frametype, MI_U32 dwCount);
    MI_BOOL IsOpened();
    MI_BOOL IsDiskFull();
    MI_BOOL ChangeWorkPdp();
    MI_U32 GetLength();
    MI_U8 m_nChannel;
    CIStorage * m_pIStorage;

    friend class CRecord;
};


#endif
