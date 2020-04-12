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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "vpuapifunc.h"
#include "coda9/coda9_regdefine.h"
#include "wave/wave5_regdefine.h"
#include "vpuerror.h"
#include "main_helper.h"
#include "misc/debug.h"
#if defined(PLATFORM_NON_OS) || defined (PLATFORM_LINUX)
#include <getopt.h>
#endif

#if defined(PLATFORM_LINUX) || defined(PLATFORM_QNX) || defined(PLATFORM_NON_OS)
#include <sys/stat.h>
#include <unistd.h>
#endif

#define BIT_DUMMY_READ_GEN          0x06000000
#define BIT_READ_LATENCY            0x06000004
#define W5_SET_READ_DELAY           0x01000000
#define W5_SET_WRITE_DELAY          0x01000004
#define MAX_CODE_BUF_SIZE           (512*1024)

#ifdef PLATFORM_WIN32
#pragma warning(disable : 4996)     //!<< disable waring C4996: The POSIX name for this item is deprecated.
#endif

char* EncPicTypeStringH264[] = {
    "IDR/I",
    "P",
};

char* EncPicTypeStringMPEG4[] = {
    "I",
    "P",
};

char* productNameList[] = {
    "CODA980",
    "CODA960",
    "WAVE512",
    "WAVE520",
    "WAVE515",
    "Unknown",
};

void SetDefaultDecTestConfig(TestDecConfig* testConfig)
{
    osal_memset(testConfig, 0, sizeof(TestDecConfig));

    testConfig->bitstreamMode   = BS_MODE_INTERRUPT;
    testConfig->feedingMode     = FEEDING_METHOD_FIXED_SIZE; //FEEDING_METHOD_FRAME_SIZE; //FEEDING_METHOD_FIXED_SIZE;
    testConfig->streamEndian    = VPU_STREAM_ENDIAN;
    testConfig->frameEndian     = VPU_FRAME_ENDIAN;
    testConfig->cbcrInterleave  = FALSE;
    testConfig->nv21            = FALSE;
    testConfig->bitFormat       = STD_HEVC;
    testConfig->renderType      = RENDER_DEVICE_NULL;
    testConfig->mapType         = COMPRESSED_FRAME_MAP;
    testConfig->enableWTL       = TRUE;
    testConfig->wtlMode         = FF_FRAME;
    testConfig->wtlFormat       = FORMAT_420;           //!<< 8 bit YUV
    testConfig->wave.bwOptimization = FALSE;            //!<< TRUE: non-ref pictures are not stored at reference picture buffer
    testConfig->inPlaceBufferMode = INPLACE_MODE_OFF;
    testConfig->maxVerticalMV     = 256;
                                                        //!<<       for saving bandwidth.
    testConfig->fps             = 30;
}

Uint32 randomSeed;
static BOOL initializedRandom;
Uint32 GetRandom(
    Uint32   start,
    Uint32   end
    )
{
    Uint32   range = end-start+1;

    if (randomSeed == 0) {
        randomSeed = (Uint32)time(NULL);
        VLOG(INFO, "======= RANDOM SEED: %08x ======\n", randomSeed);
    }
    if (initializedRandom == FALSE) {
        srand(randomSeed);
        initializedRandom = TRUE;
    }

    if (range == 0) {
        VLOG(ERR, "%s:%d RANGE IS 0\n", __FUNCTION__, __LINE__);
        return 0;
    }
    else {
        return ((rand()%range) + start);
    }
}

Int32 LoadFirmware(
    Int32       productId,
    Uint8**     retFirmware,
    Uint32*     retSizeInWord,
    const char* path
    )
{
    Int32       nread;
    Uint32      totalRead, allocSize, readSize = 1024*1024;
    Uint8*      firmware = NULL;
    osal_file_t fp;

    if ((fp=osal_fopen(path, "rb")) == NULL)
    {
        VLOG(ERR, "Failed to open %s\n", path);
        return -1;
    }

    totalRead = 0;
    if (PRODUCT_ID_W_SERIES(productId))
    {
        firmware = (Uint8*)osal_malloc(readSize);
        allocSize = readSize;
        nread = 0;
        while (TRUE)
        {
            if (allocSize < (totalRead+readSize))
            {
                allocSize += 2*nread;
                firmware = (Uint8*)realloc(firmware, allocSize);
            }
            nread = osal_fread((void*)&firmware[totalRead], 1, readSize, fp);//lint !e613
            totalRead += nread;
            if (nread < (Int32)readSize)
                break;
        }
        *retSizeInWord = (totalRead+1)/2;
    }
    else
    {
        Uint16*     pusBitCode;
        pusBitCode = (Uint16 *)osal_malloc(MAX_CODE_BUF_SIZE);
        firmware   = (Uint8*)pusBitCode;
        if (pusBitCode)
        {
            int code;
            while (!osal_feof(fp) || totalRead < (MAX_CODE_BUF_SIZE/2)) {
                code = -1;
                if (fscanf(fp, "%x", &code) <= 0) {
                    /* matching failure or EOF */
                    break;
                }
                pusBitCode[totalRead++] = (Uint16)code;
            }
        }
        *retSizeInWord = totalRead;
    }

    osal_fclose(fp);

    *retFirmware   = firmware;

    return 0;
}


void PrintVpuVersionInfo(
    Uint32   core_idx
    )
{
    Uint32 version;
    Uint32 revision;
    Uint32 productId;

    VPU_GetVersionInfo(core_idx, &version, &revision, &productId);

    VLOG(INFO, "VPU coreNum : [%d]\n", core_idx);
    VLOG(INFO, "Firmware : CustomerCode: %04x | version : %d.%d.%d rev.%d\n",
        (Uint32)(version>>16), (Uint32)((version>>(12))&0x0f), (Uint32)((version>>(8))&0x0f), (Uint32)((version)&0xff), revision);
    VLOG(INFO, "Hardware : %04x\n", productId);
    VLOG(INFO, "API      : %d.%d.%d\n\n", API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
}

BOOL OpenDisplayBufferFile(CodStd codec, char *outputPath, VpuRect rcDisplay, TiledMapType mapType, FILE *fp[])
{
    char strFile[MAX_FILE_PATH];
    int width;
    int height;

    width = rcDisplay.right - rcDisplay.left;
    height = rcDisplay.bottom - rcDisplay.top;

    if (mapType == LINEAR_FRAME_MAP) {
        if ((fp[0]=osal_fopen(outputPath, "wb")) == NULL) {
            VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, outputPath);
            goto ERR_OPEN_DISP_BUFFER_FILE;
        }
    }
    else {
        width  = (codec == STD_HEVC || codec == STD_AVS2) ? VPU_ALIGN16(width)  : VPU_ALIGN64(width);
        height = (codec == STD_HEVC || codec == STD_AVS2) ? VPU_ALIGN16(height) : VPU_ALIGN64(height);
        sprintf(strFile, "%s_%dx%d_fbc_data_y.bin", outputPath, width, height);
        if ((fp[0]=osal_fopen(strFile, "wb")) == NULL) {
            VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, strFile);
            goto ERR_OPEN_DISP_BUFFER_FILE;
        }
        sprintf(strFile, "%s_%dx%d_fbc_data_c.bin", outputPath, width, height);
        if ((fp[1]=osal_fopen(strFile, "wb")) == NULL) {
            VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, strFile);
            goto ERR_OPEN_DISP_BUFFER_FILE;
        }
        sprintf(strFile, "%s_%dx%d_fbc_table_y.bin", outputPath, width, height);
        if ((fp[2]=osal_fopen(strFile, "wb")) == NULL) {
            VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, strFile);
            goto ERR_OPEN_DISP_BUFFER_FILE;
        }
        sprintf(strFile, "%s_%dx%d_fbc_table_c.bin", outputPath, width, height);
        if ((fp[3]=osal_fopen(strFile, "wb")) == NULL) {
            VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, strFile);
            goto ERR_OPEN_DISP_BUFFER_FILE;
        }
    }
    return TRUE;
ERR_OPEN_DISP_BUFFER_FILE:
    CloseDisplayBufferFile(fp);
    return FALSE;
}

void CloseDisplayBufferFile(FILE *fp[])
{
    int i;
    for (i=0; i < OUTPUT_FP_NUMBER; i++) {
        if (fp[i] != NULL) {
            osal_fclose(fp[i]);
            fp[i] = NULL;
        }
    }
}

RetCode VPU_GetFBCOffsetTableSize(CodStd codStd, int width, int height, int* ysize, int* csize)
{
    if (ysize == NULL || csize == NULL)
        return RETCODE_INVALID_PARAM;

    *ysize = ProductCalculateAuxBufferSize(AUX_BUF_TYPE_FBC_Y_OFFSET, codStd, width, height);
    *csize = ProductCalculateAuxBufferSize(AUX_BUF_TYPE_FBC_C_OFFSET, codStd, width, height);

    return RETCODE_SUCCESS;
}

void SaveDisplayBufferToFile(DecHandle handle, CodStd codStd, FrameBuffer dispFrame, VpuRect rcDisplay, FILE *fp[])
{
    Uint32   width;
    Uint32   height;
    Uint32   bpp;
    DecGetFramebufInfo  fbInfo;

    if (dispFrame.myIndex < 0)
        return;

    if (dispFrame.mapType != COMPRESSED_FRAME_MAP) {
        size_t sizeYuv;
        Uint8*   pYuv;
        pYuv = GetYUVFromFrameBuffer(handle, &dispFrame, rcDisplay, &width, &height, &bpp, &sizeYuv);
        osal_fwrite(pYuv, 1, sizeYuv, fp[0]);
        osal_fflush(fp[0]);
        fsync(fileno(fp[0]));
        osal_free(pYuv);
    }
    else {
        Uint32  lumaTblSize;
        Uint32  chromaTblSize;
        Uint32  lSize;
        Uint32  cSize;
        Uint32  addr;
        Uint32  endian;
        Uint8*  buf;
        Uint32  coreIdx     = VPU_HANDLE_CORE_INDEX(handle);
        Uint32  productId   = VPU_HANDLE_PRODUCT_ID(handle);
        Uint32  cFrameStride;

        VPU_DecGiveCommand(handle, DEC_GET_FRAMEBUF_INFO, (void*)&fbInfo);

        width  = rcDisplay.right - rcDisplay.left;
        width  = (codStd == STD_HEVC || codStd == STD_AVS2) ? VPU_ALIGN16(width)  : VPU_ALIGN64(width);
        height = rcDisplay.bottom - rcDisplay.top;
        height = (codStd == STD_HEVC || codStd == STD_AVS2) ? VPU_ALIGN16(height) : VPU_ALIGN64(height);

        cFrameStride = CalcStride(width, height, dispFrame.format, dispFrame.cbcrInterleave, dispFrame.mapType, (codStd == STD_VP9));
        lSize        = CalcLumaSize(productId,   cFrameStride, height, dispFrame.format, dispFrame.cbcrInterleave, dispFrame.mapType, NULL);
        cSize        = CalcChromaSize(productId, cFrameStride, height, dispFrame.format, dispFrame.cbcrInterleave, dispFrame.mapType, NULL) * 2;

        // Invert the endian because the pixel order register doesn't affect the FBC data.
        endian = (~(Uint32)dispFrame.endian & 0xf) | 0x10;

        /* Dump Y compressed data */
        if ((buf=(Uint8*)osal_malloc(lSize)) != NULL) {
            vdi_read_memory(coreIdx, dispFrame.bufY, buf, lSize, endian);
            osal_fwrite((void *)buf, sizeof(Uint8), lSize, fp[0]);
            osal_fflush(fp[0]);
            osal_free(buf);
        }
        else {
            VLOG(ERR, "%s:%d Failed to save the luma compressed data!!\n", __FUNCTION__, __LINE__);
        }

        /* Dump C compressed data */
        if ((buf=(Uint8*)osal_malloc(cSize)) != NULL) {
            vdi_read_memory(coreIdx, dispFrame.bufCb, buf, cSize, endian);
            osal_fwrite((void *)buf, sizeof(Uint8), cSize, fp[1]);
            osal_fflush(fp[1]);
            osal_free(buf);
        }
        else {
            VLOG(ERR, "%s:%d Failed to save the chroma compressed data!!\n", __FUNCTION__, __LINE__);
        }

        /* Dump Y Offset table */
        VPU_GetFBCOffsetTableSize(codStd, (int)width, (int)height, (int*)&lumaTblSize, (int*)&chromaTblSize);

        addr = fbInfo.vbFbcYTbl[dispFrame.myIndex].phys_addr;

        if ((buf=(Uint8*)osal_malloc(lumaTblSize)) != NULL) {
            vdi_read_memory(coreIdx, addr, buf, lumaTblSize, endian);
            osal_fwrite((void *)buf, sizeof(Uint8), lumaTblSize, fp[2]);
            osal_fflush(fp[2]);
            osal_free(buf);
        }
        else {
            VLOG(ERR, "%s:%d Failed to save the offset table of luma!\n", __FUNCTION__, __LINE__);
        }

        /* Dump C Offset table */
        addr = fbInfo.vbFbcCTbl[dispFrame.myIndex].phys_addr;

        if ((buf=(Uint8*)osal_malloc(chromaTblSize)) != NULL) {
            vdi_read_memory(coreIdx, addr, buf, chromaTblSize, endian);
            osal_fwrite((void *)buf, sizeof(Uint8), chromaTblSize, fp[3]);
            osal_fflush(fp[3]);
            osal_free(buf);
        }
        else {
            VLOG(ERR, "%s:%d Failed to save the offset table of chroma!\n", __FUNCTION__, __LINE__);
        }
    }
}

void FreePreviousFramebuffer(
    Uint32              coreIdx,
    DecGetFramebufInfo* fb
    )
{
    int i;

    vdi_lock(coreIdx);
    if (fb->vbFrame.size > 0) {
        vdi_free_dma_memory(coreIdx, &fb->vbFrame);
        osal_memset((void*)&fb->vbFrame, 0x00, sizeof(vpu_buffer_t));
    }
    if (fb->vbWTL.size > 0) {
        vdi_free_dma_memory(coreIdx, &fb->vbWTL);
        osal_memset((void*)&fb->vbWTL, 0x00, sizeof(vpu_buffer_t));
    }
    for ( i=0 ; i<MAX_REG_FRAME; i++) {
        if (fb->vbFbcYTbl[i].size > 0) {
            vdi_free_dma_memory(coreIdx, &fb->vbFbcYTbl[i]);
            osal_memset((void*)fb->vbFbcYTbl, 0x00, sizeof(vpu_buffer_t));
        }
        if (fb->vbFbcCTbl[i].size > 0) {
            vdi_free_dma_memory(coreIdx, &fb->vbFbcCTbl[i]);
            osal_memset((void*)fb->vbFbcCTbl, 0x00, sizeof(vpu_buffer_t));
        }
    }
    vdi_unlock(coreIdx);
}

void PrintDecSeqWarningMessages(
    Uint32          productId,
    DecInitialInfo* seqInfo
    )
{
    if (PRODUCT_ID_W_SERIES(productId))
    {
        if (seqInfo->seqInitErrReason&0x00000001) VLOG(WARN, "sps_max_sub_layer_minus1 shall be 0 to 6\n");
        if (seqInfo->seqInitErrReason&0x00000002) VLOG(WARN, "general_reserved_zero_44bits shall be 0.\n");
        if (seqInfo->seqInitErrReason&0x00000004) VLOG(WARN, "reserved_zero_2bits shall be 0\n");
        if (seqInfo->seqInitErrReason&0x00000008) VLOG(WARN, "sub_layer_reserved_zero_44bits shall be 0");
        if (seqInfo->seqInitErrReason&0x00000010) VLOG(WARN, "general_level_idc shall have one of level of Table A.1\n");
        if (seqInfo->seqInitErrReason&0x00000020) VLOG(WARN, "sps_max_dec_pic_buffering[i] <= MaxDpbSize\n");
        if (seqInfo->seqInitErrReason&0x00000040) VLOG(WARN, "trailing bits shall be 1000... pattern, 7.3.2.1\n");
        if (seqInfo->seqInitErrReason&0x00100000) VLOG(WARN, "Not supported or undefined profile: %d\n", seqInfo->profile);
        if (seqInfo->seqInitErrReason&0x00200000) VLOG(WARN, "Spec over level(%d)\n", seqInfo->level);
    }
}

void DisplayDecodedInformationForHevc(
    DecHandle      handle,
    Uint32         frameNo,
    BOOL           performance,
    Uint32         cyclePerTick,
    DecOutputInfo* decodedInfo
    )
{
    Int32 logLevel;
    QueueStatusInfo queueStatus;
    DecInfo* pDeclnfo = &handle->CodecInfo->decInfo;

    VPU_DecGiveCommand(handle, DEC_GET_QUEUE_STATUS, &queueStatus);

    if (decodedInfo == NULL) {
        if ( performance == TRUE ) {
            VLOG(INFO, "                                                                                                                    | FRAME  |  HOST | SEEK_S SEEK_E    SEEK  | PARSE_S PARSE_E  PARSE  | DEC_S  DEC_E   DEC   |\n");
            VLOG(INFO, "I    NO  T     POC     NAL DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE  VLC_BUFFER  WxH      SEQ  TEMP | CYCLE  |  TICK |  TICK   TICK     CYCLE |  TICK    TICK    CYCLE  |  TICK   TICK   CYCLE | TQ IQ\n");
            VLOG(INFO, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
        }
        else {
            VLOG(INFO, "I    NO  T     POC     NAL DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE  VLC_BUFFER  WxH      SEQ  TEMP  CYCLE  (   Seek,   Parse,    Dec)    TQ IQ\n");
            VLOG(INFO, "---------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
        }
    }
    else {
        Uint32 seekCycle, parseCycle, DecodedCycle;

        if ( pDeclnfo->firstCycleCheck == FALSE ) {
            decodedInfo->frameCycle   = (decodedInfo->decDecodeEndTick - decodedInfo->decHostCmdTick)*cyclePerTick;
            pDeclnfo->firstCycleCheck = TRUE;
        }
        else
            decodedInfo->frameCycle   = (decodedInfo->decDecodeEndTick - pDeclnfo->PrevDecodeEndTick)*cyclePerTick;
        seekCycle    = (decodedInfo->decSeekEndTick   - decodedInfo->decSeekStartTick)*cyclePerTick;
        parseCycle   = (decodedInfo->decParseEndTick  - decodedInfo->decParseStartTick)*cyclePerTick;
        DecodedCycle = (decodedInfo->decDecodeEndTick - decodedInfo->decDecodeStartTick)*cyclePerTick;
        pDeclnfo->PrevDecodeEndTick = decodedInfo->decDecodeEndTick;

        logLevel = (decodedInfo->decodingSuccess&0x01) == 0 ? WARN : INFO;
        // Print informations
        if ( performance == TRUE ) {
            VLOG(logLevel, "%02d %5d %d %4d(%4d) %3d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %08x %4dx%-4d %4d  %4d  %8d %8d (%6d,%6d,%8d) (%6d,%6d,%8d) (%6d,%6d,%8d) %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->h265Info.decodedPOC, decodedInfo->h265Info.displayPOC, decodedInfo->nalType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, (decodedInfo->bytePosFrameEnd - decodedInfo->bytePosFrameStart),
                decodedInfo->VLCOutBufBase,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->h265Info.temporalId,
                decodedInfo->frameCycle, decodedInfo->decHostCmdTick,
                decodedInfo->decSeekStartTick, decodedInfo->decSeekEndTick, seekCycle,
                decodedInfo->decParseStartTick, decodedInfo->decParseEndTick, parseCycle,
                decodedInfo->decDecodeStartTick, decodedInfo->decDecodeEndTick, DecodedCycle,
                queueStatus.totalQueueCount, queueStatus.instanceQueueCount);
        }
        else {
            VLOG(logLevel, "%02d %5d %d %4d(%4d) %3d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %08x %4dx%-4d %4d  %4d  %8d(%8d,%8d,%8d) %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->h265Info.decodedPOC, decodedInfo->h265Info.displayPOC, decodedInfo->nalType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, (decodedInfo->bytePosFrameEnd - decodedInfo->bytePosFrameStart),
                decodedInfo->VLCOutBufBase,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->h265Info.temporalId,
                decodedInfo->frameCycle, seekCycle, parseCycle, DecodedCycle,
                queueStatus.totalQueueCount, queueStatus.instanceQueueCount);
        }
        if (logLevel == ERR) {
            VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n", decodedInfo->errorReason, decodedInfo->errorReasonExt);
        }
        if (decodedInfo->numOfErrMBs) {
            VLOG(WARN, "\t>> ErrorBlock: %d\n", decodedInfo->numOfErrMBs);
        }
    }
}

void DisplayDecodedInformationForAVS2(
    DecHandle      handle,
    Uint32         frameNo,
    BOOL           performance,
    Uint32         cyclePerTick,
    DecOutputInfo* decodedInfo
    )
{
    Int32 logLevel;
    QueueStatusInfo queueStatus;
    DecInfo* pDeclnfo = &handle->CodecInfo->decInfo;

    VPU_DecGiveCommand(handle, DEC_GET_QUEUE_STATUS, &queueStatus);

    if (decodedInfo == NULL) {
        if ( performance == TRUE ) {
            VLOG(INFO, "                                                                                                                    | FRAME  |  HOST | SEEK_S SEEK_E    SEEK  | PARSE_S PARSE_E  PARSE  | DEC_S  DEC_E   DEC   |\n");
            VLOG(INFO, "I    NO  T     POC     NAL DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  TEMP | CYCLE  |  TICK |  TICK   TICK     CYCLE |  TICK    TICK    CYCLE  |  TICK   TICK   CYCLE | TQ IQ\n");
        } else {
            VLOG(INFO, "I    NO  T     POC     NAL DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  TEMP  CYCLE (Seek, Parse, Dec) TQ IQ\n");
        }
        VLOG(INFO, "------------------------------------------------------------------------------------------------------------\n");
    }
    else {
        Uint32 seekCycle, parseCycle, DecodedCycle;

        if ( pDeclnfo->firstCycleCheck == FALSE ) {
            decodedInfo->frameCycle   = (decodedInfo->decDecodeEndTick - decodedInfo->decHostCmdTick)*cyclePerTick;
            pDeclnfo->firstCycleCheck = TRUE;
        }
        else
            decodedInfo->frameCycle   = (decodedInfo->decDecodeEndTick - pDeclnfo->PrevDecodeEndTick)*cyclePerTick;
        seekCycle    = (decodedInfo->decSeekEndTick   - decodedInfo->decSeekStartTick)*cyclePerTick;
        parseCycle   = (decodedInfo->decParseEndTick  - decodedInfo->decParseStartTick)*cyclePerTick;
        DecodedCycle = (decodedInfo->decDecodeEndTick - decodedInfo->decDecodeStartTick)*cyclePerTick;
        pDeclnfo->PrevDecodeEndTick = decodedInfo->decDecodeEndTick;

        logLevel = (decodedInfo->decodingSuccess&0x01) == 0 ? ERR : TRACE;
        // Print informations
        if (performance == TRUE) {
            VLOG(logLevel, "%02d %5d %d %4d(%4d) %3d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d  %4d  %8d %8d (%6d,%6d,%8d) (%6d,%6d,%8d) (%6d,%6d,%8d) %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->avs2Info.decodedPOI, decodedInfo->avs2Info.displayPOI, decodedInfo->nalType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, (decodedInfo->bytePosFrameEnd - decodedInfo->bytePosFrameStart),
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->avs2Info.temporalId,
                decodedInfo->frameCycle, decodedInfo->decHostCmdTick,
                decodedInfo->decSeekStartTick, decodedInfo->decSeekEndTick, seekCycle,
                decodedInfo->decParseStartTick, decodedInfo->decParseEndTick, parseCycle,
                decodedInfo->decDecodeStartTick, decodedInfo->decDecodeEndTick, DecodedCycle,
                queueStatus.totalQueueCount, queueStatus.instanceQueueCount);
        }
        else {
            VLOG(logLevel, "%02d %5d %d %4d(%4d) %3d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d  %4d  %8d(%8d,%8d,%8d) %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->avs2Info.decodedPOI, decodedInfo->avs2Info.displayPOI, decodedInfo->nalType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, (decodedInfo->bytePosFrameEnd - decodedInfo->bytePosFrameStart),
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->avs2Info.temporalId,
                decodedInfo->frameCycle, seekCycle, parseCycle, DecodedCycle,
                queueStatus.totalQueueCount, queueStatus.instanceQueueCount);
        }
        if (logLevel == ERR) {
            VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n", decodedInfo->errorReason, decodedInfo->errorReasonExt);
        }
        if (decodedInfo->numOfErrMBs) {
            VLOG(WARN, "\t>> ErrorBlock: %d\n", decodedInfo->numOfErrMBs);
        }
    }
}

void DisplayDecodedInformationForVP9(
    DecHandle handle,
    Uint32 frameNo,
    BOOL           performance,
    Uint32         cyclePerTick,
    DecOutputInfo* decodedInfo)
{
    Int32 logLevel;
    QueueStatusInfo queueStatus;
    DecInfo* pDeclnfo = &handle->CodecInfo->decInfo;

    VPU_DecGiveCommand(handle, DEC_GET_QUEUE_STATUS, &queueStatus);

    if (decodedInfo == NULL) {
        // Print header
        if ( performance == TRUE ) {
            VLOG(INFO, "                                                                                                | FRAME  |  HOST | SEEK_S SEEK_E    SEEK  | PARSE_S PARSE_E  PARSE  | DEC_S  DEC_E   DEC   |\n");
            VLOG(INFO, "I    NO  T  DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  | CYCLE  |  TICK |  TICK   TICK     CYCLE |  TICK    TICK    CYCLE  |  TICK   TICK   CYCLE | TQ IQ\n");
        }
        else {
            VLOG(INFO, "I    NO  T  DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE    WxH      SEQ  CYCLE (Seek, Parse, Dec)  TQ IQ\n");
        }
        VLOG(INFO, "--------------------------------------------------------------------------------------------\n");
    }
    else {
        Uint32 seekCycle, parseCycle, DecodedCycle;

        if ( pDeclnfo->firstCycleCheck == FALSE ) {
            decodedInfo->frameCycle   = (decodedInfo->decDecodeEndTick - decodedInfo->decHostCmdTick)*cyclePerTick;
            pDeclnfo->firstCycleCheck = TRUE;
        }
        else
            decodedInfo->frameCycle   = (decodedInfo->decDecodeEndTick - pDeclnfo->PrevDecodeEndTick)*cyclePerTick;
        seekCycle    = (decodedInfo->decSeekEndTick   - decodedInfo->decSeekStartTick)*cyclePerTick;
        parseCycle   = (decodedInfo->decParseEndTick  - decodedInfo->decParseStartTick)*cyclePerTick;
        DecodedCycle = (decodedInfo->decDecodeEndTick - decodedInfo->decDecodeStartTick)*cyclePerTick;
        pDeclnfo->PrevDecodeEndTick = decodedInfo->decDecodeEndTick;
        logLevel = (decodedInfo->decodingSuccess&0x01) == 0 ? ERR : TRACE;
        // Print informations
        if (performance == TRUE) {
            VLOG(logLevel, "%02d %5d %d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d %8d %8d (%6d,%6d,%8d) (%6d,%6d,%8d) (%6d,%6d,%8d) %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, (decodedInfo->bytePosFrameEnd - decodedInfo->bytePosFrameStart),
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->frameCycle, decodedInfo->decHostCmdTick,
                decodedInfo->decSeekStartTick, decodedInfo->decSeekEndTick, seekCycle,
                decodedInfo->decParseStartTick, decodedInfo->decParseEndTick, parseCycle,
                decodedInfo->decDecodeStartTick, decodedInfo->decDecodeEndTick, DecodedCycle,
                queueStatus.totalQueueCount, queueStatus.instanceQueueCount);
        }
        else {
            VLOG(logLevel, "%02d %5d %d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %4dx%-4d %4d  %8d (%8d,%8d,%8d) %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, (decodedInfo->bytePosFrameEnd - decodedInfo->bytePosFrameStart),
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->frameCycle, seekCycle, parseCycle, DecodedCycle,
                queueStatus.totalQueueCount, queueStatus.instanceQueueCount);
        }
        if (logLevel == ERR) {
            VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n", decodedInfo->errorReason, decodedInfo->errorReasonExt);
        }
        if (decodedInfo->numOfErrMBs) {
            VLOG(WARN, "\t>> ErrorBlock: %d\n", decodedInfo->numOfErrMBs);
        }
        if (decodedInfo->warnInfo) {
            VLOG(WARN, "\t>> WARNING: 0x%x\n", decodedInfo->warnInfo);
        }
    }
}

void DisplayDecodedInformationForAVC(
    DecHandle      handle,
    Uint32         frameNo,
    BOOL           performance,
    Uint32         cyclePerTick,
    DecOutputInfo* decodedInfo)
{
    Int32 logLevel;
    QueueStatusInfo queueStatus;
    DecInfo* pDeclnfo = &handle->CodecInfo->decInfo;

    VPU_DecGiveCommand(handle, DEC_GET_QUEUE_STATUS, &queueStatus);

    if (decodedInfo == NULL) {
        // Print header
        if ( performance == TRUE ) {
            VLOG(INFO, "                                                                                              | FRAME  |  HOST | SEEK_S SEEK_E    SEEK  | PARSE_S PARSE_E  PARSE  | DEC_S  DEC_E   DEC   |\n");
            VLOG(INFO, "I    NO  T  DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE  VLC_BUFFER  WxH     SEQ | CYCLE  |  TICK |  TICK   TICK     CYCLE |  TICK    TICK    CYCLE  |  TICK   TICK   CYCLE | TQ IQ\n");
        }
        else {
            VLOG(INFO, "I    NO  T  DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE  VLC_BUFFER  WxH     SEQ   CYCLE  (  Seek,   Parse,   Dec)     TQ IQ\n");
        }
        VLOG(INFO, "-----------------------------------------------------------------------------------------------------------------------------------------\n");
    }
    else {
        Uint32 seekCycle, parseCycle, DecodedCycle;

        if ( pDeclnfo->firstCycleCheck == FALSE ) {
            decodedInfo->frameCycle   = (decodedInfo->decDecodeEndTick - decodedInfo->decHostCmdTick)*cyclePerTick;
            pDeclnfo->firstCycleCheck = TRUE;
        }
        else
            decodedInfo->frameCycle   = (decodedInfo->decDecodeEndTick - pDeclnfo->PrevDecodeEndTick)*cyclePerTick;
        seekCycle    = (decodedInfo->decSeekEndTick   - decodedInfo->decSeekStartTick)*cyclePerTick;
        parseCycle   = (decodedInfo->decParseEndTick  - decodedInfo->decParseStartTick)*cyclePerTick;
        DecodedCycle = (decodedInfo->decDecodeEndTick - decodedInfo->decDecodeStartTick)*cyclePerTick;
        pDeclnfo->PrevDecodeEndTick = decodedInfo->decDecodeEndTick;
        logLevel = (decodedInfo->decodingSuccess&0x01) == 0 ? ERR : INFO;
        // Print informations
        if (performance == TRUE) {
            VLOG(logLevel, "%02d %5d %d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %08x %4dx%-4d %3d  %8d %8d (%6d,%6d,%8d) (%6d,%6d,%8d) (%6d,%6d,%8d) %d %d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, (decodedInfo->bytePosFrameEnd - decodedInfo->bytePosFrameStart),
                decodedInfo->VLCOutBufBase,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->frameCycle, decodedInfo->decHostCmdTick,
                decodedInfo->decSeekStartTick, decodedInfo->decSeekEndTick, seekCycle,
                decodedInfo->decParseStartTick, decodedInfo->decParseEndTick, parseCycle,
                decodedInfo->decDecodeStartTick, decodedInfo->decDecodeEndTick, DecodedCycle,
                queueStatus.totalQueueCount, queueStatus.instanceQueueCount
                );
        }
        else {
            VLOG(logLevel, "%02d %5d %d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %08x %4dx%-4d %3d  %8d (%8d,%8d,%8d) %2d %2d\n",
                handle->instIndex, frameNo, decodedInfo->picType,
                decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
                decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
                decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
                decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, (decodedInfo->bytePosFrameEnd - decodedInfo->bytePosFrameStart),
                decodedInfo->VLCOutBufBase,
                decodedInfo->dispPicWidth, decodedInfo->dispPicHeight, decodedInfo->sequenceNo,
                decodedInfo->frameCycle, seekCycle, parseCycle, DecodedCycle,
                queueStatus.totalQueueCount, queueStatus.instanceQueueCount
                );
        }

        if (logLevel == ERR) {
            VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n", decodedInfo->errorReason, decodedInfo->errorReasonExt);
        }
        if (decodedInfo->numOfErrMBs) {
            VLOG(WARN, "\t>> ErrorBlock: %d\n", decodedInfo->numOfErrMBs);
        }
        if (decodedInfo->warnInfo) {
            VLOG(WARN, "\t>> WARNING: 0x%x\n", decodedInfo->warnInfo);
        }
    }
}

void DisplayDecodedInformationCommon(
    DecHandle      handle,
    Uint32         frameNo,
    BOOL           performance,
    DecOutputInfo* decodedInfo)
{
    Int32 logLevel;

    if (decodedInfo == NULL) {
        // Print header
        VLOG(TRACE, "I    NO  T  DECO   DISP  DISPFLAG  RD_PTR   WR_PTR  FRM_START FRM_END FRM_SIZE WxH  \n");
        VLOG(TRACE, "---------------------------------------------------------------------------\n");
    }
    else {
        VpuRect rc    = decodedInfo->rcDisplay;
        Uint32 width  = rc.right - rc.left;
        Uint32 height = rc.bottom - rc.top;
        logLevel = (decodedInfo->decodingSuccess&0x01) == 0 ? WARN : INFO;
        // Print informations
        VLOG(logLevel, "%02d %5d %d %2d(%2d) %2d(%2d) %08x %08x %08x %08x %08x %8d %dx%d\n",
            handle->instIndex, frameNo, decodedInfo->picType,
            decodedInfo->indexFrameDecoded, decodedInfo->indexFrameDecodedForTiled,
            decodedInfo->indexFrameDisplay, decodedInfo->indexFrameDisplayForTiled,
            decodedInfo->frameDisplayFlag,decodedInfo->rdPtr, decodedInfo->wrPtr,
            decodedInfo->bytePosFrameStart, decodedInfo->bytePosFrameEnd, (decodedInfo->bytePosFrameEnd - decodedInfo->bytePosFrameStart),
            width, height);
        if (logLevel == ERR) {
            VLOG(ERR, "\t>>ERROR REASON: 0x%08x(0x%08x)\n", decodedInfo->errorReason, decodedInfo->errorReasonExt);
        }
        if (decodedInfo->numOfErrMBs) {
            VLOG(WARN, "\t>> ErrorBlock: %d\n", decodedInfo->numOfErrMBs);
        }
    }
}

/**
* \brief                   Print out decoded information such like RD_PTR, WR_PTR, PIC_TYPE, ..
* \param   decodedInfo     If this parameter is not NULL then print out decoded informations
*                          otherwise print out header.
*/
void
    DisplayDecodedInformation(
    DecHandle      handle,
    CodStd         codec,
    Uint32         frameNo,
    DecOutputInfo* decodedInfo,
    ...
    )
{
    int performance = FALSE;
    int cyclePerTick = 32768;
    va_list         ap;

    va_start(ap, decodedInfo);
    performance = va_arg(ap, Uint32);
    cyclePerTick = va_arg(ap, Uint32);
    va_end(ap);
    switch (codec)
    {
    case STD_HEVC:
        DisplayDecodedInformationForHevc(handle, frameNo, performance, cyclePerTick, decodedInfo);
        break;
    case STD_VP9:
        DisplayDecodedInformationForVP9(handle, frameNo, performance, cyclePerTick, decodedInfo);
        break;
    case STD_AVS2:
        DisplayDecodedInformationForAVS2(handle, frameNo, performance, cyclePerTick, decodedInfo);
        break;
    case STD_AVC:
        DisplayDecodedInformationForAVC(handle, frameNo, performance, cyclePerTick, decodedInfo);
        break;
    default:
        DisplayDecodedInformationCommon(handle, frameNo, performance, decodedInfo);
        break;
    }

    return;
}




void replace_character(char* str,
    char  c,
    char  r)
{
    int i=0;
    int len;
    len = strlen(str);

    for(i=0; i<len; i++)
    {
        if (str[i] == c)
            str[i] = r;
    }
}



void ChangePathStyle(
    char *str
    )
{
}

static Uint32 Format2Bitdepth(FrameBufferFormat fmt)
{
    Uint32 bpp = 0;

    switch (fmt) {
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_422_P10_16BIT_MSB:
    case FORMAT_422_P10_16BIT_LSB:
    case FORMAT_422_P10_32BIT_MSB:
    case FORMAT_422_P10_32BIT_LSB:
    case FORMAT_YUYV_P10_16BIT_MSB:
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_VYUY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
        bpp = 10;
        break;
    default:
        bpp = 8;
    }

    return bpp;
}

void ReleaseVideoMemory(
    Uint32          coreIndex,
    vpu_buffer_t*   memoryArr,
    Uint32          count
    )
{
    Uint32      idx;

    vdi_lock(coreIndex);
    for (idx=0; idx<count; idx++) {
        if (memoryArr[idx].size)
            vdi_free_dma_memory(coreIndex, &memoryArr[idx]);
    }
    vdi_unlock(coreIndex);
}

BOOL AllocateDecLinearFrameBuffer(DecHandle handle, Uint32 width, Uint32 height, Uint32 align, FrameBufferFormat fmt, BOOL semiPlanar, FrameBuffer* fb)
{
    vpu_buffer_t    vb        = {0};
    Uint32          stride;
    Uint32          fbWidth, fbHeight;
    Uint32          lumaSize, chromaSize, fbSize;
    CodStd          codec     = VPU_HANDLE_CORE_INDEX(handle);
    ProductId       product   = VPU_HANDLE_PRODUCT_ID(handle);
    Uint32          coreIndex = VPU_HANDLE_CORE_INDEX(handle);

    switch (codec) {
    case STD_VP9:
        fbWidth  = VPU_CEIL(width, 64);
        fbHeight = VPU_CEIL(height, 64);
        break;
    case STD_AVS2:
        fbWidth  = width;
        fbHeight = VPU_CEIL(height, 8);
        break;
    default:
        fbWidth  = width;
        fbHeight = height;
        break;
    }

    fbWidth = VPU_CEIL(fbWidth, align);
#ifdef FIX_SIGCHA_56
    fbHeight = VPU_CEIL(fbHeight, align);
#endif
    stride      = CalcStride(fbWidth, height, fmt, semiPlanar, LINEAR_FRAME_MAP, FALSE);
    lumaSize    = CalcLumaSize(product, stride, fbHeight, fmt, semiPlanar, LINEAR_FRAME_MAP, NULL);
    chromaSize  = CalcChromaSize(product, stride, fbHeight, fmt, semiPlanar, LINEAR_FRAME_MAP, NULL);
    fbSize      = lumaSize + chromaSize * 2;

    vb.size = fbSize;
    if (vdi_allocate_dma_memory(coreIndex, &vb) < 0) {
        VLOG(ERR, "%s:%d fail to allocate frame buffer size(%d)\n", __FUNCTION__, __LINE__, vb.size);
        return FALSE;
    }
    osal_memset((void*)fb, 0x00, sizeof(FrameBuffer));
    fb->bufY            = vb.phys_addr;
    fb->bufCb           = vb.phys_addr + lumaSize;
    fb->bufCr           = (semiPlanar == TRUE) ? -1 : fb->bufCb + chromaSize;
    fb->size            = fbSize;
    fb->width           = width;
    fb->height          = height;
    fb->stride          = stride;
    fb->cbcrInterleave  = semiPlanar;
    fb->updateFbInfo    = FALSE;
    fb->lumaBitDepth    = Format2Bitdepth(fmt);
    fb->chromaBitDepth  = fb->lumaBitDepth;
    fb->format          = fmt;

    return TRUE;
}

BOOL AllocateDecFrameBuffer(
    DecHandle       decHandle,
    TestDecConfig*  config,
    Uint32          tiledFbCount,
    Uint32          linearFbCount,
    FrameBuffer*    retFbArray,
    vpu_buffer_t*   retFbAddrs,
    Uint32*         retStride
    )
{
    Uint32                  framebufSize;
    Uint32                  totalFbCount, linearFbStartIdx = 0;
    Uint32                  coreIndex = VPU_HANDLE_CORE_INDEX(decHandle);;
    Uint32                  idx;
    FrameBufferFormat       format = config->wtlFormat;
    DecInitialInfo          seqInfo;
    FrameBufferAllocInfo    fbAllocInfo;
    RetCode                 ret;
    vpu_buffer_t*           pvb;
    size_t                  framebufStride;
    size_t                  framebufHeight;
    Uint32                  productId;
    DRAMConfig*             pDramCfg        = NULL;
    DRAMConfig              dramCfg         = {0};
    int                     height;
    int                     maxV;   // maxV = max_vertical_mv + ctu(MB) height + additional ref pixel line(=8)
    productId = VPU_HANDLE_PRODUCT_ID(decHandle);
    VPU_DecGiveCommand(decHandle, DEC_GET_SEQ_INFO, (void*)&seqInfo);

    if (productId == PRODUCT_ID_960) {
        pDramCfg = &dramCfg;
        ret = VPU_DecGiveCommand(decHandle, GET_DRAM_CONFIG, pDramCfg);
    }
    height = seqInfo.picHeight;

    /* Inplace buffer mode :
            height = picHeight + ceil4(maxVerticalMV + 8(reference pixel line))
     */
    maxV   = config->maxVerticalMV + 8;
    height = (config->inPlaceBufferMode == INPLACE_MODE_OFF) ? seqInfo.picHeight : (seqInfo.picHeight + VPU_ALIGN4(maxV));
    totalFbCount    = tiledFbCount + linearFbCount;
    linearFbStartIdx= tiledFbCount;

    if (PRODUCT_ID_W_SERIES(productId)) {
        format = (seqInfo.lumaBitdepth > 8 || seqInfo.chromaBitdepth > 8) ? FORMAT_420_P10_16BIT_LSB : FORMAT_420;
    }

    if (config->bitFormat == STD_VP9) {
        framebufStride = CalcStride(VPU_ALIGN64(seqInfo.picWidth), height, format, config->cbcrInterleave, config->mapType, TRUE);
        framebufHeight = VPU_ALIGN64(height);
        framebufSize   = VPU_GetFrameBufSize(decHandle->coreIdx, framebufStride, framebufHeight,
            config->mapType, format, config->cbcrInterleave, NULL);
        *retStride     = framebufStride;
    }
    else if (config->bitFormat == STD_AVS2) {
        framebufStride = CalcStride(seqInfo.picWidth, height, format, config->cbcrInterleave, config->mapType, FALSE);
        framebufHeight = VPU_ALIGN8(height);
        framebufSize   = VPU_GetFrameBufSize(decHandle->coreIdx, framebufStride, framebufHeight,
            config->mapType, format, config->cbcrInterleave, pDramCfg);
        *retStride     = framebufStride;
    }
    else {
        *retStride     = VPU_ALIGN32(seqInfo.picWidth);
        framebufStride = CalcStride(seqInfo.picWidth, height, format, config->cbcrInterleave, config->mapType, FALSE);
        framebufHeight = VPU_ALIGN32(height);
        framebufSize   = VPU_GetFrameBufSize(decHandle->coreIdx, framebufStride, framebufHeight,
                                             config->mapType, format, config->cbcrInterleave, pDramCfg);
    }

    osal_memset((void*)&fbAllocInfo, 0x00, sizeof(fbAllocInfo));
    osal_memset((void*)retFbArray,   0x00, sizeof(FrameBuffer)*totalFbCount);
    fbAllocInfo.format          = format;
    fbAllocInfo.cbcrInterleave  = config->cbcrInterleave;
    fbAllocInfo.mapType         = config->mapType;
    fbAllocInfo.stride          = framebufStride;
    fbAllocInfo.height          = framebufHeight;
    fbAllocInfo.size            = framebufSize;
    fbAllocInfo.lumaBitDepth    = seqInfo.lumaBitdepth;
    fbAllocInfo.chromaBitDepth  = seqInfo.chromaBitdepth;
    fbAllocInfo.num             = tiledFbCount;
    fbAllocInfo.endian          = VDI_128BIT_LITTLE_ENDIAN;    // FBC endian is fixed.
    fbAllocInfo.type            = FB_TYPE_CODEC;
    osal_memset((void*)retFbAddrs, 0x00, sizeof(vpu_buffer_t)*totalFbCount);
    APIDPRINT("ALLOC MEM - FBC data\n");
    vdi_lock(coreIndex);
    for (idx=0; idx<tiledFbCount; idx++) {
        pvb = &retFbAddrs[idx];
        pvb->size = framebufSize;
        if (vdi_allocate_dma_memory(coreIndex, pvb) < 0) {
            VLOG(ERR, "%s:%d fail to allocate frame buffer\n", __FUNCTION__, __LINE__);
            vdi_unlock(coreIndex);
            ReleaseVideoMemory(coreIndex, retFbAddrs, totalFbCount);
            return FALSE;
        }
        retFbArray[idx].bufY  = pvb->phys_addr;
        retFbArray[idx].bufCb = (PhysicalAddress)-1;
        retFbArray[idx].bufCr = (PhysicalAddress)-1;
        retFbArray[idx].updateFbInfo = TRUE;
        retFbArray[idx].size  = framebufSize;
        retFbArray[idx].width = seqInfo.picWidth;
    }
    vdi_unlock(coreIndex);

    if (tiledFbCount != 0) {
        if ((ret=VPU_DecAllocateFrameBuffer(decHandle, fbAllocInfo, retFbArray)) != RETCODE_SUCCESS) {
            VLOG(ERR, "%s:%d failed to VPU_DecAllocateFrameBuffer(), ret(%d)\n",
                __FUNCTION__, __LINE__, ret);
            ReleaseVideoMemory(coreIndex, retFbAddrs, totalFbCount);
            return FALSE;
        }
    }

    if (config->enableWTL == TRUE || linearFbCount != 0) {
        size_t  linearStride;
        size_t  picWidth;
        size_t  picHeight;
        size_t  fbHeight;
        Uint32   mapType = LINEAR_FRAME_MAP;
        FrameBufferFormat outFormat = config->wtlFormat;
        picWidth  = seqInfo.picWidth;
        picHeight = seqInfo.picHeight;
        fbHeight  = picHeight;

        if (config->bitFormat == STD_VP9) {
            fbHeight = VPU_ALIGN64(picHeight);
        }
        else if (config->bitFormat == STD_AVS2) {
            fbHeight = VPU_ALIGN8(picHeight);
        }
        else if (productId == PRODUCT_ID_960 || productId == PRODUCT_ID_980)
        {
            fbHeight = VPU_ALIGN32(picHeight);
        }
        if (config->scaleDownWidth > 0 || config->scaleDownHeight > 0) {
            ScalerInfo sclInfo;
            VPU_DecGiveCommand(decHandle, DEC_GET_SCALER_INFO, (void*)&sclInfo);
            if (sclInfo.enScaler == TRUE) {
                picWidth  = sclInfo.scaleWidth;
                picHeight = sclInfo.scaleHeight;
                if (config->bitFormat == STD_VP9) {
                    fbHeight  = VPU_ALIGN64(picHeight);
                }
                else {
                    fbHeight  = VPU_CEIL(picHeight, 2);
                }
            }
        }
        if (config->bitFormat == STD_VP9) {
            linearStride = CalcStride(VPU_ALIGN64(picWidth), picHeight, outFormat, config->cbcrInterleave, (TiledMapType)mapType, TRUE);
        }
        else {
            linearStride = CalcStride(picWidth, picHeight, outFormat, config->cbcrInterleave, (TiledMapType)mapType, FALSE);
        }
        framebufSize = VPU_GetFrameBufSize(coreIndex, linearStride, fbHeight, (TiledMapType)mapType, outFormat, config->cbcrInterleave, pDramCfg);

        vdi_lock(coreIndex);
        for (idx=linearFbStartIdx; idx<totalFbCount; idx++) {
            pvb = &retFbAddrs[idx];
            pvb->size = framebufSize;
            if (vdi_allocate_dma_memory(coreIndex, pvb) < 0) {
                vdi_unlock(coreIndex);
                VLOG(ERR, "%s:%d fail to allocate frame buffer\n", __FUNCTION__, __LINE__);
                ReleaseVideoMemory(coreIndex, retFbAddrs, totalFbCount);
                return FALSE;
            }
            retFbArray[idx].bufY  = pvb->phys_addr;
            retFbArray[idx].bufCb = (PhysicalAddress)-1;
            retFbArray[idx].bufCr = (PhysicalAddress)-1;
            retFbArray[idx].updateFbInfo = TRUE;
            retFbArray[idx].size  = framebufSize;
            retFbArray[idx].width = picWidth;
        }
        vdi_unlock(coreIndex);

        fbAllocInfo.nv21    = config->nv21;
        fbAllocInfo.format  = outFormat;
        fbAllocInfo.num     = linearFbCount;
        fbAllocInfo.mapType = (TiledMapType)mapType;
        fbAllocInfo.stride  = linearStride;
        fbAllocInfo.height  = fbHeight;
        ret = VPU_DecAllocateFrameBuffer(decHandle, fbAllocInfo, &retFbArray[linearFbStartIdx]);
        if (ret != RETCODE_SUCCESS) {
            VLOG(ERR, "%s:%d failed to VPU_DecAllocateFrameBuffer() ret:%d\n",
                __FUNCTION__, __LINE__, ret);
            ReleaseVideoMemory(coreIndex, retFbAddrs, totalFbCount);
            return FALSE;
        }
    }

    return TRUE;
}

#if defined(_WIN32) || defined(__MSDOS__)
#define DOS_FILESYSTEM
#define IS_DIR_SEPARATOR(__c) ((__c == '/') || (__c == '\\'))
#else
/* UNIX style */
#define IS_DIR_SEPARATOR(__c) (__c == '/')
#endif

char* GetDirname(
    const char* path
    )
{
    int length;
    int i;
    char* upper_dir;

    if (path == NULL) return NULL;

    length = strlen(path);
    for (i=length-1; i>=0; i--) {
        if (IS_DIR_SEPARATOR(path[i])) break;
    }

    if (i<0) {
        upper_dir = strdup(".");
    } else {
        upper_dir = strdup(path);
        upper_dir[i] = 0;
    }

    return upper_dir;
}

char* GetBasename(
    const char* pathname
    )
{
    const char* base = NULL;
    const char* p    = pathname;

    if (p == NULL) {
        return NULL;
    }

#if defined(DOS_FILESYSTEM)
    if (isalpha((int)p[0]) && p[1] == ':') {
        p += 2;
    }
#endif

    for (base=p; *p; p++) {//lint !e443
        if (IS_DIR_SEPARATOR(*p)) {
            base = p+1;
        }
    }

    return (char*)base;
}

char* GetFileExtension(
    const char* filename
    )
{
    Int32      len;
    Int32      i;

    len = strlen(filename);
    for (i=len-1; i>=0; i--) {
        if (filename[i] == '.') {
            return (char*)&filename[i+1];
        }
    }

    return NULL;
}

void byte_swap(unsigned char* data, int len)
{
    Uint8 temp;
    Int32 i;

    for (i=0; i<len; i+=2) {
        temp      = data[i];
        data[i]   = data[i+1];
        data[i+1] = temp;
    }
}

void word_swap(unsigned char* data, int len)
{
    Uint16  temp;
    Uint16* ptr = (Uint16*)data;
    Int32   i, size = len/(int)sizeof(Uint16);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

void dword_swap(unsigned char* data, int len)
{
    Uint32  temp;
    Uint32* ptr = (Uint32*)data;
    Int32   i, size = len/(int)sizeof(Uint32);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

void lword_swap(unsigned char* data, int len)
{
    Uint64  temp;
    Uint64* ptr = (Uint64*)data;
    Int32   i, size = len/(int)sizeof(Uint64);

    for (i=0; i<size; i+=2) {
        temp      = ptr[i];
        ptr[i]   = ptr[i+1];
        ptr[i+1] = temp;
    }
}

BOOL IsEndOfFile(FILE* fp)
{
    BOOL  result = FALSE;
    Int32 idx  = 0;
    char  cTemp;

    // Check current fp pos
    if (osal_feof(fp) != 0) {
        result = TRUE;
    }

    // Check next fp pos
    // Ignore newline character
    do {
        cTemp = fgetc(fp);
        idx++;

        if (osal_feof(fp) != 0) {
            result = TRUE;
            break;
        }
    } while (cTemp == '\n' || cTemp == '\r');

    // Revert fp pos
    idx *= (-1);
    osal_fseek(fp, idx, SEEK_CUR);

    return result;
}

BOOL CalcYuvSize(
    Int32   format,
    Int32   picWidth,
    Int32   picHeight,
    Int32   cbcrInterleave,
    size_t  *lumaSize,
    size_t  *chromaSize,
    size_t  *frameSize,
    Int32   *bitDepth,
    Int32   *packedFormat,
    Int32   *yuv3p4b)
{
    Int32   temp_picWidth;
    Int32   chromaWidth;

    if ( bitDepth != 0)
        *bitDepth = 0;
    if ( packedFormat != 0)
        *packedFormat = 0;
    if ( yuv3p4b != 0)
        *yuv3p4b = 0;

    if (!lumaSize || !chromaSize || !frameSize )
        return FALSE;

    switch (format)
    {
    case FORMAT_420:
        *lumaSize = picWidth * picHeight;
        *chromaSize = picWidth * picHeight / 2;
        *frameSize = picWidth * picHeight * 3 /2;
        break;
    case FORMAT_YUYV:
    case FORMAT_YVYU:
    case FORMAT_UYVY:
    case FORMAT_VYUY:
        if ( packedFormat != 0)
            *packedFormat = 1;
        *lumaSize = picWidth * picHeight;
        *chromaSize = picWidth * picHeight;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_224:
        *lumaSize = picWidth * picHeight;
        *chromaSize = picWidth * picHeight;
        *frameSize = picWidth * picHeight * 4 / 2;
        break;
    case FORMAT_422:
        *lumaSize = picWidth * picHeight;
        *chromaSize = picWidth * picHeight;
        *frameSize = picWidth * picHeight * 4 / 2;
        break;
    case FORMAT_444:
        *lumaSize  = picWidth * picHeight;
        *chromaSize = picWidth * picHeight * 2;
        *frameSize = picWidth * picHeight * 3;
        break;
    case FORMAT_400:
        *lumaSize  = picWidth * picHeight;
        *chromaSize = 0;
        *frameSize = picWidth * picHeight;
        break;
    case FORMAT_422_P10_16BIT_MSB:
    case FORMAT_422_P10_16BIT_LSB:
        if ( bitDepth != NULL) {
            *bitDepth = 10;
        }
        *lumaSize = picWidth * picHeight * 2;
        *chromaSize = *lumaSize;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_420_P10_16BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        *lumaSize = picWidth * picHeight * 2;
        *chromaSize = picWidth * picHeight;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_YUYV_P10_16BIT_MSB:   // 4:2:2 10bit packed
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        if ( packedFormat != 0)
            *packedFormat = 1;
        *lumaSize = picWidth * picHeight * 2;
        *chromaSize = picWidth * picHeight * 2;
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_420_P10_32BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        if ( yuv3p4b != 0)
            *yuv3p4b = 1;
        temp_picWidth = VPU_ALIGN32(picWidth);
        chromaWidth = ((VPU_ALIGN16(temp_picWidth/2*(1<<cbcrInterleave))+2)/3*4);
        if ( cbcrInterleave == 1)
        {
            *lumaSize = (temp_picWidth+2)/3*4 * picHeight;
            *chromaSize = chromaWidth * picHeight/2;
        } else {
            *lumaSize = (temp_picWidth+2)/3*4 * picHeight;
            *chromaSize = chromaWidth * picHeight/2*2;
        }
        *frameSize = *lumaSize + *chromaSize;
        break;
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
        if ( bitDepth != 0)
            *bitDepth = 10;
        if ( packedFormat != 0)
            *packedFormat = 1;
        if ( yuv3p4b != 0)
            *yuv3p4b = 1;
        *frameSize = ((picWidth*2)+2)/3*4 * picHeight;
        *lumaSize = *frameSize/2;
        *chromaSize = *frameSize/2;
        break;
    default:
        *frameSize = picWidth * picHeight * 3 / 2;
        VLOG(ERR, "%s:%d Not supported format(%d)\n", __FILE__, __LINE__, format);
        return FALSE;
    }
    return TRUE;
}

FrameBufferFormat GetPackedFormat (
    int srcBitDepth,
    int packedType,
    int p10bits,
    int msb)
{
    FrameBufferFormat format = FORMAT_YUYV;

    // default pixel format = P10_16BIT_LSB (p10bits = 16, msb = 0)
    if (srcBitDepth == 8) {

        switch(packedType) {
        case PACKED_YUYV:
            format = FORMAT_YUYV;
            break;
        case PACKED_YVYU:
            format = FORMAT_YVYU;
            break;
        case PACKED_UYVY:
            format = FORMAT_UYVY;
            break;
        case PACKED_VYUY:
            format = FORMAT_VYUY;
            break;
        default:
            format = FORMAT_ERR;
        }
    }
    else if (srcBitDepth == 10) {
        switch(packedType) {
        case PACKED_YUYV:
            if (p10bits == 16) {
                format = (msb == 0) ? FORMAT_YUYV_P10_16BIT_LSB : FORMAT_YUYV_P10_16BIT_MSB;
            }
            else if (p10bits == 32) {
                format = (msb == 0) ? FORMAT_YUYV_P10_32BIT_LSB : FORMAT_YUYV_P10_32BIT_MSB;
            }
            else {
                format = FORMAT_ERR;
            }
            break;
        case PACKED_YVYU:
            if (p10bits == 16) {
                format = (msb == 0) ? FORMAT_YVYU_P10_16BIT_LSB : FORMAT_YVYU_P10_16BIT_MSB;
            }
            else if (p10bits == 32) {
                format = (msb == 0) ? FORMAT_YVYU_P10_32BIT_LSB : FORMAT_YVYU_P10_32BIT_MSB;
            }
            else {
                format = FORMAT_ERR;
            }
            break;
        case PACKED_UYVY:
            if (p10bits == 16) {
                format = (msb == 0) ? FORMAT_UYVY_P10_16BIT_LSB : FORMAT_UYVY_P10_16BIT_MSB;
            }
            else if (p10bits == 32) {
                format = (msb == 0) ? FORMAT_UYVY_P10_32BIT_LSB : FORMAT_UYVY_P10_32BIT_MSB;
            }
            else {
                format = FORMAT_ERR;
            }
            break;
        case PACKED_VYUY:
            if (p10bits == 16) {
                format = (msb == 0) ? FORMAT_VYUY_P10_16BIT_LSB : FORMAT_VYUY_P10_16BIT_MSB;
            }
            else if (p10bits == 32) {
                format = (msb == 0) ? FORMAT_VYUY_P10_32BIT_LSB : FORMAT_VYUY_P10_32BIT_MSB;
            }
            else {
                format = FORMAT_ERR;
            }
            break;
        default:
            format = FORMAT_ERR;
        }
    }
    else {
        format = FORMAT_ERR;
    }

    return format;
}






#if defined(PLATFORM_NON_OS) || defined (PLATFORM_LINUX)
struct option* ConvertOptions(
    struct OptionExt*   cnmOpt,
    Uint32              nItems
    )
{
    struct option*  opt;
    Uint32          i;

    opt = (struct option*)osal_malloc(sizeof(struct option) * nItems);
    if (opt == NULL) {
        return NULL;
    }

    for (i=0; i<nItems; i++) {
        osal_memcpy((void*)&opt[i], (void*)&cnmOpt[i], sizeof(struct option));
    }

    return opt;
}
#endif

#ifdef PLATFORM_LINUX
int mkdir_recursive(
    char *path,
    mode_t omode
    )
{
    struct stat sb;
    mode_t numask, oumask;
    int first, last, retval;
    char *p;

    p = path;
    oumask = 0;
    retval = 0;
    if (p[0] == '/')        /* Skip leading '/'. */
        ++p;
    for (first = 1, last = 0; !last ; ++p) {//lint !e441 !e443
        if (p[0] == '\0')
            last = 1;
        else if (p[0] != '/')
            continue;
        *p = '\0';
        if (p[1] == '\0')
            last = 1;
        if (first) {
            /*
            * POSIX 1003.2:
            * For each dir operand that does not name an existing
            * directory, effects equivalent to those cased by the
            * following command shall occcur:
            *
            * mkdir -p -m $(umask -S),u+wx $(dirname dir) &&
            *    mkdir [-m mode] dir
            *
            * We change the user's umask and then restore it,
            * instead of doing chmod's.
            */
            oumask = umask(0);
            numask = oumask & ~(S_IWUSR | S_IXUSR);
            (void)umask(numask);
            first = 0;
        }
        if (last)
            (void)umask(oumask);
        if (mkdir(path, last ? omode : S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
            if (errno == EEXIST || errno == EISDIR) {
                if (stat(path, &sb) < 0) {
                    VLOG(INFO, "%s", path);
                    retval = 1;
                    break;
                } else if (!S_ISDIR(sb.st_mode)) {
                    if (last)
                        errno = EEXIST;
                    else
                        errno = ENOTDIR;
                    VLOG(INFO, "%s", path);
                    retval = 1;
                    break;
                }
            } else {
                VLOG(INFO, "%s", path);
                retval = 1;
                break;
            }
        } else {
            VLOG(INFO, "%s", path);
            chmod(path, omode);
        }
        if (!last)
            *p = '/';
    }
    if (!first && !last)
        (void)umask(oumask);
    return (retval);
}
#endif

int file_exist(
    char* path
    )
{
#ifdef _MSC_VER
    DWORD   attributes;
    char    temp[4096];
    LPCTSTR lp_path = (LPCTSTR)temp;

    if (path == NULL) {
        return FALSE;
    }

    strcpy(temp, path);
    replace_character(temp, '/', '\\');
    attributes = GetFileAttributes(lp_path);
    return (attributes != (DWORD)-1);
#else
    return !access(path, F_OK);
#endif
}

BOOL MkDir(
    char* path
    )
{
#if defined(PLATFORM_NON_OS) || defined(PLATFORM_QNX)
    /* need to implement */
    return FALSE;
#else
#ifdef _MSC_VER
    char cmd[4096];
#endif
    if (file_exist(path))
        return TRUE;

#ifdef _MSC_VER
    sprintf(cmd, "mkdir %s", path);
    replace_character(cmd, '/', '\\');
    if (system(cmd)) {
        return FALSE;
    }
    return TRUE;
#else
    return mkdir_recursive(path, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
#endif
}

void GetUserData(Int32 coreIdx, Uint8* pBase, vpu_buffer_t vbUserData, DecOutputInfo outputInfo)
{
    int idx;
    user_data_entry_t* pEntry = (user_data_entry_t*)pBase;

    VpuReadMem(coreIdx, vbUserData.phys_addr, pBase, vbUserData.size, VPU_USER_DATA_ENDIAN);
    VLOG(INFO, "===== USER DATA(SEI OR VUI) : NUM(%d) =====\n", outputInfo.decOutputExtData.userDataNum);

    for (idx=0; idx<32; idx++) {
        if (outputInfo.decOutputExtData.userDataHeader&(1<<idx)) {
            VLOG(INFO, "\nUSERDATA INDEX: %02d offset: %8d size: %d\n", idx, pEntry[idx].offset, pEntry[idx].size);

            if (idx == H265_USERDATA_FLAG_MASTERING_COLOR_VOL) {
                h265_mastering_display_colour_volume_t* mastering;
                int i;

                mastering = (h265_mastering_display_colour_volume_t*)(pBase + pEntry[H265_USERDATA_FLAG_MASTERING_COLOR_VOL].offset);
                VLOG(INFO, " MASTERING DISPLAY COLOR VOLUME\n");
                for (i=0; i<3; i++) {
                    VLOG(INFO, " PRIMARIES_X%d : %10d PRIMARIES_Y%d : %10d\n", i, mastering->display_primaries_x[i], i, mastering->display_primaries_y[i]);
                }
                VLOG(INFO, " WHITE_POINT_X: %10d WHITE_POINT_Y: %10d\n", mastering->white_point_x, mastering->white_point_y);
                VLOG(INFO, " MIN_LUMINANCE: %10d MAX_LUMINANCE: %10d\n", mastering->min_display_mastering_luminance, mastering->max_display_mastering_luminance);
            }

            if(idx == H265_USERDATA_FLAG_VUI)
            {
                h265_vui_param_t* vui;

                vui = (h265_vui_param_t*)(pBase + pEntry[H265_USERDATA_FLAG_VUI].offset);
                VLOG(INFO, " VUI SAR(%d, %d)\n", vui->sar_width, vui->sar_height);
                VLOG(INFO, "     VIDEO FORMAT(%d)\n", vui->video_format);
                VLOG(INFO, "     COLOUR PRIMARIES(%d)\n", vui->colour_primaries);
                VLOG(INFO, "log2_max_mv_length_horizontal: %d\n", vui->log2_max_mv_length_horizontal);
                VLOG(INFO, "log2_max_mv_length_vertical  : %d\n", vui->log2_max_mv_length_vertical);
                VLOG(INFO, "video_full_range_flag  : %d\n", vui->video_full_range_flag);
                VLOG(INFO, "transfer_characteristics  : %d\n", vui->transfer_characteristics);
                VLOG(INFO, "matrix_coeffs  : %d\n", vui->matrix_coefficients);
            }
            if (idx == H265_USERDATA_FLAG_CHROMA_RESAMPLING_FILTER_HINT) {
                h265_chroma_resampling_filter_hint_t* c_resampleing_filter_hint;
                Uint32 i,j;

                c_resampleing_filter_hint = (h265_chroma_resampling_filter_hint_t*)(pBase + pEntry[H265_USERDATA_FLAG_CHROMA_RESAMPLING_FILTER_HINT].offset);
                VLOG(INFO, " CHROMA_RESAMPLING_FILTER_HINT\n");
                VLOG(INFO, " VER_CHROMA_FILTER_IDC: %10d HOR_CHROMA_FILTER_IDC: %10d\n", c_resampleing_filter_hint->ver_chroma_filter_idc, c_resampleing_filter_hint->hor_chroma_filter_idc);
                VLOG(INFO, " VER_FILTERING_FIELD_PROCESSING_FLAG: %d \n", c_resampleing_filter_hint->ver_filtering_field_processing_flag);
                if (c_resampleing_filter_hint->ver_chroma_filter_idc == 1 || c_resampleing_filter_hint->hor_chroma_filter_idc == 1) {
                    VLOG(INFO, " TARGET_FORMAT_IDC: %d \n", c_resampleing_filter_hint->target_format_idc);
                    if (c_resampleing_filter_hint->ver_chroma_filter_idc == 1) {
                        VLOG(INFO, " NUM_VERTICAL_FILTERS: %d \n", c_resampleing_filter_hint->num_vertical_filters);
                        for (i=0; i<c_resampleing_filter_hint->num_vertical_filters; i++) {
                            VLOG(INFO, " VER_TAP_LENGTH_M1[%d]: %d \n", i, c_resampleing_filter_hint->ver_tap_length_minus1[i]);
                            for (j=0; j<c_resampleing_filter_hint->ver_tap_length_minus1[i]; j++) {
                                VLOG(INFO, " VER_FILTER_COEFF[%d][%d]: %d \n", i, j, c_resampleing_filter_hint->ver_filter_coeff[i][j]);
                            }
                        }
                    }
                    if (c_resampleing_filter_hint->hor_chroma_filter_idc == 1) {
                        VLOG(INFO, " NUM_HORIZONTAL_FILTERS: %d \n", c_resampleing_filter_hint->num_horizontal_filters);
                        for (i=0; i<c_resampleing_filter_hint->num_horizontal_filters; i++) {
                            VLOG(INFO, " HOR_TAP_LENGTH_M1[%d]: %d \n", i, c_resampleing_filter_hint->hor_tap_length_minus1[i]);
                            for (j=0; j<c_resampleing_filter_hint->hor_tap_length_minus1[i]; j++) {
                                VLOG(INFO, " HOR_FILTER_COEFF[%d][%d]: %d \n", i, j, c_resampleing_filter_hint->hor_filter_coeff[i][j]);
                            }
                        }
                    }
                }
            }

            if (idx == H265_USERDATA_FLAG_KNEE_FUNCTION_INFO) {
                h265_knee_function_info_t* knee_function;

                knee_function = (h265_knee_function_info_t*)(pBase + pEntry[H265_USERDATA_FLAG_KNEE_FUNCTION_INFO].offset);
                VLOG(INFO, " FLAG_KNEE_FUNCTION_INFO\n");
                VLOG(INFO, " KNEE_FUNCTION_ID: %10d\n", knee_function->knee_function_id);
                VLOG(INFO, " KNEE_FUNCTION_CANCEL_FLAG: %d\n", knee_function->knee_function_cancel_flag);
                if (!knee_function->knee_function_cancel_flag) {
                    int i;

                    VLOG(INFO, " KNEE_FUNCTION_PERSISTENCE_FLAG: %10d\n", knee_function->knee_function_persistence_flag);
                    VLOG(INFO, " INPUT_D_RANGE: %d\n", knee_function->input_d_range);
                    VLOG(INFO, " INPUT_DISP_LUMINANCE: %d\n", knee_function->input_disp_luminance);
                    VLOG(INFO, " OUTPUT_D_RANGE: %d\n", knee_function->output_d_range);
                    VLOG(INFO, " OUTPUT_DISP_LUMINANCE: %d\n", knee_function->output_disp_luminance);
                    VLOG(INFO, " NUM_KNEE_POINTS_M1: %d\n", knee_function->num_knee_points_minus1);
                    for (i=0; i<knee_function->num_knee_points_minus1; i++) {
                        VLOG(INFO, " INPUT_KNEE_POINT: %10d OUTPUT_KNEE_POINT: %10d\n", knee_function->input_knee_point[i], knee_function->output_knee_point[i]);
                    }
                }
            }

            if (idx == H265_USERDATA_FLAG_ITU_T_T35_PRE)
            {
                char* itu_t_t35 = (char*)(pBase + pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE].offset);
                Uint32 i;

                VLOG(INFO, "ITU_T_T35_PRE = %d bytes, offset = %d\n",pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE].size, pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE].offset);
                for(i=0; i<pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE].size; i++)
                {
                    VLOG(INFO, "%02X ", (unsigned char)(itu_t_t35[i]));
                }
                VLOG(INFO, "\n");

            }

            if (idx == H265_USERDATA_FLAG_ITU_T_T35_PRE_1)
            {
                char* itu_t_t35 = (char*)(pBase + pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_1].offset);
                Uint32 i;

                VLOG(INFO, "ITU_T_T35_PRE = %d bytes, offset = %d\n",pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_1].size, pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_1].offset);
                for(i=0; i<pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_1].size; i++)
                {
                    VLOG(INFO, "%02X ", (unsigned char)(itu_t_t35[i]));
                }
                VLOG(INFO, "\n");
            }

            if (idx == H265_USERDATA_FLAG_ITU_T_T35_PRE_2)
            {
                char* itu_t_t35 = (char*)(pBase + pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].offset);
                Uint32 i;
                //user_data_entry_t* pEntry_prev;

                VLOG(INFO, "ITU_T_T35_PRE = %d bytes, offset = %d\n",pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].size, pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].offset);
                for(i=0; i<pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].size; i++)
                {
                    VLOG(INFO, "%02X ", (unsigned char)(itu_t_t35[i]));
                }
                VLOG(INFO, "\n");

                //pEntry_prev = (user_data_entry_t*)(itu_t_t35 + pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].size - sizeof(user_data_entry_t));
                //pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].size = pEntry_prev->size;
                //pEntry[H265_USERDATA_FLAG_ITU_T_T35_PRE_2].offset = pEntry_prev->offset;
            }

            if (idx == H265_USERDATA_FLAG_COLOUR_REMAPPING_INFO) {
                int c, i;
                h265_colour_remapping_info_t* colour_remapping;
                colour_remapping = (h265_colour_remapping_info_t*)(pBase + pEntry[H265_USERDATA_FLAG_COLOUR_REMAPPING_INFO].offset);

                VLOG(INFO, " COLOUR_REMAPPING_INFO\n");
                VLOG(INFO, " COLOUR_REMAP_ID: %10d\n", colour_remapping->colour_remap_id);


                VLOG(INFO, " COLOUR_REMAP_CANCEL_FLAG: %d\n", colour_remapping->colour_remap_cancel_flag);
                if (!colour_remapping->colour_remap_cancel_flag) {
                    VLOG(INFO, " COLOUR_REMAP_PERSISTENCE_FLAG: %d\n", colour_remapping->colour_remap_persistence_flag);
                    VLOG(INFO, " COLOUR_REMAP_VIDEO_SIGNAL_INFO_PRESENT_FLAG: %d\n", colour_remapping->colour_remap_video_signal_info_present_flag);
                    if (colour_remapping->colour_remap_video_signal_info_present_flag) {
                        VLOG(INFO, " COLOUR_REMAP_FULL_RANGE_FLAG: %d\n", colour_remapping->colour_remap_full_range_flag);
                        VLOG(INFO, " COLOUR_REMAP_PRIMARIES: %d\n",	colour_remapping->colour_remap_primaries);
                        VLOG(INFO, " COLOUR_REMAP_TRANSFER_FUNCTION: %d\n", colour_remapping->colour_remap_transfer_function);
                        VLOG(INFO, " COLOUR_REMAP_MATRIX_COEFFICIENTS: %d\n", colour_remapping->colour_remap_matrix_coefficients);
                    }

                    VLOG(INFO, " COLOUR_REMAP_INPUT_BIT_DEPTH: %d\n", colour_remapping->colour_remap_input_bit_depth);
                    VLOG(INFO, " COLOUR_REMAP_BIT_DEPTH: %d\n", colour_remapping->colour_remap_bit_depth);

                    for( c = 0; c < H265_MAX_LUT_NUM_VAL; c++ )
                    {
                        VLOG(INFO, " PRE_LUT_NUM_VAL_MINUS1[%d]: %d\n", c, colour_remapping->pre_lut_num_val_minus1[c]);
                        if(colour_remapping->pre_lut_num_val_minus1[c] > 0)
                        {
                            for( i = 0; i <= colour_remapping->pre_lut_num_val_minus1[c]; i++ )
                            {
                                VLOG(INFO, " PRE_LUT_CODED_VALUE[%d][%d]: %d\n", c, i, colour_remapping->pre_lut_coded_value[c][i]);
                                VLOG(INFO, " PRE_LUT_TARGET_VALUE[%d][%d]: %d\n", c, i, colour_remapping->pre_lut_target_value[c][i]);
                            }
                        }
                    }
                    VLOG(INFO, " COLOUR_REMAP_MATRIX_PRESENT_FLAG: %d\n", colour_remapping->colour_remap_matrix_present_flag);
                    if(colour_remapping->colour_remap_matrix_present_flag) {
                        VLOG(INFO, " LOG2_MATRIX_DENOM: %d\n", colour_remapping->log2_matrix_denom);
                        for( c = 0; c < H265_MAX_COLOUR_REMAP_COEFFS; c++ )
                            for( i = 0; i < H265_MAX_COLOUR_REMAP_COEFFS; i++ )
                                VLOG(INFO, " LOG2_MATRIX_DENOM[%d][%d]: %d\n", c, i, colour_remapping->colour_remap_coeffs[c][i]);
                    }

                    for( c = 0; c < H265_MAX_LUT_NUM_VAL; c++ )
                    {
                        VLOG(INFO, " POST_LUT_NUM_VAL_MINUS1[%d]: %d\n", c, colour_remapping->post_lut_num_val_minus1[c]);
                        if(colour_remapping->post_lut_num_val_minus1[c] > 0)
                        {
                            for( i = 0; i <= colour_remapping->post_lut_num_val_minus1[c]; i++)
                            {
                                VLOG(INFO, " POST_LUT_CODED_VALUE[%d][%d]: %d\n", c, i, colour_remapping->post_lut_coded_value[c][i]);
                                VLOG(INFO, " POST_LUT_TARGET_VALUE[%d][%d]: %d\n", c, i, colour_remapping->post_lut_target_value[c][i]);
                            }
                        }
                    }
                }
            }

            if (idx == H265_USERDATA_FLAG_TONE_MAPPING_INFO) {
                h265_tone_mapping_info_t* tone_mapping;

                tone_mapping = (h265_tone_mapping_info_t*)(pBase + pEntry[H265_USERDATA_FLAG_TONE_MAPPING_INFO].offset);
                VLOG(INFO, " FLAG_TONE_MAPPING_INFO\n");
                VLOG(INFO, " TONE_MAP_ID: %10d\n", tone_mapping->tone_map_id);
                VLOG(INFO, " TONE_MAP_CANCEL_FLAG: %d\n", tone_mapping->tone_map_cancel_flag);
                if (!tone_mapping->tone_map_cancel_flag) {
                    int i;

                    VLOG(INFO, " TONE_MAP_PERSISTENCE_FLAG: %10d\n", tone_mapping->tone_map_persistence_flag);
                    VLOG(INFO, " CODED_DATA_BIT_DEPTH : %d\n", tone_mapping->coded_data_bit_depth);
                    VLOG(INFO, " TARGET_BIT_DEPTH : %d\n", tone_mapping->target_bit_depth);
                    VLOG(INFO, " TONE_MAP_MODEL_ID : %d\n", tone_mapping->tone_map_model_id);
                    VLOG(INFO, " MIN_VALUE : %d\n", tone_mapping->min_value);
                    VLOG(INFO, " MAX_VALUE : %d\n", tone_mapping->max_value);
                    VLOG(INFO, " SIGMOID_MIDPOINT : %d\n", tone_mapping->sigmoid_midpoint);
                    VLOG(INFO, " SIGMOID_MIDPOINT : %d\n", tone_mapping->sigmoid_width);
                    for (i=0; i<(1<<tone_mapping->target_bit_depth); i++) {
                        VLOG(INFO, " START_OF_CODED_INTERVAL[%d] : %d\n", i, tone_mapping->start_of_coded_interval[i]); // [1 << target_bit_depth] // 10bits
                    }

                    VLOG(INFO, " NUM_PIVOTS : %d\n", tone_mapping->num_pivots); // [(1 << coded_data_bit_depth)?1][(1 << target_bit_depth)-1] // 10bits
                    for (i=0; i<tone_mapping->num_pivots; i++) {
                        VLOG(INFO, " CODED_PIVOT_VALUE[%d] : %d, TARGET_PIVOT_VALUE[%d] : %d\n", i, tone_mapping->coded_pivot_value[i]
                        , i, tone_mapping->target_pivot_value[i]);
                    }

                    VLOG(INFO, " CAMERA_ISO_SPEED_IDC : %d\n", tone_mapping->camera_iso_speed_idc);
                    VLOG(INFO, " CAMERA_ISO_SPEED_VALUE : %d\n", tone_mapping->camera_iso_speed_value);

                    VLOG(INFO, " EXPOSURE_INDEX_IDC : %d\n", tone_mapping->exposure_index_idc);
                    VLOG(INFO, " EXPOSURE_INDEX_VALUE : %d\n", tone_mapping->exposure_index_value);
                    VLOG(INFO, " EXPOSURE_INDEX_COMPESATION_VALUE_SIGN_FLAG : %d\n", tone_mapping->exposure_compensation_value_sign_flag);
                    VLOG(INFO, " EXPOSURE_INDEX_COMPESATION_VALUE_NUMERATOR : %d\n", tone_mapping->exposure_compensation_value_numerator);
                    VLOG(INFO, " EXPOSURE_INDEX_COMPESATION_VALUE_DENOM_IDC : %d\n", tone_mapping->exposure_compensation_value_denom_idc);

                    VLOG(INFO, " REF_SCREEN_LUMINANCE_WHITE : %d\n", tone_mapping->ref_screen_luminance_white);

                    VLOG(INFO, " EXTENDED_RANGE_WHITE_LEVEL : %d\n", tone_mapping->extended_range_white_level);
                    VLOG(INFO, " NOMINAL_BLACK_LEVEL_CODE_VALUE : %d\n", tone_mapping->nominal_black_level_code_value);
                    VLOG(INFO, " NOMINAL_WHITE_LEVEL_CODE_VALUE : %d\n", tone_mapping->nominal_white_level_code_value);
                    VLOG(INFO, " EXTENDED_WHITE_LEVEL_CODE_VALUE : %d\n", tone_mapping->extended_white_level_code_value);
                }
                VLOG(INFO, "\n");
            }

            if (idx == H265_USERDATA_FLAG_CONTENT_LIGHT_LEVEL_INFO) {
                h265_content_light_level_info_t* content_light_level;

                content_light_level = (h265_content_light_level_info_t*)(pBase + pEntry[H265_USERDATA_FLAG_CONTENT_LIGHT_LEVEL_INFO].offset);
                VLOG(INFO, " CONTNET_LIGHT_INFO\n");
                VLOG(INFO, " MAX_CONTENT_LIGHT_LEVEL : %d\n", content_light_level->max_content_light_level);
                VLOG(INFO, " MAX_PIC_AVERAGE_LIGHT_LEVEL : %d\n", content_light_level->max_pic_average_light_level);
                VLOG(INFO, "\n");
            }

            if (idx == H265_USERDATA_FLAG_FILM_GRAIN_CHARACTERISTICS_INFO) {
                h265_film_grain_characteristics_t* film_grain_characteristics;
                int i,j,c;

                film_grain_characteristics = (h265_film_grain_characteristics_t*)(pBase + pEntry[H265_USERDATA_FLAG_FILM_GRAIN_CHARACTERISTICS_INFO].offset);
                VLOG(INFO, " FILM_GRAIN_CHARACTERISTICS_INFO\n");
                VLOG(INFO, " FILM_GRAIN_CHARACTERISTICS_CANCEL_FLAG: %d\n", film_grain_characteristics->film_grain_characteristics_cancel_flag);
                if (!film_grain_characteristics->film_grain_characteristics_cancel_flag) {
                    VLOG(INFO, " FILM_GRAIN_MODEL_ID: %10d\n", film_grain_characteristics->film_grain_model_id);

                    VLOG(INFO, " SEPARATE_COLOUR_DESCRIPTION_PRESENT_FLAG: %d\n", film_grain_characteristics->separate_colour_description_present_flag);
                    if (film_grain_characteristics->separate_colour_description_present_flag)
                    {
                        VLOG(INFO, " FILM_GRAIN_BIT_DEPTH_LUMA_MINUS8: %d\n", film_grain_characteristics->film_grain_bit_depth_luma_minus8);
                        VLOG(INFO, " FILM_GRAIN_BIT_DEPTH_CHROMA_MINUS8: %d\n", film_grain_characteristics->film_grain_bit_depth_chroma_minus8);
                        VLOG(INFO, " FILM_GRAIN_FULL_RANGE_FLAG: %d\n", film_grain_characteristics->film_grain_full_range_flag);
                        VLOG(INFO, " FILM_GRAIN_COLOUR_PRIMARIES: %d\n", film_grain_characteristics->film_grain_colour_primaries);
                        VLOG(INFO, " FILM_GRAIN_TRANSFER_CHARACTERISTICS: %d\n", film_grain_characteristics->film_grain_transfer_characteristics);
                        VLOG(INFO, " FILM_GRAIN_MATRIX_COEFF: %d\n", film_grain_characteristics->film_grain_matrix_coeffs);
                    }
                }

                VLOG(INFO, " BLENDING_MODE_ID: %d\n", film_grain_characteristics->blending_mode_id);
                VLOG(INFO, " LOG2_SCALE_FACTOR: %d\n", film_grain_characteristics->log2_scale_factor);
                for(c=0; c < H265_MAX_NUM_FILM_GRAIN_COMPONENT; c++ )
                {
                    VLOG(INFO, " COMP_MODEL_PRESENT_FLAG[%d]: %d\n", c, film_grain_characteristics->comp_model_present_flag[c]);
                }

                for(c=0; c < H265_MAX_NUM_FILM_GRAIN_COMPONENT; c++ )
                {
                    if(film_grain_characteristics->comp_model_present_flag[c])
                    {

                        VLOG(INFO, " NUM_INTENSITY_INTERVALS_MINUS1[%d]: %d\n", c, film_grain_characteristics->num_intensity_intervals_minus1[c]);
                        VLOG(INFO, " NUM_MODEL_VALUES_MINUS1[%d]: %d\n", c, film_grain_characteristics->num_model_values_minus1[c]);
                        for(i=0; i <= film_grain_characteristics->num_intensity_intervals_minus1[c]; i++)
                        {
                            VLOG(INFO, " INTENSITY_INTERVAL_LOWER_BOUND[%d][%d]: %d\n", c, i, film_grain_characteristics->intensity_interval_lower_bound[c][i]);
                            VLOG(INFO, " INTENSITY_INTERVAL_UPPER_BOUND[%d][%d]: %d\n", c, i, film_grain_characteristics->intensity_interval_upper_bound[c][i]);
                            for(j=0; j <= film_grain_characteristics->num_model_values_minus1[c]; j++)
                            {
                                VLOG(INFO, " COMP_MODEL_VALUE[%d][%d][%d]: %d\n", c, i, j, film_grain_characteristics->comp_model_value[c][i][j]);
                            }
                        }
                    }
                }
                VLOG(INFO, " FILM_GRAIN_CHARACTERISTICS_PERSISTENCE_FLAG: %d\n", film_grain_characteristics->film_grain_characteristics_persistence_flag);
                VLOG(INFO, "\n");
            }
        }
    }
    VLOG(INFO, "===========================================\n");
}



