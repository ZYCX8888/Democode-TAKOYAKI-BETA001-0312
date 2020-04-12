#ifndef _CHANNEL_STREAM_H_
#define _CHANNEL_STREAM_H_

#include "PacketCore.h"
#include <pthread.h>

#define STREAM_FIFO_LEN 4096

typedef struct _FifoStream
{
    MI_U32 size;
    MI_U32 in;
    MI_U32 out;
    void *ptr;
  //  pthread_mutex_t m_mutexCmd;
}FifoStream;

class CChannelStream
{
public:

    CChannelStream(MI_U32 length = STREAM_FIFO_LEN);
    ~CChannelStream();

    virtual int DoClean();

    MI_U32 GetUsedLenOfFifo(void);

    MI_S32 PutBuf2Fifo(const void *Sptr, MI_U32 len);
    MI_S32 GetBufFromFifo(void *Dptr, MI_U32 len);

private:
    MI_S32 Init();
    MI_S32 UnInit();
    MI_U32 m_FifoLen;
    FifoStream StreamFifo;
    CPacket *CStreamPacket;
};

#endif

