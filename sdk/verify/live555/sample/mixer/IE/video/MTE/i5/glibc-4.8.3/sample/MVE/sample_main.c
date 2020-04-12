/*
* sample_main.c- Sigmastar
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
#include "sample_main.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

MI_RET Sample_InitInputImageEx(MVE_SRC_IMAGE_S *image, int file_handle)
{
    int size;

    switch(image->enType) {
        // Gray
        case MVE_IMAGE_TYPE_U8C1:
            size   = image->u16Stride[0] * image->u16Height * sizeof(U8);
            read(file_handle, (char*)(image->pu8Addr[0]), size);
            break;

        case MVE_IMAGE_TYPE_U16C1:
        case MVE_IMAGE_TYPE_S16C1:
            size   = image->u16Stride[0] * image->u16Height * sizeof(U16);
            read(file_handle, (char*)(image->pu8Addr[0]), size);
            break;

        // YUV 420 semi plane
        case MVE_IMAGE_TYPE_YUV420SP:
            size   = image->u16Stride[0] * image->u16Height * sizeof(U8);
            read(file_handle, (char*)(image->pu8Addr[0]), size);
            read(file_handle, (char*)(image->pu8Addr[1]), size/2);
            break;

        // YUV 422 semi plane
        case MVE_IMAGE_TYPE_YUV422SP:
            size   = image->u16Stride[0] * image->u16Height * sizeof(U8);
            read(file_handle, (char*)(image->pu8Addr[0]), size);
            read(file_handle, (char*)(image->pu8Addr[1]), size);
            break;

        // RGB packed
        case MVE_IMAGE_TYPE_U8C3_PACKAGE:
            size   = image->u16Stride[0] * image->u16Height * sizeof(U8) * 3;
            read(file_handle, (char*)(image->pu8Addr[0]), size);
            break;

        // RGB plane
        case MVE_IMAGE_TYPE_U8C3_PLANAR:
            size   = image->u16Stride[0] * image->u16Height * sizeof(U8);
            read(file_handle, (char*)(image->pu8Addr[0]), size);
            read(file_handle, (char*)(image->pu8Addr[1]), size);
            read(file_handle, (char*)(image->pu8Addr[2]), size);
            break;

        default:
            printf("Unimplemented format %X\n", image->enType);
            return MI_MVE_RET_INVALID_PARAMETER;
    }

    return MI_RET_SUCCESS;
}

MI_RET Sample_InitInputImage(MVE_SRC_IMAGE_S *image, const char *file_name)
{
    int size, file_handle;
    MI_RET ret = MI_RET_SUCCESS;

    file_handle = open(file_name, O_RDONLY);
    if (file_handle <= 0)
    {
        printf("Can't open file_handle %s (%d: %s)\n", file_name, errno, strerror(errno));
        return MI_MVE_RET_INVALID_PARAMETER;
    }

    ret = Sample_InitInputImageEx(image, file_handle);

    close(file_handle);

    return ret;
}

MI_RET Sample_SaveOutputImageEx(MVE_DST_IMAGE_S *image, int file_handle)
{
    int size;

    // write to file_handle
    switch(image->enType)
    {
        case MVE_IMAGE_TYPE_S64C1:
        case MVE_IMAGE_TYPE_U64C1:
            size = image->u16Stride[0]*image->u16Height*sizeof(U64);
            write(file_handle, (char*)(image->pu8Addr[0]), size);
            break;

        case MVE_IMAGE_TYPE_S32C1:
        case MVE_IMAGE_TYPE_U32C1:
            size = image->u16Stride[0]*image->u16Height*sizeof(U32);
            write(file_handle, (char*)(image->pu8Addr[0]), size);
            break;

        case MVE_IMAGE_TYPE_S16C1:
        case MVE_IMAGE_TYPE_U16C1:
            size = image->u16Stride[0]*image->u16Height*sizeof(U16);
            write(file_handle, (char*)(image->pu8Addr[0]), size);
            break;

        case MVE_IMAGE_TYPE_S8C1:
        case MVE_IMAGE_TYPE_U8C1:
            size = image->u16Stride[0]*image->u16Height*sizeof(U8);
            write(file_handle, (char*)(image->pu8Addr[0]), size);
            break;

        case MVE_IMAGE_TYPE_YUV420SP:
            size = image->u16Stride[0]*image->u16Height*sizeof(U8);
            write(file_handle, (char*)(image->pu8Addr[0]), size);
            write(file_handle, (char*)(image->pu8Addr[1]), size/2);
            break;

        case MVE_IMAGE_TYPE_YUV422SP:
            size = image->u16Stride[0]*image->u16Height*sizeof(U8);
            write(file_handle, (char*)(image->pu8Addr[0]), size);
            write(file_handle, (char*)(image->pu8Addr[1]), size);
            break;

        case MVE_IMAGE_TYPE_U8C3_PACKAGE:
            size = image->u16Stride[0]*image->u16Height*sizeof(U8) * 3;
            write(file_handle, (char*)(image->pu8Addr[0]), size);
            break;

        case MVE_IMAGE_TYPE_U8C3_PLANAR:
            size = image->u16Stride[0]*image->u16Height*sizeof(U8);
            write(file_handle, (char*)(image->pu8Addr[0]), size);
            write(file_handle, (char*)(image->pu8Addr[1]), size);
            write(file_handle, (char*)(image->pu8Addr[2]), size);
            break;

        case MVE_IMAGE_TYPE_S8C2_PACKAGE:
            size = image->u16Stride[0]*image->u16Height*sizeof(U8) * 2;
            write(file_handle, (char*)(image->pu8Addr[0]), size);
            break;

        case MVE_IMAGE_TYPE_S8C2_PLANAR:
            size = image->u16Stride[0]*image->u16Height*sizeof(U8);
            write(file_handle, (char*)(image->pu8Addr[0]), size);
            write(file_handle, (char*)(image->pu8Addr[1]), size);
            break;

        default:
            printf("Format is not support!!\n");
            return MI_MVE_RET_INVALID_PARAMETER;
    }

    return MI_RET_SUCCESS;
}

MI_RET Sample_SaveOutputImage(MVE_DST_IMAGE_S *image, const char *file_name)
{
    MI_RET ret;
    int file_handle;

    file_handle = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (file_handle <= 0)
    {
        printf("Can't open file_handle %s (%d: %s)\n", file_name, errno, strerror(errno));
        return MI_MVE_RET_INVALID_PARAMETER;
    }

    ret = Sample_SaveOutputImageEx(image, file_handle);

    close(file_handle);

    return ret;
}

MI_RET Sample_InitInputBufferEx(MVE_MEM_INFO_S *buffer, int file_handle, int size)
{
    read(file_handle, (char*)(buffer->pu8VirAddr), size);

    return MI_RET_SUCCESS;
}

MI_RET Sample_InitInputBuffer(MVE_MEM_INFO_S *buffer, const char *file_name, int size)
{
    int file_handle;
    MI_RET ret = MI_RET_SUCCESS;

    file_handle = open(file_name, O_RDONLY);
    if (file_handle <= 0)
    {
        printf("Can't open file_handle %s (%d: %s)\n", file_name, errno, strerror(errno));
        return MI_MVE_RET_INVALID_PARAMETER;
    }

    ret = read(file_handle, (char*)(buffer->pu8VirAddr), size);

    close(file_handle);

    return ret;
}

MI_RET Sample_SaveOutputBufferEx(MVE_MEM_INFO_S *buffer, int file_handle, int size)
{
    write(file_handle, buffer->pu8VirAddr, size);

    return MI_RET_SUCCESS;
}

MI_RET Sample_SaveOutputBuffer(MVE_MEM_INFO_S *buffer, const char *file_name, int size)
{
    MI_RET ret;
    int file_handle;

    file_handle = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (file_handle <= 0)
    {
        printf("Can't open file_handle %s (%d: %s)\n", file_name, errno, strerror(errno));
        return MI_MVE_RET_INVALID_PARAMETER;
    }

    ret = Sample_SaveOutputBufferEx(buffer, file_handle, size);

    close(file_handle);

    return ret;
}

MI_RET Sample_SaveOutputBufferDirect(void *buffer, const char *file_name, int size)
{
    MI_RET ret = MI_RET_SUCCESS;
    int file_handle;

    file_handle = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (file_handle <= 0)
    {
        printf("Can't open file_handle %s (%d: %s)\n", file_name, errno, strerror(errno));
        return MI_MVE_RET_INVALID_PARAMETER;
    }

    write(file_handle, buffer, size);

    close(file_handle);

    return ret;
}

int main(int argc, char **argv)
{
    printf("Run Sample_Filter()\n");
    if (Sample_Filter() != MI_RET_SUCCESS)
    {
        printf("MVE filter process is failed\n");
        return -1;
    }

    printf("Run Sample_CSC()\n");
    if (Sample_CSC() != MI_RET_SUCCESS)
    {
        printf("MVE CSC process is failed\n");
        return -1;
    }

    printf("Run Sample_FilterAndCSC()\n");
    if (Sample_FilterAndCSC() != MI_RET_SUCCESS)
    {
        printf("MVE filter CSC process is failed\n");
        return -1;
    }

    printf("Run Sample_Sobel()\n");
    if (Sample_Sobel() != MI_RET_SUCCESS)
    {
        printf("MVE Sobel process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_MagAndAng()\n");
    if (Sample_MagAndAng() != MI_RET_SUCCESS)
    {
        printf("MVE Mag and Ang process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Dilate()\n");
    if (Sample_Dilate() != MI_RET_SUCCESS)
    {
        printf("MVE Dilate process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Erode()\n");
    if (Sample_Erode() != MI_RET_SUCCESS)
    {
        printf("MVE Erode process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Thresh()\n");
    if (Sample_Thresh() != MI_RET_SUCCESS)
    {
        printf("MVE Thresh process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_And()\n");
    if (Sample_And() != MI_RET_SUCCESS)
    {
        printf("MVE And process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Sub()\n");
    if (Sample_Sub() != MI_RET_SUCCESS)
    {
        printf("MVE Sub process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Or()\n");
    if (Sample_Or() != MI_RET_SUCCESS)
    {
        printf("MVE Or process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Xor()\n");
    if (Sample_Xor() != MI_RET_SUCCESS)
    {
        printf("MVE Xor process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Add()\n");
    if (Sample_Add() != MI_RET_SUCCESS)
    {
        printf("MVE Add process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Thresh_U16()\n");
    if (Sample_Thresh_U16() != MI_RET_SUCCESS)
    {
        printf("MVE Thresh_U16 process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Thresh_S16()\n");
    if (Sample_Thresh_S16() != MI_RET_SUCCESS)
    {
        printf("MVE Thresh_S16 process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_16BitTo8Bit()\n");
    if (Sample_16BitTo8Bit() != MI_RET_SUCCESS)
    {
        printf("MVE 16bit to 8bit process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_OrdStatFilter()\n");
    if (Sample_OrdStatFilter() != MI_RET_SUCCESS)
    {
        printf("MVE Order Statistic Filter process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Bernsen()\n");
    if (Sample_Bernsen() != MI_RET_SUCCESS)
    {
        printf("MVE Bernsen process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Map()\n");
    if (Sample_Map() != MI_RET_SUCCESS)
    {
        printf("MVE Map process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_NCC()\n");
    if (Sample_NCC() != MI_RET_SUCCESS)
    {
        printf("MVE NCC process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Integ()\n");
    if (Sample_Integ() != MI_RET_SUCCESS)
    {
        printf("MVE NoiseRemoveHor process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_CCL()\n");
    if (Sample_CCL() != MI_RET_SUCCESS)
	{
        printf("MVE CCL process is failed\n");
		goto RETURN;
	}

    printf("Run Sample_Hist()\n");
    if (Sample_Hist() != MI_RET_SUCCESS)
    {
        printf("MVE Histogram process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_EqualizeHist()\n");
    if (Sample_EqualizeHist() != MI_RET_SUCCESS)
    {
        printf("MVE EqualizeHist process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_SAD()\n");
    if (Sample_SAD() != MI_RET_SUCCESS)
    {
        printf("MVE SAD process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_NormGrad()\n");
    if (Sample_NormGrad() != MI_RET_SUCCESS)
    {
        printf("MVE Normalized Gradient process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_LBP()\n");
    if (Sample_LBP() != MI_RET_SUCCESS)
    {
        printf("MVE LBP process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_GMM()\n");
    if (Sample_GMM() != MI_RET_SUCCESS)
    {
        printf("MVE GMM process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Canny()\n");
    if (Sample_Canny() != MI_RET_SUCCESS)
    {
        printf("MVE Canny process is failed\n");
		goto RETURN;
    }

    printf("Run Sample_LineFilterHor()\n");
    if (Sample_LineFilterHor() != MI_RET_SUCCESS)
    {
        printf("MVE LineFilterHor process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_NoiseRemoveHor()\n");
    if (Sample_NoiseRemoveHor() != MI_RET_SUCCESS)
    {
        printf("MVE NoiseRemoveHor process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Adpthresh()\n");
    if (Sample_Adpthresh() != MI_RET_SUCCESS)
    {
        printf("MVE Adapthresh process is failed\n");
        goto RETURN;
    }

    printf("Run Sample_Lk_Optical_Flow()\n");
    if (Sample_Lk_Optical_Flow() != MI_RET_SUCCESS)
    {
        printf("MVE LK Optical Flow process is failed\n");
        goto RETURN;
    }

RETURN:
    return 0;
}
