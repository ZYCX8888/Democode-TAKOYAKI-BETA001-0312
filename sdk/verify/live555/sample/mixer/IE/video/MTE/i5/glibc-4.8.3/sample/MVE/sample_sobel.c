/*
* sample_sobel.c- Sigmastar
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

#define RAW_WIDTH               1280
#define RAW_HEIGHT              720
#define INPUT_NAME              "Img1280x720_0.raw"
#define OUTPUT_NAME_HORIZONTAL  "Output_Sobel_Horizontal.raw"
#define OUTPUT_NAME_VIRTICAL    "Output_Sobel_Vertical.raw"

MI_RET Sample_Sobel()
{
    MI_RET ret;
    MVE_HANDLE handle;
    MVE_IMG_INFO info = {RAW_WIDTH, RAW_HEIGHT};
	MVE_SRC_IMAGE_S src;
	MVE_DST_IMAGE_S dst_h, dst_v;
	MVE_SOBEL_CTRL_S ctrl =
    {
        .enOutCtrl = MVE_SOBEL_OUT_CTRL_BOTH,
        .as8Mask =
        {
             0,  0,  0,  0, 0,
             0, -1, -2, -1, 0,
             0,  0,  0,  0, 0,
             0,  1,  2,  1, 0,
             0,  0,  0,  0, 0
        }
    };

    memset(&src, 0, sizeof(src));
    memset(&dst_h, 0, sizeof(dst_h));
    memset(&dst_v, 0, sizeof(dst_v));

    // Init MVE
	handle = MI_MVE_Init(&info);
    if (handle == NULL)
	{
		printf("[MVE] Could not create MVE handle\n");
		return MI_MVE_RET_INIT_ERROR;
	}

    // Allocate input buffer
    ret = MI_MVE_AllocateImage(handle, &src, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Input", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate input buffer\n");
        goto RETURN_4;
    }

    // Allocate horizontal output buffer
    ret = MI_MVE_AllocateImage(handle, &dst_h, MVE_IMAGE_TYPE_S16C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_OutputH", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate horizontal output buffer\n");
        goto RETURN_3;
    }

    // Allocate virtual output buffer
    ret = MI_MVE_AllocateImage(handle, &dst_v, MVE_IMAGE_TYPE_S16C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_OutputV", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate virtual output buffer\n");
        goto RETURN_2;
    }

    // Init input buffer
    ret = Sample_InitInputImage(&src, INPUT_NAME);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME);
        goto RETURN_1;
    }

    // Run MI_MVE_Sobel()
	ret = MI_MVE_Sobel(handle, &src, &dst_h, &dst_v, &ctrl, 0);
    if (ret != MI_RET_SUCCESS)
    {
        printf("MI_MVE_Sobel() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save horizontal ouput image
    ret = Sample_SaveOutputImage(&dst_h, OUTPUT_NAME_HORIZONTAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't save horizontal image to output file %s\n", OUTPUT_NAME_HORIZONTAL);
        goto RETURN_1;
    }

    // Save vertical ouput image
    ret = Sample_SaveOutputImage(&dst_v, OUTPUT_NAME_VIRTICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't save vertical image to output file %s\n", OUTPUT_NAME_VIRTICAL);
        goto RETURN_1;
    }

RETURN_1:
    MI_MVE_FreeImage(handle, &dst_v);
RETURN_2:
    MI_MVE_FreeImage(handle, &dst_h);
RETURN_3:
    MI_MVE_FreeImage(handle, &src);
RETURN_4:
    MI_MVE_Uninit(handle);

    return ret;
}
