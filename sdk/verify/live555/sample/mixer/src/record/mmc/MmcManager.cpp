/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  MmcManager.cpp
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

#include "MmcManager.h"
#include "mid_common.h"
#include "mi_common_datatype.h"
#include "mid_utils.h"
#include "SStarFs.h"

#include <sys/mount.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>
#include <errno.h>
char g_SD_mountPath[96];

int scan_mmc_info(const char *fname, MMCInfo *info, int maxnum)
{
    char line[64] = {0x0};
    char name[64] = {0x0};
    FILE *fp = NULL;
    char *c = NULL, *p = NULL;
    int dev_num = 0;
    int part;

    fp = fopen(fname, "r");
    if (fp == NULL)
    {
        printf("fopen :%s\n",fname);
        return 0;
    }

    while (fgets(line, sizeof(line), fp))
    {
        c = strstr(line,"mmcblk");
        if (c)
        {
            //sscanf(line, "%d %d %d", &(info[dev_num].major), &(info[dev_num].minor), &(info[dev_num].blocks));
            if (EOF == sscanf(line, "%d %d %d", &(info[dev_num].major), &(info[dev_num].minor), &(info[dev_num].blocks)) && info[dev_num].blocks < 512) /* Invalid blocks */
            {
                continue;
            }
            info[dev_num].mmcn = atoi(&c[6]);
            sprintf(info[dev_num].name,"mmcblk%d",info[dev_num].mmcn);
            sprintf(name,"mmcblk%dp",info[dev_num].mmcn);
            p = strstr(line,name);
            if (p)
            {
                part = atoi(&p[8]);
                info[dev_num].part = part;
            }
            else
            {
                info[dev_num].part = 0;
            }
        }
        else
        {
            continue;
        }
        dev_num++ ;
        if(dev_num >= maxnum)
        {
            break;
        }
    }

    fclose(fp);
    return dev_num;
} /* end scan_scsi */

MI_S32 scan_mmc_dev(MmcDev *info, MI_S32 num, MI_U32 type)
{
    int ret = 0;
    int dev_num = 0;

    if(type & MMC_TYPE)
    {
        MI_S32 k;
        MMCInfo mmc_info[2];
        memset(mmc_info, 0x0, sizeof(mmc_info));
        MI_S32 mmc_num = scan_mmc_info(PROC_PART_INFO, mmc_info, 2);
        ret+=mmc_num;

        //printf("==>find mmc %d\n",mmc_num);

        for(k = 0; ((k < mmc_num) && (dev_num < num)); k++)
        {
            info[dev_num].type = MMC_TYPE;
            info[dev_num].blocks = mmc_info[k].blocks;
            if(mmc_info[k].part == 0)
                sprintf(info[dev_num].name,"/dev/%s", mmc_info[k].name);
            else
                sprintf(info[dev_num].name,"/dev/%sp%d", mmc_info[k].name,mmc_info[k].part);
            dev_num ++;
        }

    }
    return ret;
}


MI_S32 sstar_mmc_file_sys_init(MmcDev *info, int num)
{
    return DISK_NO_ERROR;
}

int sstar_mmc_file_sys_release()
{
    //usb_umount("/nfs");

    return MI_SUCCESS;
}

pthread_mutex_t mmc_mutex = PTHREAD_MUTEX_INITIALIZER;

#if 0
int CheckMount(char * name, char * mountpoint)
{
    int ret = 0;
    FILE * pFile = fopen("/proc/mounts", "r");
    if(!pFile)
    {
        printf("open /proc/mounts failed!\n");
        return ret;
    }

    printf("----------------------------------------\n");
    printf("check /proc/mount\n");
    printf("----------------------------------------\n");
    char buf[1024] = {0x0};
    while((fgets(buf, 1024, pFile)) != NULL)
    {
        printf("%s", buf);
        if(strstr(buf, name) && strstr(buf, mountpoint))
        {
            ret = 1;
            break;
        }
    }
    printf("----------------------------------------\n");
    printf("check /proc/mount  end\n");
    printf("----------------------------------------\n");

    fclose(pFile);
    return ret;
}
#endif

int sstar_mmc_format()
{
#if 0
    int ret = 0;

    pthread_mutex_lock(&mmc_mutex);

    MmcDev info[2];
    memset(info,0,sizeof(MmcDev)*2);
    int num = scan_mmc_dev(info,2,MMC_TYPE);
    if(2 != num)
    {
        MIXER_ERR("scan mmc dev ret = %d\n", num);
        pthread_mutex_unlock(&mmc_mutex);
        return -1;
    }

    if(CheckMount(info[1].name, MMC_MOUNT_PATH))
    {
        int count = 0;
        while(count++ < 100)
        {
            ret = umount(MMC_MOUNT_PATH);
            if(ret < 0)
            {
                //printf("umount    mmc /nfs failed! ret = %d\n", ret);
                //perror ("umount:");
                MySystemDelay(50000);
                continue;
            }
            else
            {
                printf("umount    mmc %s success -- count = %d\n",MMC_MOUNT_PATH, count);

                break;
            }
        }
        if(count >= 100)
        {
            MIXER_ERR("format umount failed!!!!!!!!!!! ret = %d\n", ret);
            perror("umount failed:");
            pthread_mutex_unlock(&mmc_mutex);
            return -1;
        }

    }

    // don't finish

    pthread_mutex_unlock(&mmc_mutex);
    return ret;

#else
    return 0;
#endif

}

int sstar_mmc_find_SD_mountPath()
{
    int ret = -1;
    char caStdOutLine[1024];
    char* pcTmpSDPath = NULL;
    int i = 0;
    memset(g_SD_mountPath,0,sizeof(g_SD_mountPath));
    do
    {
        // 通过创建一个管道，调用 fork 产生一个子进程，
        // 执行一个 shell 以运行命令来开启一个进程。
        // 这个进程必须由 pclose() 函数关闭。
        FILE* fp = popen( "mount","r" ); // 一个指向以 NULL 结束的 shell 命令字符串的指针，
        // 这行命令将被传到 bin/sh 并使用 -c 标志，
        // 那么 shell 将执行这个命令从这个字符串中读取。
        // 文件指针连接到 shell 命令的标准输出

        if ( NULL == fp )
        {
            break;
        }

        while( NULL != fgets( caStdOutLine,sizeof( caStdOutLine ),fp ) ) // 注：管道中的数据一定要读完，不然会崩溃掉的
        {
            if ( NULL == strstr( caStdOutLine, "/dev/mmcblk0" ) || NULL == strstr( caStdOutLine, "/dev/mmcblk0p1" ) ) // 前面要有空格，以防断章取义
            {
                continue;
            }

            if ( NULL == strstr( caStdOutLine, " vfat " ) )  // 前后均有空格
            {
                continue;
            }
            pcTmpSDPath = strtok( caStdOutLine, " " );
            i=0;
			int iLen = 0;
            do
            {
                i++;
                if(i==3)
                {
                    printf("[%s %d] :find mount sd path %s\n",__func__,__LINE__,pcTmpSDPath);
					iLen = strlen(pcTmpSDPath) > sizeof(g_SD_mountPath) ?  sizeof(g_SD_mountPath) : strlen(pcTmpSDPath);
                    memcpy(g_SD_mountPath, pcTmpSDPath, iLen);
                    ret = 0;
                }

            }while((pcTmpSDPath = strtok( NULL, " " )));
        }

        // 关闭标准 I/O 流，等待命令执行结束，然后返回 shell 的终止状态。
        // 如果 shell 不能被执行，
        // 则 pclose() 返回的终止状态与 shell 已执行 exit 一样。
        pclose( fp );
    }while ( 0 );
    return ret;
}
int sstar_mmc_file_sys_detect()
{
    pthread_mutex_lock(&mmc_mutex);

    if(access("/dev/mmcblk0p1", F_OK)==0)
    {
        pthread_mutex_unlock(&mmc_mutex);
        return DISK_NO_ERROR;
    }
    else if(access("/dev/mmcblk0", F_OK)==0)
    {
        pthread_mutex_unlock(&mmc_mutex);
        return DISK_NO_FORMAT;
    }
    else
    {
        pthread_mutex_unlock(&mmc_mutex);
        return DISK_NO_FIND;
    }

}



MI_S32 sstar_mmc_file_sys_remount()
{
    MmcDev info[2];
    pthread_mutex_lock(&mmc_mutex);
    if(2 == scan_mmc_dev(info, 2, MMC_TYPE))
    {
        #if 0
        const char *fstype = "vfat";
        unsigned long  mountflg = 0;
        mountflg = MS_MGC_VAL  | MS_REMOUNT;

        if (mount(info[1].name, fstype, mountflg, NULL) == -1)
        {
            MIXER_ERR("can not mount sd\n");
            pthread_mutex_unlock(&mmc_mutex);
            return DISK_ERROR;
        }
        #endif
    }
    pthread_mutex_unlock(&mmc_mutex);
    return DISK_NO_ERROR;
}




int sstar_mmc_file_sys_trymount()
{
    MmcDev info[2];
    pthread_mutex_lock(&mmc_mutex);
    if(2 == scan_mmc_dev(info,2,MMC_TYPE))
    {
        /*if(__usb_mount("/nfs",info[1].name)<0)
        {
            pthread_mutex_unlock(&mmc_mutex);
            return DISK_ERROR;
        }*/

        pthread_mutex_unlock(&mmc_mutex);
        return DISK_NO_ERROR;
    }
    pthread_mutex_unlock(&mmc_mutex);
    return DISK_NO_FORMAT;
}

extern int errno;
static void SysTime2Second(SYSTEM_TIME *Systime,  time_t *ttime)
{
    struct tm tmptm;
    memset(&tmptm, 0, sizeof(struct tm));
    tmptm.tm_mday  = Systime->day ;
    tmptm.tm_hour  = Systime->hour;
    tmptm.tm_isdst = Systime->isdst;
    tmptm.tm_min   = Systime->minute;
    tmptm.tm_mon   = Systime->month-1;
    tmptm.tm_sec   = Systime->second;
    tmptm.tm_wday  = Systime->wday;
    tmptm.tm_year  = Systime->year-1900;

    *ttime = mktime(&tmptm);
}
//KB为单为
int sstar_mmc_file_sys_get_szie(MI_U64 *capability, volatile MI_U64 *remain)
{
    struct statfs sfs;
    long long  tmp = 0x0;
    int err = 0x0;
    int count = 0x0;

    memset(&sfs, 0x0, sizeof(struct statfs));
    if(0 == strcmp(g_SD_mountPath, ""))
    {
        MIXER_DBG("%s is Empty STRING!\n",g_SD_mountPath);
        return -1;
    }
    do{
        err = statfs(g_SD_mountPath, &sfs);
        if(err < 0)
        {
            //MIXER_ERR("mmc file sys %u %lu %lu -- err=%d\n", sfs.f_bsize, sfs.f_bavail, sfs.f_blocks, errno);
            MIXER_DBG("%s not find!\n",g_SD_mountPath);
        }
        else
        {
            break;
        }

        MySystemDelay(50);
        count++;
    }while(err<0 && count<1);

    if(err<0 && count>=1)
        return -1;

    if(sfs.f_bsize >= 1024)
    {
        tmp = (long long )sfs.f_bavail * ((long long )sfs.f_bsize/1024);
    }
    else
    {
        if(sfs.f_bsize==0L) tmp=0;    //avoid divide 0.
        else tmp = (long long)sfs.f_bavail * (1024/(long long )sfs.f_bsize);
    }

    *remain = tmp;

    MmcDev info[2];
    memset(info, 0x0, sizeof(info));
    scan_mmc_dev(info,2,MMC_TYPE);

    *capability = info[0].blocks;

    return 0;
}


int sstar_mmc_file_sys_free_oldest_file()
{
    MI_U64 oldest_fileSize = 0;
    DIR *mainDir = NULL;
    struct dirent *subDirent = NULL;
    MI_U32 subDir_count=0;
    char sMinAVDir[NAME_MAX + 1]={'\0'};
    char sMinRawFile[NAME_MAX + 1]={'\0'};
    char dirpath[NAME_MAX + 100]={'\0'};
    char filepath[2 * NAME_MAX + 2] = {'\0'};
    MI_U32 _year = 0, _month = 0, _day = 0;
    MI_U32 last_year = -1, last_month = -1, last_day = -1;
    //SYSTEM_TIME time;
    time_t t = 0;
    time_t last_min_t = 0;
    memset(sMinRawFile,0,sizeof(sMinRawFile));
    DO_FIND_DIR:
    if ((mainDir = opendir(g_SD_mountPath)) != NULL)
    {
        while((subDirent = readdir(mainDir)) != NULL)
        {
            MIXER_DBG("the dirent(%s)\n", subDirent->d_name);
            {
                if(strstr(subDirent->d_name, "AV_"))
                {
	                if(EOF == sscanf(subDirent->d_name, "AV_%04d-%02d-%02d", &_year, &_month, &_day))
	                    continue;
                    if(((MI_U32)-1 == last_year) && ((MI_U32)-1==last_month) && ((MI_U32)-1==last_day))
                    {
                        last_year = _year;
                        last_month = _month;
                        last_day = _day;

                        strcpy(sMinAVDir, subDirent->d_name);
                    }
                    else if(last_year > _year)
                    {
                        last_year = _year;
                        last_month = _month;
                        last_day = _day;

                        strcpy(sMinAVDir, subDirent->d_name);
                    }
                    else if(last_year <= _year)
                    {
                        if(last_month > _month)
                        {
                            last_year = _year;
                            last_month = _month;
                            last_day = _day;

                            strcpy(sMinAVDir, subDirent->d_name);
                        }
                        else
                        {
                            if(last_day > _day)
                            {
                                last_year = _year;
                                last_month = _month;
                                last_day = _day;
                                strcpy(sMinAVDir, subDirent->d_name);
                            }
                        }
                    }
                }
            }
            subDir_count ++;
        }

        closedir(mainDir);
    }
    else
    {
        MIXER_DBG("can not readdi(%s)\n",g_SD_mountPath);
        return -1;
    }

    mainDir = NULL;
    subDirent = NULL;

    MIXER_DBG("min AV_Y-M-D dir = %s\n", sMinAVDir);
    if(0 == strcmp(sMinAVDir, ""))
    {
        printf("no AV dir in %s\n",g_SD_mountPath);
        return 0;
    }
    //DO_FIND_FILE:
    sprintf(dirpath, "%s/%s",g_SD_mountPath, sMinAVDir);
    if ((mainDir = opendir(dirpath)) != NULL)
    {
        while((subDirent = readdir(mainDir)) != NULL)
        {
            {
                if(0 == strcmp(subDirent->d_name, ".") || 0 == strcmp(subDirent->d_name, ".."))
                {
                    continue;
                }
                else if(strstr(subDirent->d_name, "h264") || strstr(subDirent->d_name, "h265") || \
                    strstr(subDirent->d_name, "mjpeg") || strstr(subDirent->d_name, "yuv420") || \
                    strstr(subDirent->d_name, "jpg") || strstr(subDirent->d_name, "yuv422") )
                {
                    MI_U32 nChannel = 0;
                    MI_U32 count = 0x0;
                    MI_U8 coderName[10];
                    SYSTEM_TIME time;
                    memset(&time,0x00,sizeof(SYSTEM_TIME));
                    memset(coderName,0,sizeof(coderName));
					if(EOF == sscanf(subDirent->d_name, "%04d%02d%02d%02d%02d%02d-ch%d-%d.%s",
                            &time.year, &time.month, &time.day,
                            &time.hour, &time.minute, &time.second,
                            &nChannel, &count, coderName))
                            continue;
                    SysTime2Second(&time, &t);
                    if(0 == last_min_t)
                    {
                        last_min_t = t;
                        strcpy(sMinRawFile, subDirent->d_name);
                    }
                    else if(last_min_t >= t)
                    {
                        last_min_t = t;
                        strcpy(sMinRawFile, subDirent->d_name);
                    }
                }

            }
        }

        closedir(mainDir);
    }
    else
    {
        MIXER_DBG("can not readdir(%s)\n", dirpath);
        return -1;
    }
    if(0 == strcmp(sMinRawFile, ""))
    {
        MIXER_DBG("rm dir %s\n", dirpath);

        char cmd[NAME_MAX + 120]={0x0};
        sprintf(cmd, "rm %s -r", dirpath);
        system(cmd);

        goto DO_FIND_DIR;
    }

    //DO_REMOVE_FILE:
    strcpy(filepath, dirpath);
    strcat(filepath,"/");
    strcat(filepath, sMinRawFile);


    //get file size which will be remove
    {
        struct stat buf;
        if(stat(filepath,&buf) == 0)
            oldest_fileSize = buf.st_size / 1024;
    }
    MIXER_DBG("remove file %s,size %lluKB \n",filepath,oldest_fileSize);
    if(remove(filepath) < 0)return -1;
    return oldest_fileSize;
}



CMmcManager *CMmcManager::instance()
{
    static CMmcManager _instance;

    return &_instance;
}


CMmcManager::CMmcManager()
{
    pthread_mutex_init(&m_mutex, NULL);
    //pthread_cond_init(&m_cond, NULL);

    //m_dwFSError = DISK_NO_FIND;
    for(int i = 0; i < DRIVER_TYPE_NR; i++)
    {
        m_dwStat[i] = FS_OK;
    }
    m_dwFSError = 0;
    m_dwRemain = 0;
    m_dwCapability = 0;
    bRecInMMC = FALSE;
}

CMmcManager::~CMmcManager()
{
    pthread_mutex_destroy(&m_mutex);
    //pthread_cond_destroy(&m_cond);
}

void CMmcManager::Init()
{
    Start();
    UpdateState();
}

MI_BOOL CMmcManager::Start()
{
    // reg timer
    if (TimerCreate(onTimerCallBack, 60000) < 0)
    {
        MIXER_ERR("Create timer failed\n");
    }

    return TRUE;
}

MI_BOOL CMmcManager::Stop()
{
    if (TimerDestroy() < 0)
    {
        MIXER_ERR("Destory timer failed\n");
    }

    return TRUE;
}

MI_BOOL CMmcManager::GetStat(MI_S32 &stat, MI_U32 driver_type)
{
    pthread_mutex_lock(&m_mutex);
    stat = m_dwStat[driver_type];
    pthread_mutex_unlock(&m_mutex);
    return TRUE;
}

MI_BOOL CMmcManager::GetCardDevInfo(CARD_STORAGE_DEVICE_INFO &cardDevInfo)
{
    cardDevInfo.RemainSpace = m_dwRemain;
    cardDevInfo.TotalSpace = m_dwCapability;

    return TRUE;
}

MI_BOOL CMmcManager::FormatDisk(MI_U32 disk_no)
{
    MIXER_DBG("this version don't support format disk\n");

    return TRUE;
}

MI_BOOL CMmcManager::ReMountDisk()
{
    pthread_mutex_lock(&m_mutex);


    TimerDestroy();

    sstar_mmc_file_sys_remount();

    TimerCreate(onTimerCallBack, 2000);

    pthread_mutex_unlock(&m_mutex);

    return TRUE;
}


MI_BOOL CMmcManager:: InitializeFS()
{
    MmcDev mmc[2];
    int ret = scan_mmc_dev(mmc, 2, MMC_TYPE);
    if(ret ==2)
    {

    }

    UpdateState();

    return TRUE;
}

MI_BOOL CMmcManager:: CUpdateState()
{
    g_MmcManager->UpdateState();
    MIXER_INFO("g_MmcManager->UpdateState()..........................................\n");
    return TRUE;
}

MI_BOOL CMmcManager:: UpdateState()
{
    pthread_mutex_lock(&m_mutex);
    MI_S32 ret = sstar_mmc_file_sys_detect();
    switch(ret)
    {
        case DISK_NO_ERROR:
        {
            if(sstar_mmc_file_sys_get_szie(&m_dwCapability, &m_dwRemain)<0)
            {
                m_dwStat[0] = FS_NO_MOUNT;
                break;
            }

            if(m_dwCapability < 512 * 1024)
            {
                m_dwStat[0] = FS_NO_MOUNT;    //it is the tmpfs,without mount sdcard.but MMC_MOUNT_PATH is exist.
            }
            else if(m_dwRemain < REMAINSPACE)
            {
                m_dwStat[0] = FS_NO_SPACE;
            }
            else
            {
                m_dwStat[0] = FS_NO_ERROR;
            }
            break;
        }
        case DISK_NO_FIND:
        {
            m_dwStat[0] = FS_NO_FIND;
            break;
        }
        case DISK_NO_FORMAT:
        {
            m_dwStat[0] = FS_NO_FORMAT;
            break;
        }
        default:
        {
            MIXER_ERR("unknown ret =%d\n", ret);
            break;
        }
    }


    switch(m_dwStat[0])
    {
        case FS_NO_ERROR:
            //MIXER_DBG("diskstate : FS_NO_ERROR\n");
            break;
        case FS_NO_FIND:
            //MIXER_DBG("diskstate : FS_NO_FIND\n");
            break;
        case FS_NO_FORMAT:
            //MIXER_DBG("diskstate : FS_NO_FORMAT\n");
            break;
        case FS_NO_SPACE:
            //MIXER_DBG("diskstate : FS_NO_SPACE\n");
            break;
        case FS_NO_MOUNT:
            //MIXER_DBG("diskstate : FS_NO_MOUNT\n");
            break;
        default:
            MIXER_ERR("unknown diskstate %d\n", m_dwStat[0]);
            break;
    }
    pthread_mutex_unlock(&m_mutex);
    return TRUE;
}

void CMmcManager:: onTimerCallBack(int argc)
{
    CUpdateState();
}

MI_S32 CMmcManager::TimerCreate(void(*pCallBack)(int), MI_U32 dwMilliSeconds)
{
    struct sigaction  act;
    struct itimerval  timer;

    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    act.sa_handler =  pCallBack;

    if (sigaction(SIGALRM, &act, NULL) < 0)
    {
        MIXER_ERR("Sigaction failed\n");
        return -1;
    }

    timer.it_interval.tv_sec  = dwMilliSeconds / 1000;
    timer.it_interval.tv_usec = (dwMilliSeconds % 1000) * 1000;
    timer.it_value = timer.it_interval;

    if (setitimer(ITIMER_REAL, &timer, NULL) < 0)
    {
        MIXER_ERR("Setitimer failed\n");
        return -1;
    }

    return 0;
}

MI_S32 CMmcManager::TimerDestroy()
{
    struct sigaction  act;
    struct itimerval  timer;

    sigemptyset(&act.sa_mask);
    act.sa_flags   = 0;
    act.sa_handler = SIG_DFL;

    if (sigaction(SIGALRM, &act, NULL) < 0)
    {
        MIXER_ERR("Sigaction failed\n");
        return -1;
    }

    timer.it_interval.tv_sec  = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value = timer.it_interval;

    if (setitimer(ITIMER_REAL, &timer, NULL) < 0)
    {
        MIXER_ERR("Setitimer failed\n");
        return -1;
    }

    return 0;
}




