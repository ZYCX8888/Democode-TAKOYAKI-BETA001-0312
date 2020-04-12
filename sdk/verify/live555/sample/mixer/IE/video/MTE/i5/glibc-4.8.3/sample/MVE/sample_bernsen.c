/*
* sample_bernsen.c- Sigmastar
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

#define RAW_WIDTH    1280
#define RAW_HEIGHT   720
#define INPUT_NAME   "Img1280x720_0.raw"
#define OUTPUT_NAME  "Output_Bernsen.raw"

MI_RET Sample_Bernsen()
{
    MI_RET ret;
    MVE_HANDLE handle;
    MVE_IMG_INFO info = {RAW_WIDTH, RAW_HEIGHT};
	MVE_SRC_IMAGE_S src;
	MVE_DST_IMAGE_S dst;
    MVE_BERNSEN_CTRL_S ctrl =
    {
        .enMode     = MVE_BERNSEN_MODE_NORMAL,
        .u8WinSize  = 3,
        .u8Thr      = 0
    };

    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));

    // Init MVE
    handle = MI_MVE_Init(&info);
	if (handle == NULL)
	{
		printf("Could not create MVE handle\n");
		return MI_MVE_RET_INIT_ERROR;
	}

    // Allocate input buffer
    ret = MI_MVE_AllocateImage(handle, &src, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Input", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate input buffer\n");
        goto RETURN_3;
    }

    // Allocate output buffer
    ret = MI_MVE_AllocateImage(handle, &dst, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Output", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate output buffer\n");
        goto RETURN_2;
    }

    // Init input buffer
    ret = Sample_InitInputImage(&src, INPUT_NAME);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME);
        goto RETURN_1;
    }

    // Run MI_MVE_Bernsen()
	ret = MI_MVE_Bernsen(handle, &src, &dst, &ctrl, 0);
    if (ret != MI_RET_SUCCESS)
    {
        printf("MI_MVE_Bernsen() return ERROR 0x%X\n", ret);
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
    MI_MVE_FreeImage(handle, &src);
RETURN_3:
    MI_MVE_Uninit(handle);

    return ret;
}
