/*
* mid_fdfr.cpp- Sigmastar
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


#if TARGET_CHIP_I5

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#include "mid_utils.h"
#include "mid_common.h"
#include "module_common.h"
//#include "ssnn.h"
#include "mi_fd.h"
#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_divp_datatype.h"

#ifndef BUILD_UCLIBC_PROG
extern MI_S32 g_ieWidth;
extern MI_S32 g_ieHeight;
extern int g_ieFrameInterval;

BOOL g_FD_InitDone;
static BOOL g_FdExit = FALSE;
static pthread_t g_pthreadFdFr;
static FD_HANDLE FD_HANDLE_0;
static FR_HANDLE FR_HANDLE_0;
extern int FDtoRECT(int chn, int FaceNum_Detected, FacePos_t* face_position, FRFaceInfo_t*  frInfo, int show);

int mid_fdfr_OsCounterGetMs(void)
{
    struct timespec t1;

    clock_gettime(CLOCK_MONOTONIC, &t1);
    unsigned int T = (1000000 * (t1.tv_sec) + (t1.tv_nsec) / 1000) / 1000 ;
    return T;
}

float mid_fdfr_AVERAGE_FD_RUN(int a)
{
    static unsigned int num = 0;
    static float avg = 0;

    if((a > 0) && (a < 2000))
    {
        if(num == 0)
            avg = 0;

        num++;
        avg = avg + (a - avg) / num;
    }

    return avg;
}

void mid_fdfr_FrLoadDatabase(char *db_file_name)
{
    FILE *db_file = 0;
    char feature_name[128];
    MI_S32  feature_size = MI_FR_GetFeatureSizes();
    int  feature_index = 0 , ret_size;
    MI_RET  ret = MI_FDFR_RET_SUCCESS ;

    MI_S8 *feature;

    do
    {
        feature = (MI_S8*)malloc(feature_size) ;

        if(feature == NULL)
        {
            printf("FR_LOAD_DB malloc %d failed\n", feature_size);
            break;
        }

        db_file = fopen(db_file_name, "rb") ;

        if(db_file == NULL)
        {
            printf("FR_LOAD_DB open %s failed\n", db_file_name);
            break;
        }

        do
        {
            ret_size = fread(feature_name, 1, sizeof(feature_name), db_file);

            if(ret_size != sizeof(feature_name))
            {
                break;
            }

            ret_size = fread(feature, 1, feature_size, db_file);

            if(ret_size != feature_size)
            {
                break;
            }

            printf("Id[%d] : %s,size:%d\n", feature_index, feature_name, feature_size);
            ret = MI_FR_SetFeatureData(feature_index, feature, (MI_S8*)feature_name);
            printf("MI_FR_SetFeatureData ret=%d\n", ret);
            feature_index++;
        }
        while(1);

        printf("Get %d faces from db:%s\n", feature_index, db_file_name);
    }
    while(0);

    if(feature)
    {
        free(feature);
    }

    if(db_file)
    {
        fclose(db_file);
    }
}


void* mid_fdfr_Task(void *argu)
{
    unsigned int T0, T1;
    clock_t tx[2];
    int i = 0, x = 0;
    FacePos_t*  face_position = NULL;
    FacePos_t   facePosArr[10] = {0,};
    int FaceNum_Detected = 0;
    FRFaceInfo_t frInfo[10];
    int Face_Recognize_En = 0;
    int nFD_option_mode_count = 0;
    int fd_cls_cnt = 0;
    int fd_cls_flag = 0;

    memset(frInfo, 0, sizeof(frInfo));

    FD_HANDLE_0 = MI_FD_Init(g_ieWidth, g_ieHeight);
    if(FD_HANDLE_0 == NULL)
    {
        printf("FD initial error width=%d,height=%d\n", g_ieWidth, g_ieHeight);
        return NULL;
    }

    MI_FD_EnableFD(FD_HANDLE_0, 1);
    printf("FD Option %d\n", (int)argu);
    if((int)argu == 1)
        MI_FD_SetOption(FD_HANDLE_0, FD_OPTION_DETECT_MODE, FULL_MODE);

    if((int)argu == 2)
        MI_FD_SetOption(FD_HANDLE_0, FD_OPTION_DETECT_MODE, TRACK_MODE);

    if((int)argu == 3)
        MI_FD_SetOption(FD_HANDLE_0, FD_OPTION_DETECT_MODE, PARTIAL_MODE);

    if((int)argu == 4)
    {
        MI_FD_SetOption(FD_HANDLE_0, FD_OPTION_DETECT_MODE, FULL_MODE);
        Face_Recognize_En = 1;
    }

    if((int)argu == 5)
    {
        MI_FD_SetOption(FD_HANDLE_0, FD_OPTION_DETECT_MODE, TRACK_MODE);
        Face_Recognize_En = 1;
    }

    if((int)argu == 6)
    {
        MI_FD_SetOption(FD_HANDLE_0, FD_OPTION_DETECT_MODE, PARTIAL_MODE);
        Face_Recognize_En = 1;
    }

    if((int)argu == 7)
    {
        MI_FD_SetOption(FD_HANDLE_0, FD_OPTION_FACE_DIRECTION, 14);
    }

    MI_FD_SetOption(FD_HANDLE_0, FD_OPTION_FACE_WIDTH, 20);
    MI_FD_SetOption(FD_HANDLE_0, FD_OPTION_PARTIAL_WIDTH, 40);
    g_FD_InitDone = TRUE;

    /*===================Face Recognition=======================*/
    if(Face_Recognize_En > 0)
    {
        FR_HANDLE_0 = MI_FR_Init(g_ieWidth, g_ieHeight);
        if(FR_HANDLE_0 == NULL)
        {
            printf("FR initial error \n");
            return NULL;
        }

        MI_FR_EnableFR(FR_HANDLE_0, 1);
        //FR_CreateDatebase("fr.dat");//todo
        mid_fdfr_FrLoadDatabase((char*)"fr.dat");
    }

    /*============================================================*/
    MI_SYS_ChnPort_t stDivpChnOutputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    stDivpChnOutputPort.eModId   = E_MI_MODULE_ID_DIVP;
    stDivpChnOutputPort.u32ChnId = MIXER_DIVP_CHNID_FOR_VDF;
    stDivpChnOutputPort.u32DevId = 0;
    stDivpChnOutputPort.u32PortId = 0;

    while(g_FdExit == FALSE)
    {
        if(nFD_option_mode_count % 3 == 0)
        {
            nFD_option_mode_count = 0;
            MI_FD_SetOption(FD_HANDLE_0, FD_OPTION_DETECT_MODE, FULL_MODE);
        }
        else
        {
            MI_FD_SetOption(FD_HANDLE_0, FD_OPTION_DETECT_MODE, TRACK_MODE);
        }

        nFD_option_mode_count++;

        if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stDivpChnOutputPort , &stBufInfo, &hHandle))
        {
            usleep(20*1000);
            continue;
        }
        //printf("stBufInfo.stFrameData.u32BufSize=%d\n",stBufInfo.stFrameData.u32BufSize);
        tx[0] = clock();
        T0 = (long)mid_fdfr_OsCounterGetMs();
        MI_FD_Run(FD_HANDLE_0, (MI_U8 *)stBufInfo.stFrameData.pVirAddr[0]);
        T1 = (long)mid_fdfr_OsCounterGetMs();
        tx[1] = clock();
        MI_SYS_ChnOutputPortPutBuf(hHandle);

        printf(" FDFR:  time dur   FDFR: %lu ms, AVG is %.2f ms\n", (long)(T1 - T0), mid_fdfr_AVERAGE_FD_RUN(T1 - T0));
        FaceNum_Detected = MI_FD_GetFaceInfo(FD_HANDLE_0, &face_position);
        if(FaceNum_Detected > 0){
            memcpy(facePosArr, face_position, sizeof(FacePos_t)*FaceNum_Detected);
        }

        if(FaceNum_Detected > 0)
        {
            printf("--Time:%d\n", (unsigned int)(tx[1] - tx[0]));
            printf("F%d-FaceNum_Detected=%d \n", i, FaceNum_Detected);
        }

        for(x = 0; x < FaceNum_Detected; x++)
        {
            printf("j=%d, left=%d, top=%d, width=%d height=%d\n", x, face_position[x].posx,
                     face_position[x].posy,
                     face_position[x].posw,
                     face_position[x].posh);
        }

        /*===================Face Recognition=======================*/
        if(Face_Recognize_En > 0)
        {
            MI_FR_GetFRInfo(FR_HANDLE_0, FaceNum_Detected, frInfo);

            for(x = 0; x < FaceNum_Detected; x++)
            {
                if(frInfo[x].store_idx == -1)
                {
                    printf("FR: face not recognition\n");
                }
                else
                {
                    printf("FR: face recognition:index=%d  strore_idx=%d percentage=%d name=%s \n", x, frInfo[x].store_idx, frInfo[x].percentage, frInfo[x].name);
                }
            }
        }

        /*============================================================*/

        if(FaceNum_Detected > 0)
        {
            FDtoRECT(0, FaceNum_Detected, facePosArr, frInfo, 1);
            fd_cls_cnt = 0;
            fd_cls_flag = 1;
        }else{
            fd_cls_cnt ++;
        }

        if(fd_cls_flag && fd_cls_cnt/g_ieFrameInterval > 5)
        {
            FDtoRECT(0, FaceNum_Detected, facePosArr, frInfo, 0);
            fd_cls_flag = 0;
        }

    }

    MI_FR_EnableFR(FR_HANDLE_0, 0);
    MI_FR_Uninit(FR_HANDLE_0);

    MI_FD_EnableFD(FD_HANDLE_0, 0);
    MI_FD_Uninit(FD_HANDLE_0);

    g_FD_InitDone = FALSE;
    return NULL;
}

int SetSystemTime(char *dt)
{
    struct tm _tm;
    struct timeval tv;
    time_t timep;

    sscanf(dt, "%d-%d-%d %d:%d:%d", &_tm.tm_year,
        &_tm.tm_mon, &_tm.tm_mday,&_tm.tm_hour,
        &_tm.tm_min, &_tm.tm_sec);

    _tm.tm_mon = _tm.tm_mon - 1;
    _tm.tm_year = _tm.tm_year - 1900;

    timep = mktime(&_tm);
    tv.tv_sec = timep;
    tv.tv_usec = 0;
    if(settimeofday(&tv, (struct timezone *) 0) < 0)
    {
        printf("Set system datatime error!/n");
        return -1;
    }
    return 0;
}

int mid_fdfr_Initial(int param)
{
    g_FdExit = FALSE;
    SetSystemTime((char *)"2018-11-08 10:50:00");
    MI_S32 ret = pthread_create(&g_pthreadFdFr, NULL, mid_fdfr_Task, (void *)param);
    if(0 == ret)
       {
         pthread_setname_np(g_pthreadFdFr , "FdFr_Task");
    }
    else
    {
      printf("mid_fdfr_Initial, g_pthreadFdFr=%ld is err[%d]",g_pthreadFdFr,ret);
    }
    return 0;
}

int mid_fdfr_Uninitial(void)
{
    g_FdExit = TRUE;
    pthread_join(g_pthreadFdFr, NULL);
    printf("[%s] thread join-\n", __FUNCTION__);
    return 0;
}
#endif
#endif