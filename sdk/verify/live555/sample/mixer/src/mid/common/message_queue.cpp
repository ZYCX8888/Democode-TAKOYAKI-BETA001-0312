/*
 * message_queue.cpp- Sigmastar
 *
 * Copyright (C) 2018 Sigmastar Technology Corp.
 *
 * Author: XXXX <XXXX@sigmastar.com.cn>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "mid_common.h"
#include "module_common.h"
#include "module_config.h"
#include "Message_queue.h"

static void* Msg_Task(void *argc)
{
    MI_S32 ret = 0x0;

    struct MixerMsgBuf msgBuf;
    CMsgQueueManager *pCMsg = (CMsgQueueManager *)argc;
    if(NULL == pCMsg)
    {
        MIXER_ERR("pCMsg is null. err\n");
        return NULL;
    }

    while(1)
    {
        memset(&msgBuf, 0x0, sizeof(msgBuf));

        ret = pCMsg->OnRecvMsg(&msgBuf);
        if( -1 == ret)
        {
            break;
        }
        else if(0x0 == ret)
        {
            #if 0
            if(ETIMEDOUT == pCMsg->GetMsgMutex().OnWaitCondTimeout(5000))
            {
                continue;
            }
            else
            {

            }
            #else
            pCMsg->GetMsgMutex().OnWaitCondTimeout(5000);
            continue;
            #endif
        }
        else
        {

        }

        if(CMD_SYSTEM_EXIT_QUEUE == msgBuf.cmdId)
        {
            break;
        }

        //MIXER_DBG("cmdid is %x. dir is %d\n", msgBuf.cmdId, msgBuf.dir);

        if(SENDCMD == msgBuf.dir)    // send cmd
        {
            switch(msgBuf.cmdId & MIXER_CMD_TYPE)
            {
                case CMD_SYSTEM:
                    system_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;

                case CMD_VIF:
                    vif_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;

                case CMD_VPE:
                    vpe_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;

                case CMD_VENC:
                    venc_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;

                case CMD_VIDEO:
                    video_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;

                case CMD_AUDIO:
                    audio_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;

#if MIXER_OSD_ENABLE
                case CMD_OSD:
                    osd_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;
#endif

                case CMD_ISP:
                    isp_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;

#if MIXER_IE_ENABLE
                case CMD_IE:
                    ie_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;
#endif

#if MIXER_UVC_ENABLE
                case CMD_UVC:
                    uvc_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;
#endif

                case CMD_REC:
                    rec_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;
#if MIXER_SED_ENABLE
                case CMD_SED:
                    sed_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;
#endif
                case CMD_MOTOR:
                    motor_process_cmd(msgBuf.cmdId, msgBuf.paramBuf, msgBuf.paramLen);
                    break;
            }
        }
        else        //send cmd result
        {
            MyMutexCond *ptr = NULL;

            ptr = pCMsg->GetResultMsgMutex((MI_U32)msgBuf.cmdId);
            if(NULL != ptr)
            {
                ptr->OnSignalCond(1);
            }
            else
            {
                MIXER_ERR("can not find this cmd module's mutex.\n");
            }

        }
    };

    MIXER_DBG("msg thread exit.\n");

    pthread_exit(NULL);
    return NULL;
}

CMsgQueueManager::CMsgQueueManager()
{
    pMixerMsgBufList = NULL;
    pMixerMsgResultBufList = NULL;
    pthread_message = (pthread_t)(-1);
    INIT_LIST_HEAD(&MsgEmptyList);
    INIT_LIST_HEAD(&MsgWorkList);

    INIT_LIST_HEAD(&MsgResultEmptyList);
    INIT_LIST_HEAD(&MsgResultWorkList);
}

CMsgQueueManager::~CMsgQueueManager()
{

}

CMsgQueueManager *CMsgQueueManager::instance()
{
    static CMsgQueueManager _instance;

    return &_instance;
}

MI_S32 CMsgQueueManager::OnSendMsg(MixerCmdId cmdId, MI_S8 *param, MI_U32 paramLen, MI_U8 dir)
{
    //allow send null param
    if(NULL == param && 0x0 != paramLen)
    {
        MIXER_ERR("param == null. err\n");
        return -1;
    }

    if(paramLen > MIXER_MAX_MSG_BUFFER_SIZE)
    {
        MIXER_ERR("%s failed. cmdId=0x%x, paramLen=%d, param=0x%x\n", __FUNCTION__, \
                        cmdId, paramLen, (int)param);
        return -1;
    }

    struct MixerMsgBuf *tmp = NULL;
    struct list_head *pHead = NULL, *ptmp = NULL;
    MsgMutex.OnLock();
    if(SENDRESULT == dir)
    {
        list_for_each_safe(pHead, ptmp, &MsgResultEmptyList)
        {
            tmp = list_entry(pHead, struct MixerMsgBuf, MsgBufList);
            if(NULL != tmp)
            {
                list_del(&tmp->MsgBufList);
                tmp->dir = !!dir;
                tmp->cmdId    = cmdId;
                tmp->paramLen = paramLen;
                memcpy(tmp->paramBuf, param, paramLen);

                list_add_tail(&tmp->MsgBufList, &MsgResultWorkList);
            }
        }
    }

    list_for_each_safe(pHead, ptmp, &MsgEmptyList)
    {
        tmp = list_entry(pHead, struct MixerMsgBuf, MsgBufList);
        if(NULL != tmp)
        {
            list_del(&tmp->MsgBufList);
            tmp->dir = !!dir;
            tmp->cmdId    = cmdId;
            if(SENDCMD == dir)
            {
                tmp->paramLen = paramLen;
                memcpy(tmp->paramBuf, param, paramLen);
            }
            else
            {
                tmp->paramLen = 0;
                memset(tmp->paramBuf, 0x0, MIXER_MAX_MSG_BUFFER_SIZE);
            }

            list_add_tail(&tmp->MsgBufList, &MsgWorkList);
            MsgMutex.OnUnLock();
            MsgMutex.OnSignalCond();
            return 0;
        }
    }
    MsgMutex.OnUnLock();
    MIXER_ERR("can not send msg. cmdId :%d, dir:%d\n", cmdId, dir);
    return -1;
}

MI_S32 CMsgQueueManager::OnRecvMsg(struct MixerMsgBuf *qbuf)
{
    if(NULL == qbuf)
    {
        MIXER_ERR("qbuf == null. err\n");
        return -1;
    }
    memset(qbuf, 0x00, sizeof(MixerMsgBuf));

    struct MixerMsgBuf *tmp = NULL;
    struct list_head *pHead = NULL, *ptmp = NULL;

    MsgMutex.OnLock();
    list_for_each_safe(pHead, ptmp, &MsgWorkList)
    {
        tmp = list_entry(pHead, struct MixerMsgBuf, MsgBufList);

        if(NULL != tmp)
        {
            list_del(&tmp->MsgBufList);
            memcpy(qbuf, tmp, sizeof(MixerMsgBuf));
            list_add_tail(&tmp->MsgBufList, &MsgEmptyList);
            MsgMutex.OnUnLock();
            return 1;
        }
    }
    //MIXER_ERR("can not recv msg\n");
    MsgMutex.OnUnLock();
    return 0;
}


MI_S32 CMsgQueueManager::OnWaitMsgResult(MixerCmdId cmdId, void *param, MI_U32 paramLen)
{
    if((NULL == param && 0x0 != paramLen))
    {
        MIXER_ERR("param is err\n");
        return -1;
    }
    MI_S32 ret = 0x0;
    MyMutexCond *ptr = NULL;

    ptr = GetResultMsgMutex(cmdId);
    if(NULL == ptr)
    {
        MIXER_ERR("this cmd module has no signal to wait. err\n");
        return -1;
    }

    ret = ptr->OnWaitCondTimeout(5000);
    if(ret != 0x0)
    {
        MIXER_ERR("cmdId:%x. mutex wait timeout, err:%d. ret:%d\n", cmdId, errno, ret);
        return -1;
    }

    struct MixerMsgBuf *tmp = NULL;
    struct list_head *pHead = NULL, *ptmp = NULL;
    MsgMutex.OnLock();
    list_for_each_safe(pHead, ptmp, &MsgResultWorkList)
    {
        tmp = list_entry(pHead, struct MixerMsgBuf, MsgBufList);

        if(NULL != tmp)
        {
            //MIXER_DBG("tmp->cmd ID=%d.\n", tmp->cmdId);
            if(tmp->cmdId == cmdId)
            {
                list_del(&tmp->MsgBufList);
                if(NULL != param)
                    memcpy(param, tmp->paramBuf, paramLen > (MI_U32)tmp->paramLen ? (MI_U32)tmp->paramLen : paramLen);
                list_add_tail(&tmp->MsgBufList, &MsgResultEmptyList);
                MsgMutex.OnUnLock();
                return 0;
            }

        }
    }
    MIXER_ERR("can not fine cmd in worklist, cmd ID=%d.\n", cmdId);
    MsgMutex.OnUnLock();
    return -1;
}

MyMutexCond & CMsgQueueManager::GetMsgMutex()
{
    return MsgMutex;
}

MyMutexCond * CMsgQueueManager::GetResultMsgMutex(MI_U32 CmdType)
{
    MI_U32 _type = CmdType >> 12;

    if(_type >= (MIXER_CMD_TYPE>>12))
        return NULL;

    return &MsgResultMutex[_type];
}


MI_S32 CMsgQueueManager::Init()
{
    MI_S32 ret =0x0;
    MI_U32 j = 0x0;

    if(NULL== pMixerMsgBufList)
    {
        if(NULL == (pMixerMsgBufList = new  struct MixerMsgBuf[MsgBufListCnt]))
        {
            MIXER_ERR("can not alloc msgbuf. err\n");
            goto exit;
        }
    }

    if(NULL== pMixerMsgResultBufList)
    {
        if(NULL == (pMixerMsgResultBufList = new  struct MixerMsgBuf[MsgBufListCnt]))
        {
            MIXER_ERR("can not alloc msgbuf. err\n");
            goto exit;
        }
    }

    for(j=0x0; j<MsgBufListCnt; j++)
    {
        INIT_LIST_HEAD(&pMixerMsgBufList[j].MsgBufList);
        list_add_tail(&pMixerMsgBufList[j].MsgBufList, &MsgEmptyList);

        INIT_LIST_HEAD(&pMixerMsgResultBufList[j].MsgBufList);
        list_add_tail(&pMixerMsgResultBufList[j].MsgBufList, &MsgResultEmptyList);
    }

    struct sched_param tSched;
    pthread_attr_t tAttr;

    pthread_attr_init(&tAttr);
    pthread_attr_getschedparam(&tAttr, &tSched);
    pthread_attr_setinheritsched(&tAttr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&tAttr, SCHED_RR);

    {
        tSched.sched_priority = 95;
    }

        pthread_attr_setschedparam(&tAttr, &tSched);

    ret = pthread_create(&pthread_message, &tAttr, Msg_Task, this);
    if(MI_SUCCESS != ret)
    {
        MIXER_ERR("can not creat msg thread. err\n");
        goto exit;
    }

    pthread_setname_np(pthread_message, "Msg_Task");

    return 0;

exit:
    if(pMixerMsgBufList)
    {
        delete []pMixerMsgBufList;
        pMixerMsgBufList =NULL;
    }

    if(pMixerMsgResultBufList)
    {
        delete []pMixerMsgResultBufList;
        pMixerMsgResultBufList =NULL;
    }
    return -1;
}

MI_S32 CMsgQueueManager::UnInit()
{
    mixer_send_cmd(CMD_SYSTEM_EXIT_QUEUE, NULL, 0);

    pthread_join(pthread_message, NULL);

    if(pMixerMsgBufList)
    {
        delete pMixerMsgBufList;
        pMixerMsgBufList = NULL;
    }


    INIT_LIST_HEAD(&MsgEmptyList);
    INIT_LIST_HEAD(&MsgWorkList);

    if(pMixerMsgResultBufList)
    {
        delete pMixerMsgResultBufList;
        pMixerMsgResultBufList = NULL;
    }


    INIT_LIST_HEAD(&MsgResultEmptyList);
    INIT_LIST_HEAD(&MsgResultWorkList);

    return 0;
}


int mixer_send_cmd(MixerCmdId cmdId, MI_S8 *param, MI_U32 paramLen)
{
    return g_CMsgManager->OnSendMsg(cmdId, param, paramLen, 0);
}

int mixer_return_cmd_result(MixerCmdId cmdId, MI_S8 *param, MI_U32 paramLen)
{
    return g_CMsgManager->OnSendMsg(cmdId, param, paramLen, 1);
}

int mixer_wait_cmdresult(MixerCmdId cmdId, void *param, MI_U32 paramLen)
{
    return g_CMsgManager->OnWaitMsgResult(cmdId, param, paramLen);
}

