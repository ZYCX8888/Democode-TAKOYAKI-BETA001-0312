/*
* sample_gmm.c- Sigmastar
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

#define RAW_WIDTH       1280
#define RAW_HEIGHT      720
#define INPUT_NAME      "Img1280x720_Video.raw"
#define OUTPUT_NAME     "Output_GMM.raw"

static int Sample_GetGmmBufferSize(U16 width, U16 height, U32 nmixtures, MVE_IMAGE_TYPE_E color)
{
    U32 buf_size = 0;

    if (color == MVE_IMAGE_TYPE_U8C1)
    {
        buf_size += (sizeof(double) * 3 * width * height * nmixtures);
    }
    else
    {
        buf_size += (sizeof(double) * 5 * width * height * nmixtures);
    }

    return (buf_size);
}


MI_RET Sample_GMM()
{
    MI_RET ret;
    int input_file, output_file;

    MVE_HANDLE handle;
    MVE_IMG_INFO info = {RAW_WIDTH, RAW_HEIGHT};
    MVE_SRC_IMAGE_S src;
    MVE_DST_IMAGE_S dst_foreground, dst_background;
    MVE_MEM_INFO_S model;
    uint32_t i, frame_count;
    MVE_GMM_CTRL_S ctrl =
    {
        .u22q10NoiseVar  = 230400,
        .u22q10MaxVar    = 2048000,
        .u22q10MinVar    = 204800,
        .u0q16LearnRate  = 327,
        .u0q16BgRatio    = 52428,
        .u8q8VarThr      = 1600,
        .u0q16InitWeight = 3276,
        .u8ModelNum      = 3
    };

    memset(&src, 0, sizeof(src));
    memset(&dst_foreground, 0, sizeof(dst_foreground));
    memset(&dst_background, 0, sizeof(dst_background));

    // Init MVE
    handle = MI_MVE_Init(&info);
    if (handle == NULL)
    {
        printf("[MVE] Could not create MVE handle\n");
        return MI_MVE_RET_INIT_ERROR;
    }

    // Allocate input buffer
    ret = MI_MVE_AllocateImage(handle, &src, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Input", MVE_MEM_ALLOC_TYPE_VIRTUAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate input buffer \n");

        goto RETURN_6;
    }

    // Allocate foreground output buffer
    ret = MI_MVE_AllocateImage(handle, &dst_foreground, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Output0", MVE_MEM_ALLOC_TYPE_VIRTUAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate foreground output buffer\n");
        goto RETURN_5;
    }

    // Allocate background output buffer
    ret = MI_MVE_AllocateImage(handle, &dst_background, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Output1", MVE_MEM_ALLOC_TYPE_VIRTUAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate background output buffer\n");
        goto RETURN_4;
    }

    // Init model buffer
    ret = MI_MVE_AllocateBuffer(handle, &model, Sample_GetGmmBufferSize(src.u16Width, src.u16Height, ctrl.u8ModelNum, src.enType), "MVE_GmmModel", MVE_MEM_ALLOC_TYPE_VIRTUAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate GMM model buffer\n");
        goto RETURN_3;
    }

    // Open input file
    input_file = open(INPUT_NAME, O_RDONLY);
    if (input_file <= 0)
    {
        printf("Can't open input file %s (%d: %s)\n", INPUT_NAME, errno, strerror(errno));
        goto RETURN_2;
    }

    // Open output file
    output_file = open(OUTPUT_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (output_file <= 0)
    {
        printf("Can't open output file %s (%d: %s)\n", OUTPUT_NAME, errno, strerror(errno));
        goto RETURN_1;
    }

    // Process loop
    for (i=0; i<3; i++)
    {
        // Read frame raw
        ret = Sample_InitInputImageEx(&src, input_file);
        if (ret != MI_RET_SUCCESS)
        {
            printf("Can't read frame data\n");
            goto RETURN_1;
        }

        // Run MI_MVE_GMM()
        ret = MI_MVE_GMM(handle, &src, &dst_foreground, &dst_background, &model, &ctrl, 0);
        if (ret != MI_RET_SUCCESS)
        {
            printf("MI_MVE_GMM() return ERROR 0x%X\n", ret);
            goto RETURN_1;
        }

        // Save ouput data
        ret = Sample_SaveOutputImageEx(&dst_foreground, output_file);
        if (ret != MI_RET_SUCCESS)
        {
            printf("Can't save foreground data to output file %s\n", OUTPUT_NAME);
            goto RETURN_1;
        }

        ret = Sample_SaveOutputImageEx(&dst_background, output_file);
        if (ret != MI_RET_SUCCESS)
        {
            printf("Can't save background data to output file %s\n", OUTPUT_NAME);
            goto RETURN_1;
        }
    }

RETURN_1:
    close(output_file);
    close(input_file);
RETURN_2:
    MI_MVE_FreeBuffer(handle, &model);
RETURN_3:
    MI_MVE_FreeImage(handle, &dst_background);
RETURN_4:
    MI_MVE_FreeImage(handle, &dst_foreground);
RETURN_5:
    MI_MVE_FreeImage(handle, &src);
RETURN_6:
    MI_MVE_Uninit(handle);

    return ret;
}
