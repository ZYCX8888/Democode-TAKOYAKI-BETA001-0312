#ifndef _CHANNEL_FRAME_H_
#define _CHANNEL_FRAME_H_

#define FIFO_LEN_DEF 16
#define FIFO_YUV_LEN_DEF 4

#include "mid_common.h"
#include "mid_sys.h"
#include "PacketCore.h"
#include <pthread.h>

typedef struct Fifo_s
{
    MI_U32 size;
    MI_U32 in;
    MI_U32 out;
    FrameInf_t *ptr;
  //  pthread_mutex_t m_mutexCmd;
}Fifo_t;


class CChannelFrame
{
public:

    CChannelFrame(MI_U32 length /*= FIFO_LEN_DEF*/);
    virtual ~CChannelFrame();

    virtual MI_S32 DoClean();

    MI_U32 GetUsedNodeNumOfFifo(void);
    MI_U32 GetUsedDataSizeOfFifo(void);
    MI_U32 ReleaseFrame(FrameInf_t &FrameInf_data);
    MI_S32 PutFrame2Fifo(const FrameInf_t &FrameInf_data);
    MI_S32 GetFrameFromFifo(FrameInf_t &FrameInf_data);
    MI_U32 GetFifoLen()
    {
        return m_FifoLen;
    };
private:
    MI_S32 Init();
    MI_S32 UnInit();
    pthread_mutex_t m_mutexCmd;
    MI_U32 m_FifoLen;
    MI_U32 m_DataSize;
    FrameInf_t *FrameInfPtr;
    Fifo_t FrameFifo_t;
    CPacket *CFramePacket;
};
#endif

