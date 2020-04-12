/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#include <string.h>
#include <assert.h>

#include "mid_common.h"
#include "ChannelFrame.h"
#include "PacketModule.h"
#include "Fifo.h"

CChannelFrame::CChannelFrame(MI_U32 length /*= FIFO_LEN_DEF*/)
{
    if( is_power_of_2(length))
        m_FifoLen = length;
    else
        m_FifoLen = roundup_pow_of_two(length);

    //PacketModule* pobjPacketModule = dynamic_cast<PacketModule*>(PacketModule::getInstance());
   // CFramePacket = (CPacket *)pobjPacketModule->MallocPacket(sizeof(FrameInf_t) * m_FifoLen);
    CFramePacket = (CPacket *)g_PacketModule->MallocPacket(sizeof(FrameInf_t) * m_FifoLen);
    assert(CFramePacket);

    FrameInfPtr = (FrameInf_t *)CFramePacket->GetBuffer();
    assert(FrameInfPtr);

    assert(0x0 == Init());
}

CChannelFrame::~CChannelFrame()
{
    UnInit();

    if (CFramePacket)
    {
        //PacketModule* pobjPacketModule = dynamic_cast<PacketModule*>(PacketModule::getInstance());

         g_PacketModule->FreePacket(CFramePacket);

        CFramePacket = NULL;
        FrameInfPtr = NULL;
    }
}

MI_S32 CChannelFrame::UnInit()
{
    pthread_mutex_destroy(&m_mutexCmd);

    memset(&FrameFifo_t, 0x0, sizeof(FrameFifo_t));

    return 0x0;
}

MI_S32 CChannelFrame::Init()
{
    if (!is_power_of_2(m_FifoLen))
        return -1;

    FrameFifo_t.size = m_FifoLen;
    FrameFifo_t.in = FrameFifo_t.out = 0x0;
    FrameFifo_t.ptr = FrameInfPtr;
    m_DataSize = 0x0;

    pthread_mutex_init(&m_mutexCmd, NULL);

    return 0x0;
}

MI_S32 CChannelFrame::PutFrame2Fifo(const FrameInf_t &FrameInf_data)
{
    MI_S32 len;
    MI_S32 l;

    pthread_mutex_lock(&m_mutexCmd);

    l = len = MIN(1, (FrameFifo_t.size + FrameFifo_t.out - FrameFifo_t.in));
   // l = Min(len, (FrameFifo_t.size - (FrameFifo_t.in & (FrameFifo_t.size - 1))));

    if (l != 0)
    {
        FrameFifo_t.ptr[(FrameFifo_t.in & (FrameFifo_t.size - 1))] = FrameInf_data;
        m_DataSize += FrameInf_data.nLen;
    }
    FrameFifo_t.in += l;


    pthread_mutex_unlock(&m_mutexCmd);

    return l;
}

MI_S32 CChannelFrame::GetFrameFromFifo(FrameInf_t &FrameInf_data)
{
    MI_S32 len;
    MI_S32 l;

    pthread_mutex_lock(&m_mutexCmd);

    l = len = MIN(1, (FrameFifo_t.in - FrameFifo_t.out));
   // l = Min(len, (FrameFifo_t.size - (FrameFifo_t.out & (FrameFifo_t.size - 1))));
    if (l != 0)
    {
        FrameInf_data = FrameFifo_t.ptr[(FrameFifo_t.out & (FrameFifo_t.size - 1))];
        m_DataSize -= FrameInf_data.nLen;
    }
    FrameFifo_t.out += l;

    pthread_mutex_unlock(&m_mutexCmd);

    return l;
}

MI_U32 CChannelFrame::ReleaseFrame(FrameInf_t &FrameInf_data)
{
    CPacket *_tmp = (CPacket *)FrameInf_data.pPacketAddr;
    if(NULL == _tmp)
        return 0;

      pthread_mutex_lock(&m_mutexCmd);
      _tmp->Release();
      FrameInf_data.pPacketAddr = NULL;
      pthread_mutex_unlock(&m_mutexCmd);

    return 0;
}

MI_U32 CChannelFrame::GetUsedNodeNumOfFifo(void)
{
    return FrameFifo_t.in - FrameFifo_t.out;
}

MI_U32 CChannelFrame::GetUsedDataSizeOfFifo(void)
{
    return m_DataSize;
}

MI_S32 CChannelFrame::DoClean()
{
    FrameInf_t tmp;
    while (FrameFifo_t.in != FrameFifo_t.out)
    {
        //memset(&tmp, 0x0, sizeof(FrameInf_t));

        if(0x0 == GetFrameFromFifo(tmp))
        break;
       #if 0
        CPacket *_tmp = (CPacket *)tmp.pPacketAddr;
        if (_tmp != NULL)
            _tmp->Release();
       #else
        ReleaseFrame(tmp);
       #endif
    }

    return 0x0;
}


