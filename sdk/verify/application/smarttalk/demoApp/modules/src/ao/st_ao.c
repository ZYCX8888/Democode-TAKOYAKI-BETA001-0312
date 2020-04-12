#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "st_ao.h"
#include "st_common.h"


static void _mi_ao_deinterleave(MI_U8* pu8LRBuf, MI_U8* pu8LBuf, MI_U8* pu8RBuf, MI_U32 u32Size)
{
    MI_S32 s32Iidx = 0;
    MI_S32 s32Jidx = 0;
    MI_U32 u32MaxSize = 0;
    MI_S32 s32LeftIdx = 0;
    MI_S32 s32RightIdx = 0;

    u32MaxSize = u32Size/4;

    for(s32Iidx = 0; s32Iidx < u32MaxSize; s32Iidx++)
    {
        pu8LBuf[s32LeftIdx++] = pu8LRBuf[s32Jidx++];
        pu8LBuf[s32LeftIdx++] = pu8LRBuf[s32Jidx++];
        pu8RBuf[s32RightIdx++] = pu8LRBuf[s32Jidx++];
        pu8RBuf[s32RightIdx++] = pu8LRBuf[s32Jidx++];
    }
}

static void _mi_ao_interleave(MI_U8* pu8LRBuf, MI_U8* pu8LBuf, MI_U8* pu8RBuf, MI_U32 u32Size)
{
    MI_S32 s32Iidx = 0;
    MI_S32 s32Jidx = 0;
    for(s32Iidx=0; s32Iidx<u32Size/2; s32Iidx++)
    {
        pu8LRBuf[s32Jidx++] = pu8LBuf[s32Iidx];
        pu8LRBuf[s32Jidx++] = pu8RBuf[s32Iidx];
    }

}






