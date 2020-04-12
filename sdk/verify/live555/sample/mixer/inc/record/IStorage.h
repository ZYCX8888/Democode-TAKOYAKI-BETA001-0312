/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  IStorage.h
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

#ifndef _IStorage_H_
#define _IStorage_H_

#include "object.h"
#include "mid_common.h"

class CIStorage : public CObject
{
public:
    CIStorage();
    virtual ~CIStorage();

    virtual MI_BOOL CreateStorage(const MI_U8* name);
    virtual MI_BOOL CloseStorage();
    virtual MI_BOOL WriteStorage(void* pBuffer,MI_U8 StreamType,  MI_U8 Frametype, MI_U32 dwCount);
    virtual MI_BOOL IsOpened();
    virtual MI_BOOL IsDiskFull();
    virtual MI_BOOL ChangeWorkPdp();
    virtual MI_U32 GetLength();
    virtual const char * GetLastFileName(void);

    virtual void GetRecStartTime(SYSTEM_TIME & time);
};

#endif
