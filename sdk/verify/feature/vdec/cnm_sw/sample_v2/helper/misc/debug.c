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
#include "config.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "wave/wave5_regdefine.h"
#include "wave/wave5.h"
#include "coda9/coda9_regdefine.h"
#if defined(PLATFORM_LINUX) || defined(PLATFORM_QNX)
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#endif
#include "main_helper.h"
#include "misc/debug.h"
#include "vpuconfig.h"


enum { False, True };

void InitializeDebugEnv(
    Uint32  options
    )
{
    UNREFERENCED_PARAMETER(options);
}

void ReleaseDebugEnv(
    void
    )
{
}

Int32 checkLineFeedInHelp(
    struct OptionExt *opt
    )
{
    int i;

    for (i=0;i<MAX_GETOPT_OPTIONS;i++) {
        if (opt[i].name==NULL)
            break;
        if (!strstr(opt[i].help, "\n")) {
            VLOG(INFO, "(%s) doesn't have \\n in options struct in main function. please add \\n\n", opt[i].help);
            return FALSE;
        }
    }
    return TRUE;
}

void PrintVpuProductInfo(
    Uint32      coreIdx,
    ProductInfo *productInfo
    )
{
    BOOL verbose = FALSE;

    if (VPU_GetProductInfo(coreIdx, productInfo) != RETCODE_SUCCESS) {
        //PrintVpuStatus(coreIdx, productInfo->productId);//TODO
    }

    VLOG(INFO, "VPU coreNum : [%d]\n", coreIdx);
    VLOG(INFO, "Firmware : CustomerCode: %04x | version : rev.%d\n", productInfo->customerId, productInfo->fwVersion);
    VLOG(INFO, "Hardware : %04x\n", productInfo->productId);
    VLOG(INFO, "API      : %d.%d.%d\n\n", API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
    if (PRODUCT_ID_W_SERIES(productInfo->productId))
    {
        VLOG(INFO, "productId       : %08x\n", productInfo->productId);
        VLOG(INFO, "fwVersion       : %08x(r%d)\n", productInfo->fwVersion, productInfo->fwVersion);
        VLOG(INFO, "productName     : %c%c%c%c%04x\n",
            productInfo->productName>>24, productInfo->productName>>16, productInfo->productName>>8, productInfo->productName,
            productInfo->productVersion);
        if ( verbose == TRUE )
        {
            Uint32 stdDef0          = productInfo->stdDef0;
            Uint32 stdDef1          = productInfo->stdDef1;
            Uint32 confFeature      = productInfo->confFeature;
            BOOL supportDownScaler  = FALSE;
            BOOL supportAfbce       = FALSE;
            char ch_ox[2]           = {'X', 'O'};

            VLOG(INFO, "==========================\n");
            VLOG(INFO, "stdDef0           : %08x\n", stdDef0);
            /* checking ONE AXI BIT FILE */
            VLOG(INFO, "MAP CONVERTER REG : %d\n", (stdDef0>>31)&1);
            VLOG(INFO, "MAP CONVERTER SIG : %d\n", (stdDef0>>30)&1);
            VLOG(INFO, "BG_DETECT         : %d\n", (stdDef0>>20)&1);
            VLOG(INFO, "3D NR             : %d\n", (stdDef0>>19)&1);
            VLOG(INFO, "ONE-PORT AXI      : %d\n", (stdDef0>>18)&1);
            VLOG(INFO, "2nd AXI           : %d\n", (stdDef0>>17)&1);
            VLOG(INFO, "GDI               : %d\n", !((stdDef0>>16)&1));//no-gdi
            VLOG(INFO, "AFBC              : %d\n", (stdDef0>>15)&1);
            VLOG(INFO, "AFBC VERSION      : %d\n", (stdDef0>>12)&7);
            VLOG(INFO, "FBC               : %d\n", (stdDef0>>11)&1);
            VLOG(INFO, "FBC  VERSION      : %d\n", (stdDef0>>8)&7);
            VLOG(INFO, "SCALER            : %d\n", (stdDef0>>7)&1);
            VLOG(INFO, "SCALER VERSION    : %d\n", (stdDef0>>4)&7);
            VLOG(INFO, "BWB               : %d\n", (stdDef0>>3)&1);
            VLOG(INFO, "==========================\n");
            VLOG(INFO, "stdDef1           : %08x\n", stdDef1);
            VLOG(INFO, "CyclePerTick      : %d\n", (stdDef1>>27)&1); //0:32768, 1:256
            VLOG(INFO, "MULTI CORE EN     : %d\n", (stdDef1>>26)&1);
            VLOG(INFO, "GCU EN            : %d\n", (stdDef1>>25)&1);
            VLOG(INFO, "CU REPORT         : %d\n", (stdDef1>>24)&1);
            VLOG(INFO, "VCORE ID 3        : %d\n", (stdDef1>>19)&1);
            VLOG(INFO, "VCORE ID 2        : %d\n", (stdDef1>>18)&1);
            VLOG(INFO, "VCORE ID 1        : %d\n", (stdDef1>>17)&1);
            VLOG(INFO, "VCORE ID 0        : %d\n", (stdDef1>>16)&1);
            VLOG(INFO, "BW OPT            : %d\n", (stdDef1>>15)&1);
            VLOG(INFO, "==========================\n");
            VLOG(INFO, "confFeature       : %08x\n", confFeature);
            VLOG(INFO, "VP9  ENC Profile2 : %d\n", (confFeature>>7)&1);
            VLOG(INFO, "VP9  ENC Profile0 : %d\n", (confFeature>>6)&1);
            VLOG(INFO, "VP9  DEC Profile2 : %d\n", (confFeature>>5)&1);
            VLOG(INFO, "VP9  DEC Profile0 : %d\n", (confFeature>>4)&1);
            VLOG(INFO, "HEVC ENC MAIN10   : %d\n", (confFeature>>3)&1);
            VLOG(INFO, "HEVC ENC MAIN     : %d\n", (confFeature>>2)&1);
            VLOG(INFO, "HEVC DEC MAIN10   : %d\n", (confFeature>>1)&1);
            VLOG(INFO, "HEVC DEC MAIN     : %d\n", (confFeature>>0)&1);
            VLOG(INFO, "==========================\n");
            VLOG(INFO, "configDate        : %d\n", productInfo->configDate);
            VLOG(INFO, "HW version        : r%d\n", productInfo->configRevision);

            supportDownScaler = (BOOL)((stdDef0>>7)&1);
            supportAfbce      = (BOOL)((stdDef0>>15)&1);

            VLOG (INFO, "------------------------------------\n");
            VLOG (INFO, "VPU CONF| SCALER | AFBCE  |\n");
            VLOG (INFO, "        |   %c    |    %c   |\n", ch_ox[supportDownScaler], ch_ox[supportAfbce]);
            VLOG (INFO, "------------------------------------\n");
            for (coreIdx=0 ; coreIdx<MAX_NUM_VCORE ; coreIdx++) {
                if (productInfo->configVcore[coreIdx]) {
                    Uint32 std_vp9, std_hevc, std_avs2;
                    if ( coreIdx == 0)
                        VLOG (INFO, "        |  HEVC  |   VP9  |  AVS2  |\n");
                    std_vp9  = (productInfo->configVcore[coreIdx] & 0x4) ? 1 : 0;
                    std_hevc = (productInfo->configVcore[coreIdx] & 0x1) ? 1 : 0;
                    std_avs2 = (productInfo->configVcore[coreIdx] & 0x8) ? 1 : 0;
                    VLOG (INFO, " VCore%d |    %c   |    %c   |    %c   |\n",
                        coreIdx, ch_ox[std_hevc], ch_ox[std_vp9], ch_ox[std_avs2]);
                    if ( coreIdx == (MAX_NUM_VCORE-1))
                        VLOG (INFO, "------------------------------------\n");
                }
            }
        }
        else {
            VLOG(INFO, "==========================\n");
            VLOG(INFO, "stdDef0          : %08x\n", productInfo->stdDef0);
            VLOG(INFO, "stdDef1          : %08x\n", productInfo->stdDef1);
            VLOG(INFO, "confFeature      : %08x\n", productInfo->confFeature);
            VLOG(INFO, "configDate       : %08x\n", productInfo->configDate);
            VLOG(INFO, "configRevision   : %08x\n", productInfo->configRevision);
            VLOG(INFO, "configType       : %08x\n", productInfo->configType);
            VLOG(INFO, "==========================\n");
        }
    }
}




Uint32 ReadRegVCE(
    Uint32 coreIdx,
    Uint32 vce_core_idx,
    Uint32 vce_addr
    )
{//lint !e18
    int     vcpu_reg_addr;
    int     udata;
    int     vce_core_base = 0x8000 + 0x1000*vce_core_idx;

    SetClockGate(coreIdx, 1);
    vdi_fio_write_register(coreIdx, VCORE_DBG_READY(vce_core_idx), 0);

    vcpu_reg_addr = vce_addr >> 2;

    vdi_fio_write_register(coreIdx, VCORE_DBG_ADDR(vce_core_idx),vcpu_reg_addr + vce_core_base);

    if (vdi_fio_read_register(0, VCORE_DBG_READY(vce_core_idx)) == 1)
        udata= vdi_fio_read_register(0, VCORE_DBG_DATA(vce_core_idx));
    else {
        VLOG(ERR, "failed to read VCE register: %d, 0x%04x\n", vce_core_idx, vce_addr);
        udata = -2;//-1 can be a valid value
    }

    SetClockGate(coreIdx, 0);
    return udata;
}

void WriteRegVCE(
    Uint32   coreIdx,
    Uint32   vce_core_idx,
    Uint32   vce_addr,
    Uint32   udata
    )
{
    int vcpu_reg_addr;

    SetClockGate(coreIdx, 1);

    vdi_fio_write_register(coreIdx, VCORE_DBG_READY(vce_core_idx),0);

    vcpu_reg_addr = vce_addr >> 2;

    vdi_fio_write_register(coreIdx, VCORE_DBG_DATA(vce_core_idx),udata);
    vdi_fio_write_register(coreIdx, VCORE_DBG_ADDR(vce_core_idx),(vcpu_reg_addr) & 0x00007FFF);

    while (vdi_fio_read_register(0, VCORE_DBG_READY(vce_core_idx)) == 0xffffffff) {
        VLOG(ERR, "failed to write VCE register: 0x%04x\n", vce_addr);
    }
    SetClockGate(coreIdx, 0);
}

#define VCE_DEC_CHECK_SUM0         0x110
#define VCE_DEC_CHECK_SUM1         0x114
#define VCE_DEC_CHECK_SUM2         0x118
#define VCE_DEC_CHECK_SUM3         0x11C
#define VCE_DEC_CHECK_SUM4         0x120
#define VCE_DEC_CHECK_SUM5         0x124
#define VCE_DEC_CHECK_SUM6         0x128
#define VCE_DEC_CHECK_SUM7         0x12C
#define VCE_DEC_CHECK_SUM8         0x130
#define VCE_DEC_CHECK_SUM9         0x134
#define VCE_DEC_CHECK_SUM10        0x138
#define VCE_DEC_CHECK_SUM11        0x13C

#define READ_BIT(val,high,low) ((((high)==31) && ((low) == 0)) ?  (val) : (((val)>>(low)) & (((1<< ((high)-(low)+1))-1))))


void PrintWave5xxDecSppStatus(
    Uint32 coreIdx
    )
{
    Uint32  regVal;
    //DECODER SDMA INFO
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5000);
    VLOG(ERR,"C_SDMA_LOAD_CMD    = 0x%x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5004);
    VLOG(ERR,"C_SDMA_AUTO_MODE  = 0x%x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5008);
    VLOG(ERR,"C_SDMA_START_ADDR  = 0x%x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x500C);
    VLOG(ERR,"C_SDMA_END_ADDR   = 0x%x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5010);
    VLOG(ERR,"C_SDMA_ENDIAN     = 0x%x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5014);
    VLOG(ERR,"C_SDMA_IRQ_CLEAR  = 0x%x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5018);
    VLOG(ERR,"C_SDMA_BUSY       = 0x%x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x501C);
    VLOG(ERR,"C_SDMA_LAST_ADDR  = 0x%x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5020);
    VLOG(ERR,"C_SDMA_SC_BASE_ADDR = 0x%x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5400);
    VLOG(ERR,"C_SHU_INIT = 0x%x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5404);
    VLOG(ERR,"C_SHU_SEEK_NXT_NAL = 0x%x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x540C);
    VLOG(ERR,"C_SHU_SATUS = 0x%x\n",regVal);
}

void PrintVpuStatus(
    Uint32 coreIdx,
    Uint32 productId
    )
{

}

void PrintDecVpuStatus(
    DecHandle   handle
    )
{
    Int32       coreIdx;
    coreIdx   = handle->coreIdx;

    SetClockGate(coreIdx, 1);
    vdi_log(coreIdx, 1, 1);
    vdi_print_vpu_status(coreIdx);
    SetClockGate(coreIdx, 0);
}


void DumpCodeBuffer(
    const char* path
    )
{
    Uint8*          buffer;
    vpu_buffer_t    vb;
    PhysicalAddress addr;
    osal_file_t     ofp;

    VLOG(ERR,"DUMP CODE AREA into %s ", path);

    buffer = (Uint8*)osal_malloc(1024*1024);
    if ((ofp=osal_fopen(path, "wb")) == NULL) {
        VLOG(ERR,"[FAIL]\n");
    }
    else {
        vdi_get_common_memory(0, &vb);

        addr   = vb.phys_addr;
        VpuReadMem(0, addr, buffer, WAVE5_MAX_CODE_BUF_SIZE, VDI_128BIT_LITTLE_ENDIAN);
        osal_fwrite(buffer, 1, WAVE5_MAX_CODE_BUF_SIZE, ofp);
        osal_fclose(ofp);
        VLOG(ERR,"[OK]\n");
    }
    osal_free(buffer);
}


void HandleDecoderError(
    DecHandle       handle,
    Uint32          frameIdx,
    DecOutputInfo*  outputInfo
    )
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(outputInfo);
}

void PrintMemoryAccessViolationReason(
    Uint32          coreIdx,
    void            *outp
    )
{
/*
#ifdef SUPPORT_MEM_PROTECT
    Uint32 err_reason=0;
    Uint32 err_addr=0;
    Uint32 err_size = 0;

    Uint32 err_size1=0;
    Uint32 err_size2=0;
    Uint32 err_size3=0;
    Uint32 product_code=0;
    DecOutputInfo *out = outp;


    SetClockGate(coreIdx, 1);
    product_code = VpuReadReg(coreIdx, VPU_PRODUCT_CODE_REGISTER);
    val = DEFAULT_TYPE_CODE;
    if (PRODUCT_CODE_W_SERIES(product_code)) {
        if (out) {
            err_reason = out->wprotErrReason;
            err_addr   = out->wprotErrAddress;
#ifdef SUPPORT_FIO_ACCESS
            err_size   = vdi_fio_read_register(coreIdx, W4_GDI_SIZE_ERR_FLAG);
#endif
        }
        else {
#ifdef SUPPORT_FIO_ACCESS
            err_reason = vdi_fio_read_register(coreIdx, W4_GDI_WPROT_ERR_RSN);
            err_addr   = vdi_fio_read_register(coreIdx, W4_GDI_WPROT_ERR_ADR);
            err_size   = vdi_fio_read_register(coreIdx, W4_GDI_SIZE_ERR_FLAG);
#endif
        }
    }
    else if (PRODUCT_CODE_NOT_W_SERIES(product_code)) {
        if (out) {
            err_reason = out->wprotErrReason;
            err_addr   = out->wprotErrAddress;
            err_size   = VpuReadReg(coreIdx, GDI_SIZE_ERR_FLAG);
        }
        else {
            err_reason = VpuReadReg(coreIdx, GDI_WPROT_ERR_RSN);
            err_addr   = VpuReadReg(coreIdx, GDI_WPROT_ERR_ADR);
            err_size   = VpuReadReg(coreIdx, GDI_SIZE_ERR_FLAG);
        }
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", product_code);
    }

    if (err_size) {
        VLOG(ERR, "~~~~~~~ GDI rd/wr zero request size violation ~~~~~~~ \n");
        if (PRODUCT_CODE_W_SERIES(product_code)) {
            Int32 productId;

            VLOG(ERR, "err_size = 0x%x\n",   err_size);
            err_size1 = VpuReadReg(coreIdx, W4_GDI_ADR_RQ_SIZE_ERR_PRI0);
            err_size2 = VpuReadReg(coreIdx, W4_GDI_ADR_RQ_SIZE_ERR_PRI1);
            err_size3 = VpuReadReg(coreIdx, W4_GDI_ADR_RQ_SIZE_ERR_PRI2);
            VLOG(ERR, "ADR_RQ_SIZE_ERR_PRI 0x%x, 0x%x, 0x%x\n", err_size1, err_size2, err_size3);
            err_size1 = VpuReadReg(coreIdx, W4_GDI_ADR_WQ_SIZE_ERR_PRI0);
            err_size2 = VpuReadReg(coreIdx, W4_GDI_ADR_WQ_SIZE_ERR_PRI1);
            err_size3 = VpuReadReg(coreIdx, W4_GDI_ADR_WQ_SIZE_ERR_PRI2);
            VLOG(ERR, "ADR_WQ_SIZE_ERR_PRI 0x%x, 0x%x, 0x%x\n", err_size1, err_size2, err_size3);
            err_size1 = VpuReadReg(coreIdx, W4_GDI_ADR_RQ_SIZE_ERR_SEC0);
            err_size2 = VpuReadReg(coreIdx, W4_GDI_ADR_RQ_SIZE_ERR_SEC1);
            err_size3 = VpuReadReg(coreIdx, W4_GDI_ADR_WQ_SIZE_ERR_SEC0);
            VLOG(ERR, "ADR_RQ_SIZE_ERR_SEC 0x%x, 0x%x, 0x%x\n", err_size1, err_size2, err_size3);
            err_size1 = VpuReadReg(coreIdx, W4_GDI_ADR_WQ_SIZE_ERR_SEC0);
            err_size2 = VpuReadReg(coreIdx, W4_GDI_ADR_WQ_SIZE_ERR_SEC1);
            err_size3 = VpuReadReg(coreIdx, W4_GDI_ADR_WQ_SIZE_ERR_SEC2);
            VLOG(ERR, "ADR_WQ_SIZE_ERR_SEC 0x%x, 0x%x, 0x%x\n", err_size1, err_size2, err_size3);
            err_size1 = VpuReadReg(coreIdx, W4_GDI_ADR_RQ_SIZE_ERR_PRI0_2D);
            err_size2 = VpuReadReg(coreIdx, W4_GDI_ADR_RQ_SIZE_ERR_PRI1_2D);
            err_size3 = VpuReadReg(coreIdx, W4_GDI_ADR_RQ_SIZE_ERR_PRI2_2D);
            VLOG(ERR, "ADR_RQ_SIZE_ERR_PRI_2D 0x%x, 0x%x, 0x%x\n", err_size1, err_size2, err_size3);
            err_size1 = VpuReadReg(coreIdx, W4_GDI_ADR_WQ_SIZE_ERR_PRI0_2D);
            err_size2 = VpuReadReg(coreIdx, W4_GDI_ADR_WQ_SIZE_ERR_PRI1_2D);
            err_size3 = VpuReadReg(coreIdx, W4_GDI_ADR_WQ_SIZE_ERR_PRI2_2D);
            VLOG(ERR, "ADR_WQ_SIZE_ERR_PRI_2D 0x%x, 0x%x, 0x%x\n", err_size1, err_size2, err_size3);
            productId = VPU_GetProductId(coreIdx);
            PrintVpuStatus(coreIdx, productId);
        }
        else if (PRODUCT_CODE_NOT_W_SERIES(product_code)) {
            VLOG(ERR, "err_size = 0x%x\n",   err_size);
            err_size1 = VpuReadReg(coreIdx, GDI_ADR_RQ_SIZE_ERR_PRI0);
            err_size2 = VpuReadReg(coreIdx, GDI_ADR_RQ_SIZE_ERR_PRI1);
            err_size3 = VpuReadReg(coreIdx, GDI_ADR_RQ_SIZE_ERR_PRI2);

            VLOG(ERR, "err_size1 = 0x%x || err_size2 = 0x%x || err_size3 = 0x%x \n", err_size1, err_size2, err_size3);
            // 		wire            pri_rq_size_zero_err    ;78
            // 		wire            pri_rq_dimension        ;77
            // 		wire    [ 1:0]  pri_rq_field_mode       ;76
            // 		wire    [ 1:0]  pri_rq_ycbcr            ;74
            // 		wire    [ 6:0]  pri_rq_pad_option       ;72
            // 		wire    [ 7:0]  pri_rq_frame_index      ;65
            // 		wire    [15:0]  pri_rq_blk_xpos         ;57
            // 		wire    [15:0]  pri_rq_blk_ypos         ;41
            // 		wire    [ 7:0]  pri_rq_blk_width        ;25
            // 		wire    [ 7:0]  pri_rq_blk_height       ;
            // 		wire    [ 7:0]  pri_rq_id               ;
            // 		wire            pri_rq_lock
        }
        else {
            VLOG(ERR, "Unknown product id : %08x\n", product_code);
        }
    }
    else
    {
        VLOG(ERR, "~~~~~~~ Memory write access violation ~~~~~~~ \n");
        VLOG(ERR, "pri/sec = %d\n",   (err_reason>>8) & 1);
        VLOG(ERR, "awid    = %d\n",   (err_reason>>4) & 0xF);
        VLOG(ERR, "awlen   = %d\n",   (err_reason>>0) & 0xF);
        VLOG(ERR, "awaddr  = 0x%X\n", err_addr);
        VLOG(ERR, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
    }
    PrintVpuStatus(coreIdx, product_code);

    SetClockGate(coreIdx, 0);
    //---------------------------------------------+
    //| Primary AXI interface (A)WID               |
    //+----+----------------------------+----------+
    //| ID |                            | sec use  |
    //+----+----------------------------+----------+
    //| 00 |  BPU MIB primary           | NA       |
    //| 01 |  Overlap filter primary    | NA       |
    //| 02 |  Deblock write-back        | NA       |
    //| 03 |  PPU                       | NA       |
    //| 04 |  Deblock sub-sampling      | NA       |
    //| 05 |  Reconstruction            | possible |
    //| 06 |  BPU MIB secondary         | possible |
    //| 07 |  BPU SPP primary           | NA       |
    //| 08 |  Spatial prediction        | possible |
    //| 09 |  Overlap filter secondary  | possible |
    //| 10 |  Deblock Y secondary       | possible |
    //| 11 |  Deblock C secondary       | possible |
    //| 12 |  JPEG write-back or Stream | NA       |
    //| 13 |  JPEG secondary            | possible |
    //| 14 |  DMAC write                | NA       |
    //| 15 |  BPU SPP secondary         | possible |
    //+----+----------------------------+----------
#else
    UNREFERENCED_PARAMETER(coreIdx);
    UNREFERENCED_PARAMETER(outp);
#endif
    */
}

/**
* \brief           Handle error cases depending on product
* \return  -1      SEQUENCE ERROR
*/
Int32 HandleDecInitSequenceError(DecHandle handle, Uint32 productId, DecOpenParam* openParam, DecInitialInfo* seqInfo, RetCode apiErrorCode)
{
    if (apiErrorCode == RETCODE_MEMORY_ACCESS_VIOLATION) {
        PrintMemoryAccessViolationReason(handle->coreIdx, NULL);
        return -1;
    }

    if (PRODUCT_ID_W_SERIES(productId)) {
        if (seqInfo->seqInitErrReason == WAVE5_ETCERR_INIT_SEQ_SPS_NOT_FOUND) {
            return -2;
        } else {
            if (seqInfo->seqInitErrReason == WAVE5_SPECERR_OVER_PICTURE_WIDTH_SIZE) {
                VLOG(ERR, "Not supported picture width: MAX_SIZE(8192): %d\n", seqInfo->picWidth);
            }
            if (seqInfo->seqInitErrReason == WAVE5_SPECERR_OVER_PICTURE_HEIGHT_SIZE) {
                VLOG(ERR, "Not supported picture height: MAX_SIZE(8192): %d\n", seqInfo->picHeight);
            }
            if (seqInfo->seqInitErrReason == WAVE5_SPECERR_OVER_CHROMA_FORMAT) {
                VLOG(ERR, "Not supported chroma idc: %d\n", seqInfo->chromaFormatIDC);
            }
            if (seqInfo->seqInitErrReason == WAVE5_SPECERR_OVER_BIT_DEPTH) {
                VLOG(ERR, "Not supported Luma or Chroma bitdepth: L(%d), C(%d)\n", seqInfo->lumaBitdepth, seqInfo->chromaBitdepth);
            }
            if (seqInfo->warnInfo == WAVE5_SPECWARN_OVER_PROFILE) {
                VLOG(INFO, "SPEC over profile: %d\n", seqInfo->profile);
            }
            if (seqInfo->warnInfo == WAVE5_ETCWARN_INIT_SEQ_VCL_NOT_FOUND) {
                VLOG(INFO, "VCL Not found : RD_PTR(0x%08x), WR_PTR(0x%08x)\n", seqInfo->rdPtr, seqInfo->wrPtr);
            }
            return -1;
        }
    }
    else {
        if (openParam->bitstreamMode == BS_MODE_PIC_END && (seqInfo->seqInitErrReason&(1UL<<31))) {
            VLOG(ERR, "SEQUENCE HEADER NOT FOUND\n");
            return -1;
        }
        else {
            return -1;
        }
    }
}



enum {
    VDI_PRODUCT_ID_980,
    VDI_PRODUCT_ID_960
};

static int read_pinfo_buffer(int coreIdx, int addr)
{
    int ack;
    int rdata;
#define VDI_LOG_GDI_PINFO_ADDR  (0x1068)
#define VDI_LOG_GDI_PINFO_REQ   (0x1060)
#define VDI_LOG_GDI_PINFO_ACK   (0x1064)
#define VDI_LOG_GDI_PINFO_DATA  (0x106c)
    //------------------------------------------
    // read pinfo - indirect read
    // 1. set read addr     (GDI_PINFO_ADDR)
    // 2. send req          (GDI_PINFO_REQ)
    // 3. wait until ack==1 (GDI_PINFO_ACK)
    // 4. read data         (GDI_PINFO_DATA)
    //------------------------------------------
    vdi_write_register(coreIdx, VDI_LOG_GDI_PINFO_ADDR, addr);
    vdi_write_register(coreIdx, VDI_LOG_GDI_PINFO_REQ, 1);

    ack = 0;
    while (ack == 0)
    {
        ack = vdi_read_register(coreIdx, VDI_LOG_GDI_PINFO_ACK);
    }

    rdata = vdi_read_register(coreIdx, VDI_LOG_GDI_PINFO_DATA);

    //VLOG(INFO, "[READ PINFO] ADDR[%x], DATA[%x]", addr, rdata);
    return rdata;
}

static void printf_gdi_info(int coreIdx, int num, int reset)
{
    int i;
    int bus_info_addr;
    int tmp;
    int val;
    int productId = 0;

    val = vdi_read_register(coreIdx, VPU_PRODUCT_CODE_REGISTER);
    val = DEFAULT_TYPE_CODE;
    if ((val&0xff00) == 0x3200) val = 0x3200;

    if (PRODUCT_CODE_W_SERIES(val)) {
        return;
    }
    else if (PRODUCT_CODE_NOT_W_SERIES(val)) {
        if (val == CODA960_CODE || val == BODA950_CODE)
            productId = VDI_PRODUCT_ID_960;
        else if (val == CODA980_CODE)
            productId = VDI_PRODUCT_ID_980;
        else
            return;
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", val);
        return;
    }

    if (productId == VDI_PRODUCT_ID_980)
        VLOG(INFO, "\n**GDI information for GDI_20\n");
    else
        VLOG(INFO, "\n**GDI information for GDI_10\n");

    for (i=0; i < num; i++)
    {

#define VDI_LOG_GDI_INFO_CONTROL 0x1400
        if (productId == VDI_PRODUCT_ID_980)
            bus_info_addr = VDI_LOG_GDI_INFO_CONTROL + i*(0x20);
        else
            bus_info_addr = VDI_LOG_GDI_INFO_CONTROL + i*0x14;
        if (reset)
        {
            vdi_write_register(coreIdx, bus_info_addr, 0x00);
            bus_info_addr += 4;
            vdi_write_register(coreIdx, bus_info_addr, 0x00);
            bus_info_addr += 4;
            vdi_write_register(coreIdx, bus_info_addr, 0x00);
            bus_info_addr += 4;
            vdi_write_register(coreIdx, bus_info_addr, 0x00);
            bus_info_addr += 4;
            vdi_write_register(coreIdx, bus_info_addr, 0x00);

            if (productId == VDI_PRODUCT_ID_980)
            {
                bus_info_addr += 4;
                vdi_write_register(coreIdx, bus_info_addr, 0x00);

                bus_info_addr += 4;
                vdi_write_register(coreIdx, bus_info_addr, 0x00);

                bus_info_addr += 4;
                vdi_write_register(coreIdx, bus_info_addr, 0x00);
            }

        }
        else
        {
            VLOG(INFO, "index = %02d", i);

            tmp = read_pinfo_buffer(coreIdx, bus_info_addr);	//TiledEn<<20 ,GdiFormat<<17,IntlvCbCr,<<16 GdiYuvBufStride
            VLOG(INFO, " control = 0x%08x", tmp);

            bus_info_addr += 4;
            tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
            VLOG(INFO, " pic_size = 0x%08x", tmp);

            bus_info_addr += 4;
            tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
            VLOG(INFO, " y-top = 0x%08x", tmp);

            bus_info_addr += 4;
            tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
            VLOG(INFO, " cb-top = 0x%08x", tmp);

            bus_info_addr += 4;
            tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
            VLOG(INFO, " cr-top = 0x%08x", tmp);
            if (productId == VDI_PRODUCT_ID_980)
            {
                bus_info_addr += 4;
                tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
                VLOG(INFO, " y-bot = 0x%08x", tmp);

                bus_info_addr += 4;
                tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
                VLOG(INFO, " cb-bot = 0x%08x", tmp);

                bus_info_addr += 4;
                tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
                VLOG(INFO, " cr-bot = 0x%08x", tmp);
            }
            VLOG(INFO, "\n");
        }
    }
}

void vdi_make_log(unsigned long coreIdx, const char *str, int step)
{
    Uint32 val;

    val = VpuReadReg(coreIdx, W5_CMD_INSTANCE_INFO);
    val &= 0xffff;
    if (step == 1)
        VLOG(INFO, "\n**%s start(%d)\n", str, val);
    else if (step == 2)	//
        VLOG(INFO, "\n**%s timeout(%d)\n", str, val);
    else
        VLOG(INFO, "\n**%s end(%d)\n", str, val);
}

void vdi_log(unsigned long coreIdx, int cmd, int step)
{
    int i;
    int productId;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return ;

    productId = VPU_GetProductId(coreIdx);

    if (PRODUCT_ID_W_SERIES(productId))
    {
        switch(cmd)
        {
        case W5_INIT_VPU:
            vdi_make_log(coreIdx, "INIT_VPU", step);
            break;
        case W5_ENC_SET_PARAM:
            vdi_make_log(coreIdx, "ENC_SET_PARAM", step);
            break;
        case W5_INIT_SEQ:
            vdi_make_log(coreIdx, "DEC INIT_SEQ", step);
            break;
        case W5_DESTROY_INSTANCE:
            vdi_make_log(coreIdx, "DESTROY_INSTANCE", step);
            break;
        case W5_DEC_PIC://ENC_PIC for ENC
            vdi_make_log(coreIdx, "DEC_PIC(ENC_PIC)", step);
            break;
        case W5_SET_FB:
            vdi_make_log(coreIdx, "SET_FRAMEBUF", step);
            break;
        case W5_FLUSH_INSTANCE:
            vdi_make_log(coreIdx, "FLUSH INSTANCE", step);
            break;
        case W5_QUERY:
            vdi_make_log(coreIdx, "QUERY", step);
            break;
        case W5_SLEEP_VPU:
            vdi_make_log(coreIdx, "SLEEP_VPU", step);
            break;
        case W5_WAKEUP_VPU:
            vdi_make_log(coreIdx, "WAKEUP_VPU", step);
            break;
        case W5_UPDATE_BS:
            vdi_make_log(coreIdx, "UPDATE_BS", step);
            break;
        case W5_CREATE_INSTANCE:
            vdi_make_log(coreIdx, "CREATE_INSTANCE", step);
            break;
        default:
            vdi_make_log(coreIdx, "ANY_CMD", step);
            break;
        }
    }
    else if (PRODUCT_ID_NOT_W_SERIES(productId))
    {
        switch(cmd)
        {
        case ENC_SEQ_INIT://DEC_SEQ_INNT
            vdi_make_log(coreIdx, "SEQ_INIT", step);
            break;
        case ENC_SEQ_END://DEC_SEQ_END
            vdi_make_log(coreIdx, "SEQ_END", step);
            break;
        case PIC_RUN:
            vdi_make_log(coreIdx, "PIC_RUN", step);
            break;
        case SET_FRAME_BUF:
            vdi_make_log(coreIdx, "SET_FRAME_BUF", step);
            break;
        case ENCODE_HEADER:
            vdi_make_log(coreIdx, "ENCODE_HEADER", step);
            break;
        case RC_CHANGE_PARAMETER:
            vdi_make_log(coreIdx, "RC_CHANGE_PARAMETER", step);
            break;
        case DEC_BUF_FLUSH:
            vdi_make_log(coreIdx, "DEC_BUF_FLUSH", step);
            break;
        case FIRMWARE_GET:
            vdi_make_log(coreIdx, "FIRMWARE_GET", step);
            break;
        case ENC_PARA_SET:
            vdi_make_log(coreIdx, "ENC_PARA_SET", step);
            break;
        case DEC_PARA_SET:
            vdi_make_log(coreIdx, "DEC_PARA_SET", step);
            break;
        default:
            vdi_make_log(coreIdx, "ANY_CMD", step);
            break;
        }
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", productId);
        return;
    }

    for (i=0x0; i<0x200; i=i+16) { // host IF register 0x100 ~ 0x200
        VLOG(INFO, "0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
            vdi_read_register(coreIdx, i), vdi_read_register(coreIdx, i+4),
            vdi_read_register(coreIdx, i+8), vdi_read_register(coreIdx, i+0xc));
    }

    if (PRODUCT_ID_W_SERIES(productId))
    {
        if (cmd == W5_INIT_VPU || cmd == W5_RESET_VPU || cmd == W5_CREATE_INSTANCE)
        {
            vdi_print_vpu_status(coreIdx);
        }
    }
    else if (PRODUCT_ID_NOT_W_SERIES(productId))
    {
        if (cmd == PIC_RUN && step== 0)
        {
            printf_gdi_info(coreIdx, 32, 0);

#define VDI_LOG_MBC_BUSY 0x0440
#define VDI_LOG_MC_BASE	 0x0C00
#define VDI_LOG_MC_BUSY	 0x0C04
#define VDI_LOG_GDI_BUS_STATUS (0x10F4)
#define VDI_LOG_ROT_SRC_IDX	 (0x400 + 0x10C)
#define VDI_LOG_ROT_DST_IDX	 (0x400 + 0x110)

            VLOG(INFO, "MBC_BUSY = %x\n", vdi_read_register(coreIdx, VDI_LOG_MBC_BUSY));
            VLOG(INFO, "MC_BUSY = %x\n", vdi_read_register(coreIdx, VDI_LOG_MC_BUSY));
            VLOG(INFO, "MC_MB_XY_DONE=(y:%d, x:%d)\n", (vdi_read_register(coreIdx, VDI_LOG_MC_BASE) >> 20) & 0x3F, (vdi_read_register(coreIdx, VDI_LOG_MC_BASE) >> 26) & 0x3F);
            VLOG(INFO, "GDI_BUS_STATUS = %x\n", vdi_read_register(coreIdx, VDI_LOG_GDI_BUS_STATUS));

            VLOG(INFO, "ROT_SRC_IDX = %x\n", vdi_read_register(coreIdx, VDI_LOG_ROT_SRC_IDX));
            VLOG(INFO, "ROT_DST_IDX = %x\n", vdi_read_register(coreIdx, VDI_LOG_ROT_DST_IDX));

            VLOG(INFO, "P_MC_PIC_INDEX_0 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x200));
            VLOG(INFO, "P_MC_PIC_INDEX_1 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x20c));
            VLOG(INFO, "P_MC_PIC_INDEX_2 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x218));
            VLOG(INFO, "P_MC_PIC_INDEX_3 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x230));
            VLOG(INFO, "P_MC_PIC_INDEX_3 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x23C));
            VLOG(INFO, "P_MC_PIC_INDEX_4 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x248));
            VLOG(INFO, "P_MC_PIC_INDEX_5 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x254));
            VLOG(INFO, "P_MC_PIC_INDEX_6 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x260));
            VLOG(INFO, "P_MC_PIC_INDEX_7 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x26C));
            VLOG(INFO, "P_MC_PIC_INDEX_8 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x278));
            VLOG(INFO, "P_MC_PIC_INDEX_9 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x284));
            VLOG(INFO, "P_MC_PIC_INDEX_a = %x\n", vdi_read_register(coreIdx, MC_BASE+0x290));
            VLOG(INFO, "P_MC_PIC_INDEX_b = %x\n", vdi_read_register(coreIdx, MC_BASE+0x29C));
            VLOG(INFO, "P_MC_PIC_INDEX_c = %x\n", vdi_read_register(coreIdx, MC_BASE+0x2A8));
            VLOG(INFO, "P_MC_PIC_INDEX_d = %x\n", vdi_read_register(coreIdx, MC_BASE+0x2B4));

            VLOG(INFO, "P_MC_PICIDX_0 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x028));
            VLOG(INFO, "P_MC_PICIDX_1 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x02C));
        }
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", productId);
        return;
    }
}

static void wave5xx_vcore_status(
    Uint32 coreIdx
    )
{
    Uint32 i;

    VLOG(INFO,"[+] BPU REG Dump\n");
    for(i = 0x8000; i < 0x80FC; i += 16) {
        VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }
    VLOG(INFO,"[-] BPU REG Dump\n");

    // --------- VCE register Dump
    VLOG(INFO,"[+] VCE REG Dump\n");
    for (i=0x000; i<0x1fc; i+=16) {
        VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
            ReadRegVCE(coreIdx, 0, (i+0x00)),
            ReadRegVCE(coreIdx, 0, (i+0x04)),
            ReadRegVCE(coreIdx, 0, (i+0x08)),
            ReadRegVCE(coreIdx, 0, (i+0x0c)));
    }
    VLOG(INFO,"[-] VCE REG Dump\n");

    //cause coredump, because addr is out of the total size 4k of registers
    if(0) {
        VLOG(INFO, "ADR_INPLACE_BUF_ENABLE =  %08x\n", VpuReadReg(coreIdx, 0xff68));
        VLOG(INFO, "ADR_INPLACE_YBUF_ENDA  =  %08x\n", VpuReadReg(coreIdx, 0xff6C));
        VLOG(INFO, "ADR_INPLACE_YBUF_SIZE  =  %08x\n", VpuReadReg(coreIdx, 0xff70));
        VLOG(INFO, "ADR_INPLACE_CBUF_ENDA  =  %08x\n", VpuReadReg(coreIdx, 0xff74));
        VLOG(INFO, "ADR_INPLACE_CBUF_SIZE  =  %08x\n", VpuReadReg(coreIdx, 0xff78));
        VLOG(INFO, "ADR_INPLACE_YBUF_LENDA =  %08x\n", VpuReadReg(coreIdx, 0xff7C));
        VLOG(INFO, "ADR_INPLACE_YBUF_LSIZE =  %08x\n", VpuReadReg(coreIdx, 0xff80));
        VLOG(INFO, "ADR_INPLACE_CBUF_LENDA =  %08x\n", VpuReadReg(coreIdx, 0xff84));
        VLOG(INFO, "ADR_INPLACE_CBUF_LSIZE =  %08x\n", VpuReadReg(coreIdx, 0xff88));
        VLOG(INFO, "ADR_INPLACE_BUF_INDEX  =  %08x\n", VpuReadReg(coreIdx, 0xff8C));
        VLOG(INFO, "ADR_INPLACE_DBG_ERRFLAG=  %08x\n", VpuReadReg(coreIdx, 0xff90));
        VLOG(INFO, "ADR_INPLACE_DBG_EXT0   =  %08x\n", VpuReadReg(coreIdx, 0xff94));
        VLOG(INFO, "ADR_INPLACE_DBG_FBC0   =  %08x\n", VpuReadReg(coreIdx, 0xff98));
        VLOG(INFO, "ADR_INPLACE_DBG_EXT1   =  %08x\n", VpuReadReg(coreIdx, 0xff9C));
        VLOG(INFO, "ADR_INPLACE_DBG_FBC1   =  %08x\n", VpuReadReg(coreIdx, 0xffA0));
        VLOG(INFO, "ADR_INPLACE_DBG_EXT2   =  %08x\n", VpuReadReg(coreIdx, 0xffA4));
        VLOG(INFO, "ADR_INPLACE_DBG_FBC2   =  %08x\n", VpuReadReg(coreIdx, 0xffA8));
        VLOG(INFO, "ADR_INPLACE_DBG_EXT3   =  %08x\n", VpuReadReg(coreIdx, 0xffAC));
        VLOG(INFO, "ADR_INPLACE_DBG_FBC3   =  %08x\n", VpuReadReg(coreIdx, 0xffB0));
    }
}

void vdi_print_vpu_status(unsigned long coreIdx)
{
    Uint32 productCode;

    productCode = vdi_read_register(coreIdx, VPU_PRODUCT_CODE_REGISTER);

    VLOG(INFO,"-------------------------------------------------------------------------------\n");

    //if (PRODUCT_CODE_W_SERIES(productCode))
    if (1)
    {
        Uint32 vcpu_reg[31]= {0,};
        Uint32 i;

        VLOG(INFO,"-------------------------------------------------------------------------------\n");
        VLOG(INFO,"------                            VCPU STATUS                             -----\n");
        VLOG(INFO,"-------------------------------------------------------------------------------\n");

        // --------- VCPU register Dump
        VLOG(INFO,"[+] VCPU REG Dump\n");
        for (i = 0; i < 25; i++) {
            VpuWriteReg (coreIdx, 0x14, (1<<9) | (i & 0xff));
            vcpu_reg[i] = VpuReadReg (coreIdx, 0x1c);

            if (i < 16) {
                VLOG(INFO,"0x%08x\t",  vcpu_reg[i]);
                if ((i % 4) == 3) VLOG(INFO,"\n");
            }
            else {
                switch (i) {
                case 16: VLOG(INFO,"CR0: 0x%08x\t", vcpu_reg[i]); break;
                case 17: VLOG(INFO,"CR1: 0x%08x\n", vcpu_reg[i]); break;
                case 18: VLOG(INFO,"ML:  0x%08x\t", vcpu_reg[i]); break;
                case 19: VLOG(INFO,"MH:  0x%08x\n", vcpu_reg[i]); break;
                case 21: VLOG(INFO,"LR:  0x%08x\n", vcpu_reg[i]); break;
                case 22: VLOG(INFO,"PC:  0x%08x\n", vcpu_reg[i]); break;
                case 23: VLOG(INFO,"SR:  0x%08x\n", vcpu_reg[i]); break;
                case 24: VLOG(INFO,"SSP: 0x%08x\n", vcpu_reg[i]); break;
                default: break;
                }
            }
        }

        VLOG(INFO,"[-] VCPU REG Dump\n");
        /// -- VCPU ENTROPY PERI DECODE Common

        VLOG(INFO,"[+] VCPU ENT DEC REG Dump\n");
        //for(i = 0x8000; i < 0x8800; i += 16)
        for(i = 0x6000; i < 0x6800; i += 16) {
            VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
                vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
                vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
                vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
                vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
        }
        VLOG(INFO,"[-] VCPU ENT DEC REG Dump\n");
        wave5xx_vcore_status(coreIdx);
        VLOG(INFO,"-------------------------------------------------------------------------------\n");
    }
    else if (PRODUCT_CODE_NOT_W_SERIES(productCode)) {
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", productCode);
    }
}

