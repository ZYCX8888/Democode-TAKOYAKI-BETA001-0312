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
#include <string.h>
#include "component.h"
#include "cnm_app.h"
#include "misc/debug.h"

#define EXTRA_FRAME_BUFFER_NUM 1

typedef enum {
    DEC_INT_STATUS_NONE,        // Interrupt not asserted yet
    DEC_INT_STATUS_EMPTY,       // Need more es
    DEC_INT_STATUS_DONE,        // Interrupt asserted
    DEC_INT_STATUS_TIMEOUT,     // Interrupt not asserted during given time.
    DEC_INT_STATUS_VLC_BUFFER
} DEC_INT_STATUS;

typedef enum {
    DEC_STATE_NONE,
    DEC_STATE_OPEN_DECODER,
    DEC_STATE_INIT_SEQ,
    DEC_STATE_REGISTER_FB,
    DEC_STATE_DECODING,
    DEC_STATE_CLOSE,
} DecoderState;

typedef struct {
    TestDecConfig       testDecConfig;
    DecOpenParam        decOpenParam;
    DecParam            decParam;
    FrameBufferFormat   wtlFormat;
    DecHandle           handle;
    Uint64              startTimeout;
    vpu_buffer_t        vbUserData;
    BOOL                doFlush;
    BOOL                stateDoing;
    DecoderState        state;
    DecInitialInfo      initialInfo;
    Uint32              numDecoded;             /*!<< The number of decoded frames */
    Uint32              numOutput;
    PhysicalAddress     decodedAddr;
    Uint32              frameNumToStop;
    BOOL                doReset;
    BOOL                scenarioTest;
    Uint32              cyclePerTick;
    osal_mutex_t        lock;
    BOOL                enableScaler;
#define MAX_FB_LIST     32
    FrameBuffer         fbList[MAX_FB_LIST];
    FrameBuffer*        pAllocatedFb;
    Uint32              scaleDownListIdx;
    Uint32              fbListCount;
#ifdef SUPPORT_RECOVER_AFTER_ERROR
    BOOL                doIframeSearchByError;
#endif
    Uint64              decTime;
    BOOL                dynamicFbAlloc;
} DecoderContext;

enum VLC_BUFFER_STATE {
    VLC_BUFFER_STATE_FREE,
    VLC_BUFFER_STATE_READY,
    VLC_BUFFER_STATE_USING,
};
typedef struct {
    vpu_buffer_t        vb;
    BOOL                state;
} VlcBuffer;
#ifdef SUPPORT_SG_FLOW
VlcBuffer        vlcBuffer[COMMAND_QUEUE_DEPTH];
osal_mutex_t     vlcBufferLock;
#else
static VlcBuffer        vlcBuffer[COMMAND_QUEUE_DEPTH];
static osal_mutex_t     vlcBufferLock;
#endif
static VlcBuffer* GetVlcBuffer(PhysicalAddress addr)
{
    Uint32      i;
    VlcBuffer*  buf = NULL;

    osal_mutex_lock(vlcBufferLock);
    for (i=0; i<COMMAND_QUEUE_DEPTH; i++) {
        if (addr == vlcBuffer[i].vb.phys_addr) {
            buf = &vlcBuffer[i];
            break;
        }
    }
    osal_mutex_unlock(vlcBufferLock);

    if (NULL == buf) {
        VLOG(ERR, "<%s> Not found address %p\n", __FUNCTION__, addr);
    }

    return buf;
}

#ifdef SUPPORT_SG_FLOW
static VlcBuffer* GetFreeVlcBuffer(int inst)
{
    VlcBuffer*  buf = NULL;

    osal_mutex_lock(vlcBufferLock);
    if (VLC_BUFFER_STATE_FREE == vlcBuffer[inst].state) 
    {
        buf         = &vlcBuffer[inst];
        buf->state  = VLC_BUFFER_STATE_READY;
    }
    else
    {
        buf = NULL;
        VLOG(ERR, "<%s> : no Free buffer \n", __FUNCTION__);
    }
    osal_mutex_unlock(vlcBufferLock);

    return buf;
}
#else
static VlcBuffer* GetFreeVlcBuffer(void)
{
    Uint32      i;
    VlcBuffer*  buf = NULL;

    osal_mutex_lock(vlcBufferLock);
    for (i=0; i<COMMAND_QUEUE_DEPTH; i++) {
        if (VLC_BUFFER_STATE_FREE == vlcBuffer[i].state) {
            buf         = &vlcBuffer[i];
            buf->state  = VLC_BUFFER_STATE_READY;
            break;
        }
    }
    osal_mutex_unlock(vlcBufferLock);

    return buf;
}
#endif

static void SetVlcBufferState(VlcBuffer* buf, Uint32 state)
{
    osal_mutex_lock(vlcBufferLock);
    if (0 < buf->vb.size) {
        buf->state = state;
    }
    osal_mutex_unlock(vlcBufferLock);
}

static BOOL ReallocVlcBuffer(Uint32 coreIdx, VlcBuffer* buf, Uint32 size)
{
    Uint32 i;
#ifdef SUPPORT_SG_FLOW
        VLOG(ERR, "<%s> Can't happen at SUPPORT_SG_FLOW \n", __FUNCTION__);
        return 0;
#endif
    if (VLC_BUFFER_STATE_USING != buf->state && 0 < buf->vb.size) {
        VLOG(ERR, "<%s> The state of the buffer is not VLC_BUFFER_STATE_USING.\n", __FUNCTION__);
        return FALSE;
    }
    osal_mutex_lock(vlcBufferLock);
    for (i=0; i<COMMAND_QUEUE_DEPTH; i++) {
        if (buf == &vlcBuffer[i]) {
            vpu_buffer_t vb = {0};
            /* Free the buffer */
            if (0 < buf->vb.size) {
                //VLOG(ERR, "<%s> !!!!! FREE VLC BUFFER %p\n", __FUNCTION__, buf->vb.phys_addr);
                vdi_free_dma_memory(coreIdx, &buf->vb);
            }
            buf->vb.size      = 0;
            buf->vb.phys_addr = 0;

            /* Reallocate the buffer with the new size */
            vb.phys_addr    = 0;
            vb.size         = size;
            if (0 > vdi_allocate_dma_memory(coreIdx, &vb)) {
                VLOG(ERR, "<%s> Failed to allocate memory(%d)\n", __FUNCTION__, size);
                return FALSE;
            }
            buf->vb = vb;
            break;
        }
    }
    osal_mutex_unlock(vlcBufferLock);

    if (COMMAND_QUEUE_DEPTH == i) {
        VLOG(ERR, "<%s> Not found buffer(buf:%p, addr: %p)\n", __FUNCTION__, buf, buf->vb.phys_addr);
        return FALSE;
    }

    return TRUE;
}

static VlcBuffer* FindVlcBuffer(PhysicalAddress addr)
{
    Uint32      i;
    VlcBuffer*  buf = NULL;

    osal_mutex_lock(vlcBufferLock);
    for (i=0; i<COMMAND_QUEUE_DEPTH; i++) {
        if (addr == vlcBuffer[i].vb.phys_addr) {
            buf = &vlcBuffer[i];
            break;
        }
    }
    osal_mutex_unlock(vlcBufferLock);

    if (NULL == buf)
        VLOG(ERR, "<%s> Not found address: %08x\n", __FUNCTION__, addr);

    return buf;
}

static BOOL AllocateVlcBuffer(DecoderContext* ctx)
{
    VLCBufInfo      info    = {0};
    RetCode         ret;
    VlcBuffer*      vlcBuf  = NULL;

    /* Query : How much memory the VPU needs to decode the frame */
    if (RETCODE_SUCCESS != (ret=VPU_DecGiveCommand(ctx->handle, DEC_GET_VLC_INFO, &info))) {
        VLOG(ERR, "<%s> Failed to DEC_GET_VLC_INFO(err:%d)\n", __FUNCTION__, ret);
        return FALSE;
    }
    //VLOG(INFO, "<%s> VLC BUFFER : <%p, %d>\n", __FUNCTION__, info.vlcBufBase, info.requiedVlcBufSize);

    if (NULL == (vlcBuf=GetVlcBuffer(info.vlcBufBase))) {
        return FALSE;
    }

    if (FALSE == ReallocVlcBuffer(ctx->testDecConfig.coreIdx, vlcBuf, info.requiedVlcBufSize)) {
        return FALSE;
    }
    SetVlcBufferState(vlcBuf, VLC_BUFFER_STATE_USING);

    info.vlcBufBase        = vlcBuf->vb.phys_addr;
    info.requiedVlcBufSize = vlcBuf->vb.size;
    if (RETCODE_SUCCESS != (ret=VPU_DecGiveCommand(ctx->handle, DEC_UPDATE_VLC_INFO, &info))) {
        VLOG(ERR, "<%s> Failed to DEC_UPDATE_VLC_INFO(err:%d)\n", __FUNCTION__, ret);
        return FALSE;
    }
    //VLOG(INFO, "<%s> (%p, %d)\n", __FUNCTION__, info.vlcBufBase, info.requiedVlcBufSize);
    return TRUE;
}

static void AddFrameBuffer(DecoderContext* ctx, FrameBuffer* fb)
{
    Uint32 i;

    if (ctx->testDecConfig.enableWTL == FALSE) {
        return;
    }

    osal_mutex_lock(ctx->lock);
    for (i=0; i<MAX_FB_LIST; i++) {
        if (ctx->fbList[i].bufY == (PhysicalAddress)0) {
            ctx->fbList[i] = *fb;
            ctx->pAllocatedFb = &ctx->fbList[i];
            ctx->fbListCount++;
            break;
        }
    }
    osal_mutex_unlock(ctx->lock);

    VLOG(INFO, "<%s:%d> fbListCount%d\n", __FUNCTION__, __LINE__, ctx->fbListCount);
    if (i == MAX_FB_LIST) VLOG(ERR, "<%s:%d> FBLIST IS FULL\n", __FUNCTION__, __LINE__);
}

static BOOL GetFrameBuffer(DecoderContext* ctx, DecParam* param)
{
    FrameBuffer fb;
    BOOL        align = 1;

    if (FALSE == ctx->testDecConfig.enableWTL) {
        return TRUE;
    }

    if (TRUE == ctx->testDecConfig.customDisableFlag) {
        return TRUE;
    }

    if (ctx->pAllocatedFb == NULL) {
        Uint32              w = ctx->initialInfo.picWidth;
        Uint32              h = ctx->initialInfo.picHeight;
        FrameBufferFormat   fmt = ctx->testDecConfig.wtlFormat;
        BOOL                semiPlanar = (BOOL)ctx->testDecConfig.cbcrInterleave;

        if (ctx->fbListCount >= MAX_FB_LIST) {
            return FALSE;
        }
        if (ctx->enableScaler == TRUE) {
            if (ctx->testDecConfig.numScaleDownList > 0) {
                w = ctx->testDecConfig.scaleDownListW[ctx->scaleDownListIdx];
                h = ctx->testDecConfig.scaleDownListH[ctx->scaleDownListIdx];
            }
            else {
                w = ctx->testDecConfig.scaleDownWidth;
                h = ctx->testDecConfig.scaleDownHeight;
            }
            align = 16;
        }

        if (AllocateDecLinearFrameBuffer(ctx->handle, w, h, align, fmt, semiPlanar, &fb) == FALSE) {
            return FALSE;
        }
        fb.endian = 0x10;//ctx->testDecConfig.frameEndian;
        fb.nv21   = ctx->testDecConfig.nv21;
        AddFrameBuffer(ctx, &fb);
    }
    else {
        osal_memcpy((void*)&fb, ctx->pAllocatedFb, sizeof(FrameBuffer));
    }

    param->addrLinearBufY   = fb.bufY;
    param->addrLinearBufCb  = fb.bufCb;
    param->addrLinearBufCr  = fb.bufCr;
    param->linearLumaStride = fb.stride;
    param->scaleWidth       = fb.width;
    param->scaleHeight      = fb.height;
    return TRUE;
}

static void SetupNextFrameBuffer(DecoderContext* ctx)
{
    if (ctx->testDecConfig.enableWTL == FALSE) {
        return;
    }

    if (ctx->enableScaler == TRUE) {
        if (ctx->testDecConfig.numScaleDownList > 0) {
            ctx->scaleDownListIdx++;
            ctx->scaleDownListIdx %= ctx->testDecConfig.numScaleDownList;
        }
    }
    ctx->pAllocatedFb = NULL;
}

static void FindFrameBuffer(DecoderContext* ctx, FrameBuffer* fb)
{
    Uint32 i;

    if (ctx->testDecConfig.enableWTL == FALSE) {
        return;
    }

    osal_mutex_lock(ctx->lock);
    for (i=0; i<MAX_FB_LIST; i++) {
        if (ctx->fbList[i].bufY == fb->bufY) {
            osal_memcpy((void*)fb, (void*)&ctx->fbList[i], sizeof(FrameBuffer));
            break;
        }
    }
    osal_mutex_unlock(ctx->lock);

    if (i == MAX_FB_LIST) VLOG(ERR, "<%s:%d> Not found address %p\n", __FUNCTION__, __LINE__, fb->bufY);

    return;
}

static void ReleaseFrameBuffer(DecoderContext* ctx, FrameBuffer* fb)
{
    Uint32          i;
    Uint32          coreIdx = ctx->testDecConfig.coreIdx;
    vpu_buffer_t    vb = {0};

    if (ctx->testDecConfig.enableWTL == FALSE) {
        return;
    }

    osal_mutex_lock(ctx->lock);

    for (i=0; i<MAX_FB_LIST; i++) {
        if (ctx->fbList[i].bufY == fb->bufY) {
            vb.phys_addr = fb->bufY;
            vb.size      = fb->size;
            vdi_free_dma_memory(coreIdx, &vb);
            osal_memset((void*)&ctx->fbList[i], 0x00, sizeof(FrameBuffer));
            ctx->fbListCount--;
            //VLOG(INFO, "%s addr(%p) count(%d)\n", __FUNCTION__, fb->bufY, ctx->fbListCount);
            break;
        }
    }
    osal_mutex_unlock(ctx->lock);

    VLOG(INFO, "<%s:%d> fbListCount%d\n", __FUNCTION__, __LINE__, ctx->fbListCount);
    if (i == MAX_FB_LIST) VLOG(ERR, "<%s:%d> Not found address %p\n", __FUNCTION__, __LINE__, fb->bufY);

    return;
}

static void ReleaseAllFrameBuffers(DecoderContext* ctx)
{
    Uint32          i;
    Uint32          coreIdx = ctx->testDecConfig.coreIdx;
    vpu_buffer_t    vb = {0};

    if (ctx->testDecConfig.enableWTL == FALSE) {
        return;
    }

    osal_mutex_lock(ctx->lock);
    for (i=0; i<MAX_FB_LIST; i++) {
        if (ctx->fbList[i].bufY != 0) {
            vb.phys_addr = ctx->fbList[i].bufY;
            vb.size      = ctx->fbList[i].size;
            vdi_free_dma_memory(coreIdx, &vb);
            osal_memset((void*)&ctx->fbList[i], 0x00, sizeof(FrameBuffer));
        }
    }
    osal_mutex_unlock(ctx->lock);

    return;
}

static BOOL RegisterFrameBuffers(ComponentImpl* com)
{
    DecoderContext*                 ctx               = (DecoderContext*)com->context;
    FrameBuffer*                    pFrame            = NULL;
    Uint32                          framebufStride    = 0;
    ParamDecFrameBuffer             paramFb;
    RetCode                         result;
    DecInitialInfo*                 codecInfo         = &ctx->initialInfo;
    BOOL                            success;
    Uint32                          numLinearFb, numFbcFb;
    CNMComponentParamRet            ret;
    CNMComListenerDecRegisterFb     lsnpRegisterFb;

    ctx->stateDoing = TRUE;
    ret = ComponentGetParameter(com, com->sinkPort.connectedComponent, GET_PARAM_RENDERER_FRAME_BUF, (void*)&paramFb);
    if (ComponentParamReturnTest(ret, &success) == FALSE) {
        return success;
    }

    ret = ComponentGetParameter(com, com->sinkPort.connectedComponent, GET_PARAM_DEC_FRAME_BUF_STRIDE, &framebufStride);
    if (ComponentParamReturnTest(ret, &success) == FALSE) {
        return success;
    }

    pFrame          = paramFb.fb;
    framebufStride  = paramFb.stride;
    numLinearFb     = paramFb.linearNum;
    numFbcFb        = paramFb.compressedNum;

    VLOG(TRACE, "<%s> COMPRESSED: %d, LINEAR: %d\n", __FUNCTION__, paramFb.compressedNum, paramFb.linearNum);
    result = VPU_DecRegisterFrameBufferEx(ctx->handle, pFrame, numFbcFb, numLinearFb, framebufStride, codecInfo->picHeight, COMPRESSED_FRAME_MAP);

    lsnpRegisterFb.handle       = ctx->handle;
    lsnpRegisterFb.fbs          = pFrame;
    lsnpRegisterFb.numReconFbs  = numFbcFb;
    lsnpRegisterFb.numOutputFbs = numLinearFb;
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_REGISTER_FB, (void*)&lsnpRegisterFb);

    if (result != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d Failed to VPU_DecRegisterFrameBufferEx(%d)\n", __FUNCTION__, __LINE__, result);
        return FALSE;
    }

    ctx->stateDoing = FALSE;

    return TRUE;
}

static BOOL SequenceChange(ComponentImpl* com, DecOutputInfo* outputInfo)
{
    DecoderContext* ctx               = (DecoderContext*)com->context;
    BOOL            dpbChanged, sizeChanged, bitDepthChanged;
    Uint32          sequenceChangeFlag = outputInfo->sequenceChanged;

    dpbChanged      = (sequenceChangeFlag&SEQ_CHANGE_ENABLE_DPB_COUNT) ? TRUE : FALSE;
    sizeChanged     = (sequenceChangeFlag&SEQ_CHANGE_ENABLE_SIZE)      ? TRUE : FALSE;
    bitDepthChanged = (sequenceChangeFlag&SEQ_CHANGE_ENABLE_BITDEPTH)  ? TRUE : FALSE;

    if (dpbChanged || sizeChanged || bitDepthChanged) {
        DecInitialInfo  initialInfo;

        VLOG(INFO, "----- SEQUENCE CHANGED -----\n");
        // Get current(changed) sequence information.
        VPU_DecGiveCommand(ctx->handle, DEC_GET_SEQ_INFO, &initialInfo);
        // Flush all remaining framebuffers of previous sequence.

        VLOG(INFO, "sequenceChanged : %x\n", sequenceChangeFlag);
        VLOG(INFO, "SEQUENCE NO     : %d\n", initialInfo.sequenceNo);
        VLOG(INFO, "DPB COUNT       : %d\n", initialInfo.minFrameBufferCount);
        VLOG(INFO, "BITDEPTH        : LUMA(%d), CHROMA(%d)\n", initialInfo.lumaBitdepth, initialInfo.chromaBitdepth);
        VLOG(INFO, "SIZE            : WIDTH(%d), HEIGHT(%d)\n", initialInfo.picWidth, initialInfo.picHeight);

        ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_FREE_FRAMEBUFFERS, (void*)&outputInfo->frameDisplayFlag);

        VPU_DecGiveCommand(ctx->handle, DEC_RESET_FRAMEBUF_INFO, NULL);

        if (ctx->testDecConfig.scaleDownWidth > 0 || ctx->testDecConfig.scaleDownHeight > 0) {
            ScalerInfo sclInfo = {0};

            sclInfo.scaleWidth  = CalcScaleDown(initialInfo.picWidth, ctx->testDecConfig.scaleDownWidth);
            sclInfo.scaleHeight = CalcScaleDown(initialInfo.picHeight, ctx->testDecConfig.scaleDownHeight);
            VLOG(INFO, "[SCALE INFO] %dx%d to %dx%d\n", initialInfo.picWidth, initialInfo.picHeight, sclInfo.scaleWidth, sclInfo.scaleHeight);
            sclInfo.enScaler    = TRUE;
            if (VPU_DecGiveCommand(ctx->handle, DEC_SET_SCALER_INFO, (void*)&sclInfo) != RETCODE_SUCCESS) {
                VLOG(ERR, "Failed to VPU_DecGiveCommand(DEC_SET_SCALER_INFO)\n");
                return FALSE;
            }
        }
        ctx->state = DEC_STATE_REGISTER_FB;
        osal_memcpy((void*)&ctx->initialInfo, (void*)&initialInfo, sizeof(DecInitialInfo));
        ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_ALLOC_FRAMEBUFFERS, NULL);

        VLOG(INFO, "----------------------------\n");
    }

    return TRUE;
}

static BOOL CheckAndDoSequenceChange(ComponentImpl* com, DecOutputInfo* outputInfo)
{
    if (outputInfo->sequenceChanged == 0) {
        return TRUE;
    }
    else {
        return SequenceChange(com, outputInfo);
    }
}

static BOOL PrepareSkip(ComponentImpl* com)
{
    DecoderContext*   ctx = (DecoderContext*)com->context;

    // Flush the decoder
    if (ctx->doFlush == TRUE) {
        Uint32          timeoutCount;
        Uint32          intReason;
        PhysicalAddress curRdPtr, curWrPtr;
        DecOutputInfo   outputInfo;

        VLOG(INFO, "========== FLUSH RENDERER           ========== \n");
        /* Send the renderer the signal to drop all frames.
         * VPU_DecClrDispFlag() is called in SE_PARAM_RENDERER_FLUSH.
         */
        ComponentSetParameter(com, com->sinkPort.connectedComponent, SET_PARAM_RENDERER_FLUSH, NULL);

        VLOG(INFO, "> EXPLICIT_END_SET_FLAG\n");
        // In order to stop processing bitstream.
        VPU_DecUpdateBitstreamBuffer(ctx->handle, EXPLICIT_END_SET_FLAG);

        VLOG(INFO, "========== FLUSH FRAMEBUFFER & CMDs ========== \n");
        timeoutCount = 0;
        while (VPU_DecFrameBufferFlush(ctx->handle, NULL, NULL) == RETCODE_VPU_STILL_RUNNING) {
            intReason = VPU_WaitInterruptEx(ctx->handle, VPU_WAIT_TIME_OUT_CQ);
            if (intReason > 0) {
                VlcBuffer* vlcBuf;
                VPU_ClearInterruptEx(ctx->handle, intReason);
                VPU_DecGetOutputInfo(ctx->handle, &outputInfo);  // ignore the return value and outputinfo
                if (NULL != (vlcBuf=FindVlcBuffer(outputInfo.VLCOutBufBase))) {
                    SetVlcBufferState(vlcBuf, VLC_BUFFER_STATE_FREE);
                }
            }

            if (timeoutCount >= VPU_BUSY_CHECK_TIMEOUT) {
                VLOG(ERR, "NO RESPONSE FROM VPU_DecFrameBufferFlush()\n");
                return FALSE;
            }
            timeoutCount++;
            osal_msleep(1);
        }

        /* Clear CPB */
        VPU_DecGetBitstreamBuffer(ctx->handle, &curRdPtr, &curWrPtr, NULL);
        VPU_DecSetRdPtr(ctx->handle, curWrPtr, TRUE);
        ctx->doFlush = FALSE;
    }

    return TRUE;
}

static DEC_INT_STATUS HandlingInterruptFlag(ComponentImpl* com)
{
    DecoderContext*      ctx               = (DecoderContext*)com->context;
    DecHandle            handle            = ctx->handle;
    Int32                interruptFlag     = 0;
    Uint32               interruptWaitTime = VPU_WAIT_TIME_OUT_CQ;
    Uint32               interruptTimeout  = VPU_DEC_TIMEOUT;
    DEC_INT_STATUS       status            = DEC_INT_STATUS_NONE;
    CNMComListenerDecInt lsn;

    if (ctx->startTimeout == 0ULL) {
        ctx->startTimeout = osal_gettime();
    }
    do {
        interruptFlag = VPU_WaitInterruptEx(handle, interruptWaitTime);
        if (interruptFlag == -1) {
            Uint64   currentTimeout = osal_gettime();
            if ((currentTimeout - ctx->startTimeout) > interruptTimeout) {
                if (ctx->scenarioTest == FALSE) {
                    CNMErrorSet(CNM_ERROR_HANGUP);
                }
                VLOG(ERR, "\n INSNTANCE #%d INTERRUPT TIMEOUT.\n", handle->instIndex);
                vdi_log(0, 5, 0);
                VLOG(ERR, "\n INSNTANCE #%d INTERRUPT TIMEOUT.\n", handle->instIndex);
                status = DEC_INT_STATUS_TIMEOUT;
                break;
            }
            interruptFlag = 0;
        }

        if (interruptFlag < 0) {
            VLOG(ERR, "<%s:%d> interruptFlag is negative value! %08x\n", __FUNCTION__, __LINE__, interruptFlag);
        }

        if (interruptFlag > 0) {
            VPU_ClearInterruptEx(handle, interruptFlag);
            ctx->startTimeout = 0ULL;
            status = DEC_INT_STATUS_DONE;
        }

        if (interruptFlag & (1<<INT_WAVE5_INIT_SEQ)) {
            break;
        }

        if (interruptFlag & (1<<INT_WAVE5_DEC_PIC)) {
            break;
        }

        if (interruptFlag & (1<<INT_WAVE5_BSBUF_EMPTY)) {
            status = DEC_INT_STATUS_EMPTY;
            break;
        }

        if (interruptFlag & (1<<INT_WAVE5_INSUFFICIENT_VLC_BUFFER)) {
            status = DEC_INT_STATUS_VLC_BUFFER;
            break;
        }
    } while (FALSE);

    if (interruptFlag != 0) {
        lsn.handle = handle;
        lsn.flag   = interruptFlag;
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_INTERRUPT, (void*)&lsn);
    }

    return status;
}

static BOOL DoReset(ComponentImpl* com)
{
    DecoderContext* ctx    = (DecoderContext*)com->context;
    BitStreamMode   bsMode = ctx->decOpenParam.bitstreamMode;
    DecHandle       handle = ctx->handle;
    BOOL            pause  = TRUE;

    VLOG(INFO, "========== %s ==========\n", __FUNCTION__);

    ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_COM_PAUSE, (void*)&pause);
    ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_FEEDER_RESET, (void*)&pause);

    VLOG(INFO, "> EXPLICIT_END_SET_FLAG\n");
    // In order to stop processing bitstream.
    VPU_DecUpdateBitstreamBuffer(handle, EXPLICIT_END_SET_FLAG);

    VLOG(INFO, "> Reset VPU\n");
    if (VPU_SWReset(handle->coreIdx, SW_RESET_SAFETY, handle) != RETCODE_SUCCESS) {
        VLOG(ERR, "<%s:%d> Failed to VPU_SWReset()\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    // Clear CPB
    if (bsMode == BS_MODE_INTERRUPT) {
        PhysicalAddress newPtr;
        newPtr = ctx->decOpenParam.bitstreamBuffer;
        VPU_DecSetRdPtr(handle, newPtr, TRUE);
        VLOG(INFO, "> Clear CPB: %08x\n", newPtr);
    }

    VLOG(INFO, "> STREAM_END_CLEAR_FLAG\n");
    // Clear stream-end flag
    VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_CLEAR_FLAG);

    VLOG(INFO, "========== %s ==========\n", __FUNCTION__);

    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_RESET_DONE, NULL);

    VLOG(INFO, "> FLUSH INPUT PORT\n");
    ComponentPortFlush(com);

    pause = FALSE;
    ComponentSetParameter(com, com->srcPort.connectedComponent, SET_PARAM_COM_PAUSE, (void*)&pause);

    ctx->doReset      = FALSE;
    ctx->startTimeout = 0ULL;
    return TRUE;
}

Uint64 _cnm_gettime(void)
{
    Uint64 u64CurTime = 0;

    struct timeval tv;

    gettimeofday(&tv, NULL);

    u64CurTime = ((Uint64)(tv.tv_sec))*1000000 + tv.tv_usec;

    return u64CurTime;
}

static BOOL Decode(ComponentImpl* com, PortContainerES* in, PortContainerDisplay* out)
{
    DecoderContext*                 ctx                 = (DecoderContext*)com->context;
    DecOutputInfo                   decOutputInfo;
    DEC_INT_STATUS                  intStatus;
    CNMComListenerStartDec          lsnpPicStart;
    BitStreamMode                   bsMode              = ctx->decOpenParam.bitstreamMode;
    RetCode                         result;
    CNMComListenerDecDone           lsnpPicDone;
    CNMComListenerDecReadyOneFrame  lsnpReadyOneFrame   = {0,};
    BOOL                            doDecode;
    VlcBuffer*                      vlcBuf              = NULL;

    lsnpReadyOneFrame.handle   = ctx->handle;
    lsnpReadyOneFrame.decParam = &ctx->decParam;
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_READY_ONE_FRAME, (void*)&lsnpReadyOneFrame);

    ctx->stateDoing = TRUE;

    if (PrepareSkip(com) == FALSE) {
        return FALSE;
    }


#ifdef SUPPORT_RECOVER_AFTER_ERROR
    if (ctx->doIframeSearchByError == TRUE) {
        ctx->decParam.skipframeMode = 1;
    }
#endif

    /* decode a frame except when the bitstream mode is PIC_END and no data */
    doDecode  = !(bsMode == BS_MODE_PIC_END && in == NULL);
    doDecode &= (BOOL)(com->pause == FALSE);

    if (GetFrameBuffer(ctx, &ctx->decParam) == FALSE) {
        doDecode = FALSE;
    }
#ifdef SUPPORT_SG_FLOW
    {
        QueueStatusInfo QueueStatus;
        VPU_DecGiveCommand(ctx->handle, DEC_GET_QUEUE_STATUS, &QueueStatus);
        if (QueueStatus.instanceQueueCount == 0)
            doDecode = TRUE;
        else
            doDecode = FALSE;
    }
#endif
    if (TRUE == doDecode) {
#ifdef SUPPORT_SG_FLOW
        if (NULL == (vlcBuf=GetFreeVlcBuffer(ctx->handle->instIndex))) {
#else
        if (NULL == (vlcBuf=GetFreeVlcBuffer())) {
#endif
            /* All vlc buffers are used */
            doDecode = FALSE;
            if (in && BS_MODE_PIC_END == bsMode) in->reuse = TRUE;
        }
    }

    if (doDecode == TRUE) {
        ctx->decParam.addrVlcBuf = vlcBuf->vb.phys_addr;
        ctx->decParam.vlcBufSize = vlcBuf->vb.size;
        result = VPU_DecStartOneFrame(ctx->handle, &ctx->decParam);

        lsnpPicStart.result   = result;
        lsnpPicStart.decParam = ctx->decParam;
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_START_ONE_FRAME, (void*)&lsnpPicStart);

        if (result == RETCODE_SUCCESS) {
            if (TRUE == ctx->dynamicFbAlloc) {
                SetupNextFrameBuffer(ctx);
            }
            SetVlcBufferState(vlcBuf, VLC_BUFFER_STATE_USING);
        }
        else if (result == RETCODE_QUEUEING_FAILURE) {
            QueueStatusInfo qStatus;

            SetVlcBufferState(vlcBuf, VLC_BUFFER_STATE_FREE);
            // Just retry
            if (in) in->reuse = (bsMode == BS_MODE_PIC_END);
            VPU_DecGiveCommand(ctx->handle, DEC_GET_QUEUE_STATUS, (void*)&qStatus);
            if (qStatus.instanceQueueCount == 0) {
                return TRUE;
            }
        }
        else if (result == RETCODE_VPU_RESPONSE_TIMEOUT) {
            SetVlcBufferState(vlcBuf, VLC_BUFFER_STATE_FREE);
            VLOG(INFO, "<%s:%d> Failed to VPU_DecStartOneFrame() ret(%d)\n", __FUNCTION__, __LINE__, result);
            CNMErrorSet(CNM_ERROR_HANGUP);
            HandleDecoderError(ctx->handle, ctx->numDecoded, NULL);
            PrintDecVpuStatus(ctx->handle);
            return FALSE;
        }
        else {
            SetVlcBufferState(vlcBuf, VLC_BUFFER_STATE_FREE);
            VLOG(ERR, "<%s:%d> Failed to VPU_DecStartOneFrame() error code: %08x\n", __FUNCTION__, __LINE__, result);
            return FALSE;
        }
    }

    if ((intStatus=HandlingInterruptFlag(com)) == DEC_INT_STATUS_TIMEOUT) {
        if (ctx->scenarioTest == TRUE) {
            VLOG(ERR, "<%s:%d> VPU INTERRUTP TIMEOUT : GO TO RESET PROCESS\n", __FUNCTION__, __LINE__);
            ctx->doReset = TRUE;
            return TRUE;
        }
        else {
            HandleDecoderError(ctx->handle, ctx->numDecoded, NULL);
            PrintDecVpuStatus(ctx->handle);
            DoReset(com);
            return FALSE;
        }
    }
    else if (intStatus == DEC_INT_STATUS_NONE || intStatus == DEC_INT_STATUS_EMPTY) {
        if (intStatus == DEC_INT_STATUS_EMPTY) {
            VLOG(INFO, "INSTANCE#%d EMPTY INTERRUPT\n", ctx->handle->instIndex);
        }
        return TRUE;    // Try again
    }
    else if (intStatus == DEC_INT_STATUS_VLC_BUFFER) {
        VLOG(TRACE, "INSTANCE#%d DEC_INT_STATUS_VLC_BUFFER\n", ctx->handle->instIndex);
        return AllocateVlcBuffer(ctx);
    }

    // Get data from the sink component.
    result = VPU_DecGetOutputInfo(ctx->handle, &decOutputInfo);
    if (TRUE == ctx->dynamicFbAlloc) {
        if (ctx->testDecConfig.enableWTL == TRUE) {
            FindFrameBuffer(ctx, &decOutputInfo.dispFrame);
            out->dynamicAlloc = ctx->testDecConfig.enableWTL ? TRUE : FALSE;
            decOutputInfo.dispPicWidth = decOutputInfo.rcDisplay.right = decOutputInfo.dispFrame.width;	// host can know linear info.
            decOutputInfo.dispPicHeight = decOutputInfo.rcDisplay.bottom = decOutputInfo.dispFrame.height;
        }
    }
    if (result == RETCODE_SUCCESS) {
        DisplayDecodedInformation(ctx->handle, ctx->decOpenParam.bitstreamFormat, ctx->numDecoded, &decOutputInfo, ctx->testDecConfig.performance, ctx->cyclePerTick);
    }
#ifdef SUPPORT_SG_FLOW
    {
        QueueStatusInfo QueueStatus;
        VPU_DecGiveCommand(ctx->handle, DEC_GET_QUEUE_STATUS, &QueueStatus);
        if (QueueStatus.instanceQueueCount > 0)
        {
            VLOG(ERR, "ERROR %d inst, QueueStatus tot=%d , inst=%d \n", ctx->handle->instIndex, QueueStatus.totalQueueCount, QueueStatus.instanceQueueCount);
            return FALSE;
        }
        if (QueueStatus.totalQueueCount < 1)
        {
            VLOG(WARN, "WARN %d inst, QueueStatus tot=%d , inst=%d \n", ctx->handle->instIndex, QueueStatus.totalQueueCount, QueueStatus.instanceQueueCount);
        }
    }
#endif
    lsnpPicDone.handle = ctx->handle;
    lsnpPicDone.ret        = result;
    lsnpPicDone.decParam   = &ctx->decParam;
    lsnpPicDone.output     = &decOutputInfo;
    lsnpPicDone.numDecoded = ctx->numDecoded;
    lsnpPicDone.vbUser     = ctx->vbUserData;
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_GET_OUTPUT_INFO, (void*)&lsnpPicDone);

    if (0 <= decOutputInfo.indexFrameDecoded || DECODED_IDX_FLAG_SKIP == decOutputInfo.indexFrameDecoded) {
        if (NULL != (vlcBuf = FindVlcBuffer(decOutputInfo.VLCOutBufBase))) {
            SetVlcBufferState(vlcBuf, VLC_BUFFER_STATE_FREE);
            VLOG(INFO, "THE VLC BUFFER MARKED AS FREE %p\n", vlcBuf->vb.phys_addr);
        }
    }

    if (result == RETCODE_REPORT_NOT_READY) {
        VLOG(INFO, "!!!! REPORT_NOT_READY!!!!\n");
        return TRUE; // Not decoded yet. Try again
    }
    else if (result != RETCODE_SUCCESS) {
        /* ERROR */
        VLOG(ERR, "Failed to decode error\n");
        return FALSE;
    }
    if (decOutputInfo.warnInfo)
        VLOG(1, "warnInfo=0x%x \n", decOutputInfo.warnInfo);
    if ((decOutputInfo.decodingSuccess & 0x01) == 0) {
        VLOG(ERR, "VPU_DecGetOutputInfo decode fail framdIdx %d error(0x%08x) reason(0x%08x), reasonExt(0x%08x)\n",
            ctx->numDecoded, decOutputInfo.decodingSuccess, decOutputInfo.errorReason, decOutputInfo.errorReasonExt);
        if ((decOutputInfo.errorReason == WAVE5_SYSERR_WATCHDOG_TIMEOUT) || (decOutputInfo.errorReason == WAVE5_SYSERR_VCPU_WATCHDOG_TIMEOUT)) {
#ifdef SUPPORT_RECOVER_AFTER_ERROR
            VLOG(ERR, "VLCoutBufBase=0x%x, WAVE5_SYSERR_WATCHDOG_TIMEOUT WAVE5_SYSERR_WATCHDOG_TIMEOUT\n", decOutputInfo.VLCOutBufBase);
            ctx->doIframeSearchByError = TRUE;
            ctx->doFlush = TRUE; // remove all of pending command in queue
#endif
        }
        else if (decOutputInfo.errorReason == WAVE5_SPECERR_OVER_PICTURE_WIDTH_SIZE || decOutputInfo.errorReason == WAVE5_SPECERR_OVER_PICTURE_HEIGHT_SIZE) {
            VLOG(ERR, "Not supported Width or Height\n");
            return FALSE;
        }
    }

    if (CheckAndDoSequenceChange(com, &decOutputInfo) == FALSE) {
        return FALSE;
    }

    if (decOutputInfo.indexFrameDecoded >= 0 || decOutputInfo.indexFrameDecoded == DECODED_IDX_FLAG_SKIP) {
        // Return a used data to a source port.
        ctx->numDecoded++;
        ctx->decodedAddr = decOutputInfo.bytePosFrameStart;
        out->reuse = FALSE;
#ifdef SUPPORT_RECOVER_AFTER_ERROR
        if (ctx->doIframeSearchByError == TRUE) {
            ctx->decParam.skipframeMode = 0;
            ctx->doIframeSearchByError = FALSE;
        }
#endif
    }

    if (decOutputInfo.indexFrameDisplay >= 0) {
        ctx->numOutput++;
        out->last  = FALSE;
        out->reuse = FALSE;
    }
    else if (decOutputInfo.indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END) {
        out->last       = TRUE;
        ctx->stateDoing = FALSE;
        com->terminate  = TRUE;
    }
    else {
        ReleaseFrameBuffer(ctx, (FrameBuffer*)&decOutputInfo.dispFrame);
    }

    if (out->reuse == FALSE) {
        osal_memcpy((void*)&out->decInfo, (void*)&decOutputInfo, sizeof(DecOutputInfo));
    }

    if (ctx->frameNumToStop > 0) {
        if (ctx->frameNumToStop == ctx->numOutput) {
            com->terminate = TRUE;
        }
    }

    return TRUE;
}

static CNMComponentParamRet GetParameterDecoder(ComponentImpl* from, ComponentImpl* com, GetParameterCMD commandType, void* data)
{
    DecoderContext*             ctx     = (DecoderContext*)com->context;
    BOOL                        result  = TRUE;
    PhysicalAddress             rdPtr, wrPtr;
    Uint32                      room;
    ParamDecBitstreamBufPos*    bsPos   = NULL;
    ParamDecNeedFrameBufferNum* fbNum;
    ParamDecStatus*             status;
    QueueStatusInfo             cqInfo;
    PortContainerES*            container;
    vpu_buffer_t                vb;

    if (ctx->handle == NULL)  return CNM_COMPONENT_PARAM_NOT_READY;
    if (ctx->doReset == TRUE) return CNM_COMPONENT_PARAM_NOT_READY;

    switch(commandType) {
    case GET_PARAM_COM_IS_CONTAINER_CONUSUMED:
        // This query command is sent from the comonponent core.
        // If input data are consumed in sequence, it should return TRUE through PortContainer::consumed.
        container = (PortContainerES*)data;
        vb = container->buf;
        if (vb.phys_addr <= ctx->decodedAddr && ctx->decodedAddr < (vb.phys_addr+vb.size)) {
            container->consumed = TRUE;
            ctx->decodedAddr = 0;
        }
        break;
    case GET_PARAM_DEC_HANDLE:
        *(DecHandle*)data = ctx->handle;
        break;
    case GET_PARAM_DEC_FRAME_BUF_NUM:
        {
            if (ctx->state <= DEC_STATE_INIT_SEQ) return CNM_COMPONENT_PARAM_NOT_READY;
            fbNum = (ParamDecNeedFrameBufferNum*)data;

            /* The inplace buffer on and the dynamic buffer mode on */
            if (0 != ctx->testDecConfig.inPlaceBufferMode) {
                fbNum->compressedNum = (Uint32)ctx->testDecConfig.inPlaceBufferMode;
                ctx->dynamicFbAlloc  = TRUE;
            }
            else {
                /* inplace mode 0 or customDisableFlag 0
                 * customerDisableFlag: the inplace mode off and the dynamic buffer allocation mode off
                 */
                fbNum->compressedNum = ctx->initialInfo.minFrameBufferCount + EXTRA_FRAME_BUFFER_NUM;   // max_dec_pic_buffering
                ctx->dynamicFbAlloc  = (FALSE == ctx->testDecConfig.customDisableFlag);                 // The linear framebuffers are allocated dynamically when the customDisableFlag is FALSE
            }

            if (ctx->decOpenParam.wtlEnable == TRUE) {
                if (TRUE == ctx->dynamicFbAlloc) {
                    fbNum->linearNum = 0;
                }
                else {
                    fbNum->linearNum = (ctx->initialInfo.frameBufDelay+1) + EXTRA_FRAME_BUFFER_NUM;     // The frameBufDelay can be zero.
                }

                if ((ctx->decOpenParam.bitstreamFormat == STD_VP9) || (ctx->decOpenParam.bitstreamFormat == STD_AVS2)) {
                    fbNum->linearNum = fbNum->compressedNum;
                }
                if (ctx->testDecConfig.performance == TRUE) {
                    if ((ctx->decOpenParam.bitstreamFormat == STD_VP9) || (ctx->decOpenParam.bitstreamFormat == STD_AVS2)) {
                        fbNum->linearNum++;
                        fbNum->compressedNum++;
                    }
                    else {
                        fbNum->linearNum += 3;
                    }
                }
            }
            else {
                fbNum->linearNum = 0;
            }
        }
        break;
    case GET_PARAM_DEC_BITSTREAM_BUF_POS:
        if (ctx->state < DEC_STATE_INIT_SEQ) return CNM_COMPONENT_PARAM_NOT_READY;
        VPU_DecGetBitstreamBuffer(ctx->handle, &rdPtr, &wrPtr, &room);
        bsPos = (ParamDecBitstreamBufPos*)data;
        bsPos->rdPtr = rdPtr;
        bsPos->wrPtr = wrPtr;
        bsPos->avail = room;
        break;
    case GET_PARAM_DEC_CODEC_INFO:
        if (ctx->state <= DEC_STATE_INIT_SEQ) return CNM_COMPONENT_PARAM_NOT_READY;
        VPU_DecGiveCommand(ctx->handle, DEC_GET_SEQ_INFO, data);
        break;
    case GET_PARAM_DEC_QUEUE_STATUS:
        if (ctx->state != DEC_STATE_DECODING) return CNM_COMPONENT_PARAM_NOT_READY;
        VPU_DecGiveCommand(ctx->handle, DEC_GET_QUEUE_STATUS, &cqInfo);
        status = (ParamDecStatus*)data;
        status->cq = cqInfo;
        break;
    default:
        result = FALSE;
        break;
    }

    return (result == TRUE) ? CNM_COMPONENT_PARAM_SUCCESS : CNM_COMPONENT_PARAM_FAILURE;
}

static CNMComponentParamRet SetParameterDecoder(ComponentImpl* from, ComponentImpl* com, SetParameterCMD commandType, void* data)
{
    BOOL            result = TRUE;
    DecoderContext* ctx    = (DecoderContext*)com->context;
    Int32           skipCmd;

    switch(commandType) {
    case SET_PARAM_COM_PAUSE:
        com->pause   = *(BOOL*)data;
        break;
    case SET_PARAM_DEC_SKIP_COMMAND:
        skipCmd = *(Int32*)data;
        ctx->decParam.skipframeMode = skipCmd;
        if (skipCmd == WAVE_SKIPMODE_NON_IRAP) {
            Uint32 userDataMask = (1<<H265_USERDATA_FLAG_RECOVERY_POINT);
            ctx->decParam.craAsBlaFlag = TRUE;
            /* For finding recovery point */
            VPU_DecGiveCommand(ctx->handle, ENABLE_REP_USERDATA, &userDataMask);
        }
        else {
            ctx->decParam.craAsBlaFlag = FALSE;
        }
        if (ctx->numDecoded > 0) {
            ctx->doFlush = (BOOL)(ctx->decParam.skipframeMode == WAVE_SKIPMODE_NON_IRAP);
        }
        break;
    case SET_PARAM_DEC_RESET:
        ctx->doReset = TRUE;
        break;
    case SET_PARAM_DEC_RETURN_FB:
        if (FALSE == ctx->dynamicFbAlloc) {
            VLOG(ERR, "<%s:%d> SET_PARAM_DEC_RETURN_FB is valid when the dynamicFbAlloc is true.\n");
            result = FALSE;
        }
        else {
            ReleaseFrameBuffer(ctx, (FrameBuffer*)data);
        }
        break;
    default:
        result = FALSE;
        break;
    }

    return (result == TRUE) ? CNM_COMPONENT_PARAM_SUCCESS : CNM_COMPONENT_PARAM_FAILURE;
}

static BOOL UpdateBitstream(DecoderContext* ctx, PortContainerES* in)
{
    RetCode       ret    = RETCODE_SUCCESS;
    BitStreamMode bsMode = ctx->decOpenParam.bitstreamMode;
    BOOL          update = TRUE;
    Uint32        updateSize;

    if (in == NULL) return TRUE;

    if (bsMode == BS_MODE_PIC_END) {
        VPU_DecSetRdPtr(ctx->handle, in->buf.phys_addr, TRUE);
    }
    else {
        if (in->size > 0) {
            PhysicalAddress rdPtr, wrPtr;
            Uint32          room;
            VPU_DecGetBitstreamBuffer(ctx->handle, &rdPtr, &wrPtr, &room);
            if ((Int32)room < in->size) {
                in->reuse = TRUE;
                return TRUE;
            }
        }
    }

    if (in->last == TRUE) {
        updateSize = (in->size == 0) ? STREAM_END_SET_FLAG : in->size;
    }
    else {
        updateSize = in->size;
        update     = (in->size > 0 && in->last == FALSE);
    }

    if (update == TRUE) {
        if ((ret=VPU_DecUpdateBitstreamBuffer(ctx->handle, updateSize)) != RETCODE_SUCCESS) {
            VLOG(INFO, "<%s:%d> Failed to VPU_DecUpdateBitstreamBuffer() ret(%d)\n", __FUNCTION__, __LINE__, ret);
            return FALSE;
        }
        if (in->last == TRUE) {
            VPU_DecUpdateBitstreamBuffer(ctx->handle, STREAM_END_SET_FLAG);
        }
    }

    in->reuse = FALSE;

    return TRUE;
}

static BOOL OpenDecoder(ComponentImpl* com)
{
    DecoderContext*         ctx     = (DecoderContext*)com->context;
    ParamDecBitstreamBuffer bsBuf;
    CNMComponentParamRet    ret;
    CNMComListenerDecOpen   lspn    = {0};
    BOOL                    success = FALSE;
    BitStreamMode           bsMode  = ctx->decOpenParam.bitstreamMode;
    vpu_buffer_t            vbUserData;
    RetCode                 retCode;

    ctx->stateDoing = TRUE;
    ret = ComponentGetParameter(com, com->srcPort.connectedComponent, GET_PARAM_FEEDER_BITSTREAM_BUF, &bsBuf);
    if (ComponentParamReturnTest(ret, &success) == FALSE) {
        return success;
    }

    ctx->decOpenParam.bitstreamBuffer     = (bsMode == BS_MODE_PIC_END) ? 0 : bsBuf.bs->phys_addr;
    ctx->decOpenParam.bitstreamBufferSize = (bsMode == BS_MODE_PIC_END) ? 0 : bsBuf.bs->size;

    ctx->decOpenParam.customDisableFlag   = ctx->testDecConfig.customDisableFlag;
    ctx->decOpenParam.inPlaceBufferMode   = ctx->testDecConfig.inPlaceBufferMode;
    ctx->decOpenParam.maxVerticalMV       = ctx->testDecConfig.maxVerticalMV;
    retCode = VPU_DecOpen(&ctx->handle, &ctx->decOpenParam);

    lspn.handle = ctx->handle;
    lspn.ret    = retCode;
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_OPEN, (void*)&lspn);

    if (retCode != RETCODE_SUCCESS) {
        VLOG(ERR, "<%s:%d> Failed to VPU_DecOpen(ret:%d)\n", __FUNCTION__, __LINE__, retCode);
        if ( retCode == RETCODE_VPU_RESPONSE_TIMEOUT)
            CNMErrorSet(CNM_ERROR_HANGUP);

        HandleDecoderError(NULL, 0, NULL);
        return FALSE;
    }
    //VPU_DecGiveCommand(ctx->handle, ENABLE_LOGGING, 0);

    if (ctx->vbUserData.size == 0) {
        vbUserData.size = (512*1024);
        vdi_lock(ctx->testDecConfig.coreIdx);
        vdi_allocate_dma_memory(ctx->testDecConfig.coreIdx, &vbUserData);
        vdi_unlock(ctx->testDecConfig.coreIdx);
    }
    VPU_DecGiveCommand(ctx->handle, SET_ADDR_REP_USERDATA, (void*)&vbUserData.phys_addr);
    VPU_DecGiveCommand(ctx->handle, SET_SIZE_REP_USERDATA, (void*)&vbUserData.size);
    VPU_DecGiveCommand(ctx->handle, ENABLE_REP_USERDATA,   (void*)&ctx->testDecConfig.enableUserData);

    if (ctx->testDecConfig.thumbnailMode == TRUE) {
        VPU_DecGiveCommand(ctx->handle, ENABLE_DEC_THUMBNAIL_MODE, NULL);
    }

    if (FALSE == ctx->testDecConfig.customDisableFlag) {
        // The wtl mode must be enabled when the inplace mode is on.
        if (ctx->testDecConfig.enableWTL == TRUE) {
            VPU_DecGiveCommand(ctx->handle, DEC_DISABLE_REORDER, NULL);
        }
    }

    ctx->vbUserData = vbUserData;
    ctx->stateDoing = FALSE;

    return TRUE;
}

static BOOL DecodeHeader(ComponentImpl* com, BOOL* done)
{
    DecoderContext*                ctx     = (DecoderContext*)com->context;
    DecHandle                      handle  = ctx->handle;
    Uint32                         coreIdx = ctx->testDecConfig.coreIdx;
    RetCode                        ret     = RETCODE_SUCCESS;
    DEC_INT_STATUS                 status;
    DecInitialInfo*                initialInfo = &ctx->initialInfo;
    SecAxiUse                      secAxiUse;
    CNMComListenerDecCompleteSeq   lsnpCompleteSeq;

    *done = FALSE;
    if (ctx->stateDoing == FALSE) {
        /* previous state done */
        ret = VPU_DecIssueSeqInit(handle);
        if (RETCODE_QUEUEING_FAILURE == ret) {
            return TRUE; // Try again
        }
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_ISSUE_SEQ, NULL);
        if (ret != RETCODE_SUCCESS) {
            VLOG(ERR, "%s:%d Failed to VPU_DecIssueSeqInit() ret(%d)\n", __FUNCTION__, __LINE__, ret);
            return FALSE;
        }
    }

    ctx->stateDoing = TRUE;

    while (com->terminate == FALSE) {
        if ((status=HandlingInterruptFlag(com)) == DEC_INT_STATUS_DONE) {
            break;
        }
        else if (status == DEC_INT_STATUS_TIMEOUT) {
            VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_SIZE);    /* To finish bitstream empty status */
            VPU_SWReset(coreIdx, SW_RESET_SAFETY, handle);
            VPU_DecUpdateBitstreamBuffer(handle, STREAM_END_CLEAR_FLAG);    /* To finish bitstream empty status */
            return FALSE;
        }
        else if (status == DEC_INT_STATUS_EMPTY) {
            PhysicalAddress rdPtr;
            PhysicalAddress wrPtr;
            Uint32 room;
            VPU_DecGetBitstreamBuffer(ctx->handle, &rdPtr, &wrPtr, &room);
            return TRUE;
        }
        else if (status == DEC_INT_STATUS_NONE) {
            return TRUE;
        }
        else {
            VLOG(INFO, "%s:%d Unknown interrupt status: %d\n", __FUNCTION__, __LINE__, status);
            return FALSE;
        }
    }

    ret = VPU_DecCompleteSeqInit(handle, initialInfo);

    strcpy(lsnpCompleteSeq.refYuvPath, ctx->testDecConfig.refYuvPath);
    lsnpCompleteSeq.ret             = ret;
    lsnpCompleteSeq.initialInfo     = initialInfo;
    lsnpCompleteSeq.wtlFormat       = ctx->wtlFormat;
    lsnpCompleteSeq.cbcrInterleave  = ctx->decOpenParam.cbcrInterleave;
    lsnpCompleteSeq.bitstreamFormat = ctx->decOpenParam.bitstreamFormat;
    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_COMPLETE_SEQ, (void*)&lsnpCompleteSeq);

    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "%s:%d FAILED TO DEC_PIC_HDR: ret(%d), SEQERR(%08x)\n", __FUNCTION__, __LINE__, ret, initialInfo->seqInitErrReason);
        return FALSE;
    }
    VLOG(INFO, "INSTANCE#%d min fb count: %d\n", ctx->handle->instIndex, initialInfo->minFrameBufferCount);

    if (ctx->decOpenParam.wtlEnable == TRUE) {
        VPU_DecGiveCommand(ctx->handle, DEC_SET_WTL_FRAME_FORMAT, &ctx->wtlFormat);
    }

   /* Set up the secondary AXI is depending on H/W configuration.
    * Note that turn off all the secondary AXI configuration
    * if target ASIC has no memory only for IP, LF and BIT.
    */
    secAxiUse.u.wave.useIpEnable    = (ctx->testDecConfig.secondaryAXI&0x01) ? TRUE : FALSE;
    secAxiUse.u.wave.useLfRowEnable = (ctx->testDecConfig.secondaryAXI&0x02) ? TRUE : FALSE;
    secAxiUse.u.wave.useBitEnable   = (ctx->testDecConfig.secondaryAXI&0x04) ? TRUE : FALSE;
    secAxiUse.u.wave.useSclEnable   = (ctx->testDecConfig.secondaryAXI&0x08) ? TRUE : FALSE;
    VPU_DecGiveCommand(ctx->handle, SET_SEC_AXI, &secAxiUse);
    ctx->enableScaler  = (ctx->testDecConfig.scaleDownWidth > 0 || ctx->testDecConfig.scaleDownHeight > 0);
    ctx->enableScaler |= (ctx->testDecConfig.numScaleDownList > 0);

    if (TRUE == ctx->testDecConfig.customDisableFlag) {
        if (TRUE == ctx->enableScaler) {
            ScalerInfo sclInfo = {0};

            sclInfo.scaleWidth  = ctx->testDecConfig.scaleDownWidth;
            sclInfo.scaleHeight = ctx->testDecConfig.scaleDownHeight;
            VLOG(INFO, "[SCALE INFO] %dx%d\n", sclInfo.scaleWidth, sclInfo.scaleHeight);
            sclInfo.enScaler    = TRUE;
            if (VPU_DecGiveCommand(handle, DEC_SET_SCALER_INFO, (void*)&sclInfo) != RETCODE_SUCCESS) {
                VLOG(ERR, "Failed to VPU_DecGiveCommand(DEC_SET_SCALER_INFO)\n");
                return FALSE;
            }
        }
    }

    ctx->stateDoing = FALSE;
    *done = TRUE;

    return TRUE;
}

static BOOL ExecuteDecoder(ComponentImpl* com, PortContainer* in , PortContainer* out)
{
    DecoderContext* ctx    = (DecoderContext*)com->context;
    BOOL            ret    = FALSE;;
    BitStreamMode   bsMode = ctx->decOpenParam.bitstreamMode;
    BOOL            done   = FALSE;
    // VLOG(WARN, "+ExecuteDecoder state=%d \n", ctx->state);
    if (in)  in->reuse = TRUE;
    if (out) out->reuse = TRUE;
    if (ctx->state == DEC_STATE_INIT_SEQ || ctx->state == DEC_STATE_DECODING) {
        if (UpdateBitstream(ctx, (PortContainerES*)in) == FALSE) {
            return FALSE;
        }
        if (in) {
            // In ring-buffer mode, it has to return back a container immediately.
            if (bsMode == BS_MODE_PIC_END) {
                if (ctx->state == DEC_STATE_INIT_SEQ) {
                    in->reuse = TRUE;
                }
                in->consumed = FALSE;
            }
            else {
                in->consumed = (in->reuse == FALSE);
            }
        }
    }

    if (ctx->doReset == TRUE) {
        if (in) in->reuse = FALSE;
        return DoReset(com);
    }

    switch (ctx->state) {
    case DEC_STATE_OPEN_DECODER:
        ret = OpenDecoder(com);
        if (ctx->stateDoing == FALSE) ctx->state = DEC_STATE_INIT_SEQ;
        break;
    case DEC_STATE_INIT_SEQ:
        ret = DecodeHeader(com, &done);
        if (TRUE == done) ctx->state = DEC_STATE_REGISTER_FB;
        break;
    case DEC_STATE_REGISTER_FB:
        ret = RegisterFrameBuffers(com);
        if (ctx->stateDoing == FALSE) {
            ctx->state = DEC_STATE_DECODING;
            DisplayDecodedInformation(ctx->handle, ctx->decOpenParam.bitstreamFormat, 0, NULL, ctx->testDecConfig.performance, 0);
        }
        break;
    case DEC_STATE_DECODING:
        ret = Decode(com, (PortContainerES*)in, (PortContainerDisplay*)out);
        break;
    default:
        ret = FALSE;
        break;
    }

    if (ret == FALSE || com->terminate == TRUE) {
        ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_DECODED_ALL, (void*)ctx->handle);
        if (out) {
            out->reuse = FALSE;
            out->last  = TRUE;
        }
    }

    // VLOG(WARN, "-ExecuteDecoder \n");
    return ret;
}

static BOOL PrepareDecoder(ComponentImpl* com, BOOL* done)
{
    *done = TRUE;

    return TRUE;
}

static void ReleaseDecoder(ComponentImpl* com)
{
    // Nothing to do
}

static BOOL DestroyDecoder(ComponentImpl* com)
{
    DecoderContext* ctx         = (DecoderContext*)com->context;
    DEC_INT_STATUS  intStatus;
    BOOL            success     = TRUE;
    Uint32          timeout     = 0;
    Uint32          index       = 0;
    Uint32          coreIdx     = ctx->testDecConfig.coreIdx;

    VPU_DecUpdateBitstreamBuffer(ctx->handle, STREAM_END_SET_FLAG);
    while (VPU_DecClose(ctx->handle) == RETCODE_VPU_STILL_RUNNING) {
        VLOG(ERR, "<%s:%d> Failed to VPU_DecClose RETCODE_VPU_STILL_RUNNING\n", __FUNCTION__, __LINE__);
        if ((intStatus=HandlingInterruptFlag(com)) == DEC_INT_STATUS_TIMEOUT) {
            HandleDecoderError(ctx->handle, ctx->numDecoded, NULL);
            VLOG(ERR, "<%s:%d> NO RESPONSE FROM VPU_DecClose()\n", __FUNCTION__, __LINE__);
            success = FALSE;
            break;
        }
        else if (intStatus == DEC_INT_STATUS_DONE) {
            DecOutputInfo outputInfo;
            VLOG(INFO, "VPU_DecClose() : CLEAR REMAIN INTERRUPT\n");
            VPU_DecGetOutputInfo(ctx->handle, &outputInfo);
            continue;
        }

        if (timeout > VPU_BUSY_CHECK_TIMEOUT) {
            VLOG(ERR, "<%s:%d> Failed to VPU_DecClose\n", __FUNCTION__, __LINE__);
        }

        for (index=0; index<MAX_REG_FRAME; index++) {
            VPU_DecClrDispFlag(ctx->handle, index);
        }

        timeout++;
        osal_msleep(10);
    }

    ComponentNotifyListeners(com, COMPONENT_EVENT_DEC_CLOSE, NULL);

    if (ctx->vbUserData.size) {
        vdi_lock(coreIdx);
        vdi_free_dma_memory(coreIdx, &ctx->vbUserData);
        vdi_unlock(coreIdx);
    }

    if (TRUE == ctx->dynamicFbAlloc) {
        ReleaseAllFrameBuffers(ctx);
    }

    if (0 == vdi_get_instance_num(coreIdx)) {
        for (index=0; index<COMMAND_QUEUE_DEPTH; index++) {
            if (0 < vlcBuffer[index].vb.size) {
#ifdef SUPPORT_SG_FLOW
#else
                vdi_free_dma_memory(coreIdx, &vlcBuffer[index].vb);
#endif
                vlcBuffer[index].vb.phys_addr = 0;
                vlcBuffer[index].vb.size      = 0;
            }
        }
        if (NULL != vlcBufferLock) {
            osal_mutex_destroy(vlcBufferLock);
            vlcBufferLock = NULL;
        }
    }

    VPU_DeInit(coreIdx);

    osal_mutex_destroy(ctx->lock);
    osal_free(ctx);

    return success;
}

static Component CreateDecoder(ComponentImpl* com, CNMComponentConfig* componentParam)
{
    DecoderContext* ctx;
    Uint32          coreIdx      = componentParam->testDecConfig.coreIdx;
    Uint16*         firmware     = (Uint16*)componentParam->bitcode;
    Uint32          firmwareSize = componentParam->sizeOfBitcode;
    RetCode         retCode;
    Int32           productId;
    ProductInfo     productInfo;


    retCode = VPU_InitWithBitcode(coreIdx, firmware, firmwareSize);
    if (retCode != RETCODE_SUCCESS && retCode != RETCODE_CALLED_BEFORE) {
        VLOG(INFO, "%s:%d Failed to VPU_InitiWithBitcode, ret(%08x)\n", __FUNCTION__, __LINE__, retCode);
        return FALSE;
    }
#ifdef SUPPORT_SG_FLOW
#else
    if (NULL == vlcBufferLock) {
        vpu_buffer_t    vb;
        Uint32          i;
        /* The Create member method is not called in concurrent. */
        vlcBufferLock = osal_mutex_create();
        osal_memset((void*)vlcBuffer, 0x00, sizeof(vlcBuffer));
        vb.phys_addr = 0;
        vb.size      = 1024;    /* Intentionally, it allocates vlc buffers with very small size
                                   for the VPU to assert the INT_WAVE5_INSUFFICIENT_VLC_BUFFER interrupt.
                                 */
        for (i=0; i<COMMAND_QUEUE_DEPTH; i++) {
            vdi_allocate_dma_memory(coreIdx, &vb);
            vlcBuffer[i].vb = vb;
        }
    }
#endif

    com->context = (DecoderContext*)osal_malloc(sizeof(DecoderContext));
    osal_memset(com->context, 0, sizeof(DecoderContext));
    ctx = (DecoderContext*)com->context;

    PrintVpuProductInfo(coreIdx, &productInfo);
    ctx->cyclePerTick = 32768;
    if ( ((productInfo.stdDef1>>27)&1) == 1 )
        ctx->cyclePerTick = 256;

    ctx->decOpenParam.bitstreamFormat = componentParam->testDecConfig.bitFormat;
    ctx->decOpenParam.coreIdx         = componentParam->testDecConfig.coreIdx;
    ctx->decOpenParam.bitstreamMode   = componentParam->testDecConfig.bitstreamMode;
    ctx->decOpenParam.wtlEnable       = componentParam->testDecConfig.enableWTL;
    ctx->decOpenParam.wtlMode         = componentParam->testDecConfig.wtlMode;
    ctx->decOpenParam.cbcrInterleave  = componentParam->testDecConfig.cbcrInterleave;
    ctx->decOpenParam.nv21            = componentParam->testDecConfig.nv21;
    ctx->decOpenParam.streamEndian    = componentParam->testDecConfig.streamEndian;
    ctx->decOpenParam.frameEndian     = componentParam->testDecConfig.frameEndian;
    ctx->decOpenParam.bwOptimization  = componentParam->testDecConfig.wave.bwOptimization;
    ctx->wtlFormat                    = componentParam->testDecConfig.wtlFormat;
    ctx->frameNumToStop               = componentParam->testDecConfig.forceOutNum;
    ctx->scenarioTest                 = componentParam->testDecConfig.scenarioTest;
    ctx->testDecConfig                = componentParam->testDecConfig;
    ctx->state                        = DEC_STATE_OPEN_DECODER;
    ctx->stateDoing                   = FALSE;
    ctx->lock                         = osal_mutex_create();
    ctx->dynamicFbAlloc               = (FALSE == componentParam->testDecConfig.customDisableFlag);

    productId = VPU_GetProductId(coreIdx);
    VLOG(INFO, "PRODUCT ID: %d\n", productId);

    return (Component)com;
}

ComponentImpl decoderComponentImpl = {
    "decoder",
    NULL,
    {0,},
    {0,},
    sizeof(PortContainerDisplay),
    5,
    CreateDecoder,
    GetParameterDecoder,
    SetParameterDecoder,
    PrepareDecoder,
    ExecuteDecoder,
    ReleaseDecoder,
    DestroyDecoder
};

