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

/** @file venc_common.c
 *
 *  The function puts here would be shared with feature/vif.
 *  The code needs to be synchronized.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#include "mi_venc.h"
#include "mi_sys.h"
#include "env.h"

#define DEF_CFG_VAR //define this before venc_common.h
#include "venc_common.h"

//These variable are gotten from environment variables
//centralize them here for easier porting
#define CFG_OUT_PATH get_cfg_str("VENC_GLOB_OUT_PATH", &bErr)
#define CFG_WAIT_IDR get_cfg_int("VENC_GLOB_OUT_WAIT_IDR", &bErr)
#define CFG_DUMP_FRAMES get_cfg_int("VENC_GLOB_DUMP_ES", &bErr)
#define CFG_FILE_PATTERN get_cfg_str("VENC_GLOB_OUT_FILE_PATTERN", &bErr)

struct ChnPrivate_s
{
    MI_U32 u32DevId;
    MI_U32 u32DumpFrame;
    MI_BOOL bDumpEachFrame;
    MI_U32 u32WaitIdr;
    MI_BOOL bWritingFile;
    VENC_FPS_t stFps;
    int fdOutEs;
    int fdJpg;
    struct { //parsed frame information, This is an output variable.
        MI_BOOL bIsGoodIdr;//should include complete stream SPS/PPS
        MI_BOOL bIsBadHeader;//invalid NALU header
        MI_BOOL bMissingSeq;//whether the sequence is continuous against previous frame.
    }stOutFrameInfo;
    MI_U32 u32ExpectedSeq;
};

#define venc_stream 1
int venc_stream_open(Chn_t *pstChn, MI_VENC_ModType_e eType)
{
    MI_S32 s32Ret;
    MI_BOOL bErr;
    char szOutputPath[128];

    if(pstChn == NULL)
        return -1;

    if(pstChn->pstPrivate == NULL)
    {
        DBG_ERR("Private data must be assigned\n");
        return -2;
    }

    memset(pstChn->pstPrivate, 0, sizeof(struct ChnPrivate_s));
    s32Ret = MI_VENC_GetChnDevid(pstChn->u32ChnId, &pstChn->pstPrivate->u32DevId);
    if(s32Ret != MI_SUCCESS)
    {
        DBG_ERR("%Xs32Ret\n", s32Ret);
        return -1;
    }

    pstChn->pstPrivate->u32DumpFrame = (MI_U32)CFG_DUMP_FRAMES;
    ExecFunc(bErr, FALSE);
    if(pstChn->pstPrivate->u32DumpFrame > 0)
    {
        char szPattern[128];
        char *szCfgFilePattern;
        int i,j;
        int iValue[2];

        szCfgFilePattern = CFG_FILE_PATTERN;
        ExecFunc(bErr, FALSE);
        for (i = 0, j = 0; i < sizeof(szOutputPath); ++i)
        {
            szOutputPath[i] = szCfgFilePattern[i];
            if(szCfgFilePattern[i] == '!')
            {
                iValue[j] = pstChn->u32ChnId;
                szOutputPath[i] = 'd';
                j++;
            }
            if(szCfgFilePattern[i] == '@')
            {
                iValue[j] = pstChn->pstPrivate->u32DevId;
                szOutputPath[i] = 'd';
                j++;
            }
            if(szCfgFilePattern[i] == '\0')
                break;
        }
        snprintf(szPattern, sizeof(szPattern) - 1, "%%s/%s", szOutputPath);
        snprintf(szOutputPath, sizeof(szOutputPath) - 1, szPattern,
            CFG_OUT_PATH,
            iValue[0], iValue[1]);
#if 0
        snprintf(szOutputPath, sizeof(szOutputPath) - 1, "%s/enc_d%dc%02d.es",
            CFG_OUT_PATH,
            pstChn->pstPrivate->u32DevId, pstChn->u32ChnId);
#endif
        pstChn->pstPrivate->fdOutEs = open(szOutputPath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        printf("open %d %s\n", pstChn->pstPrivate->fdOutEs, szOutputPath);
        if(pstChn->pstPrivate->fdOutEs < 0)
        {
            printf("unable to open es\r\n");
        }
    }

    pstChn->pstPrivate->u32WaitIdr = (MI_U32)CFG_WAIT_IDR;
    ExecFunc(bErr, FALSE);

    if(eType == E_MI_VENC_MODTYPE_JPEGE)
    {
        pstChn->pstPrivate->bDumpEachFrame = TRUE;
    }
    else
    {
        pstChn->pstPrivate->bDumpEachFrame = FALSE;
    }

    pstChn->pstPrivate->stFps.bRestart = TRUE;
    return 0;
}

int venc_stream_get_statistic(VENC_FPS_t *pstFps, MI_VENC_CHN VencChannel, MI_U32 u32ContentSize)
{
    if(pstFps->bRestart)
    {
        pstFps->bRestart = FALSE;
        pstFps->u32TotalBits = 0;
        pstFps->u32FrameCnt = 0;
        gettimeofday(&pstFps->stTimeStart, &pstFps->stTimeZone);
    }
    gettimeofday(&pstFps->stTimeEnd, &pstFps->stTimeZone);
    pstFps->u32DiffUs = TIMEVAL_US_DIFF(pstFps->stTimeStart, pstFps->stTimeEnd);
    pstFps->u32TotalBits += u32ContentSize * 8;
    pstFps->u32FrameCnt++;
    if (pstFps->u32DiffUs > 1000000 && pstFps->u32FrameCnt > 1)
    {
        printf("<%5ld.%06ld> CH %02d get %.2f fps, %d kbps\n",
                pstFps->stTimeEnd.tv_sec, pstFps->stTimeEnd.tv_usec,
                VencChannel, (float) (pstFps->u32FrameCnt-1) * 1000000.0f / pstFps->u32DiffUs,
                pstFps->u32TotalBits * 1000 / pstFps->u32DiffUs);
        //printf("VENC bps: %d kbps\n", nBitrateCalcTotalBit * 1000 / nBitrateCalcUsDiff);
        pstFps->bRestart = TRUE;
        set_result_int("FPSx100", (int)((float)pstFps->u32FrameCnt*1000000.0f/pstFps->u32DiffUs*100));
    }
    return 0;
}

void venc_stream_print(char* title, int iCh, MI_VENC_Stream_t *pstStream)
{
    char *data;
    int i;
    int msg_level = VENC_GLOB_OUT_MSG_LEVEL;
    MI_S32 s32StreamLen;
    MI_VENC_ModType_e eModType;

    if(pstStream == NULL)
        return;

    data = (char *) pstStream->pstPack[0].pu8Addr + pstStream->pstPack[0].u32Offset;
    //eModType = _astChn[iCh].eModType;
    if(eModType == E_MI_VENC_MODTYPE_MAX)//special case for dummy
    {//print it as a string.
        if(msg_level > 0)
            DBG_INFO("^^^^ CH%02d:%s\n", iCh, (char*)data);
        return;
    }

    s32StreamLen = pstStream->pstPack[0].u32Len - pstStream->pstPack[0].u32Offset;
    //---- regular stream ----
    if(msg_level == 0)
    {
        return;
    }

    //short length, assume it's dummy data
    if(s32StreamLen > 0 && s32StreamLen < 10)
    {
        DBG_INFO("data:%s\n", (char*)data);
        return;
    }

    if(msg_level >= 2)
    {
        if(eModType == E_MI_VENC_MODTYPE_H264E)
        {
            DBG_INFO("#%d %s\n", pstStream->u32Seq,
                    pstStream->stH264Info.eRefType == E_MI_VENC_BASE_IDR ? "I" : "P");
            if(msg_level >= 3)
            {
                MI_U32 i;
                DBG_INFO("[U]:offset:0x%X, len:%d type:%d data:%d\n",
                         pstStream->pstPack[0].u32Offset,
                         pstStream->pstPack[0].u32Len,
                         pstStream->pstPack[0].stDataType.eH264EType,
                         pstStream->pstPack[0].u32DataNum);
                if(pstStream->pstPack[0].u32DataNum > 10)
                {
                    DBG_ERR("Invalid Data Num\n");
                    pstStream->pstPack[0].u32DataNum = 0;
                }
                for (i = 0; i < pstStream->pstPack[0].u32DataNum; ++i)
                {
                    DBG_INFO("[%d]:offset:0x%X, len:%d type:%d\n",
                            i, pstStream->pstPack[0].asackInfo[i].u32PackOffset,
                            pstStream->pstPack[0].asackInfo[i].u32PackLength,
                            pstStream->pstPack[0].asackInfo[i].stPackType.eH264EType);
                }
            }
        }
        else if(eModType == E_MI_VENC_MODTYPE_H265E)
        {
            DBG_INFO("#%d %s\n", pstStream->u32Seq,
                    pstStream->stH265Info.eRefType == E_MI_VENC_BASE_IDR ? "I" : "P");
            if(msg_level >= 3)
            {
                MI_U32 i;
                DBG_INFO("[U]:offset:0x%X, len:%d type:%d data:%d\n",
                         pstStream->pstPack[0].u32Offset,
                         pstStream->pstPack[0].u32Len,
                         pstStream->pstPack[0].stDataType.eH265EType,
                         pstStream->pstPack[0].u32DataNum);
                if(pstStream->pstPack[0].u32DataNum > 10)
                {
                    DBG_ERR("Invalid Data Num\n");
                    pstStream->pstPack[0].u32DataNum = 0;
                }
                for (i = 0; i < pstStream->pstPack[0].u32DataNum; ++i)
                {
                    DBG_INFO("[%d]:offset:0x%X, len:%d type:%d\n",
                            i, pstStream->pstPack[0].asackInfo[i].u32PackOffset,
                            pstStream->pstPack[0].asackInfo[i].u32PackLength,
                            pstStream->pstPack[0].asackInfo[i].stPackType.eH265EType);
                }
            }
        }
    }

    if(msg_level == 1)
    {
        printf("    CH%02d\n", iCh);
    }
    else if(msg_level == 2)
    {
        printf("^^^^ CH%02d got %5d Bytes:%02X %02X %02X %02X %02X %s\n", iCh, s32StreamLen,
               data[0], data[1], data[2], data[3], data[4], (data[4] == 0x67 || data[4] == 0x40) ? " I" : "");
    }
    else
    {
        printf("%s\nCH%02d Offset(h) \n"
                "00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"
                "-----------------------------------------------", title, iCh);
        for (i = 0; i < 32; i++)
        {
            if (i % 16 == 0)
            {
                printf("\n");
            }
            printf("%02X ", data[i]);
            if(i >= s32StreamLen)
                break;
        }
        printf("\n");
    }
}

int venc_stream_close(Chn_t *pstChn)
{
    if(pstChn == NULL)
        return -1;
    if(pstChn->pstPrivate == NULL)
        return 0;

    if(pstChn->pstPrivate->u32DumpFrame > 0 && pstChn->pstPrivate->fdOutEs > 0)
    {
        close(pstChn->pstPrivate->fdOutEs);
        pstChn->pstPrivate->fdOutEs = 0;
    }
    return 0;
}

int venc_stream_check(Chn_t *pstChn, MI_VENC_Stream_t *pstStream)
{
    MI_U8 u8NaluCode;
    MI_BOOL bIFrame = TRUE;
    MI_BOOL bIsBadHeader = FALSE;

    if(pstChn == NULL || pstStream == NULL)
    {
        return -1;
    }
    if (pstChn->eModType == E_MI_VENC_MODTYPE_H264E)
    {
        u8NaluCode = pstStream->pstPack[0].stDataType.eH264EType;
        if(u8NaluCode != E_MI_VENC_H264E_NALU_SPS && u8NaluCode != E_MI_VENC_H264E_NALU_PSLICE)
        {
            DBG_ERR("Invalid type ch%d %d %X\n", pstChn->u32ChnId, u8NaluCode, u8NaluCode);
            bIsBadHeader = TRUE;
        }
        if(u8NaluCode != E_MI_VENC_H264E_NALU_SPS)//start with E_MI_VENC_H264E_NALU_ISLICE
        {
            bIFrame = FALSE;
        }

    }
    else if(pstChn->eModType == E_MI_VENC_MODTYPE_H265E)
    {
        u8NaluCode = pstStream->pstPack[0].stDataType.eH265EType;
        if(u8NaluCode != E_MI_VENC_H265E_NALU_VPS && u8NaluCode != E_MI_VENC_H265E_NALU_PSLICE)
        {
            DBG_ERR("Invalid type ch%d %d %X\n", pstChn->u32ChnId, u8NaluCode, u8NaluCode);
            bIsBadHeader = TRUE;
        }
        if(u8NaluCode != E_MI_VENC_H265E_NALU_VPS)//start with E_MI_VENC_H265E_NALU_ISLICE does not count
        {
            bIFrame = FALSE;
        }
    }

    if(pstChn->pstPrivate != NULL)
    {
        pstChn->pstPrivate->stOutFrameInfo.bMissingSeq = FALSE;
        pstChn->pstPrivate->stOutFrameInfo.bIsBadHeader = bIsBadHeader;
        pstChn->pstPrivate->stOutFrameInfo.bIsGoodIdr = bIFrame;
        if(pstChn->pstPrivate->u32ExpectedSeq != pstStream->u32Seq)
        {
            pstChn->pstPrivate->stOutFrameInfo.bMissingSeq = TRUE;
        }
        pstChn->pstPrivate->u32ExpectedSeq++;
    }
    return 0;
}

//write one frame
int venc_stream_write(Chn_t *pstChn, MI_VENC_Stream_t *pstStream, MI_U32 u32ContentSize)
{
    MI_BOOL bErr;
    char szOutputPath[128];
    void* pVirAddr = NULL;
    pVirAddr = pstStream->pstPack[0].pu8Addr;
    static int i = 0;

    if(u32ContentSize > 0)
    {
        //save main elementary stream
        if((pstChn->pstPrivate->u32DumpFrame > 0) && (pstChn->pstPrivate->fdOutEs > 0))
        {
            ssize_t ssize;
            //if requested, save from first IDR.
            if(pstChn->pstPrivate->u32ExpectedSeq == 3)
            {

            }
            if (pstChn->pstPrivate->u32WaitIdr == 0 || pstChn->pstPrivate->bWritingFile
                || pstChn->pstPrivate->stOutFrameInfo.bIsGoodIdr)
            {
                ssize = write(pstChn->pstPrivate->fdOutEs, pVirAddr, u32ContentSize);
                if(ssize < 0)
                {
                    DBG_INFO("\n==== es is too big:%d ===\n\n", u32ContentSize);
                    close(pstChn->pstPrivate->fdOutEs);
                    pstChn->pstPrivate->fdOutEs = 0;
                }
                pstChn->pstPrivate->bWritingFile = TRUE;
            }
        }

        //save each frame. assuming it's JPEG.
        if(pstChn->pstPrivate->bDumpEachFrame && pstStream->u32Seq < pstChn->pstPrivate->u32DumpFrame)
        {
            snprintf(szOutputPath, sizeof(szOutputPath) - 1, "%sd%dc%02d_%03d.jpg",
                    CFG_OUT_PATH,
                    pstChn->pstPrivate->u32DevId, pstChn->u32ChnId, pstStream->u32Seq);
            pstChn->pstPrivate->fdJpg = open(szOutputPath,O_RDWR|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            printf("open %d %s\n", pstChn->pstPrivate->fdJpg, szOutputPath);
            if(pstChn->pstPrivate->fdJpg < 0)
            {
                printf("unable to open jpeg\r\n");
                pstChn->pstPrivate->bDumpEachFrame = FALSE;
            }
            else
            {
                ssize_t ssize;
                ssize = write(pstChn->pstPrivate->fdJpg, pVirAddr, u32ContentSize);
                if(ssize < 0)
                {
                    printf("\n==== frame is too big:%d ===\n\n", u32ContentSize);
                }
                close(pstChn->pstPrivate->fdJpg);
                pstChn->pstPrivate->fdJpg = 0;
            }
        }
    }
    return 0;
}

int venc_stream_process(Chn_t *pstChn, MI_VENC_Stream_t *pstStream)
{
    MI_U32 u32ContentSize = 0;
    MI_BOOL bEndOfStream;

    if(pstChn == NULL || pstStream == NULL)
        return -1;

    u32ContentSize = pstStream->pstPack[0].u32Len - pstStream->pstPack[0].u32Offset;
    bEndOfStream = (pstStream->pstPack[0].bFrameEnd & 2)? TRUE: FALSE;

#if 0
    if(u32Retry > 0)
    {
        DBG_INFO("Retried %d times on CH%2d\n", u32Retry, VencChannel);
        u32Retry = 0;
    }
#endif

    venc_stream_print("[USER SPACE]", pstChn->u32ChnId, pstStream);

#if 0
    if(stFps.bRestart)
    {
        stFps.bRestart = FALSE;
        stFps.u32TotalBits = 0;
        stFps.u32FrameCnt = 0;
        gettimeofday(&stFps.stTimeStart, &stFps.stTimeZone);
    }
    gettimeofday(&stFps.stTimeEnd, &stFps.stTimeZone);
    stFps.u32DiffUs = TIMEVAL_US_DIFF(stFps.stTimeStart, stFps.stTimeEnd);
    stFps.u32TotalBits += u32ContentSize * 8;
    stFps.u32FrameCnt++;
    if (stFps.u32DiffUs > 1000000 && stFps.u32FrameCnt > 1)
    {
        printf("<%5ld.%06ld> CH %02d get %.2f fps, %d kbps\n",
                stFps.stTimeEnd.tv_sec, stFps.stTimeEnd.tv_usec,
                VencChannel, (float) (stFps.u32FrameCnt-1) * 1000000.0f / stFps.u32DiffUs,
                stFps.u32TotalBits * 1000 / stFps.u32DiffUs);
        //printf("VENC bps: %d kbps\n", nBitrateCalcTotalBit * 1000 / nBitrateCalcUsDiff);
        stFps.bRestart = TRUE;
        set_result_int("FPSx100", (int)((float)stFps.u32FrameCnt*1000000.0f/stFps.u32DiffUs*100));
    }
#else
    if(pstChn->pstPrivate)
    {
        venc_stream_get_statistic(&pstChn->pstPrivate->stFps, pstChn->u32ChnId, u32ContentSize);
        venc_stream_check(pstChn, pstStream);
        venc_stream_write(pstChn, pstStream, u32ContentSize);
#endif
#if 0
    if(u32ContentSize > 0)
    {
        if((pstChn->pstPrivate->u32DumpFrame > 0) && (pstChn->pstPrivate->fdOutEs > 0))
        {
            ssize_t ssize;
            ssize = write(pstChn->pstPrivate->fdOutEs, pVirAddr, u32ContentSize);
            if(ssize < 0)
            {
                DBG_INFO("\n==== es is too big:%d ===\n\n", u32ContentSize);
                close(pstChn->pstPrivate->fdOutEs);
                pstChn->pstPrivate->fdOutEs = 0;
            }
        }

        if(pstChn->pstPrivate->bDumpEachFrame && pstStream->u32Seq < pstChn->pstPrivate->u32DumpFrame)
        {
            snprintf(szOutputPath, sizeof(szOutputPath) - 1, "%sd%dc%02d_%03d.jpg",
                    CFG_OUT_PATH,
                    pstChn->pstPrivate->u32DevId, pstChn->u32ChnId, pstStream->u32Seq);
            pstChn->pstPrivate->fdJpg = open(szOutputPath,O_RDWR|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            printf("open %d %s\n", pstChn->pstPrivate->fdJpg, szOutputPath);
            if(pstChn->pstPrivate->fdJpg < 0)
            {
                printf("unable to open jpeg\r\n");
                pstChn->pstPrivate->bDumpEachFrame = FALSE;
            }
            else
            {
                ssize_t ssize;
                ssize = write(pstChn->pstPrivate->fdJpg, pVirAddr, u32ContentSize);
                if(ssize < 0)
                {
                    printf("\n==== frame is too big:%d ===\n\n", u32ContentSize);
                }
                close(pstChn->pstPrivate->fdJpg);
                pstChn->pstPrivate->fdJpg = 0;
            }
        }
    }
#endif

        //saving file and current SEQ >= requested SEQ to be saved.
        if (pstChn->pstPrivate->u32DumpFrame > 0 && pstChn->pstPrivate->fdOutEs > 0
            && pstStream->u32Seq >= pstChn->pstPrivate->u32DumpFrame)
        {
            ExecFunc(venc_stream_close(pstChn), 0);
            DBG_INFO("\n==== ch%2d %d frames wr done\n\n", pstChn->u32ChnId, pstStream->u32Seq);
        }
    }

    if(bEndOfStream)
    {
        DBG_INFO("\n==== EOS ====\n\n");
        return 1;
    }
    return 0;
}

int create_venc_channel(MI_VENC_ModType_e eModType, MI_VENC_CHN VencChannel, MI_U32 u32Width,
                        MI_U32 u32Height, VENC_Rc_t *pstVencRc)
{
    MI_VENC_ChnAttr_t stChannelVencAttr;
    //MI_SYS_ChnPort_t stSysChnOutPort;
    const MI_U32 u32QueueLen = 40;
    //MI_S32 s32Ret;// = E_MI_ERR_FAILED;
    MI_S32 s32DevId;
    //MI_S8 s8Core;

    if(NULL == pstVencRc)
    {
        return -1;
    }
    printf("CH%2d %dx%d mod:%d, rc:%d\n", VencChannel, u32Width, u32Height, eModType, pstVencRc->eRcMode);
    memset(&stChannelVencAttr, 0, sizeof(stChannelVencAttr));

    switch(eModType)
    {
        case E_MI_VENC_MODTYPE_H264E:
            stChannelVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChannelVencAttr.stVeAttr.stAttrH264e.u32PicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrH264e.u32PicHeight = u32Height;
            stChannelVencAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = u32Height;
            stChannelVencAttr.stVeAttr.stAttrH264e.bByFrame = TRUE;
            break;

        case E_MI_VENC_MODTYPE_H265E:
            stChannelVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChannelVencAttr.stVeAttr.stAttrH265e.u32PicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrH265e.u32PicHeight = u32Height;
            stChannelVencAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = u32Height;
            stChannelVencAttr.stVeAttr.stAttrH265e.bByFrame = TRUE;
            break;

        case E_MI_VENC_MODTYPE_JPEGE:
            stChannelVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChannelVencAttr.stVeAttr.stAttrJpeg.u32PicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrJpeg.u32PicHeight = u32Height;
            stChannelVencAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = u32Height;
            stChannelVencAttr.stVeAttr.stAttrJpeg.bByFrame = TRUE;
            break;
        case E_MI_VENC_MODTYPE_MAX:
            stChannelVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_MAX;
            break;
        default:
            break;
    }

    stChannelVencAttr.stRcAttr.eRcMode = pstVencRc->eRcMode;
    switch(pstVencRc->eRcMode)
    {
        case E_MI_VENC_RC_MODE_H264FIXQP:
            if(eModType != E_MI_VENC_MODTYPE_H264E)
            {
                DBG_ERR("H264 FixQP is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = pstVencRc->u32SrcFrmRateNum;
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = pstVencRc->u32SrcFrmRateDen;
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32Gop = pstVencRc->u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32IQp = pstVencRc->u32FixQp;
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32PQp = pstVencRc->u32FixQp;
            break;
        case E_MI_VENC_RC_MODE_H265FIXQP:
            if(eModType != E_MI_VENC_MODTYPE_H265E)
            {
                DBG_ERR("H265 FixQP is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum = pstVencRc->u32SrcFrmRateNum;
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = pstVencRc->u32SrcFrmRateDen;
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32Gop = pstVencRc->u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32IQp = pstVencRc->u32FixQp;
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32PQp = pstVencRc->u32FixQp;
            break;
        case E_MI_VENC_RC_MODE_H264CBR:
            if(eModType != E_MI_VENC_MODTYPE_H264E)
            {
                DBG_ERR("H264 CBR is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32Gop = pstVencRc->u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = pstVencRc->u32SrcFrmRateNum;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = pstVencRc->u32SrcFrmRateDen;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32BitRate = pstVencRc->u32Bitrate;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
            break;
        case E_MI_VENC_RC_MODE_H265CBR:
            if(eModType != E_MI_VENC_MODTYPE_H265E)
            {
                DBG_ERR("H265 CBR is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32Gop = pstVencRc->u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = pstVencRc->u32SrcFrmRateNum;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = pstVencRc->u32SrcFrmRateDen;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32BitRate = pstVencRc->u32Bitrate;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
            break;
        case E_MI_VENC_RC_MODE_H264VBR:
            if(eModType != E_MI_VENC_MODTYPE_H264E)
            {
                DBG_ERR("H264 VBR is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32Gop = pstVencRc->u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateNum = pstVencRc->u32SrcFrmRateNum;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateDen = pstVencRc->u32SrcFrmRateDen;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = pstVencRc->u32Bitrate;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32MinQp = pstVencRc->u32VbrMinQp;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = pstVencRc->u32VbrMaxQp;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32StatTime = 0;
            break;
        case E_MI_VENC_RC_MODE_H265VBR:
            if(eModType != E_MI_VENC_MODTYPE_H265E)
            {
                DBG_ERR("H265 VBR is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32Gop = pstVencRc->u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = pstVencRc->u32SrcFrmRateNum;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = pstVencRc->u32SrcFrmRateDen;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = pstVencRc->u32Bitrate;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32MinQp = pstVencRc->u32VbrMinQp;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = pstVencRc->u32VbrMaxQp;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32StatTime = 0;
            break;
        case E_MI_VENC_RC_MODE_MJPEGFIXQP:
            if(eModType != E_MI_VENC_MODTYPE_JPEGE)
            {
                DBG_ERR("JPEG RC is set but module is %d\n", eModType);
                return -1;
            }
            break;
        default:
            if(eModType == E_MI_VENC_MODTYPE_MAX)
                break;
            DBG_ERR("Unknown RC type:%d\n", pstVencRc->eRcMode);
            return -3;
            break;
    }

    ExecFunc(MI_VENC_CreateChn(VencChannel, &stChannelVencAttr), MI_SUCCESS);
    //Set port
    ExecFunc(MI_VENC_GetChnDevid(VencChannel, (MI_U32*)&s32DevId), MI_SUCCESS);

    if(s32DevId >= 0)
    {
        //stSysChnOutPort.u32DevId = s32DevId;
    }
    else
    {
        DBG_ERR("Can not find device ID %X\n", s32DevId);
        return -2;
    }

    ExecFunc(MI_VENC_SetMaxStreamCnt(VencChannel, u32QueueLen), MI_SUCCESS);

    return 0;
}


int destroy_venc_channel(MI_VENC_CHN VencChannel)
{
    /*****************************/
    /*  call sys bind interface */
    /*****************************/
    ExecFunc(MI_VENC_StopRecvPic(VencChannel), MI_SUCCESS);
    //printf("sleeping...");
    //usleep(2 * 1000 * 1000);//wait for stop channel
    //printf("sleep done\n");

    /*****************************/
    /*  call sys unbind interface */
    /*****************************/
    ExecFunc(MI_VENC_DestroyChn(VencChannel), MI_SUCCESS);

    return 0;
}

void trace_venc_channel(char * title, MI_VENC_CHN VeChn)
{
    MI_VENC_ChnStat_t stStat;
    MI_S32 s32Ret;
    if(VENC_GLOB_TRACE_QUERY == 0)
        return;

    s32Ret = MI_VENC_Query(VeChn, &stStat);
    if(s32Ret == MI_SUCCESS)
    {
        printf("%16s cp%d lp%d ep%3d rp%d sb%d sf%d\n", title, stStat.u32CurPacks, stStat.u32LeftPics, stStat.u32LeftEncPics,
                stStat.u32LeftRecvPics, stStat.u32LeftStreamBytes, stStat.u32LeftStreamFrames);
    }
    else
    {
        DBG_ERR(":%X\n", s32Ret);
    }
}

volatile MI_BOOL gbPanic = FALSE;
#define USE_FD (1)
#if USE_FD
#include <poll.h>
#endif

//pthread function for each channel
void *venc_channel_func(void *arg)
{
    Chn_t* pstChn = (Chn_t*)arg;
    ChnPrivate_t stPrivate;
    MI_VENC_CHN VencChannel;
#if venc_stream == 0
    MI_U32 VencDevId;
    MI_U32 VencPortId;
    unsigned int VENC_DUMP_FRAMES;
    int fdOutEs = -1; //output elementary stream
    int fdJpg = -1;
#endif
    MI_SYS_ChnPort_t stVencChn0OutputPort0;

    MI_S32 s32Ret;
    MI_BOOL bErr;
#if venc_stream == 0
    MI_BOOL bSaveEach = FALSE;
    char szOutputPath[128];
    MI_U32 u32Seq = 0;
    VENC_FPS_t stFps = { .bRestart = TRUE, .u32TotalBits = 0, .u32DiffUs = 0 };
#endif
    MI_VENC_ChnAttr_t stAttr;
    MI_U32 u32Retry = 0;
#if USE_FD
    MI_S32 s32FdStream = -1;
#endif

    static MI_U32 u32EncodeFrames = 0;
    if (arg == NULL)
    {
        printf("Null input\r\n");
        return NULL;
    }

#if venc_stream == 0
    stFps.bRestart = TRUE;
#endif

    pstChn->pstPrivate = &stPrivate;
    ExecFunc(venc_stream_open(pstChn, stAttr.stVeAttr.eType), 0);
    VencChannel = pstChn->u32ChnId;
#if venc_stream == 0
    VENC_DUMP_FRAMES = get_cfg_int("VENC_GLOB_DUMP_ES", &bErr);
    s32Ret = MI_VENC_GetChnDevid(VencChannel, &VencDevId);
    if(s32Ret != MI_SUCCESS)
    {
        DBG_ERR("%Xs32Ret\n", s32Ret);
        return NULL;
    }
    VencPortId = 0;
#endif
    DBG_INFO("Start of Thread %d Getting.\n", VencChannel);

#if venc_stream == 0
    memset(&stVencChn0OutputPort0, 0x0, sizeof(MI_SYS_ChnPort_t));
    stVencChn0OutputPort0.eModId = E_MI_MODULE_ID_VENC;
    stVencChn0OutputPort0.u32DevId = VencDevId;
    stVencChn0OutputPort0.u32ChnId = VencChannel;
    stVencChn0OutputPort0.u32PortId = VencPortId;
#endif

    s32Ret = MI_VENC_GetChnAttr(VencChannel, &stAttr);
    if(s32Ret != MI_SUCCESS)
    {
        DBG_ERR("%Xs32Ret\n", s32Ret);
        return NULL;
    }

#if venc_stream == 0
    if(stAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_JPEGE)
    {
        bSaveEach = TRUE;
    }
    if(VENC_DUMP_FRAMES)
    {
        snprintf(szOutputPath, sizeof(szOutputPath) - 1, "%s/enc_d%dc%02d.es",
            CFG_OUT_PATH,
            VencDevId, VencChannel);
        fdOutEs = open(szOutputPath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        printf("open %d %s\n", fdOutEs, szOutputPath);
        if(fdOutEs < 0)
        {
            printf("unable to open es\r\n");
        }
    }
#endif
#if USE_FD
    s32FdStream = MI_VENC_GetFd(VencChannel);
    if(s32FdStream <= 0)
    {
        DBG_ERR("Unable to get FD:%d for ch:%2d\n", s32FdStream, VencChannel);
        return NULL;
    }
    else
    {
        printf("CH%2d FD%d\n", VencChannel, s32FdStream);
    }
#endif

    while(!gbPanic)
    {
        MI_BOOL bEndOfStream = FALSE;
        MI_VENC_Stream_t stStream;
        MI_VENC_Pack_t stPack0;
#if USE_FD
        {
            struct pollfd pfd[1] =
            {
                {s32FdStream, POLLIN | POLLERR, 0},
            };
            int iTimeOut, rval;
            MI_S32 s32MilliSec = 200;
            MI_VENC_ChnStat_t stStat;

            if(s32MilliSec == -1)
                iTimeOut = 0x7FFFFFFF;
            else
                iTimeOut = s32MilliSec;

            rval = poll(pfd, 1, iTimeOut);

            if(rval <= 0)
            {// time-out (0), or error ( < 0)
                //DBG_ERR("CH%2d Time out\r\n", VencChannel);
                u32Retry++;
                if(u32Retry>10000000)
                {
                    s32Ret = MI_VENC_CloseFd(VencChannel);
                    if(s32Ret != 0)
                    {
                        DBG_ERR("CH%02d Ret:%X\n", VencChannel, s32Ret);
                    }

                    //DBG_ERR("CH%2d Time out for %d times\r\n", VencChannel, u32Retry - 1);
                    //return NULL;
                    s32FdStream = MI_VENC_GetFd(VencChannel);
                    if(s32FdStream <= 0)
                    {
                        DBG_ERR("Unable to get FD:%d for ch:%2d\n", s32FdStream, VencChannel);
                        return NULL;
                    }
                    else
                    {
                        printf("CH%2d FD%d\n", VencChannel, s32FdStream);
                    }
                    u32Retry = 0;
                }
                continue;
                //return NULL;
            }
            else if((pfd[0].revents & POLLIN) != POLLIN)//any error or not POLLIN
            {
                DBG_ERR("CH%2d Error\r\n", VencChannel);
                return NULL;
            }

            s32Ret = MI_VENC_Query(VencChannel, &stStat);
            trace_venc_channel("after poll stStat", VencChannel);
            if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
            {
                DBG_ERR("Unexpected query state:%X curPacks:%d\n", s32Ret, stStat.u32CurPacks);
                continue;
            }
        }
#endif
        memset(&stStream, 0, sizeof(stStream));
        memset(&stPack0, 0, sizeof(stPack0));
        stStream.pstPack = &stPack0;
        stStream.u32PackCount = 1;
        s32Ret = MI_VENC_GetStream(VencChannel, &stStream, 0);
        trace_venc_channel("after GetStream", VencChannel);

        if(s32Ret != MI_SUCCESS)
        {
            if(s32Ret != MI_ERR_VENC_NOBUF)
            {
                DBG_ERR("GetStream Err:%X seq:%d\n\n", s32Ret, stStream.u32Seq);
                break;
            }
            u32Retry++;
            //DBG_ERR("CH%2d\n", VencChannel);
        }
        else
        {
#if venc_stream == 1
            int iRet;

            if(u32Retry > 0)
            {
                DBG_INFO("Retried %d times on CH%2d\n", u32Retry, VencChannel);
                u32Retry = 0;
            }
            
            iRet = venc_stream_process(pstChn, &stStream);
            if(iRet != 0)
            {
                gbPanic = TRUE;
            }

            unsigned int VENC_DUMP_FRAMES = get_cfg_int("VENC_GLOB_DUMP_ES", &bErr);
            ++u32EncodeFrames;
            DBG_ERR("u32EncodeFrames[%u]\n", u32EncodeFrames);
            if(u32EncodeFrames >= VENC_DUMP_FRAMES)
            {
                DBG_ERR("u32EncodeFrames[%u]\n", u32EncodeFrames);
                gbPanic = TRUE;
            }
#else
            MI_U32 u32ContentSize = 0;
            {
                void* pVirAddr = NULL;
                u32ContentSize = stStream.pstPack[0].u32Len - stStream.pstPack[0].u32Offset;
                bEndOfStream = (stStream.pstPack[0].bFrameEnd & 2)? TRUE: FALSE;
                pVirAddr = stStream.pstPack[0].pu8Addr;

                if(u32Retry > 0)
                {
                    DBG_INFO("Retried %d times on CH%2d\n", u32Retry, VencChannel);
                    u32Retry = 0;
                }

                venc_stream_print("[USER SPACE]", VencChannel, &stStream);

                if(stFps.bRestart)
                {
                    stFps.bRestart = FALSE;
                    stFps.u32TotalBits = 0;
                    stFps.u32FrameCnt = 0;
                    gettimeofday(&stFps.stTimeStart, &stFps.stTimeZone);
                }
                gettimeofday(&stFps.stTimeEnd, &stFps.stTimeZone);
                stFps.u32DiffUs = TIMEVAL_US_DIFF(stFps.stTimeStart, stFps.stTimeEnd);
                stFps.u32TotalBits += u32ContentSize * 8;
                stFps.u32FrameCnt++;
                if (stFps.u32DiffUs > 1000000 && stFps.u32FrameCnt > 1)
                {
                    printf("<%5ld.%06ld> CH %02d get %.2f fps, %d kbps\n",
                            stFps.stTimeEnd.tv_sec, stFps.stTimeEnd.tv_usec,
                            VencChannel, (float) (stFps.u32FrameCnt-1) * 1000000.0f / stFps.u32DiffUs,
                            stFps.u32TotalBits * 1000 / stFps.u32DiffUs);
                    //printf("VENC bps: %d kbps\n", nBitrateCalcTotalBit * 1000 / nBitrateCalcUsDiff);
                    stFps.bRestart = TRUE;
                    set_result_int("FPSx100", (int)((float)stFps.u32FrameCnt*1000000.0f/stFps.u32DiffUs*100));
                }

                if(u32ContentSize > 0)
                {
                    if((VENC_DUMP_FRAMES > 0) && (fdOutEs > 0))
                    {
                        ssize_t ssize;
                        ssize = write(fdOutEs, pVirAddr, u32ContentSize);
                        if(ssize < 0)
                        {
                            DBG_INFO("\n==== es is too big:%d ===\n\n", u32ContentSize);
                            close(fdOutEs);
                            fdOutEs = 0;
                        }
                    }

                    if(bSaveEach && u32Seq < VENC_DUMP_FRAMES)
                    {
                        snprintf(szOutputPath, sizeof(szOutputPath) - 1, "%sd%dc%02d_%03d.jpg",
                                CFG_OUT_PATH,
                                VencDevId, VencChannel, u32Seq);
                        fdJpg = open(szOutputPath,O_RDWR|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        printf("open %d %s\n", fdJpg, szOutputPath);
                        if(fdJpg < 0)
                        {
                            printf("unable to open jpeg\r\n");
                            bSaveEach = FALSE;
                        }
                        else
                        {
                            ssize_t ssize;
                            ssize = write(fdJpg, pVirAddr, u32ContentSize);
                            if(ssize < 0)
                            {
                                printf("\n==== frame is too big:%d ===\n\n", u32ContentSize);
                            }
                            close(fdJpg);
                            fdJpg = 0;
                        }
                    }
                }
                u32Seq++;
            }
#endif
            trace_venc_channel("before RlsStream", VencChannel);
            s32Ret = MI_VENC_ReleaseStream(VencChannel, &stStream);
            trace_venc_channel("after  RlsStream", VencChannel);
            if(s32Ret != MI_SUCCESS)
            {
                DBG_ERR("Unable to put output %X\n", s32Ret);
                gbPanic = TRUE;
            }

#if venc_stream == 0
            if(VENC_DUMP_FRAMES && fdOutEs > 0 && u32Seq >= VENC_DUMP_FRAMES)
            {
                DBG_INFO("\n==== ch%2d %d frames wr done\n\n", VencChannel, u32Seq);
                close(fdOutEs);
                fdOutEs = 0;
            }

            if(bEndOfStream)
            {
                DBG_INFO("\n==== EOS ====\n\n");
                break;
            }
#endif
        }
    }

#if venc_stream == 1
    ExecFunc(venc_stream_close(pstChn), 0);
#else
    if(VENC_DUMP_FRAMES && fdOutEs > 0)
    {
        close(fdOutEs);
        //fd = 0;//not used
    }
#endif

#if USE_FD
    if(s32FdStream > 0)
    {
        s32Ret = MI_VENC_CloseFd(VencChannel);
        if(s32Ret != 0)
        {
            DBG_ERR("CH%02d Ret:%X\n", VencChannel, s32Ret);
        }
        s32FdStream = -1;
    }
#endif

    //TODO review if this is still needed now
#if venc_stream == 0
    //reset the depth to a small number
    MI_SYS_SetChnOutputPortDepth(&stVencChn0OutputPort0, 0, 3);
#endif

    DBG_INFO("Thread Getting %02d exited.\n", VencChannel);

    return NULL;
}

void sleep_ms(int milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}
