/* 
 * Copyright (c) 2018, Chips&Media
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "cnm_app.h"
#include "decoder_listener.h"
#include "misc/debug.h"
#include "misc/bw_monitor.h"

static BOOL IsDifferentResolution(DecGetFramebufInfo* fbInfo, FrameBuffer* srcFb)
{
    FrameBuffer* dstFb = &fbInfo->framebufPool[srcFb->myIndex];

    return (dstFb->width != srcFb->width || dstFb->height != srcFb->height);
}

void HandleDecCompleteSeqEvent(Component com, CNMComListenerDecCompleteSeq* param, DecListenerContext* ctx)
{
    if (ctx->compareType == YUV_COMPARE) {
        Uint32 width  = param->initialInfo->picWidth;
        Uint32 height = param->initialInfo->picHeight;

        if (ctx->enableScaler == TRUE) {
            width  = VPU_CEIL(width, 16);
            height = VPU_CEIL(height, 2);
        }
        if ((ctx->comparator = Comparator_Create(YUV_COMPARE, param->refYuvPath, width, height, param->wtlFormat,
                                                 param->cbcrInterleave, param->bitstreamFormat==STD_VP9)) == NULL) {
            VLOG(ERR, "%s:%d Failed to Comparator_Create(%s)\n", __FUNCTION__, __LINE__, param->refYuvPath);
            CNMErrorSet(CNM_ERROR_FAILURE);
            CNMAppStop();
            return ;
        }
    }
}

static void HandleDecRegisterFbEvent(Component com, CNMComListenerDecRegisterFb* param, DecListenerContext* ctx)
{
    Uint32 fps = (ctx->fps == 0) ? 30 : ctx->fps;
    //SimpleRenderer_SetFrameRate(ctx->renderer, 0);
    if ( ctx->bandwidth ) {
        ctx->bwCtx = BWMonitorSetup(param->handle, TRUE, GetBasename((const char *)ctx->inputPath));
    }
    if ( ctx->performance ) {
        ctx->pfCtx = PFMonitorSetup(param->handle->coreIdx, param->handle->instIndex, ctx->pfClock, fps, GetBasename((const char *)ctx->inputPath));
    }
}

static void HandleDecReadyOneFrameEvent(Component com, CNMComListenerDecReadyOneFrame* param, DecListenerContext* ctx)
{
}

static void HandleDecInterruptEvent(Component com, CNMComListenerDecInt* param, DecListenerContext* ctx)
{
    if (param->flag == (1<<INT_WAVE5_DEC_PIC)) {
        if (ctx->bwCtx != NULL) {
            BWMonitorUpdate(ctx->bwCtx, ctx->numVCores);
            BWMonitorReset(ctx->bwCtx);
        }
    }
}

#ifdef DUMP_VLC_BUFFER
static void DumpVLCBuffer(Component decoder, Component renderer, Uint32 coreIdx)
{
    FILE*               fp;
    char                filename[128];
    static Uint32       count=0;
    Uint8*              buf;
    ParamDecFrameBuffer paramFb;
    Uint32              i;
    vpu_buffer_t*       vb;

    ComponentGetParameter(NULL, renderer, GET_PARAM_RENDERER_FRAME_BUF, (void*)&paramFb);

    for (i=0; i<paramFb.compressedNum; i++) {
        vb = &paramFb.mem[i];
        sprintf(filename, "vlc_buffer_%d_%04d.bin", i, count++);
        if (NULL == (fp=fopen(filename, "wb"))) {
            VLOG(ERR, "Failed to open file: %s\n");
            return;
        }

        buf = (Uint8*)osal_malloc(vb->size);
        vdi_read_memory(coreIdx, vb->phys_addr, buf, vb->size, paramFb.fb[i].endian);
        fwrite(buf, 1, vb->size, fp);
        osal_free(buf);
    }

    fclose(fp);
}
#endif /* DUMP_VLC_BUFFER */

void HandleDecGetOutputEvent(Component com, CNMComListenerDecDone* param, DecListenerContext* ctx)
{
    DecOutputInfo*          output          = param->output;
    FrameBuffer*            pFb             = &output->dispFrame;
    void*                   decodedData     = NULL;
    Uint8*                  pYuv            = NULL;
    Uint32                  decodedDataSize = 0;
    VpuRect                 rcDisplay       = {0,};
    Uint32                  width=0, height = 0, Bpp;
    size_t                  frameSizeInByte = 0;

    if (param->ret != RETCODE_SUCCESS) return;

    if (ctx->pfCtx != NULL) {
        if (output->indexFrameDecoded >= 0)
            PFMonitorUpdate(param->handle->coreIdx, ctx->pfCtx, output->frameCycle);
    }
    if (ctx->bwCtx != NULL) {
        BWMonitorUpdatePrint(ctx->bwCtx, output->picType);
    }

    // Finished decoding a frame
#ifdef DUMP_VLC_BUFFER
    if (0 <= output->indexFrameDecoded) {
        DumpVLCBuffer(com, ctx->renderer, param->handle->coreIdx);
    }
#endif /* DUMP_VLC_BUFFER */
    if (output->indexFrameDisplay < 0) {
        return;
    }

    if (ctx->lastSeqNo < pFb->sequenceNo || IsDifferentResolution(&ctx->fbInfo, pFb)) {
        /* When the video sequence or the resolution of picture was changed. */
        ctx->lastSeqNo  = pFb->sequenceNo;
        VPU_DecGiveCommand(param->handle, DEC_GET_FRAMEBUF_INFO, (void*)&ctx->fbInfo);
    }

    if (ctx->compareType == YUV_COMPARE) {
        rcDisplay.right  = output->dispPicWidth;
        rcDisplay.bottom = output->dispPicHeight;
        if (ctx->enableScaler == TRUE) {
           rcDisplay.right  = VPU_CEIL(rcDisplay.right, 16);
	       rcDisplay.bottom = VPU_CEIL(rcDisplay.bottom, 2);
        }
        pYuv = GetYUVFromFrameBuffer(param->handle, &output->dispFrame, rcDisplay, &width, &height, &Bpp, &frameSizeInByte);
        decodedData     = (void*)pYuv;
        decodedDataSize = frameSizeInByte;
    }

    if (ctx->comparator) {
        if ((ctx->match=Comparator_Act(ctx->comparator, decodedData, decodedDataSize)) == FALSE) {
        }
    }

    if (ctx->compareType == YUV_COMPARE) {
        osal_free(decodedData);
    }

    /* decoding error check */
    if (output->indexFrameDecoded >= 0) {
        ctx->notDecodedCount = 0;
    }
    else {
        ctx->notDecodedCount++;
        if (ctx->notDecodedCount == MAX_NOT_DEC_COUNT) {
            VLOG(ERR, "Continuous not-decoded-count is %d\nThere is something problem in DPB control.\n", ctx->notDecodedCount);
            CNMAppStop();
        }
    }

    if (ctx->match == FALSE) CNMAppStop();
}

static void HandleDecCloseEvent(Component com, CNMComListenerDecClose* param, DecListenerContext* ctx)
{
    if (ctx->bwCtx != NULL)
        BWMonitorRelease(ctx->bwCtx);
    if (ctx->pfCtx != NULL)
        PFMonitorRelease(ctx->pfCtx);
}

void DecoderListener(Component com, Uint32 event, void* data, void* context)
{
    int key=0;
    if (osal_kbhit()) {
        key = osal_getch();
        osal_flush_ch();
        if (key == 'q' || key == 'Q') {
            CNMAppStop();
            return;
        }
    }
    switch (event) {
    case COMPONENT_EVENT_DEC_OPEN:
        break;
    case COMPONENT_EVENT_DEC_ISSUE_SEQ:
        break;
    case COMPONENT_EVENT_DEC_COMPLETE_SEQ:
        HandleDecCompleteSeqEvent(com, (CNMComListenerDecCompleteSeq*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_REGISTER_FB:
        HandleDecRegisterFbEvent(com, (CNMComListenerDecRegisterFb*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_READY_ONE_FRAME:
        HandleDecReadyOneFrameEvent(com, (CNMComListenerDecReadyOneFrame*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_START_ONE_FRAME:
        break;
    case COMPONENT_EVENT_DEC_INTERRUPT:
        HandleDecInterruptEvent(com, (CNMComListenerDecInt*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_GET_OUTPUT_INFO:
        HandleDecGetOutputEvent(com, (CNMComListenerDecDone*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_DECODED_ALL:
        break;
    case COMPONENT_EVENT_DEC_CLOSE:
        HandleDecCloseEvent(com, (CNMComListenerDecClose*)data, (DecListenerContext*)context);
        break;
    default:
        break;
    }
}

BOOL SetupDecListenerContext(DecListenerContext* ctx, CNMComponentConfig* config, Component renderer)
{
    TestDecConfig* decConfig = &config->testDecConfig;

    osal_memset((void*)ctx, 0x00, sizeof(DecListenerContext));

    if (decConfig->compareType == MD5_COMPARE) {
        if ((ctx->comparator=Comparator_Create(MD5_COMPARE, decConfig->md5Path, 12)) == NULL) {
            VLOG(ERR, "%s:%d Failed to Comparator_Create(%s)\n", __FUNCTION__, __LINE__, decConfig->md5Path);
            return FALSE;
        }
    }


    ctx->renderer      = renderer;
    ctx->lastSeqNo     = -1;
    ctx->compareType   = decConfig->compareType;
    ctx->match         = TRUE;

    ctx->performance   = decConfig->performance;
    ctx->bandwidth     = decConfig->bandwidth;
    ctx->fps           = decConfig->fps;
    ctx->pfClock       = decConfig->pfClock;
    ctx->numVCores     = decConfig->wave.numVCores;
    ctx->enableScaler  = (decConfig->scaleDownWidth > 0 || decConfig->scaleDownHeight > 0);
    ctx->enableScaler |= (decConfig->numScaleDownList > 0);
    osal_memcpy(ctx->inputPath, decConfig->inputPath, sizeof(ctx->inputPath));
    osal_memcpy(&ctx->config, decConfig, sizeof(TestDecConfig));

    return TRUE;
}

void ClearDecListenerContext(DecListenerContext* ctx)
{
    if (ctx->comparator)    Comparator_Destroy(ctx->comparator);
}


