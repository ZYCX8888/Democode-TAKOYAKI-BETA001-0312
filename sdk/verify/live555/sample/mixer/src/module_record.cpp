/*************************************************
*
* Copyright (c) 2019-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  module_record.cpp
* Author:     fisher.yang@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2019/8/2
* Description: mixer record source file
*
*
*
* History:
*
*    1. Date  :        2018/8/2
*       Author:        fisher.yang@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/


#include "module_record.h"
#include "SStarFs.h"
#include "mi_common_datatype.h"
#include "Record.h"
#include "MmcManager.h"
#include "mid_system_config.h"
extern char g_SD_mountPath[1024];
extern MI_U32 g_videoNumber;
extern MI_VideoEncoder *g_videoEncoderArray[MAX_VIDEO_NUMBER];

CRecordManager::CRecordManager()
{
    MI_S32 i = 0x0;
    bThreadOpen = FALSE;
    m_nChannelNum = 0x0;
    storage = 0x00;
    m_RecordFrameThread = (pthread_t)(-1);
    //memset((void *)m_nRecord_event, 0x0, sizeof(m_nRecord_event));
    m_nRecord_event = DISK_EVENT_INIT;
    for(i=0x0; i<REC_MAX_CHN_NUM; i++)
    {
        pRecord[i] = NULL;
        bRecOpen[i] = FALSE;
    }
}

CRecordManager::~CRecordManager()
{

}

extern int sstar_mmc_file_sys_free_oldest_file();
void *RecManagerThread(void *argv)
{
#if 1
    //SYSTEM_TIME current_time;
    MI_U32 Record_lastevent = 0x00;
    MI_S32 mmcState = 0x0;

    MI_U32 n = 0x0;

#endif

    CRecordManager * pCRecordManager = (CRecordManager *)argv;
    if(NULL == pCRecordManager)
    {
        MIXER_ERR("param err\n");
        return NULL;
    }


    while(TRUE == pCRecordManager->GetThreadOpenFlag())
    {
        if(g_MmcManager->bRecInMMC == TRUE)
        {
            //pthread_mutex_lock(&pCRecordManager->m_mutex);

            //get sd card state
            mmcState = pCRecordManager->GetStorageState();
            if(FS_NO_SPACE == mmcState)
            {
                //here need to protect no venc chn during stopVideoEncoder AND startVideoEncoder
                //for(n = 0; n < pCRecordManager->m_nChannelNum; n++)
                //{
                    Record_lastevent = pCRecordManager->m_nRecord_event;
                    pCRecordManager->m_nRecord_event |= DISK_EVENT_NO_SPACE;

                    if(DISK_EVENT_ABNORMAL != Record_lastevent)
                    {
                        MI_U64 RM_size=0UL;
                        MI_U64 RM_total_size=0UL;
                        for(n = 0; n < pCRecordManager->m_nChannelNum; n++)
                            if(pCRecordManager->pRecord[n])
                                pCRecordManager->pRecord[n]->Pause();
                        MIXER_DBG("covering start\n");
                        /******************** covering start **********************/
                        do
                        {
                            if((RM_size = sstar_mmc_file_sys_free_oldest_file()) < (int)0)
                            {
                                MIXER_ERR("during sdcard covering : maybe sdcard is disconnect!!!\n");
                                break;
                            }

                            RM_total_size += RM_size;
                        }while(RM_total_size < CLEANOUTSPACE);

                        /******************** covering finish **********************/
                        MIXER_DBG("covering finish\n");
                        g_MmcManager->CUpdateState();
                    }
                //}

                //pthread_mutex_unlock(&pCRecordManager->m_mutex);
                MySystemDelay(10);
                continue;
            }
            else if(FS_NO_ERROR != mmcState)
            {
                //for(n = 0; n < pCRecordManager->m_nChannelNum; n++)
                {
                    Record_lastevent = pCRecordManager->m_nRecord_event;
                    pCRecordManager->m_nRecord_event |= DISK_EVENT_ABNORMAL;

                    if(DISK_EVENT_ABNORMAL != Record_lastevent && DISK_EVENT_NO_SPACE!=Record_lastevent)
                    {
                        for(n = 0; n < pCRecordManager->m_nChannelNum; n++)
                            if(pCRecordManager->pRecord[n])
                                pCRecordManager->pRecord[n]->Pause();
                    }
                }

                //pthread_mutex_unlock(&pCRecordManager->m_mutex);
                MySystemDelay(10);
                continue;
            }
            else     // state == no_error
            {
                //for(n = 0; n < pCRecordManager->m_nChannelNum; n++)
                {
                    Record_lastevent = pCRecordManager->m_nRecord_event;
                    pCRecordManager->m_nRecord_event = DISK_EVENT_NORMAL;

                    if(DISK_EVENT_NORMAL != Record_lastevent)
                    {
                        for(n = 0; n < pCRecordManager->m_nChannelNum; n++)
                            if(pCRecordManager->pRecord[n])
                                pCRecordManager->pRecord[n]->Resume();
                    }
                }
            }
        }



        //sd no err, then do record
        //SystemGetCurrentTime(&current_time);
        for(n= 0; n < pCRecordManager->m_nChannelNum; n++)
        {
            MI_S32 ret =0;
            if(NULL == pCRecordManager->pRecord[n])
            {
                MIXER_ERR("m_pRecord[%d] is null!\n", n);
                continue;
            }
            ret = pCRecordManager->pRecord[n]->DoRecord();
            if(ret == REC_ERR_CLOSE || ret == REC_ERR_OPEN || ret == REC_ERR_WRITE || ret == REC_ERR_DISKFULL )
            {
                MIXER_ERR("during sdcard covering : maybe sdcard is disconnect Or diskfull!!!\n");
                g_MmcManager->CUpdateState();
            }
        }

        //pthread_mutex_unlock(&pCRecordManager->m_mutex);
        MySystemDelay(20);

    }

    MIXER_DBG("RecordManager thread exit.\n");
    return NULL;
}

MI_U32 CRecordManager::GetStorageState()
{
    MI_S32 state = 0x0;
    if(StorageSD == storage)
    {
        g_MmcManager->GetStat(state);
    }
    else if(StorageUSB == storage)
    {

    }
    else
    {
        state = FS_OK;
    }

    return (MI_U32)state;
}
extern int sstar_mmc_find_SD_mountPath();

MI_S32 CRecordManager::Start()
{
    MI_U32 i = 0x0;

    MIXER_INFO("here is CRecordManager Start\n");

    memset(path, 0x0, sizeof(path));

    //read cfg
    ModuleRecInfo_s *pModuleRecInfo_t = NULL;
    pModuleRecInfo_t = GetRecInfo();
    if(NULL != pModuleRecInfo_t)
    {
        storage = pModuleRecInfo_t->Storage;
        memcpy(path, pModuleRecInfo_t->RecPath, MIN(sizeof(path),sizeof(pModuleRecInfo_t->RecPath)));
        if(sstar_mmc_find_SD_mountPath()==0)
        {
            if(strncmp((const char *)path,(const char *)g_SD_mountPath,MIN(sizeof(path),sizeof(g_SD_mountPath)) ) == 0)
            {
                MIXER_DBG("SD mountPath %s same as storage Path %s \n",g_SD_mountPath,path);
                g_MmcManager->bRecInMMC = TRUE;
            }
            else
            {
                MIXER_DBG("SD mountPath %s different from storage Path %s \n",g_SD_mountPath,path);
                g_MmcManager->bRecInMMC = FALSE;
            }
        }
        else
        {
            MIXER_DBG("find SD mountPath failed,maybe there is not SDcard!\n");
            g_MmcManager->bRecInMMC = FALSE;
        }
    }
    else
    {
        if(sstar_mmc_find_SD_mountPath()==0)
        {
            memcpy(path, g_SD_mountPath, MIN(sizeof(path),sizeof(g_SD_mountPath)));
            MIXER_DBG("GetRecInfo() is NULL, use SD mountPath %s as storage Path \n",g_SD_mountPath);
            g_MmcManager->bRecInMMC = TRUE;
        }
        else
        {
            MIXER_DBG("GetRecInfo() is NULL, and find SD mountPath failed \n");
            g_MmcManager->bRecInMMC = FALSE;
        }
    }

    MIXER_DBG("rec path: %s. storage:%d\n", path, storage);
    //end

    // create record
    if(0x0 == g_videoNumber || REC_MAX_CHN_NUM < g_videoNumber)
    {
        MIXER_ERR("err, record managet must be called after video open\n");
        return -1;
    }
    for(i = 0; i < g_videoNumber; i++)
    {
        pRecord[i] = new CRecord(i, g_videoEncoderArray[i], path);
    }
    // end

    m_nChannelNum  = g_videoNumber;

    // create record thread
    MI_S32 s32Ret = 0x0;

    bThreadOpen = TRUE;
     s32Ret = pthread_create(&m_RecordFrameThread, NULL, RecManagerThread, (void *)this);
     if(0 == s32Ret)
        {
            pthread_setname_np(m_RecordFrameThread, "RecManThread");
        }
        else
        {
            MIXER_ERR("%s  pthread_create RecManagerThread thread failed\n", __func__);
            bThreadOpen = FALSE;
        }

    if(TRUE == bThreadOpen)
    {
        MI_S32 fs_state = 0;
        g_MmcManager->Start();
        g_MmcManager->CUpdateState();
        g_MmcManager->GetStat(fs_state);
        switch(fs_state)
        {
            case FS_OK:
                m_nRecord_event=DISK_EVENT_NORMAL;
                break;
            case FS_NO_SPACE:
                m_nRecord_event=DISK_EVENT_NO_SPACE;
                break;
            case FS_NO_FIND:
            case FS_NO_MOUNT:
            case FS_NO_FORMAT:
                m_nRecord_event=DISK_EVENT_ABNORMAL;
                break;
            default:
                MIXER_ERR("Unknow fs_state %d\n",fs_state);
                break;
        }
    }

    return s32Ret;
}

MI_S32 CRecordManager::Stop()
{
    MI_U8 i = 0x0;
    MIXER_DBG("here is CRecordManager Stop\n");
    //destroy record thread
    if(TRUE == bThreadOpen)
    {
        g_MmcManager->Stop();

        bThreadOpen = FALSE;
        pthread_join(m_RecordFrameThread, NULL);
    }
    //destory thread
    for(i = 0; i < REC_MAX_CHN_NUM; i++)
    {
        if(pRecord[i])
        {
            pRecord[i]->Stop(REC_MODE_MAN);
            pRecord[i]->UnInit();
            delete pRecord[i];
            pRecord[i] = NULL;
        }
    }
    return MI_SUCCESS;
}

MI_S32 CRecordManager::Pause(MI_U32 ch)
{
    MIXER_DBG("here is CRecordManager Pause\n");
    if(ch >= REC_MAX_CHN_NUM )
    {
        MIXER_ERR("param err: ch(%d)\n", ch);
        return -1;
    }

    //m_nRecord_event |= DISK_EVENT_STOP_MANUAL;

    if(NULL != pRecord[ch])
        pRecord[ch]->Pause();

    return 0;
}



MI_S32 CRecordManager::Resume(MI_U32 ch)
{
    MIXER_DBG("here is CRecordManager Resume\n");
    if(ch >= REC_MAX_CHN_NUM )
    {
        MIXER_ERR("param err: ch(%d)\n", ch);
        return -1;
    }
    //m_nRecord_event &= ~DISK_EVENT_STOP_MANUAL;

    if(NULL != pRecord[ch])
        pRecord[ch]->Resume();

    return 0;
}

MI_BOOL CRecordManager::GetThreadOpenFlag()
{
    return bThreadOpen;
}

CRecordManager *CRecordManager::instance()
{
    static CRecordManager _instance;

    return &_instance;
}


int rec_process_cmd(MixerCmdId id, MI_S8 *param, MI_S32 paramLen)
{
    switch(id)
    {
        case CMD_RECORD_ENABLE:
        {

            g_RecordManager->Start();

        }
            break;

        case CMD_RECORD_DISABLE:
            {
            g_RecordManager->Stop();
        }
            break;

        case CMD_RECORD_SETMODE:
        {
            MI_U32 _param[2]={0x0};
            MI_U32 ch = 0x0;
            memcpy(_param, param, paramLen);

            ch = _param[0];
            MIXER_DBG("ch(%d), param[1](%d)\n", ch, _param[1] );
            if(ch < REC_MAX_CHN_NUM && g_RecordManager->pRecord[ch])
            {
                if(0x0 == _param[1] && g_RecordManager->bRecOpen[ch] == TRUE)    //stop record
                {
                    g_RecordManager->bRecOpen[ch] = FALSE;
                    if(0 <= g_RecordManager->pRecord[ch]->Stop(REC_MODE_MAN))
                    {

                    }
                    else
                    {
                        MIXER_ERR("CMD_RECORD_SETMODE Fail:may be DISK_EVENT_ABNORMAL or DISK_EVENT_NO_SPACE");
                        //do nothing
                    }
                }
                else if(0x01 == _param[1] && g_RecordManager->bRecOpen[ch] == FALSE)        //start record
                {
                    if(0 <= g_RecordManager->pRecord[ch]->Start(REC_MODE_MAN))
                    {
                        g_RecordManager->bRecOpen[ch] = TRUE;
                    }
                    else
                    {
                        MIXER_ERR("CMD_RECORD_SETMODE Fail:may be DISK_EVENT_ABNORMAL or DISK_EVENT_NO_SPACE");
                    }
                }
            }

        }
            break;

    case CMD_RECORD_GETMODE:
        {
            MI_U32 _param[2]={0x0};
            MI_U32 ch = 0x0;
            memcpy(_param, param, paramLen);

            ch = _param[0];

            if(ch < REC_MAX_CHN_NUM && g_RecordManager->pRecord[ch])
            {
                g_RecordManager->pRecord[ch]->GetCurMode();
            }
        }
        break;

        default:
            break;
    }

    return 0;
}

