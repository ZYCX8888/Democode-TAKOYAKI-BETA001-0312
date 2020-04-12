/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  Record.h
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


#ifndef _RECORD_H_
#define _RECORD_H_

#include "mid_common.h"
#include "mid_utils.h"
#include "mid_VideoEncoder.h"
#include "VideoChannelFrame.h"
#include "RecordStorage.h"
#include "mid_VideoEncoder.h"

#define  FILE_LAST_TIME  (3*60)

class CRecord
{
public:
    CRecordStorage *m_pRecordStorage;


    CRecord(MI_U32 ch, MI_VideoEncoder *fpVideoEncoder, const MI_U8* path = NULL);
    ~CRecord();

    MI_S32 Init();
    MI_S32 UnInit();
    MI_S32 Start(MI_U32 mode);
    MI_S32 Stop(MI_U32 mode);

    MI_S32 Pause();
    MI_S32 Resume();

    MI_S32 DoRecord();
    MI_S32 DoRecordOneFrame();
    MI_BOOL CheckIsDiskFull();
    MI_BOOL RecRun();

    MI_U32 GetCurMode();
    MI_U32 GetMode();
    MI_U32 SetMode(MI_U32 mode);
    MI_U32 ClearMode(MI_U32 mode);

    pthread_cond_t m_condCtrl;
    pthread_mutex_t m_mutexCtrl;

    MI_U32 m_nChannel;
    MI_U32 m_nMode;

    MI_BOOL m_bNeedIFrame;
    SYSTEM_TIME    m_stSystemTime;
    MI_U64 LastTime;


private:
    MI_U32 RegisterVideoEncoder();
    MI_U32 UnRegisterVideoEncoder();
    MI_U32 FlushVideoBuff();
    MI_U32 RequestIDR();
    const MI_U8 *GetCodecFormat();

    MI_VideoEncoder *pVideoEncoder;
    VideoChannelFrame mRecordChannelFrame;
    friend class CRecordManager;
};
#endif

