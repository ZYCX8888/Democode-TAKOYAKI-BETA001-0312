/*
* sample_canny.c- Sigmastar
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
#define OUTPUT_NAME  "Output_Canny.raw"

MI_RET Sample_Canny()
{
    int i;
    MI_RET ret;
    MVE_HANDLE handle;
    MVE_IMG_INFO info = {RAW_WIDTH, RAW_HEIGHT};
    MVE_SRC_IMAGE_S src;
    MVE_DST_IMAGE_S dst;
    MVE_DST_MEM_INFO_S stack;
    MVE_CANNY_HYS_EDGE_CTRL_S ctrl =
    {
        .u16LowThr  = 7*2,
        .u16HighThr = 14*2,
        .as8Mask =
        {
            0,  0, 0, 0, 0,
            0, -1, 0, 1, 0,
            0, -2, 0, 2, 0,
            0, -1, 0, 1, 0,
            0,  0, 0, 0, 0
        }
    };

    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));
    memset(&ctrl.stMem, 0, sizeof(ctrl.stMem));

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
        goto RETURN_5;
    }

    // Allocate output buffer
    ret = MI_MVE_AllocateImage(handle, &dst, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Output", MVE_MEM_ALLOC_TYPE_PHYSICAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate output buffer\n");
        goto RETURN_4;
    }

    // Init stack buffer
    ret = MI_MVE_AllocateBuffer(handle, &stack, RAW_WIDTH*RAW_HEIGHT*sizeof(MVE_POINT_U16_S) + sizeof(MVE_CANNY_STACK_SIZE_S), "MVE_CannyInfo", MVE_MEM_ALLOC_TYPE_VIRTUAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate canny stack buffer\n");
        goto RETURN_3;
    }

    // Init working buffer
    ret = MI_MVE_AllocateBuffer(handle, &ctrl.stMem, RAW_WIDTH*(RAW_HEIGHT+3)*4, "MVE_CannyMem", MVE_MEM_ALLOC_TYPE_VIRTUAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate canny working buffer\n");
        goto RETURN_2;
    }

    // Init input buffer
    ret = Sample_InitInputImage(&src, INPUT_NAME);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't init input buffer\n");
        goto RETURN_1;
    }

    // Run MI_MVE_CannyHysEdge()
    ret = MI_MVE_CannyHysEdge(handle, &src, &dst, &stack, &ctrl, 0);
    if (ret != MI_RET_SUCCESS)
    {
        printf("MI_MVE_CannyHysEdge() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Run MI_MVE_CannyEdge()
    ret = MI_MVE_CannyEdge(handle, &dst, &stack);
    if (ret != MI_RET_SUCCESS)
    {
        printf("MI_MVE_CannyEdge() return ERROR 0x%X\n", ret);
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
    MI_MVE_FreeBuffer(handle, &ctrl.stMem);
RETURN_2:
    MI_MVE_FreeBuffer(handle, &stack);
RETURN_3:
    MI_MVE_FreeImage(handle, &dst);
RETURN_4:
    MI_MVE_FreeImage(handle, &src);
RETURN_5:
    MI_MVE_Uninit(handle);

    return ret;
}
