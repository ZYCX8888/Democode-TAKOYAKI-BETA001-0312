/*
* sample_mag_and_ang.c- Sigmastar
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
#define OUTPUT_NAME_MAG         "Output_MagAndAng_Mag.raw"
#define OUTPUT_NAME_ANG         "Output_MagAndAng_Ang.raw"

MI_RET Sample_MagAndAng()
{
    MI_RET ret;
    MVE_HANDLE handle;
    MVE_IMG_INFO info = {RAW_WIDTH, RAW_HEIGHT};
	MVE_SRC_IMAGE_S src;
	MVE_DST_IMAGE_S dst_mag, dst_ang;
	MVE_MAG_AND_ANG_CTRL_S ctrl =
    {
        .enOutCtrl = MVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG,
        .u16Thr    = 0,
        .as8Mask   =
        {
             0,  0,  0,  0, 0,
             0, -1, -2, -1, 0,
             0,  0,  0,  0, 0,
             0,  1,  2,  1, 0,
             0,  0,  0,  0, 0
        }
    };

    memset(&src, 0, sizeof(src));
    memset(&dst_mag, 0, sizeof(dst_mag));
    memset(&dst_ang, 0, sizeof(dst_ang));

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

    // Allocate Mag output buffer
    ret = MI_MVE_AllocateImage(handle, &dst_mag, MVE_IMAGE_TYPE_U16C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_OutputMag", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate Mag output buffer\n");
        goto RETURN_3;
    }

    // Allocate Ang output buffer
    ret = MI_MVE_AllocateImage(handle, &dst_ang, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_OutputAng", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate Ang output buffer\n");
        goto RETURN_2;
    }

    // Init input buffer
    ret = Sample_InitInputImage(&src, INPUT_NAME);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME);
        goto RETURN_1;
    }

    // Run MI_MVE_MagAndAng()
	ret = MI_MVE_MagAndAng(handle, &src, &dst_mag, &dst_ang, &ctrl, 0);
    if (ret != MI_RET_SUCCESS)
    {
        printf("MI_MVE_MagAndAng() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save Mag ouput image
    ret = Sample_SaveOutputImage(&dst_mag, OUTPUT_NAME_MAG);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't save Mag image to output file %s\n", OUTPUT_NAME_MAG);
        goto RETURN_1;
    }

    // Save Ang ouput image
    ret = Sample_SaveOutputImage(&dst_ang, OUTPUT_NAME_ANG);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't save Ang image to output file %s\n", OUTPUT_NAME_ANG);
        goto RETURN_1;
    }

RETURN_1:
    MI_MVE_FreeImage(handle, &dst_ang);
RETURN_2:
    MI_MVE_FreeImage(handle, &dst_mag);
RETURN_3:
    MI_MVE_FreeImage(handle, &src);
RETURN_4:
    MI_MVE_Uninit(handle);

    return ret;
}
