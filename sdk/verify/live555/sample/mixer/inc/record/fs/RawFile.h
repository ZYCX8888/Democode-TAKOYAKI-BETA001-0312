/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  RawFile.h
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

#ifndef _RAW_FILE_H_
#define _RAW_FILE_H_

#include "mid_common.h"
#include "mi_sys_datatype.h"
#include <stdio.h>
#include <sys/stat.h>

class CRawFile
{
public:
    CRawFile();
    ~CRawFile();
    MI_BOOL CreateRawFile(const char *FileName);
    MI_BOOL CloseRawFile();
    MI_BOOL WriteData(void *pBuffer, MI_U8 StreamType, MI_U8 FrameType, MI_U32 dwCount);
    MI_U32 GetWriteSize(void);
    MI_BOOL GetNullFileFlag();
public:
    bool mFlag;

private:

        FILE * m_RawFileHandle;
        MI_S32 WriteFrame(const MI_U8 *pBuffer, MI_U8 StreamType, MI_U8 FrameType, MI_U32 dwCount);
        MI_U32 m_Size;
        MI_BOOL m_NeedIFrame;
        MI_U32 m_FrameCount;
};

#endif
