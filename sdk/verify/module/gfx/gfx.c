#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "mi_gfx.h"
#include "mi_gfx_datatype.h"

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("[%d]exec function failed\n", __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("(%d)exec function pass\n", __LINE__);\
    }

int main(int argc, const char *argv[])
{
    MI_BOOL bWaitAllDone;
    MI_U16 u16TargetFence;
    MI_GFX_Surface_t stSrc, stDst;
    MI_GFX_Rect_t stSrcRect, stDstRect;

    MI_U32 u32ColorVal = 0x1F; // ARGB:Blue = 0x1F
    MI_U32 phyAddr = 0x2000000; // For example: Fill a rect to Fb. (u32PhyAddr = Fb Start Addr)

    MI_U32 phySrcAddr, phyDstAddr;

    ExecFunc(MI_GFX_Open(), MI_SUCCESS);

    //TODO: Dosomething bitblit/fill
    stDst.eColorFmt = E_MI_GFX_FMT_ARGB1555;
    stDst.u32Width = 1280;
    stDst.u32Height = 720;
    stDst.u32Stride = 1280 * 2;
    stDst.phyAddr = phyAddr;

    stDstRect.s32Xpos = 100;
    stDstRect.s32Ypos = 100;
    stDstRect.u32Width = 100;
    stDstRect.u32Height = 100;
    ExecFunc(MI_GFX_QuickFill(&stDst, &stDstRect, u32ColorVal, &u16TargetFence), MI_SUCCESS);
    ExecFunc(MI_GFX_WaitAllDone(FALSE, u16TargetFence), MI_SUCCESS);

    //phySrcPhyAddr ---Read data from a.bmp(argb1555) 352*288
    //phyDstPhyAddr ---Read data from b.bmp(argb1555 720*576

    stSrc.eColorFmt = E_MI_GFX_FMT_ARGB1555;
    stSrc.u32Width = 1280;
    stSrc.u32Height = 720;
    stSrc.u32Stride = 1280 * 2;
    stSrc.phyAddr = phySrcAddr;

    stSrcRect.s32Xpos = 100;
    stSrcRect.s32Ypos = 100;
    stSrcRect.u32Width = 100;
    stSrcRect.u32Height = 100;

    stDst.eColorFmt = E_MI_GFX_FMT_ARGB1555;
    stDst.u32Width = 1280;
    stDst.u32Height = 720;
    stDst.u32Stride = 1280 * 2;
    stDst.phyAddr = phyDstAddr;

    stDstRect.s32Xpos = 100;
    stDstRect.s32Ypos = 100;
    stDstRect.u32Width = 100;
    stDstRect.u32Height = 100;
    ExecFunc(MI_GFX_BitBlit(&stSrc, &stSrcRect, &stDst, &stDstRect, NULL, &u16TargetFence), MI_SUCCESS);
    ExecFunc(MI_GFX_WaitAllDone(FALSE, u16TargetFence), MI_SUCCESS);

    ExecFunc(MI_GFX_Close(), MI_SUCCESS);

    return 0;
}
