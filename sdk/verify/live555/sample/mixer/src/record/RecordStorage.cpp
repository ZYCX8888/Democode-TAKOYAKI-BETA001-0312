/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  RecordStorage.cpp
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


#include "RecordStorage.h"
#include "mid_common.h"
#include "RawStorage.h"

CRecordStorage::CRecordStorage(MI_U8 ch, const MI_U8* path)
    : m_nChannel(ch)
    , m_pIStorage(NULL)
{
    m_pIStorage = new CRawStorage(ch, path);
}

CRecordStorage::~CRecordStorage()
{
    if (m_pIStorage)
    {
        delete m_pIStorage;
    }
}

MI_BOOL CRecordStorage::CreateStorage(const MI_U8 * name)
{
    if(!m_pIStorage)
    {
        return FALSE;
    }
    return m_pIStorage->CreateStorage(name);
}

MI_BOOL CRecordStorage::CloseStorage()
{
    if(!m_pIStorage)
    {
        return FALSE;
    }
    return m_pIStorage->CloseStorage();
}

MI_BOOL CRecordStorage::WriteStorage(void *pBuffer,MI_U8 StreamType, MI_U8 Frametype, MI_U32 dwCount)
{
    if(!m_pIStorage)
    {
        return FALSE;
    }
    return m_pIStorage->WriteStorage(pBuffer, StreamType,  Frametype, dwCount);
}

MI_BOOL CRecordStorage::IsOpened()
{
    if(!m_pIStorage)
    {
        return FALSE;
    }
    return m_pIStorage->IsOpened();
}

MI_BOOL CRecordStorage::IsDiskFull()
{
    return m_pIStorage->IsDiskFull();
}

MI_BOOL CRecordStorage::ChangeWorkPdp()
{
    if(!m_pIStorage)
    {
        return FALSE;
    }
    return m_pIStorage->ChangeWorkPdp();
}

const char * CRecordStorage::GetLastFileName()
{
    if(!m_pIStorage)
    {
        return NULL;
    }
    return m_pIStorage->GetLastFileName();
}

MI_U32 CRecordStorage::GetLength()
{
    if(!m_pIStorage)
    {
        return FALSE;
    }
    return m_pIStorage->GetLength();
}

MI_BOOL CRecordStorage::GetRecStartTime(SYSTEM_TIME & time)
{
    if(!m_pIStorage)
    {
        return FALSE;
    }

    m_pIStorage->GetRecStartTime(time);

    return TRUE;
}
