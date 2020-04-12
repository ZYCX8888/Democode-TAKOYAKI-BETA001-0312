/*
* sample_lk_optical_flow.c- Sigmastar
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

#define RAW_WIDTH       (160)
#define RAW_HEIGHT      (120)
#define INPUT_NAME_0     "Img160x120_0.raw"
#define INPUT_NAME_1     "Img160x120_1.raw"
#define OUTPUT_NAME      "Output_Lk_Optical_Flow.raw"

MI_RET Sample_Lk_Optical_Flow()
{
    MI_RET ret;
    MVE_HANDLE handle;
    MVE_IMG_INFO info = {RAW_WIDTH, RAW_HEIGHT};
    MVE_SRC_IMAGE_S src_pre, src_cur;
    MVE_SRC_MEM_INFO_S point;
    MVE_MEM_INFO_S move;
    int x, y, count;
    MVE_LK_OPTICAL_FLOW_CTRL_S ctrl =
    {
        .u16CornerNum  = 49,
        .u0q8MinEigThr = 255,
        .u8IterCount   = 10,
        .u0q8Epsilon   = 26,
        .u16PyrLevel   = 15
    };

    memset(&src_pre, 0, sizeof(src_pre));
    memset(&src_cur, 0, sizeof(src_cur));

    // Init MVE
    handle = MI_MVE_Init(&info);
    if (handle == NULL)
    {
        printf("[MVE] Could not create MVE handle\n");
        return MI_MVE_RET_INIT_ERROR;
    }

    // Allocate input buffer 0
    ret = MI_MVE_AllocateImage(handle, &src_pre, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Input 0", MVE_MEM_ALLOC_TYPE_VIRTUAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate input buffer 0\n");
        goto RETURN_5;
    }

    // Allocate input buffer 1
    ret = MI_MVE_AllocateImage(handle, &src_cur, MVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT, "MVE_Input 1", MVE_MEM_ALLOC_TYPE_VIRTUAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate input buffer 1\n");
        goto RETURN_4;
    }

    // Init point buffer
    ret = MI_MVE_AllocateBuffer(handle, &point, sizeof(MVE_POINT_S25Q7_S) * ctrl.u16CornerNum, "MVE_Optical_Flow_Point", MVE_MEM_ALLOC_TYPE_VIRTUAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate Optical Flow point buffer\n");
        goto RETURN_3;
    }

    // Init move buffer
    ret = MI_MVE_AllocateBuffer(handle, &move, sizeof(MVE_MV_S9Q7_S) * ctrl.u16CornerNum, "MVE_Optical_Flow_Mv", MVE_MEM_ALLOC_TYPE_VIRTUAL);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't allocate Optical Flow move buffer\n");
        goto RETURN_2;
    }

    // Init input buffer 0
    ret = Sample_InitInputImage(&src_pre, INPUT_NAME_0);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME_0);
        goto RETURN_1;
    }

    // Init input buffer 1
    ret = Sample_InitInputImage(&src_cur, INPUT_NAME_1);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME_1);
        goto RETURN_1;
    }

    // Init point
    count = 0;
    for (x=0; x<7; x++)
    {
        for (y=0; y<7; y++)
        {
            ((MVE_POINT_S25Q7_S*)point.pu8VirAddr)[count].s25q7X = ((src_pre.u16Width  / 13)*(x+3)) << 7;
            ((MVE_POINT_S25Q7_S*)point.pu8VirAddr)[count].s25q7Y = ((src_pre.u16Height / 13)*(y+3)) << 7;
            count++;
        }
        }

    // Run MI_MVE_LKOpticalFlow
    ret = MI_MVE_LKOpticalFlow(handle, &src_pre, &src_cur, &point, &move, &ctrl, 0);
    if (ret != MI_RET_SUCCESS)
    {
        printf("MI_MVE_LKOpticalFlow() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save ouput data
    ret = Sample_SaveOutputBuffer(&move, OUTPUT_NAME, sizeof(MVE_MV_S9Q7_S) * ctrl.u16CornerNum);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't save data to output file %s\n", OUTPUT_NAME);
        goto RETURN_1;
    }

RETURN_1:
    MI_MVE_FreeBuffer(handle, &move);
RETURN_2:
    MI_MVE_FreeBuffer(handle, &point);
RETURN_3:
    MI_MVE_FreeImage(handle, &src_cur);
RETURN_4:
    MI_MVE_FreeImage(handle, &src_pre);
RETURN_5:
    MI_MVE_Uninit(handle);

    return ret;
}
