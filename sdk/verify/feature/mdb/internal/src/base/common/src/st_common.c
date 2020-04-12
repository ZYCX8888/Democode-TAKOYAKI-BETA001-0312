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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mi_sys.h"
#include "st_common.h"

#define DEFAULT_MAX_PICW 1920
#define DEFAULT_MAX_PICH 1080

MI_S32 ST_Sys_Init(void)
{
    MI_SYS_Version_t stVersion;
    MI_U64 u64Pts = 0;
    STCHECKRESULT(MI_SYS_Init()); //Sys Init
    memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));
    STCHECKRESULT(MI_SYS_GetVersion(&stVersion));
    ST_INFO("(%d) u8Version:0x%llx\n", __LINE__, stVersion.u8Version);
    STCHECKRESULT(MI_SYS_GetCurPts(&u64Pts));
    ST_INFO("(%d) u64Pts:0x%llx\n", __LINE__, u64Pts);

    u64Pts = 0xF1237890F1237890;
    STCHECKRESULT(MI_SYS_InitPtsBase(u64Pts));

    u64Pts = 0xE1237890E1237890;
    STCHECKRESULT(MI_SYS_SyncPts(u64Pts));

    return MI_SUCCESS;
}

MI_S32 ST_Sys_Exit(void)
{
    STCHECKRESULT(MI_SYS_Exit());

    return MI_SUCCESS;
}

MI_S32 ST_Sys_Bind(ST_Sys_BindInfo_t *pstBindInfo)
{
    ExecFunc(MI_SYS_BindChnPort(&pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort, \
        pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_S32 ST_Sys_UnBind(ST_Sys_BindInfo_t *pstBindInfo)
{
    ExecFunc(MI_SYS_UnBindChnPort(&pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_U64 ST_Sys_GetPts(MI_U32 u32FrameRate)
{
    if (0 == u32FrameRate)
    {
        return (MI_U64)(-1);
    }

    return (MI_U64)(1000 / u32FrameRate);
}

MI_BOOL ST_OpenSrcFile(const char *pFileName, int *pSrcFd)
{
    int src_fd = open(pFileName, O_RDONLY);
    if (src_fd < 0)
    {
        printf("src_file: %s.\n", pFileName);

        perror("open");
        return -1;
    }
    *pSrcFd = src_fd;

    return MI_SUCCESS;
}

MI_BOOL ST_OpenDestFile(const char *pFileName, int *pDestFd)
{
    int dest_fd = open(pFileName, O_WRONLY|O_CREAT|O_APPEND, 0777);
    if (dest_fd < 0)
    {
        printf("dest_file: %s.\n", pFileName);
        perror("open");
        return -1;
    }

    *pDestFd = dest_fd;

    return MI_SUCCESS;
}

MI_BOOL ST_FdRewind(int srcFd)
{
    lseek(srcFd, 0, SEEK_SET);
    return MI_SUCCESS;
}

MI_S32 ST_ReadOneFrameYUV420ByStride(int srcFd, char *pYData, char *pUvData, int ySize, int uvSize, int height, int width, int yStride, int uvStride)
{
    int size = 0;
    int i = 0;
    off_t current = lseek(srcFd,0L, SEEK_CUR);
    off_t end = lseek(srcFd,0L, SEEK_END);

    if ((end - current) < (ySize + uvSize))
    {
        return -1;
    }
    lseek(srcFd, current, SEEK_SET);

    for (i = 0; i < height; i++)
    {
        if (read(srcFd, pYData+ i * yStride, width) < width)
        {
            return 0;
        }
    }

    for (i = 0; i < height/2; i++)
    {
        if (read(srcFd, pUvData+ i * uvStride, width) < width)
        {
            return 0;
        }
    }

    return 1;
}

MI_S32 ST_ReadOneFramePackByStride(int srcFd, char *pYUVData, int height, int width, int Stride, int widthAm)
{
    int size = 0;
    int i = 0;
    off_t current = lseek(srcFd,0L, SEEK_CUR);
    off_t end = lseek(srcFd,0L, SEEK_END);

    if ((end - current) < width*height*widthAm)
    {
        return -1;
    }
    lseek(srcFd, current, SEEK_SET);

    for (i = 0; i < height; i++)
    {
        if (read(srcFd, pYUVData+ i * Stride, width*widthAm) < width*widthAm)
        {
            return 0;
        }
    }

    return 1;
}

MI_S32 ST_ReadOneFrameYUV422ByStride(int srcFd, char *pYUVData, int height, int width, int Stride)
{
    int size = 0;
    int i = 0;
    off_t current = lseek(srcFd,0L, SEEK_CUR);
    off_t end = lseek(srcFd,0L, SEEK_END);

    if ((end - current) < width*height*2)
    {
        return -1;
    }
    lseek(srcFd, current, SEEK_SET);

    for (i = 0; i < height; i++)
    {
        if (read(srcFd, pYUVData+ i * Stride, width*2) < width*2)
        {
            return 0;
        }
    }

    return 1;
}

MI_S32 ST_Write_OneFrame(int dstFd, int offset, char *pDataFrame, int line_offset, int line_size, int lineNumber)
{
    int size = 0;
    int i = 0;
    char *pData = NULL;
    int yuvSize = line_size;
    // seek to file offset
    //lseek(dstFd, offset, SEEK_SET);
    for (i = 0; i < lineNumber; i++)
    {
        pData = pDataFrame + line_offset*i;
        yuvSize = line_size;
        //printf("write lind %d begin\n", i);
        do {
            if (yuvSize < 256)
            {
                size = yuvSize;
            }
            else
            {
                size = 256;
            }

            size = write(dstFd, pData, size);
            if (size == 0)
            {
                break;
            }
            else if (size < 0)
            {
                break;
            }
            pData += size;
            yuvSize -= size;
        } while (yuvSize > 0);
        //printf("write lind %d end\n", i);
    }

    return MI_SUCCESS;
}

void ST_CloseFile(int fd)
{
    close(fd);
}

