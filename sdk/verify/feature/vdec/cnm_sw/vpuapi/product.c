//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//  This confidential and proprietary software may be used only
//  as authorized by a licensing agreement from Chips&Media Inc.
//  In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2013  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//   The entire notice above must be reproduced on all authorized copies.
//
//--=========================================================================--
#include "product.h"
#include "coda9/coda9.h"
#include "wave/wave5.h"

VpuAttr g_VpuCoreAttributes[MAX_NUM_VPU_CORE];

static Int32 s_ProductIds[MAX_NUM_VPU_CORE] = {
    PRODUCT_ID_NONE,
};

typedef struct FrameBufInfoStruct {
    Uint32 unitSizeHorLuma;
    Uint32 sizeLuma;
    Uint32 sizeChroma;
    BOOL   fieldMap;
} FrameBufInfo;


Uint32 ProductVpuScan(Uint32 coreIdx)
{
    Uint32  i, productId;
    Uint32 foundProducts = 0;

    /* Already scanned */
    if (s_ProductIds[coreIdx] != PRODUCT_ID_NONE) 
        return 1;

    for (i=0; i<MAX_NUM_VPU_CORE; i++) {
        productId = Coda9VpuGetProductId(i);
        if (productId == PRODUCT_ID_NONE) 
            productId = WaveVpuGetProductId(i);
        if (productId != PRODUCT_ID_NONE) {
            s_ProductIds[i] = productId;
            foundProducts++;
        }
    }

    return (foundProducts >= 1);
}


Int32 ProductVpuGetId(Uint32 coreIdx)
{
    return s_ProductIds[coreIdx];
}

RetCode ProductVpuGetVersion(
    Uint32  coreIdx, 
    Uint32* versionInfo, 
    Uint32* revision 
    )
{
    Int32   productId = s_ProductIds[coreIdx];
    RetCode ret = RETCODE_SUCCESS;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuGetVersion(coreIdx, versionInfo, revision);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_520:
    case PRODUCT_ID_515:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuGetVersion(coreIdx, versionInfo, revision);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}

RetCode ProductVpuGetProductInfo(
    Uint32  coreIdx, 
    ProductInfo* productInfo 
    )
{
    Int32   productId = s_ProductIds[coreIdx];
    RetCode ret = RETCODE_SUCCESS;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuGetProductInfo(coreIdx, productInfo);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}

RetCode ProductVpuInit(Uint32 coreIdx, void* firmware, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS; 
    int     productId;

    productId  = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuInit(coreIdx, firmware, size);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuInit(coreIdx, firmware, size);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}

RetCode ProductVpuReInit(Uint32 coreIdx, void* firmware, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS; 
    int     productId;

    productId  = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuReInit(coreIdx, firmware, size);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuReInit(coreIdx, firmware, size);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}

Uint32 ProductVpuIsInit(Uint32 coreIdx)
{
    Uint32  pc = 0;
    int     productId;

    productId  = s_ProductIds[coreIdx];

    if (productId == PRODUCT_ID_NONE) {
        ProductVpuScan(coreIdx);
        productId  = s_ProductIds[coreIdx];
    }

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        pc = Coda9VpuIsInit(coreIdx);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        pc = Wave5VpuIsInit(coreIdx);
        break;
    }

    return pc;
}

Int32 ProductVpuIsBusy(Uint32 coreIdx)
{
    Int32  busy;
    int    productId;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        busy = Coda9VpuIsBusy(coreIdx);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        busy = Wave5VpuIsBusy(coreIdx);
        break;
    default:
        busy = 0;
        break;
    }

    return busy;
}

Int32 ProductVpuWaitInterrupt(CodecInst *instance, Int32 timeout)
{
    int     productId;
    int     flag = -1;

    productId = s_ProductIds[instance->coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        flag = Coda9VpuWaitInterrupt(instance, timeout);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        flag = Wave5VpuWaitInterrupt(instance, timeout, FALSE);
        break;
    default:
        flag = -1;
        break;
    }

    return flag;
}

RetCode ProductVpuReset(Uint32 coreIdx, SWResetMode resetMode)
{
    int     productId;
    RetCode ret = RETCODE_SUCCESS;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuReset(coreIdx, resetMode);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuReset(coreIdx, resetMode);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
        break;
    }

    return ret;
}

RetCode ProductVpuSleepWake(Uint32 coreIdx, int iSleepWake, const Uint16* code, Uint32 size)
{
    int     productId;
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuSleepWake(coreIdx, iSleepWake, (void*)code, size);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuSleepWake(coreIdx, iSleepWake, (void*)code, size, FALSE);
        break;
    }

    return ret;
}
RetCode ProductVpuClearInterrupt(Uint32 coreIdx, Uint32 flags)
{
    int     productId;
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuClearInterrupt(coreIdx);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuClearInterrupt(coreIdx, flags);
        break;
    }

    return ret;
}

RetCode ProductVpuDecBuildUpOpenParam(CodecInst* pCodec, DecOpenParam* param)
{
    Int32   productId;
    Uint32  coreIdx;
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    coreIdx   = pCodec->coreIdx;
    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuBuildUpDecParam(pCodec, param);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuBuildUpDecParam(pCodec, param);
        break;
    }

    return ret;
}

PhysicalAddress ProductVpuDecGetRdPtr(CodecInst* instance)
{
    Int32   productId;
    Uint32  coreIdx;
    PhysicalAddress retRdPtr; 
    DecInfo*    pDecInfo;
    RetCode ret = RETCODE_SUCCESS;

    pDecInfo = VPU_HANDLE_TO_DECINFO(instance);

    coreIdx   = instance->coreIdx;
    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuDecGetRdPtr(instance, &retRdPtr);
        if (ret != RETCODE_SUCCESS)
            retRdPtr = pDecInfo->streamRdPtr;
        else {
#ifdef FIX_SET_GET_RD_PTR_BUG
			if (pDecInfo->openParam.bitstreamMode == BS_MODE_INTERRUPT) {
				pDecInfo->streamRdPtr = retRdPtr;
			}
#else
            pDecInfo->streamRdPtr = retRdPtr;
#endif
		}
        break;
    default:
        retRdPtr = VpuReadReg(coreIdx, pDecInfo->streamRdPtrRegAddr);
        break;
    }

    return retRdPtr;

}



RetCode ProductCheckDecOpenParam(DecOpenParam* param)
{
    Int32       productId; 
    Uint32      coreIdx;
    VpuAttr*    pAttr;

    if (param == 0) 
        return RETCODE_INVALID_PARAM;

    if (param->coreIdx > MAX_NUM_VPU_CORE) 
        return RETCODE_INVALID_PARAM;

    coreIdx   = param->coreIdx;
    productId = s_ProductIds[coreIdx];
    pAttr     = &g_VpuCoreAttributes[coreIdx];

    if (param->bitstreamBuffer % 8) 
        return RETCODE_INVALID_PARAM;

    if (param->bitstreamMode == BS_MODE_INTERRUPT) {
        if (param->bitstreamBufferSize % 1024 || param->bitstreamBufferSize < 1024) 
            return RETCODE_INVALID_PARAM;
    }

    if (PRODUCT_ID_W_SERIES(productId)) {
        if (param->virtAxiID > 16) {
            // Maximum number of AXI channels is 15
            return RETCODE_INVALID_PARAM;
        }
    }

    // Check bitstream mode
    if ((pAttr->supportBitstreamMode&(1<<param->bitstreamMode)) == 0)
        return RETCODE_INVALID_PARAM;

    if ((pAttr->supportDecoders&(1<<param->bitstreamFormat)) == 0) 
        return RETCODE_INVALID_PARAM;

    /* check framebuffer endian */
    if ((pAttr->supportEndianMask&(1<<param->frameEndian)) == 0) {
        APIDPRINT("%s:%d Invalid frame endian(%d)\n", __FUNCTION__, __LINE__, (Int32)param->frameEndian);
        return RETCODE_INVALID_PARAM;
    }

    /* check streambuffer endian */
    if ((pAttr->supportEndianMask&(1<<param->streamEndian)) == 0) {
        APIDPRINT("%s:%d Invalid stream endian(%d)\n", __FUNCTION__, __LINE__, (Int32)param->streamEndian);
        return RETCODE_INVALID_PARAM;
    }

    /* check WTL */
    if (param->wtlEnable) {
        if (pAttr->supportWTL == 0) 
            return RETCODE_NOT_SUPPORTED_FEATURE;
        switch (productId) {
        case PRODUCT_ID_960:
        case PRODUCT_ID_980:
            if (param->wtlMode != FF_FRAME && param->wtlMode != FF_FIELD ) 
                return RETCODE_INVALID_PARAM;
            break;
        default:
            break;
        }
    }

    /* Tiled2Linear */
    if (param->tiled2LinearEnable) {
        if (pAttr->supportTiled2Linear == 0) 
            return RETCODE_NOT_SUPPORTED_FEATURE;

        if (productId == PRODUCT_ID_960 || productId == PRODUCT_ID_980) {
            if (param->tiled2LinearMode != FF_FRAME && param->tiled2LinearMode != FF_FIELD ) {
                APIDPRINT("%s:%d Invalid Tiled2LinearMode(%d)\n", __FUNCTION__, __LINE__, (Int32)param->tiled2LinearMode);
                return RETCODE_INVALID_PARAM;
            }
        }
    }
    if (productId == PRODUCT_ID_960 || productId == PRODUCT_ID_980) {
        if( param->mp4DeblkEnable == 1 && !(param->bitstreamFormat == STD_MPEG4 || param->bitstreamFormat == STD_H263 || param->bitstreamFormat == STD_MPEG2 || param->bitstreamFormat == STD_DIV3)) 
            return RETCODE_INVALID_PARAM;
        if (param->wtlEnable && param->tiled2LinearEnable) 
            return RETCODE_INVALID_PARAM;
    } 
    else {
        if (param->mp4DeblkEnable || param->mp4Class)
            return RETCODE_INVALID_PARAM;
        if (param->avcExtension)
            return RETCODE_INVALID_PARAM;
        if (param->tiled2LinearMode != FF_NONE)
            return RETCODE_INVALID_PARAM;
    }

    return RETCODE_SUCCESS;
}

RetCode ProductVpuDecInitSeq(CodecInst* instance)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId   = instance->productId;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuDecInitSeq(instance);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuDecInitSeq(instance);
        break;
    }

    return ret;
}

RetCode ProductVpuDecFiniSeq(CodecInst* instance)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId   = instance->productId;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuFiniSeq(instance);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuDecFiniSeq(instance);
        break;
    }

    return ret;
}

RetCode ProductVpuDecGetSeqInfo(CodecInst* instance, DecInitialInfo* info)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId   = instance->productId;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuDecGetSeqInfo(instance, info);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuDecGetSeqInfo(instance, info);
        break;
    }

    return ret;
}

RetCode ProductVpuDecCheckCapability(CodecInst* instance)
{
    DecInfo* pDecInfo;
    VpuAttr* pAttr     = &g_VpuCoreAttributes[instance->coreIdx];

    pDecInfo = &instance->CodecInfo->decInfo;

    if ((pAttr->supportDecoders&(1<<pDecInfo->openParam.bitstreamFormat)) == 0)
        return RETCODE_NOT_SUPPORTED_FEATURE;

    switch (instance->productId) {
    case PRODUCT_ID_960:
        if (pDecInfo->mapType >= TILED_FRAME_NO_BANK_MAP) 
            return RETCODE_NOT_SUPPORTED_FEATURE;
        if (pDecInfo->tiled2LinearMode == FF_FIELD) 
            return RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_980:
        if (pDecInfo->mapType >= COMPRESSED_FRAME_MAP) 
            return RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        if (pDecInfo->mapType != LINEAR_FRAME_MAP && pDecInfo->mapType != COMPRESSED_FRAME_MAP)
            return RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return RETCODE_SUCCESS;
}

RetCode ProductVpuDecode(CodecInst* instance, DecParam* option)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId = instance->productId;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuDecode(instance, option);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuDecode(instance, option);
        break;
    }

    return ret;
}

RetCode ProductVpuDecGetResult(CodecInst*  instance, DecOutputInfo* result)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId = instance->productId;

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        ret = Coda9VpuDecGetResult(instance, result);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuDecGetResult(instance, result);
        break;
    }

    return ret;
}

RetCode ProductVpuDecFlush(CodecInst* instance, FramebufferIndex* retIndexes, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS;

    switch (instance->productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Wave5VpuDecFlush(instance, retIndexes, size);
        break;
    default:
        ret = Coda9VpuDecFlush(instance, retIndexes, size);
        break;
    }

    return ret;
}

/************************************************************************/
/* Decoder & Encoder                                                    */
/************************************************************************/

RetCode ProductVpuDecSetBitstreamFlag(
    CodecInst*  instance,
    BOOL        running,
    Int32       size
    )
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    BOOL        eos; 
    BOOL        checkEos;
    BOOL        explicitEnd;
    DecInfo*    pDecInfo = &instance->CodecInfo->decInfo;

    productId = instance->productId;

    eos      = (BOOL)(size == 0);
    checkEos = (BOOL)(size > 0);
    explicitEnd = (BOOL)(size == -2);

    switch (productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        if (checkEos || explicitEnd) eos = (BOOL)((pDecInfo->streamEndflag&0x04) == 0x04);
        ret = Coda9VpuDecSetBitstreamFlag(instance, running, eos);
        break;
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        if (checkEos || explicitEnd) eos = (BOOL)pDecInfo->streamEndflag;
        ret = Wave5VpuDecSetBitstreamFlag(instance, running, eos, explicitEnd);
        break;
    }

    return ret;
}

/**
 * \param   stride          stride of framebuffer in pixel.
 */
RetCode ProductVpuAllocateFramebuffer(
    CodecInst* inst, FrameBuffer* fbArr, TiledMapType mapType, Int32 num, 
    Int32 stride, Int32 height, FrameBufferFormat format, 
    BOOL cbcrInterleave, BOOL nv21, Int32 endian, 
    vpu_buffer_t* vb, Int32 gdiIndex,
    FramebufferAllocType fbType)
{
    Int32           i;
    Uint32          coreIdx;
    vpu_buffer_t    vbFrame;
    FrameBufInfo    fbInfo;
    DecInfo*        pDecInfo = &inst->CodecInfo->decInfo;
    // Variables for TILED_FRAME/FILED_MB_RASTER
    Uint32          sizeLuma;
    Uint32          sizeChroma;
    ProductId       productId     = (ProductId)inst->productId;
    RetCode         ret           = RETCODE_SUCCESS;
    
    osal_memset((void*)&vbFrame, 0x00, sizeof(vpu_buffer_t));
    osal_memset((void*)&fbInfo,  0x00, sizeof(FrameBufInfo));

    coreIdx = inst->coreIdx;

    if (inst->codecMode == W_VP9_DEC) {
        Uint32 framebufHeight = VPU_ALIGN64(height);
        sizeLuma   = CalcLumaSize(inst->productId, stride, framebufHeight, format, cbcrInterleave, mapType, NULL);
        sizeChroma = CalcChromaSize(inst->productId, stride, framebufHeight, format, cbcrInterleave, mapType, NULL);
    } 
    else {
        DRAMConfig* bufferConfig = NULL;
        if (productId == PRODUCT_ID_960) {
            bufferConfig = &pDecInfo->dramCfg;
        }
        sizeLuma   = CalcLumaSize(inst->productId, stride, height, format, cbcrInterleave, mapType, bufferConfig);
        sizeChroma = CalcChromaSize(inst->productId, stride, height, format, cbcrInterleave, mapType, bufferConfig);
    }

    // Framebuffer common informations
    for (i=0; i<num; i++) {
        if (fbArr[i].updateFbInfo == TRUE ) {
            fbArr[i].updateFbInfo = FALSE;
            fbArr[i].myIndex        = i+gdiIndex;
            fbArr[i].stride         = stride;
            fbArr[i].height         = height;
            fbArr[i].mapType        = mapType;
            fbArr[i].format         = format;
            fbArr[i].cbcrInterleave = (mapType == COMPRESSED_FRAME_MAP ? TRUE : cbcrInterleave);
            fbArr[i].nv21           = nv21;
            fbArr[i].endian         = (mapType == COMPRESSED_FRAME_MAP ? VDI_128BIT_LITTLE_ENDIAN : endian);
            fbArr[i].lumaBitDepth   = pDecInfo->initialInfo.lumaBitdepth;
            fbArr[i].chromaBitDepth = pDecInfo->initialInfo.chromaBitdepth;
        }
    }

    //********* START : framebuffers for SVAC spatial SVC **********/
    // Decoder
    if (inst->codecMode == W_SVAC_DEC && pDecInfo->initialInfo.spatialSvcEnable == TRUE && mapType == COMPRESSED_FRAME_MAP) {
        stride = (pDecInfo->initialInfo.lumaBitdepth > 8) ? VPU_ALIGN32(VPU_ALIGN32(VPU_ALIGN128(pDecInfo->initialInfo.picWidth>>1)*5)/4) : VPU_ALIGN128(pDecInfo->initialInfo.picWidth>>1);
        for (i=num; i<num*2; i++) {
            if (fbArr[i].updateFbInfo == TRUE ) {
                fbArr[i].updateFbInfo = FALSE;
                fbArr[i].myIndex        = i+gdiIndex;
                fbArr[i].stride         = stride;
                fbArr[i].height         = VPU_ALIGN128(pDecInfo->initialInfo.picHeight>>1);
                fbArr[i].mapType        = COMPRESSED_FRAME_MAP_SVAC_SVC_BL;
                fbArr[i].format         = format;
                fbArr[i].cbcrInterleave = (mapType == COMPRESSED_FRAME_MAP ? TRUE : cbcrInterleave);
                fbArr[i].nv21           = nv21;
                fbArr[i].endian         = endian;
                fbArr[i].lumaBitDepth   = pDecInfo->initialInfo.lumaBitdepth;
                fbArr[i].chromaBitDepth = pDecInfo->initialInfo.chromaBitdepth;
                fbArr[i].sourceLBurstEn = FALSE;
            }
        }
    }

    // Encoder
    //********* END : framebuffers for SVAC spatial SVC **********/


    switch (mapType) {
    case LINEAR_FRAME_MAP:
    case LINEAR_FIELD_MAP:
    case COMPRESSED_FRAME_MAP:
        ret = UpdateFrameBufferAddr(mapType, fbArr, num, sizeLuma, sizeChroma);
        if (ret != RETCODE_SUCCESS)
            break;

        if (inst->codecMode == W_SVAC_DEC && pDecInfo->initialInfo.spatialSvcEnable == TRUE && mapType == COMPRESSED_FRAME_MAP) {
            stride = (pDecInfo->initialInfo.lumaBitdepth > 8) ? VPU_ALIGN32(VPU_ALIGN32(VPU_ALIGN128(pDecInfo->initialInfo.picWidth>>1)*5)/4) : VPU_ALIGN128(pDecInfo->initialInfo.picWidth>>1);
            // Update FrameBufferAddr for SVC BL
            sizeLuma   = CalcLumaSize(inst->productId, stride, VPU_ALIGN128(pDecInfo->initialInfo.picHeight>>1), format, cbcrInterleave, mapType, NULL);
            sizeChroma = CalcChromaSize(inst->productId, stride, VPU_ALIGN128(pDecInfo->initialInfo.picHeight>>1), format, cbcrInterleave, mapType, NULL);
            ret = UpdateFrameBufferAddr(COMPRESSED_FRAME_MAP_SVAC_SVC_BL, fbArr+num, num, sizeLuma, sizeChroma);
        }
        break;

    default:
        /* Tiled map */
        if (productId == PRODUCT_ID_960) {
            DRAMConfig*     pDramCfg;
            PhysicalAddress tiledBaseAddr = 0;
            TiledMapConfig* pMapCfg;

            pDramCfg = &pDecInfo->dramCfg;
            pMapCfg  = &pDecInfo->mapCfg;
            vbFrame.phys_addr = GetTiledFrameBase(coreIdx, fbArr, num); 
            if (fbType == FB_TYPE_PPU) {
                tiledBaseAddr = pMapCfg->tiledBaseAddr;
            }
            else {
                pMapCfg->tiledBaseAddr = vbFrame.phys_addr;
                tiledBaseAddr = vbFrame.phys_addr;
            }
            *vb = vbFrame;
            ret = AllocateTiledFrameBufferGdiV1(mapType, tiledBaseAddr, fbArr, num, sizeLuma, sizeChroma, pDramCfg);
        }
        else {
            // PRODUCT_ID_980
            ret = AllocateTiledFrameBufferGdiV2(mapType, fbArr, num, sizeLuma, sizeChroma);
        }
        break;
    }
    return ret;
}

RetCode ProductVpuRegisterFramebuffer(CodecInst* instance)
{
    RetCode         ret = RETCODE_FAILURE;
    FrameBuffer*    fb;
    DecInfo*        pDecInfo = &instance->CodecInfo->decInfo;
    Int32           gdiIndex = 0;
    switch (instance->productId) {
    case PRODUCT_ID_960:
    case PRODUCT_ID_980:
        if (IS_CODA_DECODER_HANDLE(instance))
            ret = Coda9VpuDecRegisterFramebuffer(instance);
        break;
    default:
        /************************************************************************/
        /*        for WAVE5 series (512/515/520/525/511/521...)                 */
        /************************************************************************/
        if (IS_WAVE_DECODER_HANDLE(instance)) {
            if (pDecInfo->mapType != COMPRESSED_FRAME_MAP)
                return RETCODE_NOT_SUPPORTED_FEATURE;

            fb = pDecInfo->frameBufPool;

            gdiIndex = 0;
            if (pDecInfo->wtlEnable == TRUE) {
                if (fb[0].mapType == COMPRESSED_FRAME_MAP) 
                    gdiIndex = pDecInfo->numFbsForDecoding;

                if (instance->codecMode == W_SVAC_DEC && pDecInfo->initialInfo.spatialSvcEnable == TRUE)
                    gdiIndex = pDecInfo->numFbsForDecoding*2;

                ret = Wave5VpuDecRegisterFramebuffer(instance, &fb[gdiIndex], LINEAR_FRAME_MAP, pDecInfo->numFbsForWTL);
                if (ret != RETCODE_SUCCESS)
                    return ret;
                gdiIndex = gdiIndex == 0 ? pDecInfo->numFbsForDecoding: 0;
            }

            ret = Wave5VpuDecRegisterFramebuffer(instance, &fb[gdiIndex], COMPRESSED_FRAME_MAP, pDecInfo->numFbsForDecoding);
            if (ret != RETCODE_SUCCESS)
                return ret;

            if (instance->codecMode == W_SVAC_DEC && pDecInfo->initialInfo.spatialSvcEnable == TRUE) { // BL for SVC
                gdiIndex = pDecInfo->numFbsForDecoding;
                ret = Wave5VpuDecRegisterFramebuffer(instance, &fb[gdiIndex], COMPRESSED_FRAME_MAP_SVAC_SVC_BL, pDecInfo->numFbsForDecoding);
                if (ret != RETCODE_SUCCESS)
                    return ret;
            }
        }
        else {
            // ENCODER
        }
        break;
    }
    return ret;
}

RetCode ProductVpuDecUpdateFrameBuffer(CodecInst* instance, FrameBuffer* fbcFb, FrameBuffer* linearFb, Uint32 mvColIndex, Uint32 picWidth, Uint32 picHeight)
{
    RetCode ret = RETCODE_NOT_SUPPORTED_FEATURE;

    if (PRODUCT_ID_W_SERIES(instance->productId)) {
        EnterLock(instance->coreIdx);
        ret = Wave5VpuDecUpdateFramebuffer(instance, fbcFb, linearFb, mvColIndex, picWidth, picHeight);
        LeaveLock(instance->coreIdx);
    }

    return ret;
}

Int32 ProductCalculateFrameBufSize(Int32 productId, Int32 stride, Int32 height, TiledMapType mapType, FrameBufferFormat format, BOOL interleave, DRAMConfig* pDramCfg)
{
    Int32 size_dpb_lum, size_dpb_chr, size_dpb_all;

    size_dpb_lum = CalcLumaSize(productId, stride, height, format, interleave, mapType, pDramCfg);
    size_dpb_chr = CalcChromaSize(productId, stride, height, format, interleave, mapType, pDramCfg);
    size_dpb_all = size_dpb_lum + size_dpb_chr*2;

    return size_dpb_all;
}


Int32 ProductCalculateAuxBufferSize(AUX_BUF_TYPE type, CodStd codStd, Int32 width, Int32 height)
{
    Int32 size = 0;

    switch (type) {
    case AUX_BUF_TYPE_MVCOL:
        if (codStd == STD_AVC || codStd == STD_VC1 || codStd == STD_MPEG4 || codStd == STD_H263 || codStd == STD_RV || codStd == STD_AVS ) {
            size =  VPU_ALIGN32(width)*VPU_ALIGN32(height);
            size = (size*3)/2;
            size = (size+4)/5;
            size = ((size+7)/8)*8;
        } 
        else if (codStd == STD_HEVC) {
            size = WAVE5_DEC_HEVC_MVCOL_BUF_SIZE(width, height);
        }
        else if (codStd == STD_VP9) {
            size = WAVE5_DEC_VP9_MVCOL_BUF_SIZE(width, height);
        }
        else if (codStd == STD_AVS2) {
            size = WAVE5_DEC_AVS2_MVCOL_BUF_SIZE(width, height);
        }
        else {
            size = 0;
        }
        break;
    case AUX_BUF_TYPE_FBC_Y_OFFSET:
        size  = WAVE5_FBC_LUMA_TABLE_SIZE(width, height);
        break;
    case AUX_BUF_TYPE_FBC_C_OFFSET:
        size  = WAVE5_FBC_CHROMA_TABLE_SIZE(width, height);
        break;
    }

    return size;
}

RetCode ProductClrDispFlag(CodecInst* instance, Uint32 index)
{
    RetCode ret = RETCODE_SUCCESS;
    ret = Wave5DecClrDispFlag(instance, index);
    return ret;
}

RetCode ProductSetDispFlag(CodecInst* instance, Uint32 index)
{
    RetCode ret = RETCODE_SUCCESS;
    ret = Wave5DecSetDispFlag(instance, index);
    return ret;
}

RetCode ProductVpuGetBandwidth(CodecInst* instance, VPUBWData* data)
{
    if (data == 0) {
        return RETCODE_INVALID_PARAM;
    }                

    if (instance->productId < PRODUCT_ID_520)
        return RETCODE_INVALID_COMMAND;

    return Wave5VpuGetBwReport(instance, data);
}

RetCode ProductVpuDecGetVlcInfo(CodecInst* instance, VLCBufInfo* vlcInfo)
{
    if (instance->productId != PRODUCT_ID_511)
        return RETCODE_INVALID_COMMAND;

    return Wave5VpuDecGetVlcInfo(instance, vlcInfo);
}

RetCode ProductVpuDecUpdateVlcInfo(CodecInst* instance, VLCBufInfo* vlcInfo)
{
    if (instance->productId != PRODUCT_ID_511)
        return RETCODE_INVALID_COMMAND;

    return Wave5VpuDecUpdateVlcInfo(instance, vlcInfo);
}


/************************************************************************/
/* ENCODER                                                              */
/************************************************************************/
 
