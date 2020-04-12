/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"
#include "mi_warp.h"

#include "st_common.h"
#include "st_warp.h"

MI_S32 ST_Warp_Init(ST_Warp_Timming_e eTimming)
{
    printf("ST Warp Init!!!\n");
    ExecFunc(MI_WARP_CreateDevice(0), MI_WARP_OK);
    ExecFunc(MI_WARP_StartDev(0), MI_WARP_OK);
    switch(eTimming)
    {
        case E_WARP_320_240_NV12:
        {
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_BOUND_BOX_TABLE, "warp/disp_abs_320x240_nv12/bb_320x240_nv12.bin"), MI_WARP_OK);
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_DISP_ABSOLUTE_TABLE, "warp/disp_abs_320x240_nv12/mi_320x240_abs_disp.bin"), MI_WARP_OK);
        }
        break;
        case E_WARP_640_480_NV12:
        {
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_BOUND_BOX_TABLE, "warp/disp_abs_640x480_nv12/bb_640x480_nv12.bin"), MI_WARP_OK);
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_DISP_ABSOLUTE_TABLE, "warp/disp_abs_640x480_nv12/mi_640x480_abs_disp.bin"), MI_WARP_OK);
        }
        break;
        case E_WARP_1280_720_NV12:
        {
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_BOUND_BOX_TABLE, "warp/disp_abs_1280x720_nv12/bb_1280x720_nv12.bin"), MI_WARP_OK);
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_DISP_ABSOLUTE_TABLE, "warp/disp_abs_1280x720_nv12/mi_1280x720_abs_disp.bin"), MI_WARP_OK);
        }
        break;
        case E_WARP_1920_1080_NV12:
        {
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_BOUND_BOX_TABLE, "warp/disp_abs_1920x1080_nv12/bb_1920x1080_nv12.bin"), MI_WARP_OK);
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_DISP_ABSOLUTE_TABLE, "warp/disp_abs_1920x1080_nv12/mi_1920x1080_abs_disp.bin"), MI_WARP_OK);
        }
        break;
        case E_WARP_2560_1440_NV12:
        {
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_BOUND_BOX_TABLE, "warp/disp_abs_2560x1440_nv12/bb_2560x1440_nv12.bin"), MI_WARP_OK);
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_DISP_ABSOLUTE_TABLE, "warp/disp_abs_2560x1440_nv12/mi_2560x1440_abs_disp.bin"), MI_WARP_OK);
        }
        break;
        case E_WARP_3840_2160_NV12:
        {
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_BOUND_BOX_TABLE, "warp/disp_abs_3840x2160_nv12/bb_3840x2160_nv12.bin"), MI_WARP_OK);
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_DISP_ABSOLUTE_TABLE, "warp/disp_abs_3840x2160_nv12/mi_3840x2160_abs_disp.bin"), MI_WARP_OK);
        }
        break;
        case E_WARP_320_240_NV16:
        {
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_BOUND_BOX_TABLE, "warp/disp_abs_320x240_nv16/bb_320x240_nv16.bin"), MI_WARP_OK);
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_DISP_ABSOLUTE_TABLE, "warp/disp_abs_320x240_nv16/mi_320x240_abs_disp.bin"), MI_WARP_OK);
        }
        break;
        case E_WARP_640_480_NV16:
        {
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_BOUND_BOX_TABLE, "warp/disp_abs_640x480_nv16/bb_640x480_nv16.bin"), MI_WARP_OK);
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_DISP_ABSOLUTE_TABLE, "warp/disp_abs_640x480_nv16/mi_640x480_abs_disp.bin"), MI_WARP_OK);
        }
        break;
        case E_WARP_1280_720_NV16:
        {
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_BOUND_BOX_TABLE, "warp/disp_abs_1280x720_nv16/bb_1280x720_nv16.bin"), MI_WARP_OK);
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_DISP_ABSOLUTE_TABLE, "warp/disp_abs_1280x720_nv16/mi_1280x720_abs_disp.bin"), MI_WARP_OK);
        }
        break;
        case E_WARP_1920_1080_NV16:
        {
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_BOUND_BOX_TABLE, "warp/disp_abs_1920x1080_nv16/bb_1920x1080_nv16.bin"), MI_WARP_OK);
            ExecFunc(MI_WARP_SetChnBin(0, 0, MI_WARP_DISP_ABSOLUTE_TABLE, "warp/disp_abs_1920x1080_nv16/mi_1920x1080_abs_disp.bin"), MI_WARP_OK);
        }
        break;
        default:
            printf("error warp timming!\n");
            break;
    }

    return MI_SUCCESS;
}

MI_S32 ST_Warp_Exit(void)
{
    ExecFunc(MI_WARP_StopDev(0), MI_WARP_OK);
    ExecFunc(MI_WARP_DestroyDevice(0), MI_WARP_OK);

    return MI_SUCCESS;
}

MI_S32 ST_Warp_CreateChannel(MI_WARP_CHN warpChn)
{

    ExecFunc(MI_WARP_CreateChannel(0, warpChn), MI_WARP_OK);

    return MI_SUCCESS;
}

MI_S32 ST_Warp_DestroyChannel(MI_WARP_CHN warpChn)
{
    ExecFunc(MI_WARP_DestroyChannel(0, warpChn), MI_WARP_OK);

    return MI_SUCCESS;
}


