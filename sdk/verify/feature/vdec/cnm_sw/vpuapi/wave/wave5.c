#include "product.h"
#include "wave/wave5.h"
#include "vpuerror.h"
#include "wave/wave5_regdefine.h"

Uint32 Wave5VpuIsInit(Uint32 coreIdx)
{
    Uint32 pc;

    pc = (Uint32)VpuReadReg(coreIdx, W5_VCPU_CUR_PC);

    return pc;
}

Int32 Wave5VpuIsBusy(Uint32 coreIdx)
{
    return VpuReadReg(coreIdx, W5_VPU_BUSY_STATUS);
}

Int32 WaveVpuGetProductId(Uint32  coreIdx)
{
    Uint32  productId = PRODUCT_ID_NONE;
    Uint32  val;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return PRODUCT_ID_NONE;

    val = VpuReadReg(coreIdx, W5_PRODUCT_NUMBER);
    // for temp
    val = WAVE511_CODE;

    switch (val) {
    case WAVE512_CODE:   productId = PRODUCT_ID_512;   break;
    case WAVE520_CODE:   productId = PRODUCT_ID_520;   break;
    case WAVE515_CODE:   productId = PRODUCT_ID_515;   break;
    case WAVE525_CODE:   productId = PRODUCT_ID_525;   break;
    case WAVE521_CODE:   productId = PRODUCT_ID_521;   break;
    case WAVE521C_CODE:  productId = PRODUCT_ID_521;   break;
    case WAVE511_CODE:   productId = PRODUCT_ID_511;   break;
    default:
        VLOG(ERR, "Check productId(%d)\n", val);
        break;
    }
    return productId;
}

void Wave5BitIssueCommand(CodecInst* instance, Uint32 cmd)
{
    Uint32 instanceIndex = 0;
    Uint32 codecMode     = 0;
    Uint32 coreIdx;

    if (instance == NULL) {
        return ;
    }

    instanceIndex = instance->instIndex;
    codecMode     = instance->codecMode;
    coreIdx = instance->coreIdx;

    VpuWriteReg(coreIdx, W5_CMD_INSTANCE_INFO,  (codecMode<<16)|(instanceIndex&0xffff));
    VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 1);
    VpuWriteReg(coreIdx, W5_COMMAND, cmd);

    if ((instance != NULL && instance->loggingEnable))
        vdi_log(coreIdx, cmd, 1);

    VpuWriteReg(coreIdx, W5_VPU_HOST_INT_REQ, 1);
    return;
}

static RetCode SendQuery(CodecInst* instance, QUERY_OPT queryOpt)
{
    // Send QUERY cmd
    VpuWriteReg(instance->coreIdx, W5_QUERY_OPTION, queryOpt);
    VpuWriteReg(instance->coreIdx, W5_VPU_BUSY_STATUS, 1);
    Wave5BitIssueCommand(instance, W5_QUERY);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        if (instance->loggingEnable)
            vdi_log(instance->coreIdx, W5_QUERY, 2);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE)
        return RETCODE_FAILURE;

    return RETCODE_SUCCESS;

}

RetCode Wave5VpuEncGiveCommand(CodecInst *pCodecInst, CodecCommand cmd, void *param)
{
    RetCode ret = RETCODE_SUCCESS;

    switch (cmd) {
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
    }

    return ret;
}

static RetCode SetupWave5Properties(Uint32 coreIdx)
{
    VpuAttr*    pAttr = &g_VpuCoreAttributes[coreIdx];
    Uint32      regVal;
    Uint8*      str;
    RetCode     ret = RETCODE_SUCCESS;

    VpuWriteReg(coreIdx, W5_QUERY_OPTION, GET_VPU_INFO);
    VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 1);
    VpuWriteReg(coreIdx, W5_COMMAND, W5_QUERY);
    VpuWriteReg(coreIdx, W5_VPU_HOST_INT_REQ, 1);
    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(coreIdx, W5_RET_SUCCESS) == FALSE) {
        ret = RETCODE_QUERY_FAILURE;
    }
    else {
        regVal = VpuReadReg(coreIdx, W5_RET_PRODUCT_NAME);
        str    = (Uint8*)&regVal;
        pAttr->productName[0] = str[3];
        pAttr->productName[1] = str[2];
        pAttr->productName[2] = str[1];
        pAttr->productName[3] = str[0];
        pAttr->productName[4] = 0;

        pAttr->productId       = WaveVpuGetProductId(coreIdx);

        pAttr->hwConfigDef0    = VpuReadReg(coreIdx, W5_RET_STD_DEF0);
        pAttr->hwConfigDef1    = VpuReadReg(coreIdx, W5_RET_STD_DEF1);
        pAttr->hwConfigFeature = VpuReadReg(coreIdx, W5_RET_CONF_FEATURE);
        pAttr->hwConfigDate    = VpuReadReg(coreIdx, W5_RET_CONF_DATE);
        pAttr->hwConfigRev     = VpuReadReg(coreIdx, W5_RET_CONF_REVISION);
        pAttr->hwConfigType    = VpuReadReg(coreIdx, W5_RET_CONF_TYPE);

        pAttr->supportGDIHW          = TRUE;
        pAttr->supportDecoders       = (1<<STD_HEVC);
        if (pAttr->productId == PRODUCT_ID_512) {
            pAttr->supportDecoders       |= (1<<STD_VP9);
        }
        if (pAttr->productId == PRODUCT_ID_515) {
            pAttr->supportDecoders       |= (1<<STD_VP9);
            pAttr->supportDecoders       |= (1<<STD_AVS2);
        }

        pAttr->supportEncoders       = 0;
        if (pAttr->productId == PRODUCT_ID_520) {
            pAttr->supportEncoders |= (1<<STD_HEVC);
            pAttr->supportBackbone  = TRUE;
        }

        if (pAttr->productId == PRODUCT_ID_525) {
            pAttr->supportDecoders       = (1<<STD_HEVC);
            pAttr->supportDecoders       |= (1<<STD_SVAC);
            pAttr->supportEncoders       = (1<<STD_HEVC);
            pAttr->supportEncoders       |= (1<<STD_SVAC);
            pAttr->supportBackbone       = TRUE;
        }

        if (pAttr->productId == PRODUCT_ID_521) {
            pAttr->supportDecoders       |= (1<<STD_AVC);
            pAttr->supportEncoders       = (1<<STD_HEVC);
            pAttr->supportEncoders       |= (1<<STD_AVC);
            pAttr->supportBackbone       = TRUE;
        }

        if (pAttr->productId == PRODUCT_ID_511) {
            pAttr->supportDecoders       |= (1<<STD_AVC);
            if ( (pAttr->hwConfigDef0>>16)&1 ) {
                pAttr->supportBackbone = TRUE;
            }
        }

        pAttr->support2AlignScaler   = (BOOL)((pAttr->hwConfigDef0>>23)&1);
        pAttr->supportCommandQueue   = TRUE;

        pAttr->supportFBCBWOptimization = (BOOL)((pAttr->hwConfigDef1>>15)&0x01);
        pAttr->supportNewTimer          = (BOOL)((pAttr->hwConfigDef1>>27)&0x01);
        if (pAttr->productId == PRODUCT_ID_520)
            pAttr->supportWTL        = FALSE;
        else
            pAttr->supportWTL        = TRUE;

        pAttr->supportTiled2Linear   = FALSE;
        pAttr->supportMapTypes       = FALSE;
        pAttr->support128bitBus      = TRUE;
        pAttr->supportThumbnailMode  = TRUE;
        pAttr->supportEndianMask     = (Uint32)((1<<VDI_LITTLE_ENDIAN) | (1<<VDI_BIG_ENDIAN) | (1<<VDI_32BIT_LITTLE_ENDIAN) | (1<<VDI_32BIT_BIG_ENDIAN) | (0xffffUL<<16));
        pAttr->supportBitstreamMode  = (1<<BS_MODE_INTERRUPT) | (1<<BS_MODE_PIC_END);
        pAttr->framebufferCacheType  = FramebufCacheNone;
        pAttr->bitstreamBufferMargin = 0;
        pAttr->numberOfVCores        = MAX_NUM_VCORE;
        pAttr->numberOfMemProtectRgns = 10;
    }

    return ret;
}

RetCode Wave5VpuGetVersion(Uint32 coreIdx, Uint32* versionInfo, Uint32* revision)
{
    Uint32          regVal;

    VpuWriteReg(coreIdx, W5_QUERY_OPTION, GET_VPU_INFO);
    VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 1);
    VpuWriteReg(coreIdx, W5_COMMAND, W5_QUERY);
    VpuWriteReg(coreIdx, W5_VPU_HOST_INT_REQ, 1);
    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        VLOG(ERR, "Wave5VpuGetVersion timeout\n");
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(coreIdx, W5_RET_SUCCESS) == FALSE) {
        VLOG(ERR, "Wave5VpuGetVersion FALSE\n");
        return RETCODE_QUERY_FAILURE;
    }

    regVal = VpuReadReg(coreIdx, W5_RET_FW_VERSION);
    if (versionInfo != NULL) {
        *versionInfo = 0;
    }
    if (revision != NULL) {
        *revision    = regVal;
    }

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuGetProductInfo(Uint32 coreIdx, ProductInfo *productInfo)
{
    /* GET FIRMWARE&HARDWARE INFORMATION */
    VpuWriteReg(coreIdx, W5_QUERY_OPTION, GET_VPU_INFO);
    VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 1);
    VpuWriteReg(coreIdx, W5_COMMAND, W5_QUERY);
    VpuWriteReg(coreIdx, W5_VPU_HOST_INT_REQ, 1);
    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        VLOG(ERR, "Wave5VpuGetProductInfo timeout\n");
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(coreIdx, W5_RET_SUCCESS) == FALSE) {
        VLOG(ERR, "Wave5VpuGetProductInfo FALSE\n");
        return RETCODE_QUERY_FAILURE;
    }

    productInfo->fwVersion      = VpuReadReg(coreIdx, W5_RET_FW_VERSION);
    productInfo->productName    = VpuReadReg(coreIdx, W5_RET_PRODUCT_NAME);
    productInfo->productVersion = VpuReadReg(coreIdx, W5_RET_PRODUCT_VERSION);
    productInfo->customerId     = VpuReadReg(coreIdx, W5_RET_CUSTOMER_ID);
    productInfo->stdDef0        = VpuReadReg(coreIdx, W5_RET_STD_DEF0);
    productInfo->stdDef1        = VpuReadReg(coreIdx, W5_RET_STD_DEF1);
    productInfo->confFeature    = VpuReadReg(coreIdx, W5_RET_CONF_FEATURE);
    productInfo->configDate     = VpuReadReg(coreIdx, W5_RET_CONF_DATE);
    productInfo->configRevision = VpuReadReg(coreIdx, W5_RET_CONF_REVISION);
    productInfo->configType     = VpuReadReg(coreIdx, W5_RET_CONF_TYPE);

    productInfo->configVcore[0]  = 0;
    productInfo->configVcore[1]  = 0;
    productInfo->configVcore[2]  = 0;
    productInfo->configVcore[3]  = 0;

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuInit(Uint32 coreIdx, void* firmware, Uint32 size)
{
    vpu_buffer_t    vb;
    PhysicalAddress codeBase, tempBase;
    PhysicalAddress taskBufBase;
    Uint32          codeSize, tempSize;
    Uint32          i, regVal, remapSize;
    Uint32          hwOption    = 0;
    RetCode         ret = RETCODE_SUCCESS;

    vdi_get_common_memory(coreIdx, &vb);

    codeBase  = vb.phys_addr;
    /* ALIGN TO 4KB */
    codeSize = (WAVE5_MAX_CODE_BUF_SIZE&~0xfff);
    if (codeSize < size*2) {
        return RETCODE_INSUFFICIENT_RESOURCE;
    }

    tempBase = vb.phys_addr + WAVE5_TEMPBUF_OFFSET;
    tempSize = WAVE5_TEMPBUF_SIZE;

    VLOG(INFO, "\nVPU INIT Start!!!\n");

    VpuWriteMem(coreIdx, codeBase, (unsigned char*)firmware, size*2, VDI_128BIT_LITTLE_ENDIAN);
    vdi_set_bit_firmware_to_pm(coreIdx, (Uint16*)firmware);

    regVal = 0;
    VpuWriteReg(coreIdx, W5_PO_CONF, regVal);

	/* clear registers */

	for (i=W5_CMD_REG_BASE; i<W5_CMD_REG_END; i+=4)
	{
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
		if (i == W5_SW_UART_STATUS)
			continue;
#endif
		VpuWriteReg(coreIdx, i, 0x00);
	}

    /* remap page size */
    remapSize = (codeSize >> 12) &0x1ff;
    regVal = 0x80000000 | (WAVE5_AXI_ID<<20) | (0 << 16) | (W5_REMAP_CODE_INDEX<<12) | (1<<11) | remapSize;
    VpuWriteReg(coreIdx, W5_VPU_REMAP_CTRL,     regVal);
    VpuWriteReg(coreIdx, W5_VPU_REMAP_VADDR,    0x00000000);    /* DO NOT CHANGE! */
    VpuWriteReg(coreIdx, W5_VPU_REMAP_PADDR,    codeBase);
    VpuWriteReg(coreIdx, W5_ADDR_CODE_BASE,     codeBase);
    VpuWriteReg(coreIdx, W5_CODE_SIZE,          codeSize);
    VpuWriteReg(coreIdx, W5_CODE_PARAM,         (WAVE5_AXI_ID<<4) | 0);
    VpuWriteReg(coreIdx, W5_ADDR_TEMP_BASE,     tempBase);
    VpuWriteReg(coreIdx, W5_TEMP_SIZE,          tempSize);
    VpuWriteReg(coreIdx, W5_TIMEOUT_CNT, 0xffff);

    VpuWriteReg(coreIdx, W5_HW_OPTION, hwOption);

    /* Interrupt */
    // decoder
    regVal = (1<<W5_INT_INIT_SEQ);
    regVal |= (1<<W5_INT_DEC_PIC);
    regVal |= (1<<W5_INT_BSBUF_EMPTY);
    regVal |= (1<<W5_INT_DEC_INSUFFICIENT_VLC_BUFFER);

    VpuWriteReg(coreIdx, W5_VPU_VINT_ENABLE,  regVal);

    VpuWriteReg(coreIdx, W5_CMD_INIT_NUM_TASK_BUF, COMMAND_QUEUE_DEPTH);
    VpuWriteReg(coreIdx, W5_CMD_INIT_TASK_BUF_SIZE, ONE_PARAMBUF_SIZE_FOR_CQ);

    vdi_get_common_memory(coreIdx, &vb);
    for (i = 0; i < COMMAND_QUEUE_DEPTH; i++) {
        taskBufBase = vb.phys_addr + WAVE5_TASK_BUF_OFFSET + (i*ONE_PARAMBUF_SIZE_FOR_CQ);
        VpuWriteReg(coreIdx, W5_CMD_INIT_ADDR_TASK_BUF0 + (i*4), taskBufBase);
    }

    if (vdi_get_sram_memory(coreIdx, &vb) < 0)  // get SRAM base/size
        return RETCODE_INSUFFICIENT_RESOURCE;

    VpuWriteReg(coreIdx, W5_ADDR_SEC_AXI, vb.phys_addr);
    VpuWriteReg(coreIdx, W5_SEC_AXI_SIZE, vb.size);
    VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 1);
    VpuWriteReg(coreIdx, W5_COMMAND, W5_INIT_VPU);
    VpuWriteReg(coreIdx, W5_VPU_REMAP_CORE_START, 1);
    //vdi_log(coreIdx, 1, 0);
    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        VLOG(ERR, "VPU init(W5_VPU_REMAP_CORE_START) timeout\n");
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }
    //vdi_log(coreIdx, 1, 0);

    regVal = VpuReadReg(coreIdx, W5_RET_SUCCESS);
    if (regVal == 0) {
        Uint32      reasonCode = VpuReadReg(coreIdx, W5_RET_FAIL_REASON);
        VLOG(ERR, "VPU init(W5_RET_SUCCESS) failed(%d) REASON CODE(%08x)\n", regVal, reasonCode);
        return RETCODE_FAILURE;
    }
    ret = SetupWave5Properties(coreIdx);
    return ret;
}

RetCode Wave5VpuBuildUpDecParam(CodecInst* instance, DecOpenParam* param)
{
    RetCode     ret = RETCODE_SUCCESS;
    DecInfo*    pDecInfo;
    VpuAttr*    pAttr = &g_VpuCoreAttributes[instance->coreIdx];
    Uint32      bsEndian = 0;
    vpu_buffer_t vb;
    pDecInfo    = VPU_HANDLE_TO_DECINFO(instance);

    pDecInfo->streamRdPtrRegAddr      = W5_RET_DEC_BS_RD_PTR;
    pDecInfo->streamWrPtrRegAddr      = W5_BS_WR_PTR;
    pDecInfo->frameDisplayFlagRegAddr = W5_RET_DEC_DISP_IDC;
    pDecInfo->currentPC               = W5_VCPU_CUR_PC;
    pDecInfo->busyFlagAddr            = W5_VPU_BUSY_STATUS;
    if ((pAttr->supportDecoders&(1<<param->bitstreamFormat)) == 0)
        return RETCODE_NOT_SUPPORTED_FEATURE;
    switch (param->bitstreamFormat) {
    case STD_HEVC:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_HEVC;
        break;
    case STD_VP9:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_VP9;
        break;
    case STD_AVS2:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_AVS2;
        break;
    case STD_AVC:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_AVC;
        break;
    case STD_SVAC:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_SVAC;
        break;
    default:
        return RETCODE_NOT_SUPPORTED_FEATURE;
    }

    pDecInfo->scaleWidth              = 0;
    pDecInfo->scaleHeight             = 0;

    pDecInfo->targetSubLayerId       = (param->bitstreamFormat == STD_AVS2) ? AVS2_MAX_SUB_LAYER_ID : HEVC_MAX_SUB_LAYER_ID;

    if (param->vbWork.size > 0) {
        pDecInfo->vbWork = param->vbWork;
        pDecInfo->workBufferAllocExt = TRUE;
        vdi_attach_dma_memory(instance->coreIdx, &param->vbWork);
    }
    else {
        if (instance->productId == PRODUCT_ID_512) {
            pDecInfo->vbWork.size       = WAVE512DEC_WORKBUF_SIZE;
        }
        else if (instance->productId == PRODUCT_ID_515) {
            pDecInfo->vbWork.size       = WAVE515DEC_WORKBUF_SIZE;
        }
        else if (instance->productId == PRODUCT_ID_525) {
            if (param->bitstreamFormat == STD_SVAC)
                pDecInfo->vbWork.size       = WAVE525_SVAC_DEC_WORKBUF_SIZE;
            else
                pDecInfo->vbWork.size       = (Uint32)WAVE525DEC_WORKBUF_SIZE;
        }
        else if (instance->productId == PRODUCT_ID_521) {
            pDecInfo->vbWork.size       = (Uint32)WAVE521DEC_WORKBUF_SIZE;  // FIX ME
        }
        else if (instance->productId == PRODUCT_ID_511) {
            pDecInfo->vbWork.size       = (Uint32)WAVE521DEC_WORKBUF_SIZE;  // FIX ME
        }
        pDecInfo->workBufferAllocExt    = FALSE;
        APIDPRINT("ALLOC MEM - WORK\n");
        if (vdi_allocate_dma_memory(instance->coreIdx, &pDecInfo->vbWork) < 0) {
            pDecInfo->vbWork.base       = 0;
            pDecInfo->vbWork.phys_addr  = 0;
            pDecInfo->vbWork.size       = 0;
            pDecInfo->vbWork.virt_addr  = 0;
            return RETCODE_INSUFFICIENT_RESOURCE;
        }
    }

    VpuWriteReg(instance->coreIdx, W5_CMD_CUSTOM_DISABLE_FLAG, pDecInfo->customDisableFlag);
    vdi_get_common_memory(instance->coreIdx, &vb);
    pDecInfo->vbTemp.phys_addr = vb.phys_addr + WAVE5_TEMPBUF_OFFSET;
    pDecInfo->vbTemp.size      = WAVE5_TEMPBUF_SIZE;

    vdi_clear_memory(instance->coreIdx, pDecInfo->vbWork.phys_addr, pDecInfo->vbWork.size, 0);

    VpuWriteReg(instance->coreIdx, W5_ADDR_WORK_BASE, pDecInfo->vbWork.phys_addr);
    VpuWriteReg(instance->coreIdx, W5_WORK_SIZE,      pDecInfo->vbWork.size);

    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_BS_START_ADDR, pDecInfo->streamBufStartAddr);
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_BS_SIZE, pDecInfo->streamBufSize);

    bsEndian = vdi_convert_endian(instance->coreIdx, param->streamEndian);
    /* NOTE: When endian mode is 0, SDMA reads MSB first */
    bsEndian = (~bsEndian&VDI_128BIT_ENDIAN_MASK);
    VpuWriteReg(instance->coreIdx, W5_CMD_BS_PARAM, bsEndian);

    VpuWriteReg(instance->coreIdx, W5_VPU_BUSY_STATUS, 1);
    VpuWriteReg(instance->coreIdx, W5_RET_SUCCESS, 0);	//for debug

    Wave5BitIssueCommand(instance, W5_CREATE_INSTANCE);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {   // Check QUEUE_DONE
        if (instance->loggingEnable)
            vdi_log(instance->coreIdx, W5_CREATE_INSTANCE, 2);
        vdi_free_dma_memory(instance->coreIdx, &pDecInfo->vbWork);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {           // FAILED for adding into VCPU QUEUE
        vdi_free_dma_memory(instance->coreIdx, &pDecInfo->vbWork);
        ret = RETCODE_FAILURE;
    }

    return ret;
}

RetCode Wave5VpuDecInitSeq(CodecInst* instance)
{
    RetCode     ret = RETCODE_SUCCESS;
    DecInfo*    pDecInfo;
    Uint32      cmdOption = INIT_SEQ_NORMAL, bsOption;
    Uint32      regVal;

    if (instance == NULL)
        return RETCODE_INVALID_PARAM;

    pDecInfo = VPU_HANDLE_TO_DECINFO(instance);
    if (pDecInfo->thumbnailMode)
        cmdOption = INIT_SEQ_W_THUMBNAIL;


    /* Set attributes of bitstream buffer controller */
    bsOption = 0;
    switch (pDecInfo->openParam.bitstreamMode) {
    case BS_MODE_INTERRUPT:
        if(pDecInfo->seqInitEscape == TRUE)
            bsOption = BSOPTION_ENABLE_EXPLICIT_END;
        break;
    case BS_MODE_PIC_END:
        bsOption = BSOPTION_ENABLE_EXPLICIT_END;
        break;
    default:
        return RETCODE_INVALID_PARAM;
    }

    if (pDecInfo->streamEndflag == 1)
        bsOption = 3;

    VpuWriteReg(instance->coreIdx, W5_BS_RD_PTR, pDecInfo->streamRdPtr);
    VpuWriteReg(instance->coreIdx, W5_BS_WR_PTR, pDecInfo->streamWrPtr);

    VpuWriteReg(instance->coreIdx, W5_BS_OPTION, (1UL<<31) | bsOption);

    VpuWriteReg(instance->coreIdx, W5_COMMAND_OPTION, cmdOption);
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_USER_MASK, pDecInfo->userDataEnable);

    Wave5BitIssueCommand(instance, W5_INIT_SEQ);

    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {   // Check QUEUE_DONE
        if (instance->loggingEnable)
            vdi_log(instance->coreIdx, W5_INIT_SEQ, 2);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_STATUS);

    pDecInfo->instanceQueueCount = (regVal>>16)&0xff;
    pDecInfo->totalQueueCount    = (regVal & 0xffff);

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {           // FAILED for adding a command into VCPU QUEUE
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if ( regVal == WAVE5_QUEUEING_FAIL)
            ret = RETCODE_QUEUEING_FAILURE;
        else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            ret =  RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            ret =  RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_DEC_VLC_BUF_FULL)
            ret = RETCODE_VLC_BUF_FULL;
        else
            ret = RETCODE_FAILURE;
    }

    return ret;
}

static void GetDecSequenceResult(CodecInst* instance, DecInitialInfo* info)
{
    DecInfo*   pDecInfo   = &instance->CodecInfo->decInfo;
    Uint32     regVal;
    Uint32     profileCompatibilityFlag;
    Uint32     left, right, top, bottom;
    Uint32     progressiveFlag, interlacedFlag, outputBitDepthMinus8, frameMbsOnlyFlag = 0;

    pDecInfo->streamRdPtr = info->rdPtr = ProductVpuDecGetRdPtr(instance);

    pDecInfo->frameDisplayFlag = VpuReadReg(instance->coreIdx, W5_RET_DEC_DISP_IDC);
    /*regVal = VpuReadReg(instance->coreIdx, W4_BS_OPTION);
    pDecInfo->streamEndflag    = (regVal&0x02) ? TRUE : FALSE;*/

    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_SIZE);
    info->picWidth            = ( (regVal >> 16) & 0xffff );
    info->picHeight           = ( regVal & 0xffff );
    info->minFrameBufferCount = VpuReadReg(instance->coreIdx, W5_RET_DEC_NUM_REQUIRED_FB);
    info->frameBufDelay       = VpuReadReg(instance->coreIdx, W5_RET_DEC_NUM_REORDER_DELAY);

    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_CROP_LEFT_RIGHT);
    left   = (regVal >> 16) & 0xffff;
    right  = regVal & 0xffff;
    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_CROP_TOP_BOTTOM);
    top    = (regVal >> 16) & 0xffff;
    bottom = regVal & 0xffff;

    info->picCropRect.left   = left;
    info->picCropRect.right  = info->picWidth - right;
    info->picCropRect.top    = top;
    info->picCropRect.bottom = info->picHeight - bottom;
    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_SEQ_PARAM);
    info->level                  = regVal & 0xff;
    interlacedFlag               = (regVal>>10) & 0x1;
    progressiveFlag              = (regVal>>11) & 0x1;
    profileCompatibilityFlag     = (regVal>>12)&0xff;
    info->profile                = (regVal >> 24)&0x1f;
    info->tier                   = (regVal >> 29)&0x01;
    info->maxSubLayers           = (regVal >> 21)&0x07;
    outputBitDepthMinus8         = (regVal >> 30)&0x03;

    if (instance->codecMode == W_AVC_DEC) {
        frameMbsOnlyFlag             = (regVal >> 8) & 0x01;
        info->constraint_set_flag[0] = (regVal >> 16) & 0x01;
        info->constraint_set_flag[1] = (regVal >> 17) & 0x01;
        info->constraint_set_flag[2] = (regVal >> 18) & 0x01;
        info->constraint_set_flag[3] = (regVal >> 19) & 0x01;
    }

    info->fRateNumerator         = VpuReadReg(instance->coreIdx, W5_RET_DEC_FRAME_RATE_NR);
    info->fRateDenominator       = VpuReadReg(instance->coreIdx, W5_RET_DEC_FRAME_RATE_DR);

    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_COLOR_SAMPLE_INFO);
    info->lumaBitdepth           = (regVal>>0)&0x0f;
    info->chromaBitdepth         = (regVal>>4)&0x0f;
    info->chromaFormatIDC        = (regVal>>8)&0x0f;
    info->aspectRateInfo         = (regVal>>16)&0xff;
    info->isExtSAR               = (info->aspectRateInfo == 255 ? TRUE : FALSE);
    if (info->isExtSAR == TRUE) {
        info->aspectRateInfo     = VpuReadReg(instance->coreIdx, W5_RET_DEC_ASPECT_RATIO);  /* [0:15] - vertical size, [16:31] - horizontal size */
    }
    info->bitRate                = VpuReadReg(instance->coreIdx, W5_RET_DEC_BIT_RATE);

    if ( instance->codecMode == W_HEVC_DEC ) {
        /* Guessing Profile */
        if (info->profile == 0) {
            if ((profileCompatibilityFlag&0x06) == 0x06)        info->profile = 1;      /* Main profile */
            else if ((profileCompatibilityFlag&0x04) == 0x04)   info->profile = 2;      /* Main10 profile */
            else if ((profileCompatibilityFlag&0x08) == 0x08)   info->profile = 3;      /* Main Still Picture profile */
            else                                                info->profile = 1;      /* For old version HM */
        }

        if (progressiveFlag == 1 && interlacedFlag == 0)
            info->interlace = 0;
        else
            info->interlace = 1;
    }
    else if (instance->codecMode == W_AVS2_DEC) {
        if ((info->lumaBitdepth == 10) && (outputBitDepthMinus8 == 2))
            info->outputBitDepth = 10;
        else
            info->outputBitDepth = 8;

        if (progressiveFlag == 1)
            info->interlace = 0;
        else
            info->interlace = 1;
    }
    else if (instance->codecMode == W_AVC_DEC) {
        if (frameMbsOnlyFlag == 1)
            info->interlace = 0;
        else
            info->interlace = 0; // AVC on WAVE5 can't support interlace
    }
    return;
}

RetCode Wave5VpuDecGetSeqInfo(CodecInst* instance, DecInitialInfo* info)
{
    RetCode     ret = RETCODE_SUCCESS;
    Uint32      regVal, i;
    DecInfo*    pDecInfo;

    pDecInfo = VPU_HANDLE_TO_DECINFO(instance);

    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_ADDR_REPORT_BASE, pDecInfo->userDataBufAddr);
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_REPORT_SIZE, pDecInfo->userDataBufSize);
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_REPORT_PARAM, VPU_USER_DATA_ENDIAN&VDI_128BIT_ENDIAN_MASK);

    // Send QUERY cmd
    ret = SendQuery(instance, GET_RESULT);
    if (ret != RETCODE_SUCCESS) {
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal == WAVE5_RESULT_NOT_READY)
            return RETCODE_REPORT_NOT_READY;
        else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_DEC_VLC_BUF_FULL)
            return RETCODE_VLC_BUF_FULL;
        else
            return RETCODE_QUERY_FAILURE;
    }

    if (instance->loggingEnable)
        vdi_log(instance->coreIdx, W5_INIT_SEQ, 0);

    regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_STATUS);

    pDecInfo->instanceQueueCount = (regVal>>16)&0xff;
    pDecInfo->totalQueueCount    = (regVal & 0xffff);

    if (VpuReadReg(instance->coreIdx, W5_RET_DEC_DECODING_SUCCESS) != 1) {
#ifdef SUPPORT_SW_UART
		 ret = RETCODE_FAILURE;
#else
        info->seqInitErrReason = VpuReadReg(instance->coreIdx, W5_RET_DEC_ERR_INFO);
        ret = RETCODE_FAILURE;
#endif
    }
    else {
#ifdef SUPPORT_SW_UART
		info->warnInfo = 0;
#else
        info->warnInfo = VpuReadReg(instance->coreIdx, W5_RET_DEC_WARN_INFO);
#endif
    }


    // Get Sequence Info
    info->userDataSize   = 0;
    info->userDataNum    = 0;
    info->userDataBufFull= 0;
    info->userDataHeader = VpuReadReg(instance->coreIdx, W5_RET_DEC_USERDATA_IDC);
    if (info->userDataHeader != 0) {
        regVal = info->userDataHeader;
        for (i=0; i<32; i++) {
            if (i == 1) {
                if (regVal & (1<<i))
                    info->userDataBufFull = 1;
            }
            else {
                if (regVal & (1<<i))
                    info->userDataNum++;
            }
        }
        info->userDataSize = pDecInfo->userDataBufSize;
    }

    regVal = VpuReadReg(instance->coreIdx, W5_RET_DONE_INSTANCE_INFO);

    if (instance->codecMode == W_SVAC_DEC) {
        regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_SUB_LAYER_INFO);
        info->spatialSvcEnable = (regVal>>16)&0x1;
        info->spatialSvcMode  = (regVal>>17)&0x1;
    }

    GetDecSequenceResult(instance, info);

    return ret;
}

RetCode Wave5VpuDecRegisterFramebuffer(CodecInst* inst, FrameBuffer* fbArr, TiledMapType mapType, Uint32 count)
{
    RetCode      ret = RETCODE_SUCCESS;
    DecInfo*     pDecInfo = &inst->CodecInfo->decInfo;
    DecInitialInfo* sequenceInfo = &inst->CodecInfo->decInfo.initialInfo;
    Int32        q, j, i, remain, idx, svcBLbaseIdx;
    Uint32 mvCount;
    Uint32       k;
    Int32        coreIdx, startNo, endNo;
    Uint32       regVal, cbcrInterleave, nv21, picSize;
    Uint32       endian, yuvFormat = 0;
    Uint32       addrY, addrCb, addrCr;
    Uint32       mvColSize, fbcYTblSize, fbcCTblSize;
    vpu_buffer_t vbBuffer;
    Uint32       stride;
    Uint32       colorFormat  = 0;
    Uint32       outputFormat = 0;
    Uint32       axiID;
    Uint32       initPicWidth = 0, initPicHeight = 0;
    Uint32       lumaSize=0, chromaSize=0;

    Uint32 scalerFlag = 0;
    Uint32 bwbFlag = 0;


    if (pDecInfo->openParam.inPlaceBufferMode != INPLACE_MODE_OFF && mapType == COMPRESSED_FRAME_MAP)
    {
        count = pDecInfo->initialInfo.minFrameBufferCount;
        VLOG(INFO, "Wave5VpuDecRegisterFrameBuffer count=%d \n", pDecInfo->initialInfo.minFrameBufferCount);
    }
    coreIdx        = inst->coreIdx;
    axiID          = pDecInfo->openParam.virtAxiID;
    cbcrInterleave = pDecInfo->openParam.cbcrInterleave;
    nv21           = pDecInfo->openParam.nv21;
    mvColSize      = fbcYTblSize = fbcCTblSize = 0;

    initPicWidth   = pDecInfo->initialInfo.picWidth;
    initPicHeight  = pDecInfo->initialInfo.picHeight;

    if (inst->codecMode == W_SVAC_DEC && mapType == COMPRESSED_FRAME_MAP_SVAC_SVC_BL) {
        initPicWidth   = pDecInfo->initialInfo.picWidth>>1;      // BL size is half as EL
        initPicHeight  = pDecInfo->initialInfo.picHeight>>1;
        svcBLbaseIdx        = count;
    }
    else
        svcBLbaseIdx        = 0;

    if (mapType == COMPRESSED_FRAME_MAP || mapType == COMPRESSED_FRAME_MAP_SVAC_SVC_BL) {
        cbcrInterleave = 0;
        nv21           = 0;

        if (inst->codecMode == W_HEVC_DEC) {
            mvColSize          = WAVE5_DEC_HEVC_MVCOL_BUF_SIZE(initPicWidth, initPicHeight);
        }
        else if(inst->codecMode == W_VP9_DEC) {
            mvColSize          = WAVE5_DEC_VP9_MVCOL_BUF_SIZE(initPicWidth, initPicHeight);
        }
        else if(inst->codecMode == W_AVS2_DEC) {
            mvColSize          = WAVE5_DEC_AVS2_MVCOL_BUF_SIZE(initPicWidth, initPicHeight);
        }
        else if (inst->codecMode == W_SVAC_DEC) {
            mvColSize          = 0;    // mvColbuffer included in WorkBuffer due to sequence change.
        }
        else if (inst->codecMode == W_AVC_DEC) {
            mvColSize          = WAVE5_DEC_AVC_MVCOL_BUF_SIZE(initPicWidth, initPicHeight);
        }
        else {
            /* Unknown codec */
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
        // for calculating inplace ringbuffer end address.
        if (pDecInfo->openParam.inPlaceBufferMode != INPLACE_MODE_OFF && mapType == COMPRESSED_FRAME_MAP && (inst->codecMode == W_HEVC_DEC || inst->codecMode == W_AVC_DEC)) {
            lumaSize  = CalcLumaSize(inst->productId, fbArr[0].stride, fbArr[0].height, fbArr[0].format, fbArr[0].cbcrInterleave, fbArr[0].mapType, NULL);
            chromaSize= CalcChromaSize(inst->productId, fbArr[0].stride, fbArr[0].height, fbArr[0].format, fbArr[0].cbcrInterleave, fbArr[0].mapType, NULL);
            chromaSize *= 2;
        }
        mvColSize          = VPU_ALIGN16(mvColSize);
        vbBuffer.phys_addr = 0;
        if (inst->codecMode == W_HEVC_DEC || inst->codecMode == W_AVS2_DEC || inst->codecMode == W_VP9_DEC || inst->codecMode == W_AVC_DEC) {
            vbBuffer.size      = ((mvColSize+4095)&~4095)+4096;   /* 4096 is a margin */
            mvCount = count;

            APIDPRINT("ALLOC MEM - MV\n");
            for (k=0  ; k<mvCount ; k++) {
                if ( pDecInfo->vbMV[k].size == 0) {
                    if (vdi_allocate_dma_memory(inst->coreIdx, &vbBuffer) < 0)
                        return RETCODE_INSUFFICIENT_RESOURCE;
                    pDecInfo->vbMV[k] = vbBuffer;
                }
            }
        }

        //VP9 Decoded size : 64 aligned.
        if (inst->codecMode == W_HEVC_DEC){
            fbcYTblSize        = WAVE5_FBC_LUMA_TABLE_SIZE(initPicWidth, initPicHeight);
        }
        else if (inst->codecMode == W_VP9_DEC) {
            fbcYTblSize        = WAVE5_FBC_LUMA_TABLE_SIZE(VPU_ALIGN64(initPicWidth), VPU_ALIGN64(initPicHeight));
        }
        else if (inst->codecMode == W_AVS2_DEC) {
            fbcYTblSize        = WAVE5_FBC_LUMA_TABLE_SIZE(initPicWidth, initPicHeight);
        }
        else if (inst->codecMode == W_SVAC_DEC) {
            fbcYTblSize        = WAVE5_FBC_LUMA_TABLE_SIZE(VPU_ALIGN128(initPicWidth), VPU_ALIGN128(initPicHeight));
        }
        else if (inst->codecMode == W_AVC_DEC){
            fbcYTblSize        = WAVE5_FBC_LUMA_TABLE_SIZE(initPicWidth, initPicHeight);
        }
        else {
            /* Unknown codec */
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }

        fbcYTblSize        = VPU_ALIGN16(fbcYTblSize);
        vbBuffer.phys_addr = 0;
        vbBuffer.size      = ((fbcYTblSize+4095)&~4095)+4096;
        APIDPRINT("ALLOC MEM - FBC Y TBL\n");
        for (k=0  ; k<count ; k++) {
            if ( pDecInfo->vbFbcYTbl[k+svcBLbaseIdx].size == 0) {
                if (vdi_allocate_dma_memory(inst->coreIdx, &vbBuffer) < 0)
                    return RETCODE_INSUFFICIENT_RESOURCE;
                pDecInfo->vbFbcYTbl[k+svcBLbaseIdx] = vbBuffer;
            }
        }

        if (inst->codecMode == W_HEVC_DEC) {
            fbcCTblSize        = WAVE5_FBC_CHROMA_TABLE_SIZE(initPicWidth, initPicHeight);
        }
        else if (inst->codecMode == W_VP9_DEC) {
            fbcCTblSize        = WAVE5_FBC_CHROMA_TABLE_SIZE(VPU_ALIGN64(initPicWidth), VPU_ALIGN64(initPicHeight));
        }
        else if (inst->codecMode == W_AVS2_DEC) {
            fbcCTblSize        = WAVE5_FBC_CHROMA_TABLE_SIZE(initPicWidth, initPicHeight);
        }
        else if (inst->codecMode == W_SVAC_DEC) {
            fbcCTblSize        = WAVE5_FBC_CHROMA_TABLE_SIZE(VPU_ALIGN64(initPicWidth), VPU_ALIGN64(initPicHeight));
        }
        else if (inst->codecMode == W_AVC_DEC) {
            fbcCTblSize        = WAVE5_FBC_CHROMA_TABLE_SIZE(initPicWidth, initPicHeight);
        }
        else {
            /* Unknown codec */
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }

        fbcCTblSize        = VPU_ALIGN16(fbcCTblSize);
        vbBuffer.phys_addr = 0;
        vbBuffer.size      = ((fbcCTblSize+4095)&~4095)+4096;
        APIDPRINT("ALLOC MEM - FBC C TBL\n");
        for (k=0  ; k<count ; k++) {
            if ( pDecInfo->vbFbcCTbl[k+svcBLbaseIdx].size == 0) {
                if (vdi_allocate_dma_memory(inst->coreIdx, &vbBuffer) < 0)
                    return RETCODE_INSUFFICIENT_RESOURCE;
                pDecInfo->vbFbcCTbl[k+svcBLbaseIdx] = vbBuffer;
            }
        }
        picSize = (initPicWidth<<16)|(initPicHeight);
    }
    else
    {
        picSize = (initPicWidth<<16)|(initPicHeight);
        pDecInfo->scalerEnable = TRUE;
        pDecInfo->scaleWidth  = (pDecInfo->scaleWidth == 0)  ? initPicWidth  : pDecInfo->scaleWidth;
        pDecInfo->scaleHeight = (pDecInfo->scaleHeight == 0) ? initPicHeight : pDecInfo->scaleHeight;
        if (pDecInfo->scalerEnable == TRUE) {
            picSize = (pDecInfo->scaleWidth << 16) | (pDecInfo->scaleHeight);
        }
    }
    endian = vdi_convert_endian(coreIdx, fbArr[0].endian);

    VpuWriteReg(coreIdx, W5_PIC_SIZE, picSize);

    VpuWriteReg(coreIdx, W5_SET_FB_INPLACE_MODE, pDecInfo->openParam.inPlaceBufferMode | (pDecInfo->openParam.maxVerticalMV<<16));

    yuvFormat = 0; /* YUV420 8bit */
    if (mapType == LINEAR_FRAME_MAP) {
        BOOL   justified = WTL_RIGHT_JUSTIFIED;
        Uint32 formatNo  = WTL_PIXEL_8BIT;
        switch (pDecInfo->wtlFormat) {
        case FORMAT_420_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_MSB:
            justified = WTL_RIGHT_JUSTIFIED;
            formatNo  = WTL_PIXEL_16BIT;
            break;
        case FORMAT_420_P10_16BIT_LSB:
        case FORMAT_422_P10_16BIT_LSB:
            justified = WTL_LEFT_JUSTIFIED;
            formatNo  = WTL_PIXEL_16BIT;
            break;
        case FORMAT_420_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_MSB:
            justified = WTL_RIGHT_JUSTIFIED;
            formatNo  = WTL_PIXEL_32BIT;
            break;
        case FORMAT_420_P10_32BIT_LSB:
        case FORMAT_422_P10_32BIT_LSB:
            justified = WTL_LEFT_JUSTIFIED;
            formatNo  = WTL_PIXEL_32BIT;
            break;
        default:
            break;
        }
        yuvFormat = justified<<2 | formatNo;
    }

    stride = fbArr[0].stride;
    if (mapType == COMPRESSED_FRAME_MAP) {
        if ( pDecInfo->chFbcFrameIdx != -1 )
            stride = fbArr[pDecInfo->chFbcFrameIdx].stride;
    } else {
        if ( pDecInfo->chBwbFrameIdx != -1 )
            stride = fbArr[pDecInfo->chBwbFrameIdx].stride;
    }

    if (mapType == LINEAR_FRAME_MAP) {
        scalerFlag = pDecInfo->scalerEnable;
        switch (pDecInfo->wtlFormat) {
        case FORMAT_422:
        case FORMAT_422_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_LSB:
        case FORMAT_422_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_LSB:
            colorFormat   = 1;
            outputFormat  = 0;
            outputFormat |= (nv21 << 1);
            outputFormat |= (cbcrInterleave << 0);
            break;
        default:
            colorFormat   = 0;
            outputFormat  = 0;
            outputFormat |= (nv21 << 1);
            outputFormat |= (cbcrInterleave << 0);
            break;
        }
    }


    if (mapType == LINEAR_FRAME_MAP) {
        bwbFlag = 1;
    }
    regVal =
        (scalerFlag << 29) |
        (bwbFlag << 28)   |
        (axiID << 24)                           |
        (1<< 23)                                |   /* PIXEL ORDER in 128bit. first pixel in low address */
        (yuvFormat     << 20)                   |
        (colorFormat  << 19)                    |
        (outputFormat << 16)                    |
        (stride);


    VpuWriteReg(coreIdx, W5_COMMON_PIC_INFO, regVal);

    if (mapType == LINEAR_FRAME_MAP) {
        VpuWriteReg(coreIdx, W5_SFB_OPTION, (1<<4 | 1<<3)); // set SETUP_DONE immediately to indicate no linear buffer when using inplace mode.
        VpuWriteReg(coreIdx, W5_SET_FB_NUM, 0);
        Wave5BitIssueCommand(inst, W5_SET_FB);
        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
    }

    remain = count;
    q      = (remain+7)/8;
    idx    = 0;

    for (j=0; j<q; j++) {
        regVal = (endian<<16) | (j==q-1)<<4 | ((j==0)<<3);//lint !e514
        regVal |= (pDecInfo->openParam.bwOptimization<<26);
        regVal |= (pDecInfo->initialInfo.spatialSvcEnable == TRUE ) ? (mapType == COMPRESSED_FRAME_MAP_SVAC_SVC_BL ? 0 : 1<<27) : 0 ;
        regVal |= (pDecInfo->initialInfo.spatialSvcEnable<<28);
        VpuWriteReg(coreIdx, W5_SFB_OPTION, regVal);
        startNo = j*8;
        endNo   = startNo + (remain>=8 ? 8 : remain) - 1;

        VpuWriteReg(coreIdx, W5_SET_FB_NUM, (startNo<<8)|endNo);

        for (i=0; i<8 && i<remain; i++) {
            if (mapType == LINEAR_FRAME_MAP && pDecInfo->openParam.cbcrOrder == CBCR_ORDER_REVERSED) {
                addrY  = fbArr[i+startNo].bufY;
                addrCb = fbArr[i+startNo].bufCr;
                addrCr = fbArr[i+startNo].bufCb;
            }
            else {
                addrY  = fbArr[i+startNo].bufY;
                addrCb = fbArr[i+startNo].bufCb;
                addrCr = fbArr[i+startNo].bufCr;
            }
            VpuWriteReg(coreIdx, W5_ADDR_LUMA_BASE0  + (i<<4), addrY);
            VpuWriteReg(coreIdx, W5_ADDR_CB_BASE0    + (i<<4), addrCb);
            if ((i+startNo== 0) && (pDecInfo->openParam.inPlaceBufferMode == INPLACE_MODE_SHORTTERM || pDecInfo->openParam.inPlaceBufferMode == INPLACE_MODE_LONGTERM)) {
                VpuWriteReg(coreIdx, W5_ADDR_LUMA_END0, addrY + lumaSize-1);
                VpuWriteReg(coreIdx, W5_ADDR_CHROMA_END0, addrCb + chromaSize-1);
            }

            if ((i+startNo== 1) && (pDecInfo->openParam.inPlaceBufferMode == INPLACE_MODE_LONGTERM)) {
                VpuWriteReg(coreIdx, W5_ADDR_LUMA_END1, addrY + lumaSize-1);
                VpuWriteReg(coreIdx, W5_ADDR_CHROMA_END1, addrCb + chromaSize-1);
            }
            APIDPRINT("REGISTER FB[%02d] Y(0x%08x), Cb(0x%08x) ", i, addrY, addrCb);
            if (mapType == COMPRESSED_FRAME_MAP || mapType == COMPRESSED_FRAME_MAP_SVAC_SVC_BL) {
                VpuWriteReg(coreIdx, W5_ADDR_FBC_Y_OFFSET0 + (i<<4), pDecInfo->vbFbcYTbl[idx+svcBLbaseIdx].phys_addr); /* Luma FBC offset table */
                VpuWriteReg(coreIdx, W5_ADDR_FBC_C_OFFSET0 + (i<<4), pDecInfo->vbFbcCTbl[idx+svcBLbaseIdx].phys_addr);        /* Chroma FBC offset table */
                VpuWriteReg(coreIdx, W5_ADDR_MV_COL0  + (i<<2), pDecInfo->vbMV[idx+svcBLbaseIdx].phys_addr);
                APIDPRINT("Yo(0x%08x) Co(0x%08x), Mv(0x%08x)\n",
                    pDecInfo->vbFbcYTbl[idx].phys_addr,
                    pDecInfo->vbFbcCTbl[idx].phys_addr,
                    pDecInfo->vbMV[idx].phys_addr);
            }
            else {
                VpuWriteReg(coreIdx, W5_ADDR_CR_BASE0 + (i<<4), addrCr);
                VpuWriteReg(coreIdx, W5_ADDR_FBC_C_OFFSET0 + (i<<4), 0);
                VpuWriteReg(coreIdx, W5_ADDR_MV_COL0  + (i<<2), 0);
                APIDPRINT("Cr(0x%08x)\n", addrCr);
            }
            idx++;
        }
        remain -= i;

        Wave5BitIssueCommand(inst, W5_SET_FB);
        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
    }
    regVal = VpuReadReg(coreIdx, W5_RET_SUCCESS);
    if (regVal == 0) {
        return RETCODE_FAILURE;
    }

    if (ConfigSecAXIWave(coreIdx, inst->codecMode,
        &pDecInfo->secAxiInfo, initPicWidth, initPicHeight,
        sequenceInfo->profile, sequenceInfo->level) == 0) {
            return RETCODE_INSUFFICIENT_RESOURCE;
    }

    return ret;
}

RetCode Wave5VpuDecUpdateFramebuffer(CodecInst* inst, FrameBuffer* fbcFb, FrameBuffer* linearFb, Int32 mvIndex, Int32 picWidth, Int32 picHeight)
{
    RetCode         ret = RETCODE_SUCCESS;
    DecInfo*        pDecInfo = &inst->CodecInfo->decInfo;
    DecInitialInfo* sequenceInfo = &inst->CodecInfo->decInfo.initialInfo;
    Int8            fbcIndex, linearIndex;
    Uint32          coreIdx, regVal;
    Uint32          mvColSize, fbcYTblSize, fbcCTblSize;
    Uint32          linearStride, fbcStride;
    vpu_buffer_t*   pvbMv = NULL;
    vpu_buffer_t*   pvbFbcYOffset = NULL;
    vpu_buffer_t*   pvbFbcCOffset = NULL;
    CodStd          codec;
    unsigned long   fbcYoffsetAddr = 0;
    unsigned long   fbcCoffsetAddr = 0;
    coreIdx     = inst->coreIdx;
    linearIndex = (linearFb == NULL) ? -1 : linearFb->myIndex - pDecInfo->numFbsForDecoding;
    fbcIndex    = (fbcFb    == NULL) ? -1 : fbcFb->myIndex;
    mvColSize   = fbcYTblSize = fbcCTblSize = 0;
    codec       = pDecInfo->openParam.bitstreamFormat;
    if (codec != STD_VP9) {
        return RETCODE_NOT_SUPPORTED_FEATURE;
    }

    mvColSize = WAVE5_DEC_VP9_MVCOL_BUF_SIZE(picWidth, picHeight);

    if ((fbcFb != NULL) && (fbcIndex >= 0)) {
        pDecInfo->frameBufPool[fbcIndex] = *fbcFb;
    }
    if ((linearFb != NULL) && (linearIndex >= 0)) {
        pDecInfo->frameBufPool[pDecInfo->numFbsForDecoding + linearIndex] = *linearFb;
    }

    if (mvIndex >= 0) {
        pvbMv = &pDecInfo->vbMV[mvIndex];
        vdi_free_dma_memory(inst->coreIdx, pvbMv);
        pvbMv->size = ((mvColSize+4095)&~4095) + 4096;
        if (vdi_allocate_dma_memory(inst->coreIdx, pvbMv) < 0) {
            return RETCODE_INSUFFICIENT_RESOURCE;
        }
    }

    /* Reallocate FBC offset tables */
    if (codec == STD_HEVC){
        fbcYTblSize = WAVE5_FBC_LUMA_TABLE_SIZE(picWidth, picHeight);
    }
    else if (codec == STD_VP9) {
        //VP9 Decoded size : 64 aligned.
        fbcYTblSize = WAVE5_FBC_LUMA_TABLE_SIZE(VPU_ALIGN64(picWidth), VPU_ALIGN64(picHeight));
    }
    else if (codec == STD_AVS2){
        fbcYTblSize = WAVE5_FBC_LUMA_TABLE_SIZE(picWidth, picHeight);
    }
    else if (codec == STD_AVC){
        fbcYTblSize = WAVE5_FBC_LUMA_TABLE_SIZE(picWidth, picHeight); // FIX ME
    }
    else {
        /* Unknown codec */
        return RETCODE_NOT_SUPPORTED_FEATURE;
    }

    if (fbcIndex >= 0) {
        pvbFbcYOffset = &pDecInfo->vbFbcYTbl[fbcIndex];
        vdi_free_dma_memory(inst->coreIdx, pvbFbcYOffset);
        pvbFbcYOffset->phys_addr = 0;
        pvbFbcYOffset->size      = ((fbcYTblSize+4095)&~4095)+4096;
        if (vdi_allocate_dma_memory(inst->coreIdx, pvbFbcYOffset) < 0) {
            return RETCODE_INSUFFICIENT_RESOURCE;
        }
        fbcYoffsetAddr = pvbFbcYOffset->phys_addr;
    }

    if (codec == STD_HEVC) {
        fbcCTblSize = WAVE5_FBC_CHROMA_TABLE_SIZE(picWidth, picHeight);
    }
    else if (codec == STD_VP9) {
        fbcCTblSize = WAVE5_FBC_CHROMA_TABLE_SIZE(VPU_ALIGN64(picWidth), VPU_ALIGN64(picHeight));
    }
    else if (codec == STD_AVS2) {
        fbcCTblSize = WAVE5_FBC_CHROMA_TABLE_SIZE(picWidth, picHeight);
    }
    else if (codec == STD_AVC) {
        fbcCTblSize = WAVE5_FBC_CHROMA_TABLE_SIZE(picWidth, picHeight); // FIX ME
    }
    else {
        /* Unknown codec */
        return RETCODE_NOT_SUPPORTED_FEATURE;
    }

    if (fbcIndex >= 0) {
        pvbFbcCOffset = &pDecInfo->vbFbcCTbl[fbcIndex];
        vdi_free_dma_memory(inst->coreIdx, pvbFbcCOffset);
        pvbFbcCOffset->phys_addr = 0;
        pvbFbcCOffset->size      = ((fbcCTblSize+4095)&~4095)+4096;
        if (vdi_allocate_dma_memory(inst->coreIdx, pvbFbcCOffset) < 0) {
            return RETCODE_INSUFFICIENT_RESOURCE;
        }
        fbcCoffsetAddr = pvbFbcCOffset->phys_addr;
    }

    linearStride = linearFb == NULL ? 0 : linearFb->stride;
    fbcStride    = fbcFb == NULL ? 0 : fbcFb->stride;
    regVal = linearStride<<16 | fbcStride;
    VpuWriteReg(coreIdx, W5_CMD_SET_FB_STRIDE, regVal);

    regVal = (picWidth<<16) | picHeight;
    if (pDecInfo->scalerEnable == TRUE) {
        regVal = (pDecInfo->scaleWidth << 16) | (pDecInfo->scaleHeight);
    }
    VpuWriteReg(coreIdx, W5_PIC_SIZE, regVal);

    VLOG(INFO, "fbcIndex(%d), linearIndex(%d), mvIndex(%d)\n", fbcIndex, linearIndex, mvIndex);
    regVal = (mvIndex&0xff) << 16 | (linearIndex&0xff) << 8 | (fbcIndex&0xff);
    VpuWriteReg(coreIdx, W5_CMD_SET_FB_INDEX, regVal);

    VpuWriteReg(coreIdx, W5_ADDR_LUMA_BASE,     linearFb == NULL ? 0 : linearFb->bufY);
    VpuWriteReg(coreIdx, W5_ADDR_CB_BASE,       linearFb == NULL ? 0 : linearFb->bufCb);
    VpuWriteReg(coreIdx, W5_ADDR_CR_BASE,       linearFb == NULL ? 0 : linearFb->bufCr);
    VpuWriteReg(coreIdx, W5_ADDR_MV_COL,        pvbMv == NULL ? 0 : pvbMv->phys_addr);
    VpuWriteReg(coreIdx, W5_ADDR_FBC_Y_BASE,    fbcFb == NULL ? 0 : fbcFb->bufY);
    VpuWriteReg(coreIdx, W5_ADDR_FBC_C_BASE,    fbcFb == NULL ? 0 : fbcFb->bufCb);
    VpuWriteReg(coreIdx, W5_ADDR_FBC_Y_OFFSET,  fbcYoffsetAddr);
    VpuWriteReg(coreIdx, W5_ADDR_FBC_C_OFFSET,  fbcCoffsetAddr);
    VpuWriteReg(coreIdx, W5_SFB_OPTION,         1); /* UPDATE FRAMEBUFFER */

    Wave5BitIssueCommand(inst, W5_SET_FB);

    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    regVal = VpuReadReg(coreIdx, W5_RET_SUCCESS);
    if (regVal == 0) {
        return RETCODE_FAILURE;
    }

    if (ConfigSecAXIWave(coreIdx, inst->codecMode,
                         &pDecInfo->secAxiInfo, pDecInfo->initialInfo.picWidth, pDecInfo->initialInfo.picHeight,
                         sequenceInfo->profile, sequenceInfo->level) == 0) {
        return RETCODE_INSUFFICIENT_RESOURCE;
    }

    return ret;
}

RetCode Wave5VpuDecode(CodecInst* instance, DecParam* option)
{
    Uint32      modeOption = DEC_PIC_NORMAL,  bsOption, regVal;
    DecOpenParam*   pOpenParam;
    Int32       forceLatency = -1;
#ifdef FIX_SET_GET_RD_PTR_BUG
#else
	Int32       rdptr_valid = 0;
#endif
    DecInfo*    pDecInfo = &instance->CodecInfo->decInfo;

    pOpenParam = &pDecInfo->openParam;

    if (pDecInfo->thumbnailMode) {
        modeOption = DEC_PIC_W_THUMBNAIL;
    }
    else if (option->skipframeMode) {
        switch (option->skipframeMode) {
        case WAVE_SKIPMODE_NON_IRAP:
            modeOption   = SKIP_NON_IRAP;
            forceLatency = 0;
            break;
        case WAVE_SKIPMODE_NON_REF:
            modeOption = SKIP_NON_REF_PIC;
            break;
        default:
            // skip off
            break;
        }
    }

    if (instance->codecMode == W_SVAC_DEC) {
        switch (option->selSvacLayer) {
            case SEL_SVAC_ALL_LAYER:
                break;
            case SEL_SVAC_BL:
                modeOption |= SKIP_SVAC_EL;
                break;
            case SEL_SVAC_EL:
                modeOption |= SKIP_SVAC_BL;
                break;
            default:
                break;
        }
    }

    if (pDecInfo->targetSubLayerId < (pDecInfo->initialInfo.maxSubLayers-1)) {
        modeOption = SKIP_TEMPORAL_LAYER;
    }
    if (option->craAsBlaFlag == TRUE) {
        modeOption |= (1<<5);
    }

    // set disable reorder
    if (pDecInfo->reorderEnable == FALSE) {
        forceLatency = 0;
    }

    /* Set attributes of bitstream buffer controller */
    bsOption = 0;
    regVal = 0;
    switch (pOpenParam->bitstreamMode) {
    case BS_MODE_INTERRUPT:
        bsOption = 0;
        break;
    case BS_MODE_PIC_END:
        bsOption = BSOPTION_ENABLE_EXPLICIT_END;
        break;
    default:
        return RETCODE_INVALID_PARAM;
    }

    VpuWriteReg(instance->coreIdx, W5_BS_RD_PTR,     pDecInfo->streamRdPtr);
    VpuWriteReg(instance->coreIdx, W5_BS_WR_PTR,     pDecInfo->streamWrPtr);
    if (pDecInfo->streamEndflag == 1)
        bsOption = 3;   // (streamEndFlag<<1) | EXPLICIT_END
#ifdef FIX_SET_GET_RD_PTR_BUG
	if (pOpenParam->bitstreamMode == BS_MODE_PIC_END) {
		bsOption |= (1<<31);
	}
	VpuWriteReg(instance->coreIdx, W5_BS_OPTION, bsOption);
#else
    if (pOpenParam->bitstreamMode == BS_MODE_PIC_END || pDecInfo->rdPtrValidFlag == TRUE)
        rdptr_valid = 1;

    VpuWriteReg(instance->coreIdx, W5_BS_OPTION,  (rdptr_valid<<31) | bsOption);

    pDecInfo->rdPtrValidFlag = FALSE;       // reset rdptrValidFlag.
#endif
    /* Secondary AXI */
    regVal = (pDecInfo->secAxiInfo.u.wave.useBitEnable<<0)    |
             (pDecInfo->secAxiInfo.u.wave.useIpEnable<<9)     |
             (pDecInfo->secAxiInfo.u.wave.useLfRowEnable<<15);
    regVal |= (pDecInfo->secAxiInfo.u.wave.useSclEnable<<5);
    VpuWriteReg(instance->coreIdx, W5_USE_SEC_AXI,  regVal);

    if (FALSE == pDecInfo->customDisableFlag) {
        if (TRUE == pDecInfo->scalerEnable) {
            // Check maximum 1/8 scale-down limitation.
            Uint32   minScaleWidth  = VPU_CEIL(pDecInfo->initialInfo.picWidth>>3, 2);
            Uint32   minScaleHeight = VPU_CEIL(pDecInfo->initialInfo.picHeight>>3, 2);

            if (minScaleWidth == 0)  minScaleWidth  = 8;
            if (minScaleHeight == 0) minScaleHeight = 8;

            if (option->scaleWidth < minScaleWidth || option->scaleHeight < minScaleHeight) {
                return RETCODE_INVALID_PARAM;
            }
            if (option->scaleWidth > VPU_ALIGN8(pDecInfo->initialInfo.picWidth))
                return RETCODE_INVALID_PARAM;

            if (option->scaleHeight > VPU_ALIGN8(pDecInfo->initialInfo.picHeight))
                return RETCODE_INVALID_PARAM;
        }
        VpuWriteReg(instance->coreIdx, W5_CMD_DEC_SCALE_SIZE, (option->scaleWidth << 16) | (option->scaleHeight));
        VpuWriteReg(instance->coreIdx, W5_CMD_ADDR_LINEAR_BUF_Y, option->addrLinearBufY);
        VpuWriteReg(instance->coreIdx, W5_CMD_ADDR_LINEAR_BUF_CB, option->addrLinearBufCb);
        VpuWriteReg(instance->coreIdx, W5_CMD_ADDR_LINEAR_BUF_CR, option->addrLinearBufCr);
        VpuWriteReg(instance->coreIdx, W5_CMD_DEC_LINEAR_BUF_STRIDE, option->linearLumaStride);
        pDecInfo->scaleWidth    = option->scaleWidth;
        pDecInfo->scaleHeight   = option->scaleHeight;
    }
    else {
        if (TRUE == pDecInfo->scalerEnable)
            VpuWriteReg(instance->coreIdx, W5_CMD_DEC_SCALE_SIZE, (pDecInfo->scaleWidth << 16) | (pDecInfo->scaleHeight));
    }

    /* Set attributes of User buffer */
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_USER_MASK, pDecInfo->userDataEnable);

    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_VCORE_LIMIT, 1);

    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_VLC_BUF_BASE, option->addrVlcBuf);
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_VLC_BUF_SIZE, option->vlcBufSize);

    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_TEMPORAL_ID_PLUS1, pDecInfo->targetSubLayerId+1);
    VpuWriteReg(instance->coreIdx, W5_CMD_SEQ_CHANGE_ENABLE_FLAG, pDecInfo->seqChangeMask);
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_FORCE_FB_LATENCY_PLUS1, forceLatency+1);
    VpuWriteReg(instance->coreIdx, W5_COMMAND_OPTION, modeOption);

    Wave5BitIssueCommand(instance, W5_DEC_PIC);

    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {   // Check QUEUE_DONE
        if (instance->loggingEnable)
            vdi_log(instance->coreIdx, W5_DEC_PIC, 2);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_STATUS);

    pDecInfo->instanceQueueCount = (regVal>>16)&0xff;
    pDecInfo->totalQueueCount    = (regVal & 0xffff);

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {           // FAILED for adding a command into VCPU QUEUE
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if ( regVal == WAVE5_QUEUEING_FAIL)
            return RETCODE_QUEUEING_FAILURE;
        else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_DEC_VLC_BUF_FULL)
            return RETCODE_VLC_BUF_FULL;
        else
            return RETCODE_FAILURE;
    }

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuDecGetResult(CodecInst* instance, DecOutputInfo* result)
{
    RetCode     ret = RETCODE_SUCCESS;
    Uint32      regVal, index, nalUnitType;
    DecInfo*    pDecInfo;

    pDecInfo = VPU_HANDLE_TO_DECINFO(instance);

    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_ADDR_REPORT_BASE, pDecInfo->userDataBufAddr);
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_REPORT_SIZE,      pDecInfo->userDataBufSize);
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_REPORT_PARAM,     VPU_USER_DATA_ENDIAN&VDI_128BIT_ENDIAN_MASK);

    // Send QUERY cmd
    result->VLCOutBufBase = 0;
    ret = SendQuery(instance, GET_RESULT);
    if (ret != RETCODE_SUCCESS) {
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal == WAVE5_RESULT_NOT_READY)
            return RETCODE_REPORT_NOT_READY;
        else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_DEC_VLC_BUF_FULL)
            return RETCODE_VLC_BUF_FULL;
        else
            return RETCODE_QUERY_FAILURE;
    }

    if (instance->loggingEnable)
        vdi_log(instance->coreIdx, W5_DEC_PIC, 0);

    regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_STATUS);

    pDecInfo->instanceQueueCount = (regVal>>16)&0xff;
    pDecInfo->totalQueueCount    = (regVal & 0xffff);

    result->decodingSuccess = VpuReadReg(instance->coreIdx, W5_RET_DEC_DECODING_SUCCESS);
#ifdef SUPPORT_SW_UART
#else
    if (result->decodingSuccess == FALSE) {
		result->errorReason = VpuReadReg(instance->coreIdx, W5_RET_DEC_ERR_INFO);
    }
    else {
        result->warnInfo = VpuReadReg(instance->coreIdx, W5_RET_DEC_WARN_INFO);
    }
#endif

    result->decOutputExtData.userDataSize   = 0;
    result->decOutputExtData.userDataNum    = 0;
    result->decOutputExtData.userDataBufFull= 0;
    result->decOutputExtData.userDataHeader = VpuReadReg(instance->coreIdx, W5_RET_DEC_USERDATA_IDC);
    if (result->decOutputExtData.userDataHeader != 0) {
        regVal = result->decOutputExtData.userDataHeader;
        for (index=0; index<32; index++) {
            if (index == 1) {
                if (regVal & (1<<index))
                    result->decOutputExtData.userDataBufFull = 1;
            }
            else {
                if (regVal & (1<<index))
                    result->decOutputExtData.userDataNum++;
            }
        }
        result->decOutputExtData.userDataSize = pDecInfo->userDataBufSize;
    }

    result->decHostCmdTick     = VpuReadReg(instance->coreIdx, W5_RET_DEC_HOST_CMD_TICK);
    result->decSeekStartTick   = VpuReadReg(instance->coreIdx, W5_RET_DEC_SEEK_START_TICK);
    result->decSeekEndTick     = VpuReadReg(instance->coreIdx, W5_RET_DEC_SEEK_END__TICK);
    result->decParseStartTick  = VpuReadReg(instance->coreIdx, W5_RET_DEC_PARSING_START_TICK);
    result->decParseEndTick    = VpuReadReg(instance->coreIdx, W5_RET_DEC_PARSING_END_TICK);
    result->decDecodeStartTick = VpuReadReg(instance->coreIdx, W5_RET_DEC_DECODING_START_TICK);
    result->decDecodeEndTick   = VpuReadReg(instance->coreIdx, W5_RET_DEC_DECODING_ENC_TICK);

    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_TYPE);

    if (instance->codecMode == W_VP9_DEC) {
        if      (regVal&0x01) result->picType = PIC_TYPE_I;
        else if (regVal&0x02) result->picType = PIC_TYPE_P;
        else if (regVal&0x04) result->picType = PIC_TYPE_REPEAT;
        else                  result->picType = PIC_TYPE_MAX;
    }
    else if (instance->codecMode == W_HEVC_DEC) {
        if      (regVal&0x04) result->picType = PIC_TYPE_B;
        else if (regVal&0x02) result->picType = PIC_TYPE_P;
        else if (regVal&0x01) result->picType = PIC_TYPE_I;
        else                  result->picType = PIC_TYPE_MAX;
    }
    else if (instance->codecMode == W_AVC_DEC) {
        if      (regVal&0x04) result->picType = PIC_TYPE_B;
        else if (regVal&0x02) result->picType = PIC_TYPE_P;
        else if (regVal&0x01) result->picType = PIC_TYPE_I;
        else                  result->picType = PIC_TYPE_MAX;
    }
    else if (instance->codecMode == W_SVAC_DEC) {
        if (regVal&0x01)      result->picType = PIC_TYPE_KEY;
        else if(regVal&0x02)  result->picType = PIC_TYPE_INTER;
        else                  result->picType = PIC_TYPE_MAX;
    }
    else {  // AVS2
        switch(regVal&0x07) {
        case 0: result->picType = PIC_TYPE_I;      break;
        case 1: result->picType = PIC_TYPE_P;      break;
        case 2: result->picType = PIC_TYPE_B;      break;
        case 3: result->picType = PIC_TYPE_AVS2_F; break;
        case 4: result->picType = PIC_TYPE_AVS2_S; break;
        case 5: result->picType = PIC_TYPE_AVS2_G; break;
        case 6: result->picType = PIC_TYPE_AVS2_GB;break;
        default:
             result->picType = PIC_TYPE_MAX; break;
        }
    }
    result->outputFlag      = (regVal>>31)&0x1;

    nalUnitType = (regVal & 0x3f0) >> 4;
    if ((nalUnitType == 19 || nalUnitType == 20) && result->picType == PIC_TYPE_I) {
        /* IDR_W_RADL, IDR_N_LP */
        result->picType = PIC_TYPE_IDR;
    }
    result->nalType                   = nalUnitType;
    result->ctuSize                   = 16<<((regVal>>10)&0x3);
    index                             = VpuReadReg(instance->coreIdx, W5_RET_DEC_DISPLAY_INDEX);
    result->indexFrameDisplay         = index;

    result->indexFrameDisplayForTiled = index;
    index                             = VpuReadReg(instance->coreIdx, W5_RET_DEC_DECODED_INDEX);
    result->indexFrameDecoded         = index;
    result->indexFrameDecodedForTiled = index;

	if (instance->codecMode == W_HEVC_DEC) {
		result->h265Info.decodedPOC = -1;
		result->h265Info.displayPOC = -1;
		if (result->indexFrameDecoded >= 0 || result->indexFrameDecoded  == DECODED_IDX_FLAG_SKIP)
			result->h265Info.decodedPOC = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_POC);
		result->h265Info.temporalId = VpuReadReg(instance->coreIdx, W5_RET_DEC_SUB_LAYER_INFO) & 0xff;
	}
	else if (instance->codecMode == W_SVAC_DEC) {
		regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_SUB_LAYER_INFO);
		result->svacInfo.temporalId     = regVal & 0xff;
        result->svacInfo.maxTemporalId  = (regVal >>8) & 0xff;
        result->svacInfo.spatialSvcFlag = (regVal >>16) & 0x1;
		result->svacInfo.spatialSvcMode = (regVal >> 17) & 0x1;
		result->svacInfo.spatialSvcLayer= (regVal >> 18) & 0x1;
	}
	else if (instance->codecMode == W_AVS2_DEC) {
		result->avs2Info.decodedPOI = -1;
		result->avs2Info.displayPOI = -1;
		if (result->indexFrameDecoded >= 0)
			result->avs2Info.decodedPOI = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_POC);

		result->avs2Info.temporalId = VpuReadReg(instance->coreIdx, W5_RET_DEC_SUB_LAYER_INFO) & 0xff;
	}
    else if (instance->codecMode == W_AVC_DEC) {
        // need to implement
    }

    result->sequenceChanged   = VpuReadReg(instance->coreIdx, W5_RET_DEC_NOTIFICATION);

    /*
     * If current picture is the last of the current sequence and sequence-change flag is not 0, then
     * the width and height of the current picture is set to the width and height of the current sequence.
     */
    if (result->sequenceChanged == 0) {
        regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_SIZE);
        result->decPicWidth   = regVal>>16;
        result->decPicHeight  = regVal&0xffff;
    }
    else {
        if (result->indexFrameDecoded < 0) {
            result->decPicWidth   = 0;
            result->decPicHeight  = 0;
        }
        else {
            result->decPicWidth   = pDecInfo->initialInfo.picWidth;
            result->decPicHeight  = pDecInfo->initialInfo.picHeight;
        }
        if ( instance->codecMode == W_VP9_DEC ) {
            if ( result->sequenceChanged & SEQ_CHANGE_INTER_RES_CHANGE) {
                regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_SIZE);
                result->decPicWidth   = regVal>>16;
                result->decPicHeight  = regVal&0xffff;
                result->indexInterFrameDecoded = VpuReadReg(instance->coreIdx, W5_RET_DEC_REALLOC_INDEX);
            }
        }
        osal_memcpy((void*)&pDecInfo->newSeqInfo, (void*)&pDecInfo->initialInfo, sizeof(DecInitialInfo));
        GetDecSequenceResult(instance, &pDecInfo->newSeqInfo);
    }
    result->numOfErrMBs       = VpuReadReg(instance->coreIdx, W5_RET_DEC_ERR_CTB_NUM)>>16;
    result->numOfTotMBs       = VpuReadReg(instance->coreIdx, W5_RET_DEC_ERR_CTB_NUM)&0xffff;
    result->bytePosFrameStart = VpuReadReg(instance->coreIdx, W5_RET_DEC_AU_START_POS);
    result->bytePosFrameEnd   = VpuReadReg(instance->coreIdx, W5_RET_DEC_AU_END_POS);
#ifdef FIX_SET_GET_RD_PTR_BUG
#else
    pDecInfo->prevFrameEndPos = result->bytePosFrameEnd;
#endif

    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_RECOVERY_POINT);
    result->h265RpSei.recoveryPocCnt = regVal & 0xFFFF;            // [15:0]
    result->h265RpSei.exactMatchFlag = (regVal >> 16)&0x01;        // [16]
    result->h265RpSei.brokenLinkFlag = (regVal >> 17)&0x01;        // [17]
    result->h265RpSei.exist =  (regVal >> 18)&0x01;                // [18]
    if(result->h265RpSei.exist == 0) {
        result->h265RpSei.recoveryPocCnt = 0;
        result->h265RpSei.exactMatchFlag = 0;
        result->h265RpSei.brokenLinkFlag = 0;
    }

    if ( result->sequenceChanged  && (instance->codecMode != W_VP9_DEC)) {
        pDecInfo->scaleWidth  = pDecInfo->newSeqInfo.picWidth;
        pDecInfo->scaleHeight = pDecInfo->newSeqInfo.picHeight;
    }

    result->VLCOutBufBase = VpuReadReg(instance->coreIdx, W5_RET_QUERY_VLC_BASE);

    if (FALSE == pDecInfo->customDisableFlag) {
        if (pDecInfo->openParam.wtlEnable == TRUE || pDecInfo->scalerEnable == TRUE) {
            result->dispFrame.bufY   = VpuReadReg(instance->coreIdx, W5_QUERY_LINEAR_Y_BASE);
            result->dispFrame.bufCb  = VpuReadReg(instance->coreIdx, W5_QUERY_LINEAR_CB_BASE);
            result->dispFrame.bufCr  = VpuReadReg(instance->coreIdx, W5_QUERY_LINEAR_CR_BASE);
        }
    }
    if (pDecInfo->openParam.inPlaceBufferMode) {
        result->inplaceBufY     = VpuReadReg(instance->coreIdx, W5_QUERY_INPLACE_Y_BASE);
        result->inplaceBufC     = VpuReadReg(instance->coreIdx, W5_QUERY_INPLACE_C_BASE);
    }
    return RETCODE_SUCCESS;
}

RetCode Wave5VpuDecFlush(CodecInst* instance, FramebufferIndex* framebufferIndexes, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS;

    Wave5BitIssueCommand(instance, W5_FLUSH_INSTANCE);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {
        Uint32 regVal;
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal == WAVE5_VPU_STILL_RUNNING)
            return RETCODE_VPU_STILL_RUNNING;
        else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_DEC_VLC_BUF_FULL)
            return RETCODE_VLC_BUF_FULL;
        else
            return RETCODE_QUERY_FAILURE;
    }

    return ret;

}

RetCode Wave5VpuDecGetVlcInfo(CodecInst* instance, VLCBufInfo* vlcInfo)
{
    RetCode ret = RETCODE_SUCCESS;

    ret = SendQuery(instance, GET_VLC_INFO);

    if (ret != RETCODE_SUCCESS)
    {
        return RETCODE_QUERY_FAILURE;
    }

    vlcInfo->vlcBufBase         = VpuReadReg(instance->coreIdx, W5_RET_DEC_VLC_BASE);
    vlcInfo->requiedVlcBufSize  = VpuReadReg(instance->coreIdx, W5_RET_DEC_REQUIRED_VLC_SIZE);

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuDecUpdateVlcInfo(CodecInst* instance, VLCBufInfo* vlcInfo)
{

    VpuWriteReg(instance->coreIdx, W5_CMD_UPDATE_VLC_BASE, vlcInfo->vlcBufBase);
    VpuWriteReg(instance->coreIdx, W5_CMD_UPDATE_VLC_SIZE, vlcInfo->requiedVlcBufSize);

    Wave5BitIssueCommand(instance, W5_UPDATE_VLC_BUF);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == 0) {
        return RETCODE_FAILURE;
    }

    return RETCODE_SUCCESS;
}


RetCode Wave5VpuReInit(Uint32 coreIdx, void* firmware, Uint32 size)
{
    vpu_buffer_t    vb;
    PhysicalAddress codeBase, tempBase, taskBufBase;
    PhysicalAddress oldCodeBase, tempSize;
    Uint32          codeSize;
    Uint32          regVal, remapSize, i=0;

    vdi_get_common_memory(coreIdx, &vb);

    codeBase  = vb.phys_addr;
    /* ALIGN TO 4KB */
    codeSize = (WAVE5_MAX_CODE_BUF_SIZE&~0xfff);
    if (codeSize < size*2) {
        return RETCODE_INSUFFICIENT_RESOURCE;
    }
    tempBase = vb.phys_addr + WAVE5_TEMPBUF_OFFSET;
    tempSize = WAVE5_TEMPBUF_SIZE;
    oldCodeBase = VpuReadReg(coreIdx, W5_VPU_REMAP_PADDR);

    if (oldCodeBase != codeBase) {
        VpuAttr*    pAttr = &g_VpuCoreAttributes[coreIdx];

        VpuWriteMem(coreIdx, codeBase, (unsigned char*)firmware, size*2, VDI_128BIT_LITTLE_ENDIAN);
        vdi_set_bit_firmware_to_pm(coreIdx, (Uint16*)firmware);

        regVal = 0;
        VpuWriteReg(coreIdx, W5_PO_CONF, regVal);

        if (pAttr->supportBackbone == TRUE) {
            // Step1 : disable request
            vdi_fio_write_register(coreIdx, W5_BACKBONE_GDI_BUS_CTRL, 0x4);

            // Step2 : Waiting for completion of bus transaction
            if (vdi_wait_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_BACKBONE_GDI_BUS_STATUS) == -1) {
                vdi_fio_write_register(coreIdx, W5_BACKBONE_GDI_BUS_CTRL, 0x00);
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            }
        }
        else {
            // Step1 : disable request
            vdi_fio_write_register(coreIdx, W5_GDI_BUS_CTRL, 0x100);

            // Step2 : Waiting for completion of bus transaction
            if (vdi_wait_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_GDI_BUS_STATUS) == -1) {
                vdi_fio_write_register(coreIdx, W5_GDI_BUS_CTRL, 0x00);
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            }
        }

        // Step3 : Waiting for completion of VCPU bus transaction
        if (vdi_wait_vcpu_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_VCPU_STATUS) == -1) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }

        /* Reset All blocks */
        regVal = 0x7ffffff;
        VpuWriteReg(coreIdx, W5_VPU_RESET_REQ, regVal);    // Reset All blocks
        /* Waiting reset done */

        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_RESET_STATUS) == -1) {
            VpuWriteReg(coreIdx, W5_VPU_RESET_REQ, 0);
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }

        VpuWriteReg(coreIdx, W5_VPU_RESET_REQ, 0);
        // Step3 : must clear GDI_BUS_CTRL after done SW_RESET
        if (pAttr->supportBackbone == TRUE) {
            vdi_fio_write_register(coreIdx, W5_BACKBONE_GDI_BUS_CTRL, 0x00);
        }
        else {
            vdi_fio_write_register(coreIdx, W5_GDI_BUS_CTRL, 0x00);
        }

        /* remap page size */
        remapSize = (codeSize >> 12) &0x1ff;
        regVal = 0x80000000 | (WAVE5_AXI_ID<<20) | (W5_REMAP_CODE_INDEX<<12) | (0 << 16) | (1<<11) | remapSize;
        VpuWriteReg(coreIdx, W5_VPU_REMAP_CTRL,     regVal);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_VADDR,    0x00000000);    /* DO NOT CHANGE! */
        VpuWriteReg(coreIdx, W5_VPU_REMAP_PADDR,    codeBase);
        VpuWriteReg(coreIdx, W5_ADDR_CODE_BASE,     codeBase);
        VpuWriteReg(coreIdx, W5_CODE_SIZE,          codeSize);
        VpuWriteReg(coreIdx, W5_CODE_PARAM,         (WAVE5_AXI_ID<<4) | 0);
        VpuWriteReg(coreIdx, W5_ADDR_TEMP_BASE,     tempBase);
        VpuWriteReg(coreIdx, W5_TEMP_SIZE,          tempSize);
        VpuWriteReg(coreIdx, W5_TIMEOUT_CNT,   0);

        VpuWriteReg(coreIdx, W5_HW_OPTION, 0);
        /* Interrupt */
        // encoder
        regVal  = (1<<W5_INT_ENC_SET_PARAM);
        regVal |= (1<<W5_INT_ENC_PIC);
        regVal |= (1<<W5_INT_ENC_LINEBUF_FULL);
        regVal |= (1<<W5_INT_ENC_LOW_LATENCY);
        // decoder
        regVal  = (1<<W5_INT_INIT_SEQ);
        regVal |= (1<<W5_INT_DEC_PIC);
        regVal |= (1<<W5_INT_BSBUF_EMPTY);
        regVal |= (1<<W5_INT_DEC_INSUFFICIENT_VLC_BUFFER);

        VpuWriteReg(coreIdx, W5_VPU_VINT_ENABLE,  regVal);

        VpuWriteReg(coreIdx,W5_CMD_INIT_NUM_TASK_BUF, COMMAND_QUEUE_DEPTH);

        vdi_get_common_memory(coreIdx, &vb);
        for (i = 0; i < COMMAND_QUEUE_DEPTH; i++) {
            taskBufBase = vb.phys_addr + WAVE5_TASK_BUF_OFFSET + (i*ONE_PARAMBUF_SIZE_FOR_CQ);
            VpuWriteReg(coreIdx, W5_CMD_INIT_ADDR_TASK_BUF0 + (i*4), taskBufBase);
        }

        if (vdi_get_sram_memory(coreIdx, &vb) < 0)  // get SRAM base/size
            return RETCODE_INSUFFICIENT_RESOURCE;

        VpuWriteReg(coreIdx, W5_ADDR_SEC_AXI, vb.phys_addr);
        VpuWriteReg(coreIdx, W5_SEC_AXI_SIZE, vb.size);
        VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 1);
        VpuWriteReg(coreIdx, W5_COMMAND, W5_INIT_VPU);
        VpuWriteReg(coreIdx, W5_VPU_HOST_INT_REQ, 1);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_CORE_START, 1);

        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }

        regVal = VpuReadReg(coreIdx, W5_RET_SUCCESS);
        if (regVal == 0)
            return RETCODE_FAILURE;

    }
    SetupWave5Properties(coreIdx);

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuSleepWake(Uint32 coreIdx, int iSleepWake, const Uint16* code, Uint32 size, BOOL reset)
{
    Uint32          regVal;
    vpu_buffer_t    vb;
    PhysicalAddress codeBase, tempBase;
    Uint32          codeSize, tempSize;
    Uint32          remapSize;

    if(iSleepWake==1)  //saves
    {
        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }

        VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 1);
        VpuWriteReg(coreIdx, W5_COMMAND, W5_SLEEP_VPU);
        VpuWriteReg(coreIdx, W5_VPU_HOST_INT_REQ, 1);

        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1)
        {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
        regVal = VpuReadReg(coreIdx, W5_RET_SUCCESS);
        if (regVal == 0)
        {
            APIDPRINT("SLEEP_VPU failed [0x%x]", VpuReadReg(coreIdx, W5_RET_FAIL_REASON));
            return RETCODE_FAILURE;
        }
    }
    else //restore
    {
        Uint32  hwOption  = 0;
        Uint32  i;
        PhysicalAddress taskBufBase;

        vdi_get_common_memory(coreIdx, &vb);
        codeBase  = vb.phys_addr;
        /* ALIGN TO 4KB */
        codeSize = (WAVE5_MAX_CODE_BUF_SIZE&~0xfff);
        if (codeSize < size*2) {
            return RETCODE_INSUFFICIENT_RESOURCE;
        }

        tempBase = vb.phys_addr + WAVE5_TEMPBUF_OFFSET;
        tempSize = WAVE5_TEMPBUF_SIZE;

        regVal = 0;
        VpuWriteReg(coreIdx, W5_PO_CONF, regVal);

        /* SW_RESET_SAFETY */
        regVal = W5_RST_BLOCK_ALL;
        VpuWriteReg(coreIdx, W5_VPU_RESET_REQ, regVal);    // Reset All blocks

        /* Waiting reset done */
        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_RESET_STATUS) == -1) {
            VpuWriteReg(coreIdx, W5_VPU_RESET_REQ, 0);
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }

        VpuWriteReg(coreIdx, W5_VPU_RESET_REQ, 0);

        /* remap page size */
        remapSize = (codeSize >> 12) &0x1ff;
        regVal = 0x80000000 | (WAVE5_AXI_ID<<20) | (W5_REMAP_CODE_INDEX<<12) | (0 << 16) | (1<<11) | remapSize;
        VpuWriteReg(coreIdx, W5_VPU_REMAP_CTRL,     regVal);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_VADDR,    0x00000000);    /* DO NOT CHANGE! */
        VpuWriteReg(coreIdx, W5_VPU_REMAP_PADDR,    codeBase);
        VpuWriteReg(coreIdx, W5_ADDR_CODE_BASE,     codeBase);
        VpuWriteReg(coreIdx, W5_CODE_SIZE,          codeSize);
        VpuWriteReg(coreIdx, W5_CODE_PARAM,         (WAVE5_AXI_ID<<4) | 0);
        VpuWriteReg(coreIdx, W5_ADDR_TEMP_BASE,     tempBase);
        VpuWriteReg(coreIdx, W5_TEMP_SIZE,          tempSize);
        VpuWriteReg(coreIdx, W5_TIMEOUT_CNT,   0);

        VpuWriteReg(coreIdx, W5_HW_OPTION, hwOption);

        // encoder
        regVal  = (1<<W5_INT_ENC_SET_PARAM);
        regVal |= (1<<W5_INT_ENC_PIC);
        regVal |= (1<<W5_INT_ENC_LINEBUF_FULL);
        regVal |= (1<<W5_INT_ENC_LOW_LATENCY);
        // decoder
        regVal  = (1<<W5_INT_INIT_SEQ);
        regVal |= (1<<W5_INT_DEC_PIC);
        regVal |= (1<<W5_INT_BSBUF_EMPTY);
        regVal |= (1<<W5_INT_DEC_INSUFFICIENT_VLC_BUFFER);

        VpuWriteReg(coreIdx, W5_VPU_VINT_ENABLE,  regVal);

        VpuWriteReg(coreIdx, W5_CMD_INIT_NUM_TASK_BUF, COMMAND_QUEUE_DEPTH);

        VpuWriteReg(coreIdx, W5_CMD_INIT_TASK_BUF_SIZE, ONE_PARAMBUF_SIZE_FOR_CQ);

        vdi_get_common_memory(coreIdx, &vb);
        for (i = 0; i < COMMAND_QUEUE_DEPTH; i++) {
            taskBufBase = vb.phys_addr + WAVE5_TASK_BUF_OFFSET + (i*ONE_PARAMBUF_SIZE_FOR_CQ);
            VpuWriteReg(coreIdx, W5_CMD_INIT_ADDR_TASK_BUF0 + (i*4), taskBufBase);
        }

        if (vdi_get_sram_memory(coreIdx, &vb) < 0)  // get SRAM base/size
            return RETCODE_INSUFFICIENT_RESOURCE;

        VpuWriteReg(coreIdx, W5_ADDR_SEC_AXI, vb.phys_addr);
        VpuWriteReg(coreIdx, W5_SEC_AXI_SIZE, vb.size);
        VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 1);
        VpuWriteReg(coreIdx, W5_COMMAND, (reset==TRUE ? W5_INIT_VPU : W5_WAKEUP_VPU));
        VpuWriteReg(coreIdx, W5_VPU_REMAP_CORE_START, 1);

        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }

        regVal = VpuReadReg(coreIdx, W5_RET_SUCCESS);
        if (regVal == 0) {
            return RETCODE_FAILURE;
        }
        VpuWriteReg(coreIdx, W5_VPU_VINT_REASON_CLR, 0xffff);
        VpuWriteReg(coreIdx, W5_VPU_VINT_REASON_USR, 0);
        VpuWriteReg(coreIdx, W5_VPU_VINT_CLEAR, 0x1);

    }

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuReset(Uint32 coreIdx, SWResetMode resetMode)
{
    Uint32      val = 0;
    RetCode     ret = RETCODE_SUCCESS;
    VpuAttr*    pAttr = &g_VpuCoreAttributes[coreIdx];
    ProductId   productId = (ProductId)VPU_GetProductId(coreIdx);

    // VPU doesn't send response. Force to set BUSY flag to 0.
    VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 0);

    if (productId == PRODUCT_ID_520 || productId == PRODUCT_ID_525 || productId == PRODUCT_ID_521 || productId == PRODUCT_ID_511)
        pAttr->supportBackbone = TRUE;

    // Waiting for completion of bus transaction
    if (pAttr->supportBackbone == TRUE) {
        // Step1 : disable request
        vdi_fio_write_register(coreIdx, W5_BACKBONE_GDI_BUS_CTRL, 0x4);

        // Step2 : Waiting for completion of bus transaction
        if (vdi_wait_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_BACKBONE_GDI_BUS_STATUS) == -1) {
            vdi_fio_write_register(coreIdx, W5_BACKBONE_GDI_BUS_CTRL, 0x00);
            VLOG(ERR, "VpuReset Error = %d\n", pAttr->supportBackbone);
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
    }
    else {
        // Step1 : disable request
        vdi_fio_write_register(coreIdx, W5_GDI_BUS_CTRL, 0x100);

        // Step2 : Waiting for completion of bus transaction
        if (vdi_wait_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_GDI_BUS_STATUS) == -1) {
            vdi_fio_write_register(coreIdx, W5_GDI_BUS_CTRL, 0x00);
            VLOG(ERR, "VpuReset Error = %d\n", pAttr->supportBackbone);
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
    }

    // Step3 : Waiting for completion of VCPU bus transaction
    if (resetMode != SW_RESET_ON_BOOT ) {
        if (vdi_wait_vcpu_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_VCPU_STATUS) == -1) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
    }

    if (resetMode == SW_RESET_SAFETY) {
        if ((ret=Wave5VpuSleepWake(coreIdx, TRUE, NULL, 0, TRUE)) != RETCODE_SUCCESS) {
            return ret;
        }
    }

    switch (resetMode) {
    case SW_RESET_ON_BOOT:
    case SW_RESET_FORCE:
    case SW_RESET_SAFETY:
        val = W5_RST_BLOCK_ALL;
        break;
    default:
        return RETCODE_INVALID_PARAM;
    }

    if (val) {
        VpuWriteReg(coreIdx, W5_VPU_RESET_REQ, val);

        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_RESET_STATUS) == -1) {
            VpuWriteReg(coreIdx, W5_VPU_RESET_REQ, 0);
            vdi_log(coreIdx, W5_RESET_VPU, 2);
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
        VpuWriteReg(coreIdx, W5_VPU_RESET_REQ, 0);
    }
    // Step3 : must clear GDI_BUS_CTRL after done SW_RESET
    if (pAttr->supportBackbone == TRUE) {
        vdi_fio_write_register(coreIdx, W5_BACKBONE_GDI_BUS_CTRL, 0x00);
    }
    else {
        vdi_fio_write_register(coreIdx, W5_GDI_BUS_CTRL, 0x00);
    }
    if (resetMode == SW_RESET_SAFETY || resetMode == SW_RESET_FORCE ) {
        ret = Wave5VpuSleepWake(coreIdx, FALSE, NULL, 0, TRUE);
    }

    return ret;
}

RetCode Wave5VpuDecFiniSeq(CodecInst* instance)
{
    RetCode ret = RETCODE_SUCCESS;

    Wave5BitIssueCommand(instance, W5_DESTROY_INSTANCE);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {
        Uint32 regVal;
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);

        if (regVal == WAVE5_VPU_STILL_RUNNING)
            ret = RETCODE_VPU_STILL_RUNNING;
        else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_DEC_VLC_BUF_FULL)
            return RETCODE_VLC_BUF_FULL;
        else
            ret = RETCODE_FAILURE;
    }

    return ret;
}

RetCode Wave5VpuDecSetBitstreamFlag(CodecInst* instance, BOOL running, BOOL eos, BOOL explictEnd)
{
    DecInfo* pDecInfo = &instance->CodecInfo->decInfo;
    BitStreamMode bsMode = (BitStreamMode)pDecInfo->openParam.bitstreamMode;
    pDecInfo->streamEndflag = (eos == 1) ? TRUE : FALSE;

    if (bsMode == BS_MODE_INTERRUPT) {
        if (pDecInfo->streamEndflag == TRUE) explictEnd = TRUE;

        VpuWriteReg(instance->coreIdx, W5_BS_OPTION, (pDecInfo->streamEndflag<<1) | explictEnd);
        VpuWriteReg(instance->coreIdx, W5_BS_WR_PTR, pDecInfo->streamWrPtr);

        Wave5BitIssueCommand(instance, W5_UPDATE_BS);
        if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }

        if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == 0) {
            return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}


RetCode Wave5DecClrDispFlag(CodecInst* instance, Uint32 index)
{
    RetCode ret = RETCODE_SUCCESS;
    DecInfo * pDecInfo;
    pDecInfo   = &instance->CodecInfo->decInfo;

    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_CLR_DISP_IDC, (1<<index));
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_SET_DISP_IDC, 0);
    ret = SendQuery(instance, UPDATE_DISP_FLAG);

    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Wave5DecClrDispFlag QUERY FAILURE\n");
        return RETCODE_QUERY_FAILURE;
    }

    pDecInfo->frameDisplayFlag = VpuReadReg(instance->coreIdx, pDecInfo->frameDisplayFlagRegAddr);

    return RETCODE_SUCCESS;
}

RetCode Wave5DecSetDispFlag(CodecInst* instance, Uint32 index)
{
    RetCode ret = RETCODE_SUCCESS;

    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_CLR_DISP_IDC, 0);
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_SET_DISP_IDC, (1<<index));
    ret = SendQuery(instance, UPDATE_DISP_FLAG);

    return ret;
}

Int32 Wave5VpuWaitInterrupt(CodecInst* instance, Int32 timeout, BOOL pending)
{
    Int32  reason = -1;
#ifdef SUPPORT_MULTI_INST_INTR
#else
    Int32  orgReason = -1;
    Int32  remain_intr = -1; // to set VPU_VINT_REASON for remain interrupt.
    Int32  ownInt      = 0;
    Uint32 regVal;
    Uint32 IntrMask = ((1 << INT_WAVE5_BSBUF_EMPTY) | (1 << INT_WAVE5_DEC_PIC) | (1 << INT_WAVE5_INIT_SEQ) | (1 << INT_WAVE5_ENC_SET_PARAM) | (1 << INT_WAVE5_INSUFFICIENT_VLC_BUFFER));
#endif
#ifdef SUPPORT_MULTI_INST_INTR
	// check an interrupt for my instance during timeout
	reason = vdi_wait_interrupt(instance->coreIdx, instance->instIndex, timeout);
#else
    EnterLock(instance->coreIdx);

    // check one interrupt for current instance even if the number of interrupt triggered more than one.
    if ((reason = vdi_wait_interrupt(instance->coreIdx, timeout)) > 0) {

        remain_intr = reason;

        if (reason & (1 << INT_WAVE5_BSBUF_EMPTY)) {
            regVal = VpuReadReg(instance->coreIdx, W5_RET_BS_EMPTY_INST);
			regVal = (regVal & 0xffff);
            if (regVal & (1 << instance->instIndex)) {
                ownInt = 1;
                reason = (1 << INT_WAVE5_BSBUF_EMPTY);
                remain_intr &= ~(Uint32)reason;

				regVal = regVal & ~(1UL << instance->instIndex);
				VpuWriteReg(instance->coreIdx, W5_RET_BS_EMPTY_INST, regVal);
            }
        }

        if (reason & (1 << INT_WAVE5_INSUFFICIENT_VLC_BUFFER)) {
            regVal = VpuReadReg(instance->coreIdx, W5_RET_VCL_INSUFF_INST);
            regVal = (regVal & 0xffff);
            if (regVal & (1 << instance->instIndex)) {
                ownInt = 1;
                reason = (1 << INT_WAVE5_INSUFFICIENT_VLC_BUFFER);
                remain_intr &= ~(Uint32)reason;

                regVal = regVal & ~(1UL << instance->instIndex);
                VpuWriteReg(instance->coreIdx, W5_RET_VCL_INSUFF_INST, regVal);
            }
        }

        if (reason & (1 << INT_WAVE5_INIT_SEQ)) {
            regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_CMD_DONE_INST);
			regVal = (regVal & 0xffff);
            if (regVal & (1 << instance->instIndex)) {
                ownInt = 1;
                reason = (1 << INT_WAVE5_INIT_SEQ);
                remain_intr &= ~(Uint32)reason;

				regVal = regVal & ~(1UL << instance->instIndex);
				VpuWriteReg(instance->coreIdx, W5_RET_QUEUE_CMD_DONE_INST, regVal);
            }
        }

        if (reason & (1 << INT_WAVE5_DEC_PIC)) {
            regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_CMD_DONE_INST);
			regVal = (regVal & 0xffff);
            if (regVal & (1 << instance->instIndex)) {
                ownInt = 1;
                orgReason = reason;
                reason = (1 << INT_WAVE5_DEC_PIC);
                remain_intr &= ~(Uint32)reason;
                /* Clear Low Latency Interrupt if two interrupts are occured */
                if (orgReason & (1 << INT_WAVE5_ENC_LOW_LATENCY)) {
                    regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_CMD_DONE_INST);
					regVal = (regVal>>16);
                    if (regVal & (1 << instance->instIndex)) {
                        remain_intr &= ~(1<<INT_WAVE5_ENC_LOW_LATENCY);
                        Wave5VpuClearInterrupt(instance->coreIdx, 1<<INT_WAVE5_ENC_LOW_LATENCY);
                    }
                }
				regVal = regVal & ~(1UL << instance->instIndex);
				VpuWriteReg(instance->coreIdx, W5_RET_QUEUE_CMD_DONE_INST, regVal);
            }
        }



        if (reason & (1 << INT_WAVE5_ENC_SET_PARAM)) {
            regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_CMD_DONE_INST);
			regVal = (regVal & 0xffff);
            if (regVal & (1 << instance->instIndex)) {
                ownInt = 1;
                reason = (1 << INT_WAVE5_ENC_SET_PARAM);
                remain_intr &= ~(Uint32)reason;

				regVal = regVal & ~(1UL << instance->instIndex);
				VpuWriteReg(instance->coreIdx, W5_RET_QUEUE_CMD_DONE_INST, regVal);
            }
        }

		if (reason & (1 << INT_WAVE5_ENC_LOW_LATENCY)) {
			regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_CMD_DONE_INST);
			regVal = (regVal>>16);
			if (regVal & (1 << instance->instIndex)) {
				ownInt = 1;
				reason = (1 << INT_WAVE5_ENC_LOW_LATENCY);
				remain_intr &= ~(Uint32)reason;

				regVal = regVal & ~(1UL << instance->instIndex);
				regVal = (regVal << 16);
				VpuWriteReg(instance->coreIdx, W5_RET_QUEUE_CMD_DONE_INST, regVal);
			}
		}

        if (reason & ~IntrMask) {    // when interrupt is not for empty, dec_pic, init_seq.
            regVal = VpuReadReg(instance->coreIdx, W5_RET_DONE_INSTANCE_INFO)&0xFF;
            if (regVal == instance->instIndex) {
                ownInt = 1;
                reason = (reason & ~IntrMask);
                remain_intr &= ~(Uint32)reason;
            }
        }

        VpuWriteReg(instance->coreIdx, W5_VPU_VINT_REASON, remain_intr);     // set remain interrupt flag to trigger interrupt next time.

        if (!ownInt)
            reason = -1;    // if there was no interrupt for current instance id, reason should be -1;
    }
    LeaveLock(instance->coreIdx);
#endif
    return reason;
}

RetCode Wave5VpuClearInterrupt(Uint32 coreIdx, Uint32 flags)
{
    Uint32 interruptReason;

    interruptReason = VpuReadReg(coreIdx, W5_VPU_VINT_REASON_USR);
    interruptReason &= ~flags;
    VpuWriteReg(coreIdx, W5_VPU_VINT_REASON_USR, interruptReason);

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuDecGetRdPtr(CodecInst* instance, PhysicalAddress *rdPtr)
{
    RetCode ret = RETCODE_SUCCESS;

    ret = SendQuery(instance, GET_BS_RD_PTR);

    if (ret != RETCODE_SUCCESS)
        return RETCODE_QUERY_FAILURE;

    *rdPtr = VpuReadReg(instance->coreIdx, W5_RET_QUERY_DEC_BS_RD_PTR);

    return RETCODE_SUCCESS;
}

#ifdef FIX_SET_GET_RD_PTR_BUG
RetCode Wave5VpuDecSetRdPtr(CodecInst* instance, PhysicalAddress rdPtr)
{
	RetCode ret = RETCODE_SUCCESS;

	VpuWriteReg(instance->coreIdx, W5_RET_QUERY_DEC_SET_BS_RD_PTR, rdPtr);

	ret = SendQuery(instance, GET_BS_SET_RD_PTR);

	if (ret != RETCODE_SUCCESS)
		return RETCODE_QUERY_FAILURE;

	return RETCODE_SUCCESS;
}
#endif

RetCode Wave5VpuGetBwReport(CodecInst* instance, VPUBWData* bwMon)
{
    RetCode     ret = RETCODE_SUCCESS;
    Int32       coreIdx;

    coreIdx = instance->coreIdx;

    ret = SendQuery(instance, GET_BW_REPORT);
    if (ret != RETCODE_SUCCESS) {
        if (VpuReadReg(coreIdx, W5_RET_FAIL_REASON) == WAVE5_RESULT_NOT_READY)
            return RETCODE_REPORT_NOT_READY;
        else
            return RETCODE_QUERY_FAILURE;
    }

    bwMon->prpBwRead    = VpuReadReg(coreIdx, RET_QUERY_BW_PRP_AXI_READ)    * 16;
    bwMon->prpBwWrite   = VpuReadReg(coreIdx, RET_QUERY_BW_PRP_AXI_WRITE)   * 16;
    bwMon->fbdYRead     = VpuReadReg(coreIdx, RET_QUERY_BW_FBD_Y_AXI_READ)  * 16;
    bwMon->fbcYWrite    = VpuReadReg(coreIdx, RET_QUERY_BW_FBC_Y_AXI_WRITE) * 16;
    bwMon->fbdCRead     = VpuReadReg(coreIdx, RET_QUERY_BW_FBD_C_AXI_READ)  * 16;
    bwMon->fbcCWrite    = VpuReadReg(coreIdx, RET_QUERY_BW_FBC_C_AXI_WRITE) * 16;
    bwMon->priBwRead    = VpuReadReg(coreIdx, RET_QUERY_BW_PRI_AXI_READ)    * 16;
    bwMon->priBwWrite   = VpuReadReg(coreIdx, RET_QUERY_BW_PRI_AXI_WRITE)   * 16;
    bwMon->secBwRead    = VpuReadReg(coreIdx, RET_QUERY_BW_SEC_AXI_READ)    * 16;
    bwMon->secBwWrite   = VpuReadReg(coreIdx, RET_QUERY_BW_SEC_AXI_WRITE)   * 16;
    bwMon->procBwRead   = VpuReadReg(coreIdx, RET_QUERY_BW_PROC_AXI_READ)   * 16;
    bwMon->procBwWrite  = VpuReadReg(coreIdx, RET_QUERY_BW_PROC_AXI_WRITE)  * 16;

    return RETCODE_SUCCESS;
}


/************************************************************************/
/*                       ENCODER functions                              */
/************************************************************************/


