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
#include "mi_gfx.h"
#include "mi_gfx_datatype.h"
#include "mi_sys.h"

#define SRC_WIDTH 1280
#define SRC_HEIGHT 720
#define DST_WIDTH 640
#define DST_HEIGHT 360
#define SRC_FILE_NAME "/var/1280X720.argb1555"
#define DST_FILE_NAME "/var/640X360.argb1555"
#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("[%s][%d]exec function failed\n", __FUNCTION__, __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("[%s][%d]exec function pass\n", __FUNCTION__, __LINE__);\
    }

int main(int argc, const char *argv[])
{
    MI_BOOL bWaitAllDone;
    MI_U16 u16TargetFence;
    MI_GFX_Surface_t stDst;//, stSrc;
    MI_GFX_Rect_t stDstRect;//stSrcRect;//, stDstRect;

    MI_U32 u32ColorVal = 0xFFFFFFFF; // ARGB:Blue = 0x1F
    MI_PHY phyAddr; // For example: Fill a rect to Fb. (u32PhyAddr = Fb Start Addr)
    MI_U64 u64VirAddr;

    MI_PHY phyAddr2;
    MI_U64 u64VirAddr2;
    MI_GFX_Surface_t stSrc;
    MI_GFX_Rect_t stSrcRect;
    MI_GFX_Opt_t stOpt;

    ExecFunc(MI_SYS_Init(), MI_SUCCESS);
    FILE *fp = NULL;
    FILE *dstfp = NULL;
    fp = fopen(SRC_FILE_NAME, "wb");
    dstfp = fopen(DST_FILE_NAME, "wb");
    ExecFunc(MI_SYS_MMA_Alloc("mma_heap_name0", SRC_WIDTH*SRC_HEIGHT*2, &phyAddr), MI_SUCCESS);
    ExecFunc(MI_SYS_MMA_Alloc("mma_heap_name0", DST_WIDTH*360*2, &phyAddr2), MI_SUCCESS);

    //MI_U32 phySrcAddr, phyDstAddr;
    ExecFunc(MI_GFX_Open(), MI_SUCCESS);
    ExecFunc(MI_SYS_Mmap(phyAddr, SRC_WIDTH*SRC_HEIGHT*2, &u64VirAddr , FALSE), MI_SUCCESS);
    memset(u64VirAddr, 0x22, SRC_WIDTH*SRC_HEIGHT*2);

    ExecFunc(MI_SYS_Mmap(phyAddr2, DST_WIDTH*DST_HEIGHT*2, &u64VirAddr2 , FALSE), MI_SUCCESS);
    memset(u64VirAddr2, 0x0F, DST_WIDTH*DST_HEIGHT*2);

    //TODO: Dosomething bitblit/fill

    //fillrect
    stSrc.eColorFmt = E_MI_GFX_FMT_ARGB1555;
    stSrc.u32Width = SRC_WIDTH;
    stSrc.u32Height = SRC_HEIGHT;
    stSrc.u32Stride = SRC_WIDTH * 2;
    stSrc.phyAddr = phyAddr;

    stSrcRect.s32Xpos = 100;
    stSrcRect.s32Ypos = 100;
    stSrcRect.u32Width = 100;
    stSrcRect.u32Height = 100;
    ExecFunc(MI_GFX_QuickFill(&stSrc, &stSrcRect, u32ColorVal, &u16TargetFence), MI_SUCCESS);
    ExecFunc(MI_GFX_WaitAllDone(FALSE, u16TargetFence), MI_SUCCESS);
    if (NULL != fp)
    {
        fwrite(u64VirAddr, 1, SRC_WIDTH*SRC_HEIGHT*2, fp);
        fclose(fp);
        fp = NULL;
    }

    //bitblit
    stSrc.eColorFmt = E_MI_GFX_FMT_ARGB1555;
    stSrc.u32Width = SRC_WIDTH;
    stSrc.u32Height = SRC_HEIGHT;
    stSrc.u32Stride = SRC_WIDTH * 2;
    stSrc.phyAddr = phyAddr;

    stSrcRect.s32Xpos = 100;
    stSrcRect.s32Ypos = 100;
    stSrcRect.u32Width = 300;
    stSrcRect.u32Height = 300;

    stDst.eColorFmt = E_MI_GFX_FMT_ARGB1555;
    stDst.u32Width = DST_WIDTH;
    stDst.u32Height = DST_HEIGHT;
    stDst.u32Stride = DST_WIDTH * 2;
    stDst.phyAddr = phyAddr2;

    stDstRect.s32Xpos = 200;
    stDstRect.s32Ypos = 100;
    stDstRect.u32Width = 200;
    stDstRect.u32Height = 100;
    
    memset(&stOpt, 0, sizeof(stOpt));

    stOpt.eSrcDfbBldOp = E_MI_GFX_DFB_BLD_ONE;
    stOpt.eDstDfbBldOp = E_MI_GFX_DFB_BLD_ZERO;
    stOpt.eMirror = E_MI_GFX_MIRROR_NONE;
    stOpt.eRotate = E_MI_GFX_ROTATE_0;

    stOpt.stClipRect.s32Xpos = stDstRect.s32Xpos;
    stOpt.stClipRect.s32Ypos = stDstRect.s32Ypos;
    stOpt.stClipRect.u32Width = stDstRect.u32Width;
    stOpt.stClipRect.u32Height = stDstRect.u32Height;
    stOpt.u32GlobalSrcConstColor = 0xFF000000;
    stOpt.u32GlobalDstConstColor = 0xFF000000;
    
    ExecFunc(MI_GFX_BitBlit(&stSrc, &stSrcRect, &stDst, &stDstRect, &stOpt, &u16TargetFence), MI_SUCCESS);
    ExecFunc(MI_GFX_WaitAllDone(FALSE, u16TargetFence), MI_SUCCESS);
    if (NULL != dstfp)
    {
        fwrite(u64VirAddr2, 1, DST_WIDTH*DST_HEIGHT*2, dstfp);
        fclose(dstfp);
        dstfp = NULL;
    }

    ExecFunc(MI_SYS_Munmap(u64VirAddr, SRC_WIDTH*SRC_HEIGHT*2), MI_SUCCESS);
    ExecFunc(MI_SYS_MMA_Free(phyAddr), MI_SUCCESS);

    ExecFunc(MI_SYS_Munmap(u64VirAddr2, DST_WIDTH*DST_HEIGHT*2), MI_SUCCESS);
    ExecFunc(MI_SYS_MMA_Free(phyAddr2), MI_SUCCESS);

    ExecFunc(MI_GFX_Close(), MI_SUCCESS);

    return 0;
}
