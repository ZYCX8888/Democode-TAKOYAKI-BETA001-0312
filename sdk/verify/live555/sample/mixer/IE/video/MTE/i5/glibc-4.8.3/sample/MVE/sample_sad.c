/*
* sample_sad.c- Sigmastar
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

#define RAW_WIDTH       1280
#define RAW_HEIGHT      720
#define INPUT_NAME_0    "Img1280x720_0.raw"
#define INPUT_NAME_1    "Img1280x720_1.raw"
#define OUTPUT_NAME_0   "Output_SAD_0.raw"
#define OUTPUT_NAME_1   "Output_SAD_1.raw"

MI_RET Sample_SAD()
{
    MI_RET ret;
    MVE_HANDLE handle;
    MVE_IMG_INFO info = {RAW_WIDTH, RAW_HEIGHT};
    MVE_SRC_IMAGE_S src0, src1;
    MVE_DST_IMAGE_S dst0, dst1;
    MVE_SAD_CTRL_S ctrl =
    {
        .enMode    = MVE_SAD_MODE_MB_8X8,
        .enOutCtrl = MVE_SAD_OUT_CTRL_16BIT_BOTH,
        .u16Thr    = 8160,
        .u8MinVal  = 0,
        .u8MaxVal  = 255
    };

    memset(&src0, 0, sizeof(src0));
    memset(&src1, 0, sizeof(src1));
    memset(&dst0, 0, sizeof(dst0));
    memset(&dst1, 0, sizeof(dst0));

    // Init MVE
    handle = MI_MVE_Init(&info);
    if (handle == NULL)
    {
        printf("[MVE] Could not create MVE handle\n");
		return MI_MVE_RET_INIT_ERROR;
    }

    // Allocate input buffer 0
    ret = MI_MVE_AllocateImage(handle, &src0, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Input0", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate input buffer 0\n");
        goto RETURN_5;
    }

    // Allocate input buffer 1
    ret = MI_MVE_AllocateImage(handle, &src1, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Input1", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate input buffer 1\n");
        goto RETURN_4;
    }

    // Allocate output buffer 0
    ret = MI_MVE_AllocateImage(handle, &dst0, MVE_IMAGE_TYPE_U16C1, RAW_WIDTH/8, RAW_WIDTH/8, RAW_HEIGHT/8, "MVE_Output0", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate output buffer 0\n");
        goto RETURN_3;
    }

    // Allocate output buffer 1
    ret = MI_MVE_AllocateImage(handle, &dst1, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH/8, RAW_WIDTH/8, RAW_HEIGHT/8, "MVE_Output1", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate output buffer 1\n");
        goto RETURN_2;
    }

    // Init input buffer 0
    ret = Sample_InitInputImage(&src0, INPUT_NAME_0);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME_0);
        goto RETURN_1;
    }

    // Init input buffer 0
    ret = Sample_InitInputImage(&src1, INPUT_NAME_1);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME_1);
        goto RETURN_1;
    }

    // Run MI_MVE_SAD()
    ret = MI_MVE_SAD(handle, &src0, &src1, &dst0, &dst1, &ctrl, 0);
    if (ret != MI_RET_SUCCESS)
    {
        printf("MI_MVE_SAD() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save ouput data
    ret = Sample_SaveOutputImage(&dst0, OUTPUT_NAME_0);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't save output 0 to output file %s\n", OUTPUT_NAME_0);
        goto RETURN_1;
    }

    ret = Sample_SaveOutputImage(&dst1, OUTPUT_NAME_1);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't save output 1 to output file %s\n", OUTPUT_NAME_1);
        goto RETURN_1;
    }

RETURN_1:
    MI_MVE_FreeImage(handle, &dst1);
RETURN_2:
    MI_MVE_FreeImage(handle, &dst0);
RETURN_3:
    MI_MVE_FreeImage(handle, &src1);
RETURN_4:
    MI_MVE_FreeImage(handle, &src0);
RETURN_5:
    MI_MVE_Uninit(handle);

    return ret;
}
