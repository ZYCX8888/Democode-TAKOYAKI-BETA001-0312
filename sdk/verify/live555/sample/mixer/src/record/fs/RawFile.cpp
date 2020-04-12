/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  RawFile.cpp
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

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

#include "mid_sys.h"
#include "mid_common.h"
#include "RawFile.h"

#define MAX_FRAME_SIZE (800 * 1024)

CRawFile::CRawFile()
{
    mFlag = 0x00;
    m_RawFileHandle = NULL;
    m_Size = 0;
    m_NeedIFrame = FALSE;
    m_FrameCount = 0x00;
}

CRawFile::~CRawFile()
{

}

MI_BOOL CRawFile::GetNullFileFlag()
{
    return mFlag;
}

MI_BOOL CRawFile::CreateRawFile(const char *FileName)
{
    if(NULL == FileName)
    {
        MIXER_ERR("the filename is null\n");
        return FALSE;
    }

    m_RawFileHandle = fopen(FileName, "wb");
    if(NULL == m_RawFileHandle)
    {
        MIXER_ERR("can not open file:%s, err:%d\n", FileName, errno);
        return FALSE;
    }

    m_NeedIFrame = TRUE;
    m_Size = 0x0;
    m_FrameCount = 0x0;
    return TRUE;
}

MI_BOOL CRawFile::CloseRawFile()
{
    mFlag = FALSE;
    if(NULL == m_RawFileHandle)
    {
        MIXER_ERR("can not close handle %p\n, err:%d", m_RawFileHandle, errno);
        return FALSE;
    }

    //delete 0KB file
    fseek(m_RawFileHandle,0L,2);
    MI_U32 ret = ftell(m_RawFileHandle);
    if(ret == 0)
    {
        mFlag = TRUE;
    }

    fflush(m_RawFileHandle);
    fclose(m_RawFileHandle);
    m_RawFileHandle = NULL;

    return TRUE;
}


MI_BOOL CRawFile::WriteData(void *pBuffer,MI_U8 StreamType, MI_U8 FrameType, MI_U32 dwCount)
{
    return WriteFrame((MI_U8*)pBuffer, StreamType, FrameType, dwCount) <= 0 ? FALSE : TRUE;
}

MI_U32 CRawFile::GetWriteSize(void)
{
    return m_Size;
}

MI_S32 CRawFile::WriteFrame(const MI_U8 *pBuffer, MI_U8 StreamType, MI_U8 FrameType, MI_U32 dwCount)
{

    MI_U32 size = 0x0;

    /*MIXER_DBG("buf(%p), FrameType(%d), dwCount(%d)\n", \
                        pBuffer, \
                        FrameType, \
                        dwCount);*/

#if 1
    if(NULL == m_RawFileHandle)
    {
        MIXER_ERR("please open RawFile first\n");
        return FALSE;
    }
    if(NULL == pBuffer || 0x0 == dwCount)
    {
        MIXER_ERR("pBuffer is %p, dwCount is %d\n", pBuffer, dwCount);
        return FALSE;
    }

    if(AUDIO_STREAM == StreamType)
    {
        if(m_RawFileHandle)
        {
            size = fwrite(pBuffer, 1, dwCount, m_RawFileHandle);

            if(size != dwCount)
            {
                MIXER_WARN("write size(%d) not eq dwCount(%d)\n", size, dwCount);
            }
        }
        m_Size += size;
    }
    else if(VIDEO_YUV_STREAM >= StreamType)
    {
        if(TRUE == m_NeedIFrame)
        {
            //parse nal unit, if not I frame, then return
            if((VIDEO_FRAME_TYPE_I != FrameType)&&(VIDEO_FRAME_TYPE_YUV != FrameType)&&(VIDEO_FRAME_TYPE_JPEG != FrameType))
            {
                MIXER_ERR("it is not I frame, %2x %2x %2x %2x %2x\n", \
                        pBuffer[0], \
                        pBuffer[1], \
                        pBuffer[2], \
                        pBuffer[3], \
                        pBuffer[4]);
                return FALSE;
            }
            else
            {
                m_NeedIFrame = FALSE;
            }
        }

        if(((VIDEO_FRAME_TYPE_I == FrameType) &&  (m_FrameCount > 1000)) || \
            ((VIDEO_FRAME_TYPE_YUV == FrameType) && (m_FrameCount > 200)) ||\
            ((VIDEO_FRAME_TYPE_JPEG == FrameType) && (m_FrameCount > 100)) )
        {
            m_FrameCount = 0x0;
            fflush(m_RawFileHandle);
        }

        if(m_RawFileHandle)
        {
            size = fwrite(pBuffer, 1, dwCount, m_RawFileHandle);

            if(size != dwCount)
            {
                MIXER_WARN("write size(%d) not eq dwCount(%d)\n", size, dwCount);
            }
        }
        m_Size += size;
    }
    else
    {
        MIXER_ERR("unknow type(%d)\n", FrameType);
    }

    m_FrameCount++;

#endif
    return (MI_S32)size;
}

