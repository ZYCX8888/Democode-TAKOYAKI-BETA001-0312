/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  IStorage.cpp
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


#include "mid_common.h"
#include "IStorage.h"
#include "mi_common_datatype.h"

CIStorage::CIStorage()
{

}

CIStorage::~CIStorage()
{

}

MI_BOOL CIStorage::CreateStorage(const MI_U8* name)
{
    return FALSE;
}
MI_BOOL CIStorage::CloseStorage()
{
    return FALSE;
}
MI_BOOL CIStorage::WriteStorage(void* pBuffer,MI_U8 StreamType, MI_U8 Frametype, MI_U32 dwCount)
{
    return FALSE;
}
MI_BOOL CIStorage::IsOpened()
{
    return FALSE;
}
MI_BOOL CIStorage::IsDiskFull()
{
    return TRUE;
}
MI_BOOL CIStorage::ChangeWorkPdp()
{
    return FALSE;
}

MI_U32 CIStorage::GetLength()
{
    return 0;
}

const char * CIStorage::GetLastFileName(void)
{
    return NULL;
}

void CIStorage::GetRecStartTime(SYSTEM_TIME & time)
{

}
