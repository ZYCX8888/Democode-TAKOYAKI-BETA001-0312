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
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include "cnm_app.h"
#include "decoder_listener.h"
#include "main_helper.h"
#include "misc/debug.h"


static Uint32           sizeInWord;
static Uint16*          pusBitCode;

enum waitStatus {
    WAIT_AFTER_INIT         = 0,

    WAIT_AFTER_DEC_OPEN     = 1,
    WAIT_AFTER_SEQ_INIT     = 2,
    WAIT_AFTER_REG_BUF      = 3,
    WAIT_BEFORE_DEC_START   = 4,
    WAIT_BEFORE_DEC_CLOSE   = 5,

    WAIT_AFTER_ENC_OPEN     = 6,
    WAIT_AFTER_INIT_SEQ     = 7,
    WAIT_AFTER_REG_FRAME    = 8,
    WAIT_BEFORE_ENC_START   = 9,
    WAIT_BEFORE_ENC_CLOSE   = 10,
    WAIT_NOT_DEFINED        = 11
};

typedef struct {
    char             inputFilePath[256];
    char             localFile[256];
    char             outputFilePath[256];
    char             refFilePath[256];
    CodStd           stdMode;
    BOOL             afbce;
    BOOL             afbcd;
    BOOL             scaler;
    size_t           sclw;
    size_t           sclh;
    Int32            bsmode;
    BOOL             enableWTL;
    BOOL             enableMVC;
    int              compareType;
    TestDecConfig    decConfig;
    BOOL             isEncoder;
} InstTestConfig;

typedef struct {
    Uint32          totProcNum;             /* the number of process */
    Uint32          curProcNum;
    Uint32          numMulti;
    InstTestConfig  instConfig[MAX_NUM_INSTANCE];
} TestMultiConfig;

static void WaitForNextStep(Int32 instIdx, Int32 curStepName)
{
}


static void Help(const char *programName)
{
    VLOG(INFO, "------------------------------------------------------------------------------\n");
    VLOG(INFO, "%s(API v%d.%d.%d)\n", GetBasename(programName), API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
    VLOG(INFO, "\tAll rights reserved by Chips&Media(C)\n");
    VLOG(INFO, "\tSample program controlling the Chips&Media VPU\n");
    VLOG(INFO, "------------------------------------------------------------------------------\n");
    VLOG(INFO, "%s --multicmde=<stream list file, aka cmd file>\n", GetBasename(programName));
    VLOG(INFO, "-h                          help\n");
    VLOG(INFO, "--multicmd=FILE             test multi-instance with an instance list file.");
}


void SetDecMultiParam(
    TestMultiConfig* multiConfig,
    int idx
    )
{
    multiConfig->instConfig[idx].decConfig.bitFormat            = multiConfig->instConfig[idx].stdMode;
    multiConfig->instConfig[idx].decConfig.bitstreamMode        = multiConfig->instConfig[idx].bsmode;
    multiConfig->instConfig[idx].decConfig.feedingMode          = (multiConfig->instConfig[idx].bsmode == BS_MODE_INTERRUPT) ? FEEDING_METHOD_FIXED_SIZE : FEEDING_METHOD_FRAME_SIZE;
    multiConfig->instConfig[idx].decConfig.streamEndian         = VPU_STREAM_ENDIAN;
    multiConfig->instConfig[idx].decConfig.frameEndian          = VPU_FRAME_ENDIAN;
    multiConfig->instConfig[idx].decConfig.cbcrInterleave       = FALSE;
    multiConfig->instConfig[idx].decConfig.nv21                 = FALSE;
    multiConfig->instConfig[idx].decConfig.enableWTL            = multiConfig->instConfig[idx].enableWTL;
    multiConfig->instConfig[idx].decConfig.coda9.enableMvc      = multiConfig->instConfig[idx].enableMVC;
    multiConfig->instConfig[idx].decConfig.wtlMode              = FF_FRAME;
    multiConfig->instConfig[idx].decConfig.wtlFormat            = FORMAT_420;
    multiConfig->instConfig[idx].decConfig.mapType              = LINEAR_FRAME_MAP;
    if (multiConfig->instConfig[idx].stdMode == STD_HEVC || multiConfig->instConfig[idx].stdMode == STD_VP9  || 
        multiConfig->instConfig[idx].stdMode == STD_AVS2 || multiConfig->instConfig[idx].stdMode == STD_SVAC || multiConfig->instConfig[idx].stdMode == STD_AVC)
        multiConfig->instConfig[idx].decConfig.mapType          = COMPRESSED_FRAME_MAP;
    multiConfig->instConfig[idx].decConfig.bsSize               = (12*1024*1024);
    multiConfig->instConfig[idx].decConfig.forceOutNum          = 0;
    multiConfig->instConfig[idx].decConfig.wave.bwOptimization  = TRUE;
    if ( multiConfig->instConfig[idx].scaler == TRUE ) {
        multiConfig->instConfig[idx].decConfig.scaleDownWidth   = multiConfig->instConfig[idx].sclw;
        multiConfig->instConfig[idx].decConfig.scaleDownHeight  = multiConfig->instConfig[idx].sclh;
    }

    strcpy(multiConfig->instConfig[idx].decConfig.inputPath, multiConfig->instConfig[idx].inputFilePath);
    if (multiConfig->instConfig[idx].localFile[0]) {
        strcpy(multiConfig->instConfig[idx].decConfig.inputPath, multiConfig->instConfig[idx].localFile);
    }
    strcpy(multiConfig->instConfig[idx].decConfig.outputPath, multiConfig->instConfig[idx].outputFilePath);
    if (multiConfig->instConfig[idx].compareType == MD5_COMPARE) {
        multiConfig->instConfig[idx].decConfig.compareType = MD5_COMPARE;
        strcpy(multiConfig->instConfig[idx].decConfig.md5Path, multiConfig->instConfig[idx].refFilePath);
    }
    else {
        multiConfig->instConfig[idx].decConfig.compareType = YUV_COMPARE;
        strcpy(multiConfig->instConfig[idx].decConfig.refYuvPath, multiConfig->instConfig[idx].refFilePath);
    }

    multiConfig->instConfig[idx].decConfig.customDisableFlag = TRUE;
}

void SetDecMultiRandomParam(
    TestMultiConfig* multiConfig,
    int idx
    )
{
    InstTestConfig *instConfig = &multiConfig->instConfig[idx];
    TestDecConfig *decCfg = &instConfig->decConfig;

    decCfg->bitFormat            = instConfig->stdMode;
    decCfg->bitstreamMode        = instConfig->bsmode;
    decCfg->feedingMode          = (instConfig->bsmode == BS_MODE_INTERRUPT) ? FEEDING_METHOD_FIXED_SIZE : FEEDING_METHOD_FRAME_SIZE;
    if (instConfig->stdMode == STD_HEVC || instConfig->stdMode == STD_VP9 || instConfig->stdMode == STD_AVS2 || instConfig->stdMode == STD_SVAC || instConfig->stdMode == STD_AVC) {
        decCfg->streamEndian         = GetRandom(16, 31);
        decCfg->frameEndian          = GetRandom(16, 31);
    } else {
        decCfg->streamEndian         = GetRandom(0, 3);
        decCfg->frameEndian          = GetRandom(0, 3);
    }
    decCfg->cbcrInterleave       = FALSE;
    decCfg->nv21                 = FALSE;
    decCfg->enableWTL            = instConfig->enableWTL;
    decCfg->coda9.enableMvc      = instConfig->enableMVC;
    decCfg->wtlMode              = FF_FRAME;
    decCfg->wtlFormat            = FORMAT_420;
    decCfg->mapType              = LINEAR_FRAME_MAP;
    if (instConfig->stdMode == STD_HEVC || instConfig->stdMode == STD_VP9 || instConfig->stdMode == STD_AVS2 || instConfig->stdMode == STD_SVAC || instConfig->stdMode == STD_AVC)
        decCfg->mapType              = COMPRESSED_FRAME_MAP;
    decCfg->bsSize               = (12*1024*1024);
    decCfg->forceOutNum          = 0;
    decCfg->wave.bwOptimization  = TRUE;      // see MIC-2704, MDR-28

    if ( instConfig->scaler == TRUE ) {
        decCfg->scaleDownWidth   = instConfig->sclw;
        decCfg->scaleDownHeight  = instConfig->sclh;
    }
    strcpy(decCfg->inputPath, instConfig->inputFilePath);
    if (instConfig->localFile[0]) {
        strcpy(decCfg->inputPath, instConfig->localFile);
    }
    strcpy(decCfg->outputPath, instConfig->outputFilePath);
    if (instConfig->compareType == MD5_COMPARE) {
        decCfg->compareType = MD5_COMPARE;
        strcpy(decCfg->md5Path, instConfig->refFilePath);
    }
    else { // instConfig->compareType == YUV_COMPARE
        decCfg->compareType = YUV_COMPARE;
        strcpy(decCfg->refYuvPath, instConfig->refFilePath);
    }
}

static void MultiDecoderListener(Component com, Uint32 event, void* data, void* context)
{
    DecHandle               decHandle = NULL;

    ComponentGetParameter(NULL, com, GET_PARAM_DEC_HANDLE, &decHandle);
    if (decHandle == NULL && event != COMPONENT_EVENT_DEC_DECODED_ALL) {
        // Terminated state 
        return;
    }

    switch (event) {
    case COMPONENT_EVENT_DEC_OPEN:
        WaitForNextStep(decHandle->instIndex, WAIT_AFTER_DEC_OPEN);
        break;
    case COMPONENT_EVENT_DEC_COMPLETE_SEQ:
        HandleDecCompleteSeqEvent(com, (CNMComListenerDecCompleteSeq*)data, (DecListenerContext*)context);
        WaitForNextStep(decHandle->instIndex, WAIT_AFTER_SEQ_INIT);
        break;
    case COMPONENT_EVENT_DEC_REGISTER_FB:
        WaitForNextStep(decHandle->instIndex, WAIT_AFTER_REG_BUF);
        break;
    case COMPONENT_EVENT_DEC_READY_ONE_FRAME:
        WaitForNextStep(decHandle->instIndex, WAIT_BEFORE_DEC_START);
        break;
    case COMPONENT_EVENT_DEC_GET_OUTPUT_INFO:
        HandleDecGetOutputEvent(com, (CNMComListenerDecDone*)data, (DecListenerContext*)context);
        break;
    case COMPONENT_EVENT_DEC_DECODED_ALL:
        // It isn't possible to get handle when a component is terminated state.
        decHandle = (DecHandle)data;
        if (decHandle) WaitForNextStep(decHandle->instIndex, WAIT_BEFORE_DEC_CLOSE);
        break;
    default:
        break;
    }
}

static BOOL CreateDecoderTask(CNMTask task, CNMComponentConfig* config, DecListenerContext* lsnCtx)
{
    Component feeder   = ComponentCreate("feeder",   config);
    Component decoder  = ComponentCreate("decoder",  config);
    Component renderer = ComponentCreate("renderer", config);
    BOOL      success  = FALSE;

    CNMTaskAdd(task, feeder);
    CNMTaskAdd(task, decoder);
    CNMTaskAdd(task, renderer);
    
    if ((success=SetupDecListenerContext(lsnCtx, config, renderer)) == TRUE) {
        ComponentRegisterListener(decoder, COMPONENT_EVENT_DEC_ALL, MultiDecoderListener, (void*)lsnCtx);
    }

    return success;
}


static BOOL MultiInstanceTest(TestMultiConfig* multiConfig, Uint16* fw, Uint32 size)
{
    Uint32              i;
    CNMComponentConfig  config;
    CNMTask             task;
    DecListenerContext* decListenerCtx = (DecListenerContext*)osal_malloc(sizeof(DecListenerContext) * MAX_NUM_INSTANCE);
    BOOL                ret     = FALSE;
    BOOL                success = TRUE;
    BOOL                match   = TRUE;

    CNMAppInit();

    for (i=0; i < multiConfig->numMulti; i++) {
        task = CNMTaskCreate();
        memset((void*)&config, 0x00, sizeof(CNMComponentConfig));
        config.bitcode       = (Uint8*)fw;
        config.sizeOfBitcode = size;
        if (multiConfig->instConfig[i].isEncoder == FALSE) {
            memcpy((void*)&config.testDecConfig, &multiConfig->instConfig[i].decConfig, sizeof(TestDecConfig));
            success = CreateDecoderTask(task, &config, &decListenerCtx[i]);
        }

        CNMAppAdd(task);
        if (success == FALSE) {
            CNMAppStop();
            break;
        }
    }

    ret = CNMAppRun();

    for (i=0; i<multiConfig->numMulti; i++) {
        if (multiConfig->instConfig[i].isEncoder == FALSE) {
            match &= decListenerCtx[i].match;
            ClearDecListenerContext(&decListenerCtx[i]);
        }
    }
    if (ret == TRUE) ret = match;

    osal_free(decListenerCtx);

    return ret;
}

static BOOL LoadCmdFile(TestMultiConfig* config, char* cmdPath)
{
    char    buf[4096];
    char    streamPath[1024] = {'\0'};
    char    goldenPath[1024] = {'\0'};
    Uint32  decenc, codec, compare;
    Uint32  inplaceMode, vrange, idx, loopCount;
    FILE*   fp;

    if ((fp=fopen(cmdPath, "r")) == NULL) {
        VLOG(ERR, "<%s:%d> failed to open file: %s\n", __FUNCTION__, __LINE__, cmdPath);
        return FALSE;
    }

    idx = 0;
    while (!feof(fp)) {
        if (fgets(buf, sizeof(buf), fp) == NULL) break;
        if (strlen(buf) == 0) break;

        if (buf[0] == ';') continue;    // comment

        sscanf(buf, "%d %d %d %d %d %d %s %s", &codec, &decenc, &compare, &inplaceMode, &vrange, &loopCount, streamPath, goldenPath);
        config->instConfig[idx].decConfig.bitFormat            = (CodStd)codec;
        config->instConfig[idx].decConfig.bitstreamMode        = BS_MODE_INTERRUPT;
#ifdef SUPPORT_SG_FLOW
        config->instConfig[idx].decConfig.feedingMode          = FEEDING_METHOD_FRAME_SIZE;
#else
        config->instConfig[idx].decConfig.feedingMode          = FEEDING_METHOD_FIXED_SIZE;
#endif
        config->instConfig[idx].decConfig.streamEndian         = VPU_STREAM_ENDIAN;
        config->instConfig[idx].decConfig.frameEndian          = VPU_FRAME_ENDIAN;
        config->instConfig[idx].decConfig.cbcrInterleave       = FALSE;
        config->instConfig[idx].decConfig.nv21                 = FALSE;
        config->instConfig[idx].decConfig.enableWTL            = TRUE;
        config->instConfig[idx].decConfig.mapType              = COMPRESSED_FRAME_MAP;
        config->instConfig[idx].decConfig.bsSize               = (1*1024*1024);
        config->instConfig[idx].decConfig.inPlaceBufferMode    = inplaceMode;
        config->instConfig[idx].decConfig.maxVerticalMV        = vrange;
        strcpy(config->instConfig[idx].decConfig.inputPath, streamPath);
        config->instConfig[idx].compareType                    = compare;
        config->instConfig[idx].decConfig.loopCount = loopCount;

        if (config->instConfig[idx].compareType == MD5_COMPARE) {
            config->instConfig[idx].decConfig.compareType = MD5_COMPARE;
            strcpy(config->instConfig[idx].decConfig.md5Path, goldenPath);
        }
        else if (config->instConfig[idx].compareType == YUV_COMPARE) {
            config->instConfig[idx].decConfig.compareType = YUV_COMPARE;
            strcpy(config->instConfig[idx].decConfig.refYuvPath, goldenPath);
        }
        else {
            config->instConfig[idx].decConfig.compareType = NO_COMPARE;
        }
        idx++;
    }
    config->numMulti = idx;
    printf("[%s][%d] go here numMulti[%u]\n", __FUNCTION__, __LINE__, config->numMulti);

    fclose(fp);

    return TRUE;
}

int main(int argc, char **argv)
{
    Int32           coreIndex   = 0;
    Uint32          productId   = 0;
    TestMultiConfig multiConfig;
    Int32           opt, index;
    char*           optString   = "c:h";
    struct option   options[]   = {
        {"multicmd",              1, NULL, 0},
        {NULL,                    0, NULL, 0},
    };
    const char*     name;
    char*           cmdFile      = NULL;
    char*           firmwarePath = NULL;
    Uint32          ret = 0;

    osal_memset(&multiConfig, 0x00, sizeof(multiConfig));
    osal_memset(&multiConfig.instConfig[0], 0x00, sizeof(InstTestConfig)*MAX_NUM_INSTANCE);
    osal_memset(&multiConfig.instConfig[0], 0x00, sizeof(InstTestConfig)*MAX_NUM_INSTANCE);


    while ((opt=getopt_long(argc, argv, optString, options, &index)) != -1) {
        switch (opt) {
        case 'h':
            Help(argv[0]);
            return 0;
        case 0:
            name = options[index].name;
            if (strcmp("multicmd", name) == 0) {
                cmdFile = optarg;
            }
            else {
                VLOG(ERR, "unknown --%s\n", name);
                Help(argv[0]);
                return 1;
            }
            break;
        default:
            return 1;
        }
    }

    InitLog();


    productId = VPU_GetProductId(coreIndex);
    switch (productId) {
    case PRODUCT_ID_511: firmwarePath = CORE_6_BIT_CODE_FILE_PATH; break;
    case PRODUCT_ID_521: firmwarePath = CORE_6_BIT_CODE_FILE_PATH; break;
    default: 
        VLOG(ERR, "<%s:%d> Unknown productId(%d)\n", __FUNCTION__, __LINE__, productId);
        return 1;
    }

    if (LoadFirmware(productId, (Uint8**)&pusBitCode, &sizeInWord, firmwarePath) < 0) {
        VLOG(ERR, "%s:%d Failed to load firmware: %s\n", __FUNCTION__, __LINE__, firmwarePath);
        return 1;
    }

    if (cmdFile) {
        if (LoadCmdFile(&multiConfig, cmdFile) == TRUE) {
            if (MultiInstanceTest(&multiConfig, pusBitCode, sizeInWord) == FALSE) {
                VLOG(ERR, "Failed to MultiInstanceTest()\n");
                ret = 1;
            }
        }
        else {
            ret = 1;
        }
    }

    osal_free(pusBitCode);

    return ret;
}
 
