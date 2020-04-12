/*
* sample_ccl.c- Sigmastar
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
#define OUTPUT_NAME  "Output_CCL.raw"

MI_RET Sample_CCL()
{
    MI_RET ret;
    MVE_IMG_INFO info;
    MVE_HANDLE handle;
    MVE_IMAGE_S src;
    MVE_CCBLOB_S blob;
    MVE_CCL_CTRL_S ctrl =
    {
        .u16InitAreaThr = 4,
        .u16Step        = 2
    };

    memset(&src, 0, sizeof(src));

    info.width = RAW_WIDTH;
    info.height = RAW_HEIGHT;

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
        goto RETURN_3;
    }

    // Init input buffer
    ret = Sample_InitInputImage(&src, INPUT_NAME);
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't init input buffer\n");
        goto RETURN_1;
    }

    // Run MI_MVE_LBP()
    ret = MI_MVE_CCL(handle, &src, &blob, &ctrl, 0);
    if (ret != MI_RET_SUCCESS)
    {
        printf("MI_MVE_CCL() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save ouput data
    ret = Sample_SaveOutputBufferDirect((void*)&blob, OUTPUT_NAME, sizeof(blob));
    if (ret != MI_RET_SUCCESS)
    {
        printf("Can't save data to output file %s\n", OUTPUT_NAME);
        goto RETURN_1;
    }

RETURN_1:
    MI_MVE_FreeImage(handle, &src);
RETURN_3:
    MI_MVE_Uninit(handle);

    return ret;
}
