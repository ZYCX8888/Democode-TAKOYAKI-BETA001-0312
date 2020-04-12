/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  RawStorage.cpp
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

#include "RawStorage.h"
#include "mid_common.h"
#include "SStarFs.h"
#include "mid_utils.h"
#include "MmcManager.h"

CRawStorage::CRawStorage(MI_U8 ch, const MI_U8 *path)
{
    nChannel = ch;
    mbOpened = FALSE;

    memset(mLastFileName, 0x0, sizeof(mLastFileName));
    memset(mfilename, 0x0, sizeof(mfilename));
    memset(mStoragePathName,0x00,sizeof(mStoragePathName));
    memset(&mStarttime,0x00,sizeof(SYSTEM_TIME));
    if(path != NULL && 0 != strcmp((const char*)path, ""))
    {
        strcpy(mStoragePathName, (const char *)path);
    }
    else
    {
        strcpy(mStoragePathName,STORAGE_PATH);
    }
    printf("CRawStorage::CRawStorage : mStoragePathName=%s\n",mStoragePathName);
}

CRawStorage::~CRawStorage()
{

}

MI_BOOL CRawStorage::CreateStorage(const MI_U8* name)
{
    if(IsOpened() == TRUE)    //目前有MP4 文件在写，没有被close
        return FALSE;

    //char type;
    MI_U32 count = 0x0;
    MI_S32 ret = 0x0l;

    SystemGetCurrentTime(&mStarttime);

    sprintf(path, "%s/""AV_%04d-%02d-%02d", mStoragePathName, \
                                                mStarttime.year,\
                                                mStarttime.month,\
                                                mStarttime.day);

    ret = access(path, F_OK);
    if(ret)
    {
        char chTemp[256] = { 0 };
        sprintf(chTemp, "mkdir -p %s", path);
        system(chTemp);
        //mkdir(path, 777);
    }

    do{
        memset(mLastFileName, 0x0, sizeof(mLastFileName));
        memset(mfilename, 0x0, sizeof(mfilename));


        sprintf(mLastFileName, "%04d%02d%02d%02d%02d%02d-ch%d-%d.%s",
                mStarttime.year,mStarttime.month,mStarttime.day,
                mStarttime.hour,mStarttime.minute,mStarttime.second, \
                nChannel, count, NULL == name ? "null": (const char *)name);

        sprintf(mfilename, "%s/%s", path, mLastFileName);

        ret = access(mfilename, F_OK);
        if(ret)
        {
            break;
        }
        count++;
    }while(!ret);

    MIXER_DBG("filename:%s\n", mfilename);

    mbOpened = mRawFile.CreateRawFile(mfilename);

    return mbOpened;
}

MI_BOOL CRawStorage::CloseStorage()
{
    if(FALSE == mbOpened)
    {
        MIXER_ERR("CMp4Storage::CloseStorage failed! file not opened\n");
        return FALSE;
    }

    if(FALSE == mRawFile.CloseRawFile())
    {
        return FALSE;
    }

    mbOpened = FALSE;

    if(TRUE == mRawFile.GetNullFileFlag())
    {
        if(0!=remove(mfilename))
        {
			MIXER_ERR("CMp4Storage::CloseStorage remove file %s failed!\n",mfilename);
		}
	
    }

    //sync files to storage

    //end

    return TRUE;
}

MI_BOOL CRawStorage::WriteStorage(void* pBuffer, MI_U8 StreamType, MI_U8 Frametype, MI_U32 dwCount)
{
    MI_U8 type = Frametype;

    if(!mbOpened)
    {
        MIXER_ERR("CRawStorage::WriteStorage failed! file not opened\n");
        return FALSE;
    }

    return mRawFile.WriteData(pBuffer,StreamType,  type, dwCount);
}

MI_U32 CRawStorage::GetLength()
{
    if(!mbOpened)
    {
        printf("CRawStorage::GetLength failed! file not opened\n");
        return 0;
    }

    return mRawFile.GetWriteSize();
}

MI_BOOL CRawStorage::IsOpened()
{
    MI_BOOL Tmp;

    do{
        Tmp = mbOpened;
    }while(Tmp != mbOpened);

    return Tmp;
}

MI_BOOL CRawStorage::IsDiskFull()
{
    MI_BOOL bFull = FALSE;

    if(mbOpened)
    {
        #if USE_SD_STORAGE
        CARD_STORAGE_DEVICE_INFO cardDevInfo;
        g_MmcManager->GetCardDevInfo(cardDevInfo);

        bFull = (cardDevInfo.RemainSpace <= REMAINSPACE) ? TRUE : FALSE;
        #endif
    }
    else
    {
        #if USE_SD_STORAGE
        MI_S32 disk_stat;
        g_MmcManager->GetStat(disk_stat);

        bFull = (FS_NO_ERROR != disk_stat) ? TRUE : FALSE;
        #endif
    }

    return bFull;
}

const char * CRawStorage::GetLastFileName(void)
{
    return mLastFileName;
}

void CRawStorage::GetRecStartTime(SYSTEM_TIME & time)
{
    time = mStarttime;
}
