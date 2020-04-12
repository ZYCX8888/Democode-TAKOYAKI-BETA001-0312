/*
* sample_adp_thresh.c- Sigmastar
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

#define RAW_WIDTH    640
#define RAW_HEIGHT   480
#define INPUT_NAME   "Img640x480.raw"
#define OUTPUT_NAME  "Output_Adpthresh.raw"

MI_RET Sample_Adpthresh()
{
	MI_RET ret;
	MVE_HANDLE handle;
	MVE_IMG_INFO info = {RAW_WIDTH, RAW_HEIGHT};
	MVE_SRC_IMAGE_S src , integ_src;
	MVE_DST_IMAGE_S dst;
	MVE_INTEG_CTRL_S integ_ctrl =
    {
        .enOutCtrl = MVE_INTEG_OUT_CTRL_SUM
    };

	MVE_ADP_THRESH_CTRL_S adp_thresh_ctrl =
    {
        .u8RateThr   = 10,      //Adaptive threshold ratio(range : 1 ~ 20) (For OpenCV : 10)
	    .u8HalfMaskx = 20,      //Half Mask x : 1 ~ 40
	    .u8HalfMasky = 20,      //Half Mask y : 1 ~ 40
	    .s16Offset   = -128,    //Range : -128 ~ 127
	    .u8ValueThr  = 100      //Range : 1 ~ 255
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
	ret = MI_MVE_AllocateImage(handle, &src, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Input", MVE_MEM_ALLOC_TYPE_VIRTUAL);
	if (ret != MI_RET_SUCCESS)
	{
		printf("Can't allocate input buffer\n");
		goto RETURN_4;
	}

	// Allocate input buffer 2
	ret = MI_MVE_AllocateImage(handle, &integ_src, MVE_IMAGE_TYPE_U32C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Input1", MVE_MEM_ALLOC_TYPE_VIRTUAL);
	if (ret != MI_RET_SUCCESS)
	{
		printf("Can't allocate input2 buffer\n");
		goto RETURN_3;
	}

	// Allocate output buffer
	ret = MI_MVE_AllocateImage(handle, &dst, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Output", MVE_MEM_ALLOC_TYPE_VIRTUAL);
	if (ret != MI_RET_SUCCESS)
	{
		printf("Can't allocate output buffer\n");
		goto RETURN_2;
	}

	 // Init input buffer
	ret = Sample_InitInputImage(&src, INPUT_NAME);
	if (ret != MI_RET_SUCCESS)
	{
		printf("Can't init input buffer\n");
		goto RETURN_1;
	}

	// Run MI_MVE_Integ()
	ret = MI_MVE_Integ(handle, &src, &integ_src, &integ_ctrl, 0);
	if (ret != MI_RET_SUCCESS)
	{
		printf("MI_MVE_Filter() return ERROR 0x%X\n", ret);
		goto RETURN_1;
	}

	// Run MI_Adp_Thresh()
	ret = MI_MVE_AdpThresh(handle, &src, &integ_src, &dst, &adp_thresh_ctrl, 0);
	if (ret != MI_RET_SUCCESS)
	{
		printf("MI_MVE_AdpThresh() return ERROR 0x%X\n", ret);
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
	MI_MVE_FreeImage(handle, &integ_src);
RETURN_3:
	MI_MVE_FreeImage(handle, &src);
RETURN_4:
	MI_MVE_Uninit(handle);

	return ret;
}
