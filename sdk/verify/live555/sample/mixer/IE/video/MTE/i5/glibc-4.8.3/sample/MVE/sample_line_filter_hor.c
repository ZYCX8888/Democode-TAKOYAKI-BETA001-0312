/*
* sample_line_filter_hor.c- Sigmastar
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
#define OUTPUT_NAME  "Output_LineFilterHor.raw"

MI_RET Sample_LineFilterHor()
{
	MI_RET ret;
	MVE_HANDLE handle;
    MVE_IMG_INFO info = {RAW_WIDTH,RAW_HEIGHT};
	MVE_IMAGE_S src;
    MVE_LINE_FILTER_HOR_CTRL_S ctrl =
    {
        .u8GapMinLen  = 10,  //Gap min size (range : 1 ~ 20 ; default : 10)
        .u8DensityThr = 20,  //Density threshold (range : 1 ~ 50 ; default : 20)
        .u8HorThr     = 20   //Horizontal size threshold (range : 1 ~ 50 ; default : 20)
    };

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
		 goto RETURN_3;
	 }

	 // Init input buffer
	 ret = Sample_InitInputImage(&src, INPUT_NAME);
	 if (ret != MI_RET_SUCCESS)
	 {
		 printf("Can't init input buffer\n");
		 goto RETURN_1;
	 }

	 // Run MI_MVE_LineFilterHor()
	 ret = MI_MVE_LineFilterHor(handle, &src, &ctrl, 0);
	 if (ret != MI_RET_SUCCESS)
	 {
		 printf("MI_MVE_LineFilterHor() return ERROR 0x%X\n", ret);
		 goto RETURN_1;
	 }

RETURN_1:
 MI_MVE_FreeImage(handle, &src);
RETURN_3:
 MI_MVE_Uninit(handle);

		return MI_RET_SUCCESS;
}
