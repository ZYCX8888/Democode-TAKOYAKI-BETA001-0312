#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "sstardisp.h"
#include "vdisp_common.h"


MI_U32 _MI_VDISP_IMPL_CalFrameSize(MI_SYS_PixelFormat_e ePixelFmt, MI_U16 u16Width, MI_U16 u16Height)
{
    MI_U32 size = 0;

    switch(ePixelFmt) {
        case E_MI_SYS_PIXEL_FRAME_YUV422_YUYV:
            size = u16Width *  2;
            size *= u16Height;
            break;

        case E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420://:need two address
            size = u16Width;
            size = size * u16Height * 3 / 2;
            break;

        default:
            printf("unsupported pixel format %d \n", ePixelFmt);
            break;
    }

    return size;
}

void vdisp_func_420sp(
    MI_SYS_FrameData_t *pstOutFrame,
    MI_SYS_WindowRect_t *dstRect,
    MI_SYS_FrameData_t *pstInFrame,
    MI_SYS_WindowRect_t *srcRect)
{
    MI_GFX_Surface_t stSrc;
    MI_GFX_Rect_t stSrcRect;
    MI_GFX_Surface_t stDst;
    MI_GFX_Rect_t stDstRect;
    MI_GFX_Opt_t stOpt;
    MI_U16 u16Fence;

    for(int i = 0; i < 2; i++) {
        stDst.eColorFmt = E_MI_GFX_FMT_I8;
        stDst.phyAddr = pstOutFrame->phyAddr[i];
        stDst.u32Width = pstOutFrame->u16Width ;
        stDst.u32Height = i ? pstOutFrame->u16Height / 2 : pstOutFrame->u16Height ;
        stDst.u32Stride = pstOutFrame->u32Stride[i];

        stDstRect.s32Xpos = dstRect[i].u16X;
        stDstRect.s32Ypos = i ? dstRect[i].u16Y / 2 : dstRect[i].u16Y;
        stDstRect.u32Width = dstRect[i].u16Width;
        stDstRect.u32Height = dstRect[i].u16Height;

        stSrc.eColorFmt = E_MI_GFX_FMT_I8;
        stSrc.phyAddr = pstInFrame->phyAddr[i];
        stSrc.u32Width =  pstInFrame->u16Width ;
        stSrc.u32Height =  pstInFrame->u16Height;
        stSrc.u32Stride =  pstInFrame->u32Stride[i];

        stSrcRect.s32Xpos = srcRect[i].u16X;
        stSrcRect.s32Ypos = srcRect[i].u16Y;
        stSrcRect.u32Width = srcRect[i].u16Width;
        stSrcRect.u32Height = srcRect[i].u16Height;

        memset(&stOpt, 0, sizeof(stOpt));

        stOpt.eSrcDfbBldOp = E_MI_GFX_DFB_BLD_ONE;
        stOpt.eDstDfbBldOp = E_MI_GFX_DFB_BLD_ZERO;
        stOpt.eMirror = E_MI_GFX_MIRROR_NONE;
        stOpt.eRotate = E_MI_GFX_ROTATE_0;

        stOpt.stClipRect.s32Xpos = stDstRect.s32Xpos;
        stOpt.stClipRect.s32Ypos = stDstRect.s32Ypos;
        stOpt.stClipRect.u32Width = stDstRect.u32Width ;

        stOpt.stClipRect.u32Height = stDstRect.u32Height ;

        stOpt.u32GlobalSrcConstColor = 0xFF000000;
        stOpt.u32GlobalDstConstColor = 0xFF000000;
        /*
                printf("(0x%llx %d %d %d) (%d %d %d %d) \n (0x%llx %d %d %d) (%d %d %d %d)\n \n",
                    stDst.phyAddr,stDst.u32Width,stDst.u32Height,stDst.u32Stride,\
                    stDstRect.s32Xpos,stDstRect.s32Ypos,stDstRect.u32Width,stDstRect.u32Height,\
                    stSrc.phyAddr,stSrc.u32Width,stSrc.u32Height,stSrc.u32Stride,\
                    stSrcRect.s32Xpos,stSrcRect.s32Ypos,stSrcRect.u32Width,stSrcRect.u32Height);
        */
        MI_GFX_BitBlit(&stSrc, &stSrcRect, &stDst, &stDstRect, &stOpt, &u16Fence);

    }

    //printf("===============================================\n");
    MI_GFX_WaitAllDone(FALSE, u16Fence);
}


void vdisp_configOutputputMeta(vdispOutputMeta *outPutMeta, MI_U8 fps, stTimingArray_t *pstTiming)
{
    outPutMeta->outPort = 0;
    outPutMeta->outAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    outPutMeta->outAttr.u32BgColor = YUYV_BLACK;
    outPutMeta->outAttr.u32FrmRate  = fps;
    outPutMeta->outAttr.u32Height = pstTiming->u16Height;
    outPutMeta->outAttr.u32Width = pstTiming->u16Width;
    outPutMeta->outAttr.u64pts = 0;
}

void vdisp_configInputMeta(vdispInputMeta *inPutMeta, MI_U8 cellNum, vdispOutputMeta *outPutMeta)
{
    int factor = 0;

    for(int i = 1; i < 10; i++) {
        if(i * i >= cellNum) {
            factor = i;
            break;
        }
    }

    MI_U16 cellW = outPutMeta->outAttr.u32Width / factor;
    MI_U16 cellH = outPutMeta->outAttr.u32Height / factor;

    for(int i = 0, k = 0; i < factor; i++) {
        for(int j = 0; j < factor && k < cellNum; j++, k++) {
            inPutMeta[k].inChn = k;
            inPutMeta[k].inAttr.u32OutX = cellW * j;
            inPutMeta[k].inAttr.u32OutY = cellH * i;
            inPutMeta[k].inAttr.u32OutWidth = cellW;
            inPutMeta[k].inAttr.u32OutHeight = cellH;
            inPutMeta[k].inAttr.s32IsFreeRun = TRUE;
            printf("%s %d %d: %d %d %d %d\n", __FUNCTION__, __LINE__, k,
                   inPutMeta[k].inAttr.u32OutX, \
                   inPutMeta[k].inAttr.u32OutY, \
                   inPutMeta[k].inAttr.u32OutWidth, \
                   inPutMeta[k].inAttr.u32OutHeight);
        }
    }
}
void  vdisp_readFile(void *data, FILE *fp, MI_VDISP_InputChnAttr_t *inAttr)
{
    size_t n;
fetchframe:
    n = fread(data, \
              inAttr->u32OutWidth * inAttr->u32OutHeight, 1, fp);
    //printf("%s %d %d\n", __FUNCTION__, __LINE__, n);

    n = fread(data + inAttr->u32OutWidth * inAttr->u32OutHeight, \
              inAttr->u32OutWidth * inAttr->u32OutHeight / 2, 1, fp);
    //printf("%s %d %d\n", __FUNCTION__, __LINE__, n);

    if(n <= 0) {
        fseek(fp, 0, SEEK_SET);
        goto fetchframe;
    }
}

void vdisp_process_config(
    MI_SYS_FrameData_t *pstDstbuf,
    MI_SYS_WindowRect_t *pstDstRect,
    MI_SYS_FrameData_t *pstSrcBuf,
    MI_SYS_WindowRect_t *pstSrcRect,
    char *data,
    MI_U64 phyData,
    MI_SYS_FrameData_t *pstOutFrame,
    vdispInputMeta *inMeta,
    vdispOutputMeta *outMeta)
{

    pstDstbuf->phyAddr[0] = pstOutFrame->phyAddr[0];
    pstDstbuf->pVirAddr[0] = pstOutFrame->pVirAddr[0];
    pstDstbuf->phyAddr[1] = pstOutFrame->phyAddr[1];
    pstDstbuf->pVirAddr[1] = pstOutFrame->pVirAddr[1];
    pstDstbuf->u16Height = outMeta->outAttr.u32Height;
    pstDstbuf->u16Width = outMeta->outAttr.u32Width;
    //printf("%s %d %d %d\n",__FUNCTION__,__LINE__, pstDstbuf->u16Width,pstDstbuf->u16Height);
    pstDstbuf->u32Stride[0] = pstOutFrame->u32Stride[0];
    pstDstbuf->u32Stride[1] = pstOutFrame->u32Stride[1];

    pstDstRect[0].u16X = inMeta->inAttr.u32OutX;
    pstDstRect[0].u16Y = inMeta->inAttr.u32OutY;
    pstDstRect[0].u16Width =  inMeta->inAttr.u32OutWidth;
    pstDstRect[0].u16Height = inMeta->inAttr.u32OutHeight;

    pstDstRect[1].u16X = inMeta->inAttr.u32OutX;
    pstDstRect[1].u16Y = inMeta->inAttr.u32OutY;
    pstDstRect[1].u16Width =  inMeta->inAttr.u32OutWidth;
    pstDstRect[1].u16Height = inMeta->inAttr.u32OutHeight / 2;


    pstSrcBuf->ePixelFormat = pstOutFrame->ePixelFormat;
    pstSrcBuf->phyAddr[0] = phyData;
    pstSrcBuf->pVirAddr[0] = data;
    pstSrcBuf->phyAddr[1] = phyData + (inMeta->inAttr.u32OutHeight) * inMeta->inAttr.u32OutWidth;
    pstSrcBuf->pVirAddr[1] = data + (inMeta->inAttr.u32OutHeight) * inMeta->inAttr.u32OutWidth;
    pstSrcBuf->u16Height = inMeta->inAttr.u32OutHeight;
    pstSrcBuf->u16Width = inMeta->inAttr.u32OutWidth;
    pstSrcBuf->u32BufSize = _MI_VDISP_IMPL_CalFrameSize(pstOutFrame->ePixelFormat, \
                            inMeta->inAttr.u32OutWidth, inMeta->inAttr.u32OutHeight);
    pstSrcBuf->u32Stride[0] = inMeta->inAttr.u32OutWidth;
    pstSrcBuf->u32Stride[1] = inMeta->inAttr.u32OutWidth;

    pstSrcRect[0].u16X = 0;
    pstSrcRect[0].u16Y = 0;
    pstSrcRect[0].u16Width = inMeta->inAttr.u32OutWidth;
    pstSrcRect[0].u16Height = inMeta->inAttr.u32OutHeight;

    pstSrcRect[1].u16X = 0;
    pstSrcRect[1].u16Y = 0;
    pstSrcRect[1].u16Width = inMeta->inAttr.u32OutWidth;
    pstSrcRect[1].u16Height = inMeta->inAttr.u32OutHeight / 2;


}

