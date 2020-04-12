/*
* sample_and.c- Sigmastar
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
#define OUTPUT_NAME     "Output_And.raw"

MI_RET Sample_And()
{
    MI_RET ret;
    MVE_HANDLE handle;
    MVE_IMG_INFO info = {RAW_WIDTH, RAW_HEIGHT};
    MVE_SRC_IMAGE_S src0, src1;
    MVE_DST_IMAGE_S dst;

    memset(&src0, 0, sizeof(src0));
    memset(&src1, 0, sizeof(src1));
    memset(&dst, 0, sizeof(dst));

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
        goto RETURN_4;
    }

    // Allocate input buffer 1
    ret = MI_MVE_AllocateImage(handle, &src1, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Input1", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate input buffer 1\n");
        goto RETURN_3;
    }

    // Allocate output buffer
    ret = MI_MVE_AllocateImage(handle, &dst, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Output", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate output buffer\n");
        goto RETURN_2;
    }

    // Init input buffer 0
    ret = Sample_InitInputImage(&src0, INPUT_NAME_0);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME_0);
        goto RETURN_1;
    }

    // Init input buffer 1
    ret = Sample_InitInputImage(&src1, INPUT_NAME_1);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME_1);
        goto RETURN_1;
    }

    // Run MI_MVE_And
    ret = MI_MVE_And(handle, &src0, &src1, &dst, 0);
    if (ret != MI_RET_SUCCESS)
    {
        printf("MI_MVE_And() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save ouput data
    ret = Sample_SaveOutputImage(&dst, OUTPUT_NAME);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't save data to output file %s\n", OUTPUT_NAME);
        goto RETURN_1;
    }

RETURN_1:
    MI_MVE_FreeImage(handle, &dst);
RETURN_2:
    MI_MVE_FreeImage(handle, &src1);
RETURN_3:
    MI_MVE_FreeImage(handle, &src0);
RETURN_4:
    MI_MVE_Uninit(handle);

    return ret;
}
