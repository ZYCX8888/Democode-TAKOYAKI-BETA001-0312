/*


*/

#ifndef _MESSAGE_QUEUE_H_
#define _MESSAGE_QUEUE_H_

#include "MyMutex.h"
#include "List.h"

#define MsgBufListCnt 30
#define SENDCMD 0
#define SENDRESULT (!SENDCMD)

struct MixerMsgBuf
{
    MI_U8 dir;
    MixerCmdId cmdId;
    MI_S32     paramLen;
    MI_S8       paramBuf[MIXER_MAX_MSG_BUFFER_SIZE];
    struct list_head  MsgBufList;
};

class CMsgQueueManager
{
public:
    static CMsgQueueManager *instance();

    CMsgQueueManager();
    ~CMsgQueueManager();

public:
    MI_S32 Init();
    MI_S32 UnInit();

    MI_S32 OnSendMsg(MixerCmdId cmdId, MI_S8 *param, MI_U32 paramLen, MI_U8 dir=0x0);
    MI_S32 OnRecvMsg(struct MixerMsgBuf *qbuf);
    MI_S32 OnWaitMsgResult(MixerCmdId cmdId, void *param, MI_U32 paramLen);
    MyMutexCond & GetMsgMutex();
    MyMutexCond * GetResultMsgMutex(MI_U32 CmdType);

    pthread_t pthread_message;

private:
    MyMutexCond   MsgMutex;
    MyMutexCond   MsgResultMutex[MIXER_CMD_TYPE>>12];

    struct list_head  MsgEmptyList;
    struct list_head  MsgWorkList;
    struct MixerMsgBuf *pMixerMsgBufList;

    struct list_head  MsgResultEmptyList;
    struct list_head  MsgResultWorkList;
    struct MixerMsgBuf *pMixerMsgResultBufList;
};

#define g_CMsgManager (CMsgQueueManager::instance())

#endif

