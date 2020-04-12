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
#include <malloc.h>
#include <assert.h>

#include "ChannelStream.h"
#include "PacketModule.h"
#include "mid_sys.h"
#include "Fifo.h"

CChannelStream::CChannelStream(MI_U32 length /*= STREAM_FIFO_LEN*/)
{
    m_FifoLen = roundup_pow_of_two(length);

    Init();
}

CChannelStream::~CChannelStream()
{
    UnInit();
}

MI_S32 CChannelStream::Init()
{
//     PacketModule* pobjPacketModule = dynamic_cast<PacketModule*>(PacketModule::getInstance());
//     CStreamPacket = (CPacket *)pobjPacketModule->MallocPacket(sizeof(char) * m_FifoLen);
    CStreamPacket = (CPacket*)malloc(m_FifoLen);
    assert(CStreamPacket);

    StreamFifo.ptr = CStreamPacket;
    StreamFifo.in = StreamFifo.out = 0x0;
    StreamFifo.size = m_FifoLen;

    //pthread_mutex_init(&StreamFifo.m_mutexCmd, NULL);

    return 0x0;
}

MI_S32 CChannelStream::UnInit()
{
    //pthread_mutex_destroy(&StreamFifo.m_mutexCmd);
    if(CStreamPacket)
    {
        free(CStreamPacket);
        CStreamPacket = NULL;

        StreamFifo.ptr = NULL;
    }
    StreamFifo.in = StreamFifo.out = StreamFifo.size = 0x0;

    return 0x0;
}

MI_S32 CChannelStream::PutBuf2Fifo(const void *Sptr, MI_U32 len)
{
    MI_U32 l;

    //pthread_mutex_lock(&StreamFifo.m_mutexCmd);

    len = MIN(len, StreamFifo.size - StreamFifo.in + StreamFifo.out);
    l = MIN(len, StreamFifo.size - (StreamFifo.in & (StreamFifo.size - 1)));
    memcpy((char*)StreamFifo.ptr + (StreamFifo.in & (StreamFifo.size - 1)), (char *)Sptr, l);
    memcpy((char*)(StreamFifo.ptr), (char *)Sptr + l, len - l);

    StreamFifo.in += len;

    //pthread_mutex_unlock(&StreamFifo.m_mutexCmd);

    return len;
}

MI_S32 CChannelStream::GetBufFromFifo(void *Dptr, MI_U32 len)
{
    MI_U32 l;

    //pthread_mutex_lock(&StreamFifo.m_mutexCmd);

    len = MIN(len, StreamFifo.in - StreamFifo.out);
    l = MIN(len, StreamFifo.size - (StreamFifo.out & (StreamFifo.size - 1)));
    memcpy((char *)Dptr, (char*)StreamFifo.ptr + (StreamFifo.out & (StreamFifo.size - 1)), l);
    memcpy((char *)Dptr + l, (char*)(StreamFifo.ptr), len - l);

    StreamFifo.out += len;

    //pthread_mutex_unlock(&StreamFifo.m_mutexCmd);

    return len;
}

MI_U32 CChannelStream::GetUsedLenOfFifo(void)
{
    MI_U32 len;

    do{
        len = StreamFifo.in - StreamFifo.out;
    }while(len != (StreamFifo.in - StreamFifo.out));

    return len;
}


MI_S32 CChannelStream::DoClean()
{
    StreamFifo.in = StreamFifo.out = 0x0;
    return 0x0;
}

