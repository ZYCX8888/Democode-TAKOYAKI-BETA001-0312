/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>

#include "st_main_venc.h"

#define ASCII_COLOR_RED                          "\033[1;31m"
#define ASCII_COLOR_WHITE                        "\033[1;37m"
#define ASCII_COLOR_YELLOW                       "\033[1;33m"
#define ASCII_COLOR_BLUE                         "\033[1;36m"
#define ASCII_COLOR_GREEN                        "\033[1;32m"
#define ASCII_COLOR_END                          "\033[0m"

#define venc_info(fmt, args...)     ({do{printf(ASCII_COLOR_WHITE"[APP INFO]:%s[%d]: ", __FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_END);}while(0);})
#define venc_err(fmt, args...)      ({do{printf(ASCII_COLOR_RED  "[APP ERR ]:%s[%d]: ", __FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_END);}while(0);})

#define VENC_CONFIG_PATH "venc_config.json"
#define MAX_VENC_NUM    16
#define MAX_ROI_NUM     8
#define MAX_SEI_LEN     16
#define MAX_JSON_STR_LEN    16

typedef struct
{
    bool bStart;
    MI_VENC_CHN venc_chn;
    int yuv_width;
    int yuv_height;
    int yuv_format;
    char *yuv_path;
    MI_U32 pic_width;
    MI_U32 pic_height;
    char *es_path;
    MI_U32 bitrate;
    MI_U32 qp;
    MI_U32 gop;
    MI_U32 fps;
    MI_U32 profile;
    MI_VENC_ModType_e eModType;
    MI_VENC_RcMode_e eRcMode;
    int vencFd;

    uint32_t test_flag;
    MI_U8 ins_data[MAX_SEI_LEN];
    MI_U8 ins_len;
    MI_VENC_RoiCfg_t roi[MAX_ROI_NUM];
    int roi_num;
    MI_VENC_ParamH264Entropy_t entropy;
    MI_VENC_ParamRef_t ref;
    MI_VENC_CropCfg_t crop;
    MI_VENC_SuperFrameCfg_t supfrm;
    DECL_VS2(MI_VENC_Param, SliceSplit_t, slisplt)
    DECL_VS2(MI_VENC_Param, IntraPred_t, intra)
    DECL_VS2(MI_VENC_Param, InterPred_t, inter)
    DECL_VS2(MI_VENC_Param, Trans_t, trans)
    DECL_VS2(MI_VENC_Param, Vui_t, vui)
    DECL_VS2(MI_VENC_Param, Dblk_t, dblk)

    pthread_t pt_input;
    pthread_t pt_output;
} VENC_ChnConf_t;

typedef struct
{
    //config reader
    FILE* fd;
    char buff[128];
    uint16_t pos;

    //channel config
    VENC_ChnConf_t conf[2];
} VENC_Ctx_t;

static MI_BOOL g_bExit = FALSE;
static MI_S32 g_skipFrame = 35;
static MI_U32 g_frameCnt = 0;
// static MI_S32 g_dumpFrame = 60;

void venc_print_help(const char *porgName)
{
    printf("%s [-options] source.yuv\n", porgName);
    printf(" -s <width> <height> .... source resolution.\n");
    printf(" -p <yuv format> ........ 0: yuv420, 1: yuv422.\n");
    printf(" -h ..................... print this help\n");
    exit(0);
}

void venc_parse_options(VENC_Ctx_t *ctx, int argc, char **argv)
{
    if (argc <= 1)
    {
        venc_print_help(argv[0]);
    }

    int i = 0;
    for (i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help"))
        {
            venc_print_help(argv[0]);
        }
        else if (!strcmp(argv[i], "-s"))
        {
            ctx->conf[0].yuv_width = atoi(argv[++i]);
            ctx->conf[0].yuv_height = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-p"))
        {
            ctx->conf[0].yuv_format = atoi(argv[++i]);
        }
        else if (ctx->conf[0].yuv_path == NULL)
        {
            ctx->conf[0].yuv_path = argv[i];
        }
    }
    printf("source yuv info: format=%d,width=%d,height=%d,file=%s\n",
        ctx->conf[0].yuv_format, ctx->conf[0].yuv_width, ctx->conf[0].yuv_height, ctx->conf[0].yuv_path);
}

int venc_skip_blank_space(VENC_Ctx_t * ctx)
{
    assert(ctx);
    while(1)
    {
        if(ctx->pos >= sizeof(ctx->buff) || ctx->buff[ctx->pos] == '\n'
                || ctx->buff[ctx->pos] == '\0')
        {
            if(fgets(ctx->buff, sizeof(ctx->buff), ctx->fd) == NULL)
                return -1;
            ctx->pos = 0;
        }
        if(ctx->buff[ctx->pos] != ' ' && ctx->buff[ctx->pos] != '\t')
            return 0;
        ctx->pos++;
    }
}

int venc_find_next_char(VENC_Ctx_t * ctx, char c)
{
    assert(ctx);
    while(1)
    {
        if(ctx->pos >= sizeof(ctx->buff) || ctx->buff[ctx->pos] == '\n')
        {
            if(fgets(ctx->buff, sizeof(ctx->buff), ctx->fd) == NULL)
                return -1;
            ctx->pos = 0;
        }
        if(ctx->buff[ctx->pos] == c)
            return 0;
        ctx->pos++;
    }
}

char venc_get_next_char(VENC_Ctx_t * ctx)
{
    assert(ctx);
    char c = 0;
    ctx->pos++;
    if(venc_skip_blank_space(ctx))
        return -1;
    c = ctx->buff[ctx->pos];
    return c;
}

char venc_get_curr_char(VENC_Ctx_t * ctx)
{
    assert(ctx);
    char c = 0;
    if(venc_skip_blank_space(ctx))
        return -1;
    c = ctx->buff[ctx->pos];
    return c;
}

int venc_get_item_name(VENC_Ctx_t * ctx, char* c, size_t size)
{
    int n = 0;
    assert(ctx && c);
    assert(size > 0);

    venc_skip_blank_space(ctx);
    if(ctx->buff[ctx->pos] != '\"')
        return 1;
    ctx->pos++;
    while(n < size)
    {
        if(ctx->pos >= sizeof(ctx->buff) || ctx->buff[ctx->pos] == '\n')
        {
            if(fgets(ctx->buff, sizeof(ctx->buff), ctx->fd) == NULL)
                return -1;
            ctx->pos = 0;
        }
        if(ctx->buff[ctx->pos] == '\"')
        {
            ctx->pos++;
            c[n] = '\0';
            return 0;
        }
        c[n] = ctx->buff[ctx->pos];
        n++;
        ctx->pos++;
    }
    venc_err("char array size too small.\n");
    return -1;
}

int venc_get_item_value(VENC_Ctx_t * ctx, char* c, size_t size)
{
    int n = 0;
    bool start = false;
    assert(ctx && c);
    assert(size > 0);

    while(n < size)
    {
        if(ctx->pos >= sizeof(ctx->buff) || ctx->buff[ctx->pos] == '\n')
        {
            if(fgets(ctx->buff, sizeof(ctx->buff), ctx->fd) == NULL)
                return -1;
            ctx->pos = 0;
        }
        if(ctx->buff[ctx->pos] == ':')
        {
            ctx->pos++;
            start = true;
            venc_skip_blank_space(ctx);
            continue;
        }
        if(start)
        {
            if(ctx->buff[ctx->pos] != ' ' && ctx->buff[ctx->pos] != ',')
            {
                c[n] = ctx->buff[ctx->pos];
                c[n+1] = '\0';
                n++;
            }
            else
                return 0;
        }
        ctx->pos++;
    }
    venc_err("char array size too small.\n");
    return -1;

}

int venc_check_attr_config(VENC_Ctx_t *ctx, int chn)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    assert(ctx);
    assert(chn<2 && chn>=0);

    venc_find_next_char(ctx, '{');
    ctx->pos++;

    while(venc_get_item_name(ctx, name, MAX_JSON_STR_LEN) == 0)
    {
        venc_get_item_value(ctx, val_c, MAX_JSON_STR_LEN);
        if(!strcmp(name, "chn"))
            ctx->conf[chn].venc_chn = atoi(val_c);
        else if(!strcmp(name, "w"))
            ctx->conf[chn].pic_width = atoi(val_c);
        else if(!strcmp(name, "h"))
            ctx->conf[chn].pic_height = atoi(val_c);
        else if(!strcmp(name, "codec"))
        {
            if(!strcmp(val_c, "\"h264\""))
                ctx->conf[chn].eModType = E_MI_VENC_MODTYPE_H264E;
            else if(!strcmp(val_c, "\"h265\""))
                ctx->conf[chn].eModType = E_MI_VENC_MODTYPE_H265E;
            else
                ctx->conf[chn].eModType = E_MI_VENC_MODTYPE_JPEGE;
        }
        else if(!strcmp(name, "gop"))
            ctx->conf[chn].gop = atoi(val_c);
        else if(!strcmp(name, "rc"))
        {
            if(!strcmp(val_c, "\"cbr\""))
                if(ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H264E)
                    ctx->conf[chn].eRcMode = E_MI_VENC_RC_MODE_H264CBR;
                else if(ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H265E)
                    ctx->conf[chn].eRcMode = E_MI_VENC_RC_MODE_H265CBR;
                else
                    ctx->conf[chn].eRcMode = E_MI_VENC_RC_MODE_MJPEGCBR;
            else if(!strcmp(val_c, "\"vbr\""))
                if(ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H264E)
                    ctx->conf[chn].eRcMode = E_MI_VENC_RC_MODE_H264VBR;
                else if(ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H265E)
                    ctx->conf[chn].eRcMode = E_MI_VENC_RC_MODE_H265VBR;
                else
                    venc_err("jpeg is not support vbr.");
            else
                if(ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H264E)
                    ctx->conf[chn].eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
                else if(ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H265E)
                    ctx->conf[chn].eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
                else
                    ctx->conf[chn].eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
        }
        else if(!strcmp(name, "bps"))
            ctx->conf[chn].bitrate = atoi(val_c);
        else if(!strcmp(name, "qp"))
            ctx->conf[chn].qp = atoi(val_c);
        else if(!strcmp(name, "fps"))
        {
            ctx->conf[chn].fps = atoi(val_c);
        }
        else if(!strcmp(name, "prof"))
        {
            ctx->conf[chn].profile = atoi(val_c);
        }

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'

    return -1;
}

int venc_check_ins_config(VENC_Ctx_t *ctx, int chn)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    int set = 0;

    if(venc_get_next_char(ctx) != '{')
        return -1;

    ctx->pos++;

    while(venc_get_item_name(ctx, name, MAX_JSON_STR_LEN) == 0)
    {
        venc_get_item_value(ctx, val_c, MAX_JSON_STR_LEN);
        if(!strcmp(name, "set"))
        {
            set = atoi(val_c);
            venc_info("ins:%d\n", set);
            if(set)
                ctx->conf[chn].test_flag |= VENC_TEST_INSERT;
            else
                ctx->conf[chn].test_flag &= ~VENC_TEST_INSERT;
        }
        else if(!strcmp(name, "data"))
        {
            memset(ctx->conf[chn].ins_data, 0, sizeof(ctx->conf[chn].ins_data));
            memcpy(ctx->conf[chn].ins_data, val_c, strlen(val_c));
            ctx->conf[chn].ins_len = strlen(val_c);
        }

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'
    return 0;
}

int venc_check_entropy_config(VENC_Ctx_t *ctx, int chn)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    int set = 0;

    if(venc_get_next_char(ctx) != '{')
        return -1;

    ctx->pos++;

    while(venc_get_item_name(ctx, name, MAX_JSON_STR_LEN) == 0)
    {
        venc_get_item_value(ctx, val_c, MAX_JSON_STR_LEN);
        if(!strcmp(name, "set"))
        {
            set = atoi(val_c);
            venc_info("entropy:%d\n", set);
            if(set)
                ctx->conf[chn].test_flag |= VENC_TEST_ENTROPY;
            else
                ctx->conf[chn].test_flag &= ~VENC_TEST_ENTROPY;
        }
        else if(!strcmp(name, "mode_i"))           ctx->conf[chn].entropy.u32EntropyEncModeI = atoi(val_c);
        else if(!strcmp(name, "mode_p"))           ctx->conf[chn].entropy.u32EntropyEncModeP = atoi(val_c);

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'
    return 0;
}

int venc_check_ref_config(VENC_Ctx_t *ctx, int chn)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    int set = 0;

    if(venc_get_next_char(ctx) != '{')
        return -1;

    ctx->pos++;

    while(venc_get_item_name(ctx, name, MAX_JSON_STR_LEN) == 0)
    {
        venc_get_item_value(ctx, val_c, MAX_JSON_STR_LEN);
        if(!strcmp(name, "set"))
        {
            set = atoi(val_c);
            venc_info("ref:%d\n", set);
            if(set)
                ctx->conf[chn].test_flag |= VENC_TEST_REF;
            else
                ctx->conf[chn].test_flag &= ~VENC_TEST_REF;
        }
        else if(!strcmp(name, "base"))      ctx->conf[chn].ref.u32Base = atoi(val_c);
        else if(!strcmp(name, "enhance"))   ctx->conf[chn].ref.u32Enhance = atoi(val_c);
        else if(!strcmp(name, "pred"))      ctx->conf[chn].ref.bEnablePred = atoi(val_c);

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'
    return 0;
}

int venc_check_crop_config(VENC_Ctx_t *ctx, int chn)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    int set = 0;

    if(venc_get_next_char(ctx) != '{')
        return -1;

    ctx->pos++;

    while(venc_get_item_name(ctx, name, MAX_JSON_STR_LEN) == 0)
    {
        venc_get_item_value(ctx, val_c, MAX_JSON_STR_LEN);
        if(!strcmp(name, "set"))
        {
            set = atoi(val_c);
            venc_info("crop:%d\n", set);
            if(set)
                ctx->conf[chn].test_flag |= VENC_TEST_CROP;
            else
                ctx->conf[chn].test_flag &= ~VENC_TEST_CROP;
        }
        else if(!strcmp(name, "enable"))      ctx->conf[chn].crop.bEnable = atoi(val_c);
        else if(!strcmp(name, "left"))   ctx->conf[chn].crop.stRect.u32Left = atoi(val_c);
        else if(!strcmp(name, "top"))      ctx->conf[chn].crop.stRect.u32Top = atoi(val_c);
        else if(!strcmp(name, "w"))      ctx->conf[chn].crop.stRect.u32Width = atoi(val_c);
        else if(!strcmp(name, "h"))      ctx->conf[chn].crop.stRect.u32Height = atoi(val_c);

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'
    return 0;
}

int venc_check_slisplt_config(VENC_Ctx_t *ctx, int chn)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    int set = 0;

    if(venc_get_next_char(ctx) != '{')
        return -1;

    ctx->pos++;

    while(venc_get_item_name(ctx, name, 16) == 0)
    {
        venc_get_item_value(ctx, val_c, 16);
        if(!strcmp(name, "set"))
        {
            set = atoi(val_c);
            venc_info("slisplt:%d\n", set);
            if(set)
                ctx->conf[chn].test_flag |= VENC_TEST_SLISPLT;
            else
                ctx->conf[chn].test_flag &= ~VENC_TEST_SLISPLT;
        }
        else if(!strcmp(name, "enable"))
        {
            if(ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H264E)
                ctx->conf[chn].slisplt_h264.bSplitEnable = atoi(val_c);
            else
                ctx->conf[chn].slisplt_h265.bSplitEnable = atoi(val_c);
        }
        else if(!strcmp(name, "row"))
        {
            if(ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H264E)
                ctx->conf[chn].slisplt_h264.u32SliceRowCount = atoi(val_c);
            else
                ctx->conf[chn].slisplt_h265.u32SliceRowCount = atoi(val_c);
        }

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'
    return 0;
}

int venc_check_intra_config(VENC_Ctx_t *ctx, int chn)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    int set = 0;
    bool is_h264 = 0;

    if(venc_get_next_char(ctx) != '{')
        return -1;

    ctx->pos++;

    while(venc_get_item_name(ctx, name, MAX_JSON_STR_LEN) == 0)
    {
        venc_get_item_value(ctx, val_c, MAX_JSON_STR_LEN);
        if(!strcmp(name, "set"))
        {
            set = atoi(val_c);
        }
        else if(!strcmp(name, "codec"))
        {
            if(!strcmp(val_c, "\"h264\""))      is_h264 = true;
            else if(!strcmp(val_c, "\"h265\"")) is_h264 = false;
            else venc_err("intra format error.");

            if((is_h264 && ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H264E) ||
                (!is_h264 && ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H265E))
            {
                venc_info("intra:%d\n", set);
                if(set)
                    ctx->conf[chn].test_flag |= VENC_TEST_INTRA;
                else
                    ctx->conf[chn].test_flag &= ~VENC_TEST_INTRA;
            }
        }
        else if(!strcmp(name, "16x16_en"))
        {
            if(is_h264)
                ctx->conf[chn].intra_h264.bIntra16x16PredEn = atoi(val_c);
            else
                ctx->conf[chn].intra_h265.bIntra16x16PredEn = atoi(val_c);
        }
        else if(!strcmp(name, "nxn_en"))
            ctx->conf[chn].intra_h264.bIntraNxNPredEn = atoi(val_c);
        else if(!strcmp(name, "cipf"))
        {
            if(is_h264)
                ctx->conf[chn].intra_h264.bConstrainedIntraPredFlag = atoi(val_c);
            else
                ctx->conf[chn].intra_h265.bConstrainedIntraPredFlag = atoi(val_c);
        }
        else if(!strcmp(name, "ipcm"))
            ctx->conf[chn].intra_h264.bIpcmEn = atoi(val_c);
        else if(!strcmp(name, "16x16p"))
        {
            if(is_h264)
                ctx->conf[chn].intra_h264.u32Intra16x16Penalty = atoi(val_c);
            else
                ctx->conf[chn].intra_h265.u32Intra16x16Penalty = atoi(val_c);
        }
        else if(!strcmp(name, "4x4p"))
            ctx->conf[chn].intra_h264.u32Intra4x4Penalty = atoi(val_c);
        else if(!strcmp(name, "ipp"))
            ctx->conf[chn].intra_h264.bIntraPlanarPenalty = atoi(val_c);
        else if(!strcmp(name, "32x32_en"))
            ctx->conf[chn].intra_h265.bIntra32x32PredEn = atoi(val_c);
        else if(!strcmp(name, "8x8_en"))
            ctx->conf[chn].intra_h265.bIntra8x8PredEn = atoi(val_c);
        else if(!strcmp(name, "32x32p"))
            ctx->conf[chn].intra_h265.u32Intra32x32Penalty = atoi(val_c);
        else if(!strcmp(name, "8x8p"))
            ctx->conf[chn].intra_h265.u32Intra8x8Penalty = atoi(val_c);

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'
    return 0;
}

int venc_check_inter_config(VENC_Ctx_t *ctx, int chn)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    int set = 0;
    bool is_h264 = 0;

    if(venc_get_next_char(ctx) != '{')
        return -1;

    ctx->pos++;

    while(venc_get_item_name(ctx, name, MAX_JSON_STR_LEN) == 0)
    {
        venc_get_item_value(ctx, val_c, MAX_JSON_STR_LEN);
        if(!strcmp(name, "set"))
        {
            set = atoi(val_c);
        }
        else if(!strcmp(name, "codec"))
        {
            if(!strcmp(val_c, "\"h264\""))      is_h264 = true;
            else if(!strcmp(val_c, "\"h265\"")) is_h264 = false;
            else venc_err("inter format error.");

            if((is_h264 && ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H264E) ||
                (!is_h264 && ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H265E))
            {
                venc_info("inter:%d\n", set);
                if(set)
                    ctx->conf[chn].test_flag |= VENC_TEST_INTER;
                else
                    ctx->conf[chn].test_flag &= ~VENC_TEST_INTER;
            }
        }
        else if(!strcmp(name, "h-size"))
        {
            if(is_h264)
                ctx->conf[chn].inter_h264.u32HWSize = atoi(val_c);
            else
                ctx->conf[chn].inter_h265.u32HWSize = atoi(val_c);
        }
        else if(!strcmp(name, "v-size"))
        {
            if(is_h264)
                ctx->conf[chn].inter_h264.u32VWSize = atoi(val_c);
            else
                ctx->conf[chn].inter_h265.u32VWSize = atoi(val_c);
        }
        else if(!strcmp(name, "16x16_en"))
        {
            if(is_h264)
                ctx->conf[chn].inter_h264.bInter16x16PredEn = atoi(val_c);
            else
                ctx->conf[chn].inter_h265.bInter16x16PredEn = atoi(val_c);
        }
        else if(!strcmp(name, "16x8_en"))
        {
            if(is_h264)
                ctx->conf[chn].inter_h264.bInter16x8PredEn = atoi(val_c);
            else
                ctx->conf[chn].inter_h265.bInter16x8PredEn = atoi(val_c);
        }
        else if(!strcmp(name, "8x16_en"))
        {
            if(is_h264)
                ctx->conf[chn].inter_h264.bInter8x16PredEn = atoi(val_c);
            else
                ctx->conf[chn].inter_h265.bInter8x16PredEn = atoi(val_c);
        }
        else if(!strcmp(name, "8x8_en"))
        {
            if(is_h264)
                ctx->conf[chn].inter_h264.bInter8x8PredEn = atoi(val_c);
            else
                ctx->conf[chn].inter_h265.bInter8x8PredEn = atoi(val_c);
        }
        else if(!strcmp(name, "8x4_en"))
        {
            if(is_h264)
                ctx->conf[chn].inter_h264.bInter8x4PredEn = atoi(val_c);
            else
                ctx->conf[chn].inter_h265.bInter8x4PredEn = atoi(val_c);
        }
        else if(!strcmp(name, "4x8_en"))
        {
            if(is_h264)
                ctx->conf[chn].inter_h264.bInter4x8PredEn = atoi(val_c);
            else
                ctx->conf[chn].inter_h265.bInter4x8PredEn = atoi(val_c);
        }
        else if(!strcmp(name, "4x4_en"))
        {
            if(is_h264)
                ctx->conf[chn].inter_h264.bInter4x4PredEn = atoi(val_c);
            else
                ctx->conf[chn].inter_h265.bInter4x4PredEn = atoi(val_c);
        }
        else if(!strcmp(name, "ext_en"))
        {
            if(is_h264)
                ctx->conf[chn].inter_h264.bExtedgeEn = atoi(val_c);
            else
                ctx->conf[chn].inter_h265.bExtedgeEn = atoi(val_c);
        }
        else if(!strcmp(name, "32x32p"))
            ctx->conf[chn].inter_h265.u32Inter32x32Penalty = atoi(val_c);
        else if(!strcmp(name, "16x16p"))
            ctx->conf[chn].inter_h265.u32Inter16x16Penalty = atoi(val_c);
        else if(!strcmp(name, "8x8p"))
            ctx->conf[chn].inter_h265.u32Inter8x8Penalty = atoi(val_c);

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'
    return 0;
}

int venc_check_roi_config(VENC_Ctx_t *ctx, int chn, int n)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    int set = 0;

    if(n >= MAX_ROI_NUM)
        return -1;

    if(venc_get_next_char(ctx) != '{')
        return -1;

    ctx->pos++;

    while(venc_get_item_name(ctx, name, MAX_JSON_STR_LEN) == 0)
    {
        venc_get_item_value(ctx, val_c, MAX_JSON_STR_LEN);
        if(!strcmp(name, "set"))
        {
            set = atoi(val_c);
            venc_info("roi:%d\n", set);
            if(set)
                ctx->conf[chn].test_flag |= VENC_TEST_ROI;
            else
                ctx->conf[chn].test_flag &= ~VENC_TEST_ROI;
        }
        else if(!strcmp(name, "index"))     ctx->conf[chn].roi[n].u32Index = atoi(val_c);
        else if(!strcmp(name, "enable"))    ctx->conf[chn].roi[n].bEnable = atoi(val_c);
        else if(!strcmp(name, "abs"))       ctx->conf[chn].roi[n].bAbsQp = atoi(val_c);
        else if(!strcmp(name, "qp"))        ctx->conf[chn].roi[n].s32Qp = atoi(val_c);
        else if(!strcmp(name, "left"))      ctx->conf[chn].roi[n].stRect.u32Left = atoi(val_c);
        else if(!strcmp(name, "top"))       ctx->conf[chn].roi[n].stRect.u32Top = atoi(val_c);
        else if(!strcmp(name, "w"))       ctx->conf[chn].roi[n].stRect.u32Width = atoi(val_c);
        else if(!strcmp(name, "h"))       ctx->conf[chn].roi[n].stRect.u32Height = atoi(val_c);

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'
    return 0;
}

int venc_check_trans_config(VENC_Ctx_t *ctx, int chn)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    int set = 0;

    if(venc_get_next_char(ctx) != '{')
        return -1;

    ctx->pos++;

    while(venc_get_item_name(ctx, name, 16) == 0)
    {
        venc_get_item_value(ctx, val_c, 16);
        if(!strcmp(name, "set"))
        {
            set = atoi(val_c);
            venc_info("trans:%d\n", set);
            if(set)
                ctx->conf[chn].test_flag |= VENC_TEST_TRANS;
            else
                ctx->conf[chn].test_flag &= ~VENC_TEST_TRANS;
        }
        else if(!strcmp(name, "intra-m"))
        {
            if(ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H264E)
                ctx->conf[chn].trans_h264.u32IntraTransMode = atoi(val_c);
            else
                ctx->conf[chn].trans_h265.u32IntraTransMode = atoi(val_c);
        }
        else if(!strcmp(name, "inter-m"))
        {
            if(ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H264E)
                ctx->conf[chn].trans_h264.u32InterTransMode = atoi(val_c);
            else
                ctx->conf[chn].trans_h265.u32InterTransMode = atoi(val_c);
        }
        else if(!strcmp(name, "cqio"))
        {
            if(ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H264E)
                ctx->conf[chn].trans_h264.s32ChromaQpIndexOffset = atoi(val_c);
            else
                ctx->conf[chn].trans_h265.s32ChromaQpIndexOffset = atoi(val_c);
        }

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'
    return 0;
}

int venc_check_vui_config(VENC_Ctx_t *ctx, int chn)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    int set = 0;
    bool is_h264 = 0;

    if(venc_get_next_char(ctx) != '{')
        return -1;

    ctx->pos++;

    while(venc_get_item_name(ctx, name, MAX_JSON_STR_LEN) == 0)
    {
        venc_get_item_value(ctx, val_c, MAX_JSON_STR_LEN);
        if(!strcmp(name, "set"))
        {
            set = atoi(val_c);
        }
        else if(!strcmp(name, "codec"))
        {
            if(!strcmp(val_c, "\"h264\""))      is_h264 = true;
            else if(!strcmp(val_c, "\"h265\"")) is_h264 = false;
            else venc_err("vui format error.");

            if((is_h264 && ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H264E) ||
                (!is_h264 && ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H265E))
            {
                venc_info("vui:%d\n", set);
                if(set)
                    ctx->conf[chn].test_flag |= VENC_TEST_VUI;
                else
                    ctx->conf[chn].test_flag &= ~VENC_TEST_VUI;
            }
        }
        else if(!strcmp(name, "asp-ratio-flg"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiAspectRatio.u8AspectRatioInfoPresentFlag = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiAspectRatio.u8AspectRatioInfoPresentFlag = atoi(val_c);
        }
        else if(!strcmp(name, "asp-ratio-idc"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiAspectRatio.u8AspectRatioInfoPresentFlag = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiAspectRatio.u8AspectRatioInfoPresentFlag = atoi(val_c);
        }
        else if(!strcmp(name, "ovrscn-if-flg"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiAspectRatio.u8OverscanInfoPresentFlag = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiAspectRatio.u8OverscanInfoPresentFlag = atoi(val_c);
        }
        else if(!strcmp(name, "ovrscn-ap-flg"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiAspectRatio.u8OverscanAppropriateFlag = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiAspectRatio.u8OverscanAppropriateFlag = atoi(val_c);
        }
        else if(!strcmp(name, "sar-w"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiAspectRatio.u16SarWidth = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiAspectRatio.u16SarWidth = atoi(val_c);
        }
        else if(!strcmp(name, "sar-h"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiAspectRatio.u16SarHeight = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiAspectRatio.u16SarHeight = atoi(val_c);
        }
        else if(!strcmp(name, "time-info-flg"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiTimeInfo.u8TimingInfoPresentFlag = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiTimeInfo.u8TimingInfoPresentFlag = atoi(val_c);
        }
        else if(!strcmp(name, "fix-frm-flg"))
            ctx->conf[chn].vui_h264.stVuiTimeInfo.u8FixedFrameRateFlag = atoi(val_c);
        else if(!strcmp(name, "num-uts-in-tk"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiTimeInfo.u32NumUnitsInTick = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiTimeInfo.u32NumUnitsInTick = atoi(val_c);
        }
        else if(!strcmp(name, "time-scl"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiTimeInfo.u32TimeScale = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiTimeInfo.u32TimeScale = atoi(val_c);
        }
        else if(!strcmp(name, "v-sig-flg"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiVideoSignal.u8VideoSignalTypePresentFlag = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiVideoSignal.u8VideoSignalTypePresentFlag = atoi(val_c);
        }
        else if(!strcmp(name, "v-formt"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiVideoSignal.u8VideoFormat = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiVideoSignal.u8VideoFormat = atoi(val_c);
        }
        else if(!strcmp(name, "v-ful-rng-flg"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiVideoSignal.u8VideoFullRangeFlag = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiVideoSignal.u8VideoFullRangeFlag = atoi(val_c);
        }
        else if(!strcmp(name, "clor-desp-flg"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiVideoSignal.u8ColourDescriptionPresentFlag = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiVideoSignal.u8ColourDescriptionPresentFlag = atoi(val_c);
        }
        else if(!strcmp(name, "clor-prim"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiVideoSignal.u8ColourPrimaries = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiVideoSignal.u8ColourPrimaries = atoi(val_c);
        }
        else if(!strcmp(name, "tra-chartic"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiVideoSignal.u8TransferCharacteristics = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiVideoSignal.u8TransferCharacteristics = atoi(val_c);
        }
        else if(!strcmp(name, "mtx-coff"))
        {
            if(is_h264)
                ctx->conf[chn].vui_h264.stVuiVideoSignal.u8MatrixCoefficients = atoi(val_c);
            else
                ctx->conf[chn].vui_h265.stVuiVideoSignal.u8MatrixCoefficients = atoi(val_c);
        }

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'
    return 0;
}

int venc_check_dblk_config(VENC_Ctx_t *ctx, int chn)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    int set = 0;
    bool is_h264 = 0;

    if(venc_get_next_char(ctx) != '{')
        return -1;

    ctx->pos++;

    while(venc_get_item_name(ctx, name, MAX_JSON_STR_LEN) == 0)
    {
        venc_get_item_value(ctx, val_c, MAX_JSON_STR_LEN);
        if(!strcmp(name, "set"))
        {
            set = atoi(val_c);
        }
        else if(!strcmp(name, "codec"))
        {
            if(!strcmp(val_c, "\"h264\""))      is_h264 = true;
            else if(!strcmp(val_c, "\"h265\"")) is_h264 = false;
            else venc_err("dblk format error.");

            if((is_h264 && ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H264E) ||
                (!is_h264 && ctx->conf[chn].eModType == E_MI_VENC_MODTYPE_H265E))
            {
                venc_info("dblk:%d\n", set);
                if(set)
                    ctx->conf[chn].test_flag |= VENC_TEST_DBLK;
                else
                    ctx->conf[chn].test_flag &= ~VENC_TEST_DBLK;
            }
        }
        else if(!strcmp(name, "dis-dblk-flt"))
        {
            if(is_h264)
                ctx->conf[chn].dblk_h264.disable_deblocking_filter_idc = atoi(val_c);
            else
                ctx->conf[chn].dblk_h265.disable_deblocking_filter_idc = atoi(val_c);
        }
        else if(!strcmp(name, "sli-beta-off"))
        {
            if(is_h264)
                ctx->conf[chn].dblk_h264.slice_beta_offset_div2 = atoi(val_c);
            else
                ctx->conf[chn].dblk_h265.slice_beta_offset_div2 = atoi(val_c);
        }
        else if(!strcmp(name, "sli-apha-coff"))
            ctx->conf[chn].dblk_h264.slice_alpha_c0_offset_div2 = atoi(val_c);
        else if(!strcmp(name, "sli-tc-off"))
            ctx->conf[chn].dblk_h265.slice_tc_offset_div2 = atoi(val_c);

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'
    return 0;
}

int venc_check_config_chn(VENC_Ctx_t *ctx)
{
    char name[MAX_JSON_STR_LEN];
    char val_c[MAX_JSON_STR_LEN];
    int chn = 0;

    if(venc_get_next_char(ctx) != '{')
        return -1;

    ctx->pos++;

    while(venc_get_item_name(ctx, name, MAX_JSON_STR_LEN) == 0)
    {
        if(!strcmp(name, "id"))
        {
            venc_get_item_value(ctx, val_c, MAX_JSON_STR_LEN);
            chn = atoi(val_c);
            venc_info("chn:%d\n", chn);
        }
        else if(!strcmp(name, "attr"))      venc_check_attr_config(ctx, chn);
        else if(!strcmp(name, "ins"))       venc_check_ins_config(ctx, chn);
        else if(!strcmp(name, "roi"))
        {
            venc_find_next_char(ctx, ':');
            if(venc_get_next_char(ctx) == '[')
            {
                ctx->conf[chn].roi_num = 0;
                do{
                    ctx->pos++;
                    venc_check_roi_config(ctx, chn, ctx->conf[chn].roi_num);
                    ctx->conf[chn].roi_num++;
                }while(venc_get_curr_char(ctx) == ',');
                venc_find_next_char(ctx, ']');
                ctx->pos++; // skip ']'
            }
            else
            {
                venc_check_roi_config(ctx, chn, 0);
            }
        }
        else if(!strcmp(name, "entropy"))   venc_check_entropy_config(ctx, chn);
        else if(!strcmp(name, "ref"))   venc_check_ref_config(ctx, chn);
        else if(!strcmp(name, "crop"))   venc_check_crop_config(ctx, chn);
        else if(!strcmp(name, "slisplt"))   venc_check_slisplt_config(ctx, chn);
        else if(!strcmp(name, "intra"))
        {
            venc_find_next_char(ctx, ':');
            if(venc_get_next_char(ctx) == '[')
            {
                do{
                    ctx->pos++;
                    venc_check_intra_config(ctx, chn);
                }while(venc_get_curr_char(ctx) == ',');
                venc_find_next_char(ctx, ']');
                ctx->pos++; // skip ']'
            }
            else
            {
                venc_check_intra_config(ctx, chn);
            }
        }
        else if(!strcmp(name, "inter"))
        {
            venc_find_next_char(ctx, ':');
            if(venc_get_next_char(ctx) == '[')
            {
                do{
                    ctx->pos++;
                    venc_check_inter_config(ctx, chn);
                }while(venc_get_curr_char(ctx) == ',');
                venc_find_next_char(ctx, ']');
                ctx->pos++; // skip ']'
            }
            else
            {
                venc_check_inter_config(ctx, chn);
            }
        }
        else if(!strcmp(name, "trans"))   venc_check_trans_config(ctx, chn);
        else if(!strcmp(name, "vui"))
        {
            venc_find_next_char(ctx, ':');
            if(venc_get_next_char(ctx) == '[')
            {
                do{
                    ctx->pos++;
                    venc_check_vui_config(ctx, chn);
                }while(venc_get_curr_char(ctx) == ',');
                venc_find_next_char(ctx, ']');
                ctx->pos++; // skip ']'
            }
            else
            {
                venc_check_vui_config(ctx, chn);
            }
        }
        else if(!strcmp(name, "dblk"))
        {
            venc_find_next_char(ctx, ':');
            if(venc_get_next_char(ctx) == '[')
            {
                do{
                    ctx->pos++;
                    venc_check_dblk_config(ctx, chn);
                }while(venc_get_curr_char(ctx) == ',');
                venc_find_next_char(ctx, ']');
                ctx->pos++; // skip ']'
            }
            else
            {
                venc_check_dblk_config(ctx, chn);
            }
        }

        if(venc_get_curr_char(ctx) != ',')
        {
            break;
        }
        ctx->pos++; // skip ','
    }

    venc_find_next_char(ctx, '}'); //go to chn config end
    ctx->pos++; // skip '}'
    return 0;
}


int init_venc_config(VENC_Ctx_t *ctx)
{
    char name[MAX_JSON_STR_LEN];
    if(fgets(ctx->buff, sizeof(ctx->buff), ctx->fd) == NULL)
        return -1;
    ctx->pos = 0;

    venc_find_next_char(ctx, '{');
    ctx->pos++;
    venc_get_item_name(ctx, name, MAX_JSON_STR_LEN);

    if(!strcmp(name, "chn"))
    {
        venc_find_next_char(ctx, ':');
        if(venc_get_next_char(ctx) == '[')
        {
            do{
                ctx->pos++;
                venc_check_config_chn(ctx);
            }while(venc_get_next_char(ctx) == ',');
            venc_find_next_char(ctx, ']');
        }
        else
        {
            venc_check_config_chn(ctx);
        }
    }

    return 0;
}

int venc_read_config_file(VENC_Ctx_t *ctx)
{
    FILE *fd = fopen(VENC_CONFIG_PATH, "r");
    if(!fd)
    {
        venc_err("%s open failed.\n", VENC_CONFIG_PATH);
        return -1;
    }
    ctx->fd = fd;
    return 0;
}

void venc_handle_sig(MI_S32 signo)
{
    if (signo == SIGINT)
    {
        venc_info("catch Ctrl + C, exit\n");

        g_bExit = TRUE;
        exit(0);
    }
}

void venc_set_config(VENC_ChnConf_t *conf)
{
    MI_S32 s32Ret;

    if(conf->test_flag & VENC_TEST_INSERT)
    {
        s32Ret = MI_VENC_InsertUserData(conf->venc_chn, conf->ins_data, conf->ins_len);
        if(s32Ret)
            venc_err("MI_VENC_InsertUserData error, %X\n", s32Ret);
        else
            venc_info("MI_VENC_InsertUserData Success\n");
    }
    if(conf->test_flag & VENC_TEST_ROI)
    {
        int i = 0;
        for(i = 0; i < conf->roi_num && i < MAX_ROI_NUM; i++)
        {
            s32Ret = MI_VENC_SetRoiCfg(conf->venc_chn, &conf->roi[i]);
            if(s32Ret)
                venc_err("MI_VENC_SetRoiCfg %d error, %X\n", i, s32Ret);
            else
                venc_info("MI_VENC_SetRoiCfg %d Success\n", i);
        }
    }
    if(conf->test_flag & VENC_TEST_ENTROPY && conf->eModType == E_MI_VENC_MODTYPE_H264E)
    {
        s32Ret = MI_VENC_SetH264Entropy(conf->venc_chn, &conf->entropy);
        if(s32Ret)
            venc_err("MI_VENC_SetH264Entropy error, %X\n", s32Ret);
        else
            venc_info("MI_VENC_SetH264Entropy Success\n");
    }
    if(conf->test_flag & VENC_TEST_REF)
    {
        s32Ret = MI_VENC_SetRefParam(conf->venc_chn, &conf->ref);
        if(s32Ret)
            venc_err("MI_VENC_SetRefParam error, %X\n", s32Ret);
        else
            venc_info("MI_VENC_SetRefParam Success\n");
    }
    if(conf->test_flag & VENC_TEST_CROP)
    {
        s32Ret = MI_VENC_SetCrop(conf->venc_chn, &conf->crop);
        if(s32Ret)
            venc_err("MI_VENC_SetCrop error, %X\n", s32Ret);
        else
            venc_info("MI_VENC_SetCrop Success\n");
    }
    if(conf->test_flag & VENC_TEST_SLISPLT)
    {
        if(conf->eModType == E_MI_VENC_MODTYPE_H264E)
            s32Ret = MI_VENC_SetH264SliceSplit(conf->venc_chn, &conf->slisplt_h264);
        else if(conf->eModType == E_MI_VENC_MODTYPE_H265E)
            s32Ret = MI_VENC_SetH265SliceSplit(conf->venc_chn, &conf->slisplt_h265);
        else
            s32Ret = -1;
        if(s32Ret)
            venc_err("MI_VENC_SetH26xSliceSplit error, %X\n", s32Ret);
        else
            venc_info("MI_VENC_SetH26xSliceSplit Success\n");
    }
    if(conf->test_flag & VENC_TEST_INTRA)
    {
        if(conf->eModType == E_MI_VENC_MODTYPE_H264E)
            s32Ret = MI_VENC_SetH264IntraPred(conf->venc_chn, &conf->intra_h264);
        else if(conf->eModType == E_MI_VENC_MODTYPE_H265E)
            s32Ret = MI_VENC_SetH265IntraPred(conf->venc_chn, &conf->intra_h265);
        else
            s32Ret = -1;
        if(s32Ret)
            venc_err("MI_VENC_SetH26xIntraPred error, %X\n", s32Ret);
        else
            venc_info("MI_VENC_SetH26xIntraPred Success\n");
    }
    if(conf->test_flag & VENC_TEST_INTER)
    {
        if(conf->eModType == E_MI_VENC_MODTYPE_H264E)
            s32Ret = MI_VENC_SetH264InterPred(conf->venc_chn, &conf->inter_h264);
        else if(conf->eModType == E_MI_VENC_MODTYPE_H265E)
            s32Ret = MI_VENC_SetH265InterPred(conf->venc_chn, &conf->inter_h265);
        else
            s32Ret = -1;
        if(s32Ret)
            venc_err("MI_VENC_SetH26xInterPred error, %X\n", s32Ret);
        else
            venc_info("MI_VENC_SetH26xInterPred Success\n");
    }
    if(conf->test_flag & VENC_TEST_TRANS)
    {
        if(conf->eModType == E_MI_VENC_MODTYPE_H264E)
            s32Ret = MI_VENC_SetH264Trans(conf->venc_chn, &conf->trans_h264);
        else if(conf->eModType == E_MI_VENC_MODTYPE_H265E)
            s32Ret = MI_VENC_SetH265Trans(conf->venc_chn, &conf->trans_h265);
        else
            s32Ret = -1;
        if(s32Ret)
            venc_err("MI_VENC_SetH26xTrans error, %X\n", s32Ret);
        else
            venc_info("MI_VENC_SetH26xTrans Success\n");
    }
    if(conf->test_flag & VENC_TEST_VUI)
    {
        if(conf->eModType == E_MI_VENC_MODTYPE_H264E)
            s32Ret = MI_VENC_SetH264Vui(conf->venc_chn, &conf->vui_h264);
        else if(conf->eModType == E_MI_VENC_MODTYPE_H265E)
            s32Ret = MI_VENC_SetH265Vui(conf->venc_chn, &conf->vui_h265);
        else
            s32Ret = -1;
        if(s32Ret)
            venc_err("MI_VENC_SetH26xVui error, %X\n", s32Ret);
        else
            venc_info("MI_VENC_SetH26xVui Success\n");
    }
    if(conf->test_flag & VENC_TEST_DBLK)
    {
        if(conf->eModType == E_MI_VENC_MODTYPE_H264E)
            s32Ret = MI_VENC_SetH264Dblk(conf->venc_chn, &conf->dblk_h264);
        else if(conf->eModType == E_MI_VENC_MODTYPE_H265E)
            s32Ret = MI_VENC_SetH265Dblk(conf->venc_chn, &conf->dblk_h265);
        else
            s32Ret = -1;
        if(s32Ret)
            venc_err("MI_VENC_SetH26xDblk error, %X\n", s32Ret);
        else
            venc_info("MI_VENC_SetH26xDblk Success\n");
    }

}

void venc_output_thread(void *args)
{
    VENC_ChnConf_t *pConf = (VENC_ChnConf_t *)args;

    MI_SYS_ChnPort_t stVencChnInputPort;
    char szFileName[128];
    int fd = -1;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0;
    MI_U32 u32DevId = 0;
    MI_S32 vencFd = -1;
    fd_set read_fds;

    MI_VENC_GetChnDevid(pConf->venc_chn, &u32DevId);

    stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnInputPort.u32DevId = u32DevId;
    stVencChnInputPort.u32ChnId = pConf->venc_chn;
    stVencChnInputPort.u32PortId = 0;

    memset(szFileName, 0, sizeof(szFileName));

    len = snprintf(szFileName, sizeof(szFileName) - 1, "venc_dev%d_chn%d_port%d_%dx%d.",
        stVencChnInputPort.u32DevId, stVencChnInputPort.u32ChnId, stVencChnInputPort.u32PortId,
        pConf->pic_width, pConf->pic_height);
    if (pConf->eModType == E_MI_VENC_MODTYPE_H264E)
    {
        snprintf(szFileName + len, sizeof(szFileName) - 1, "%s", "h264");
    }
    else if (pConf->eModType == E_MI_VENC_MODTYPE_H265E)
    {
        snprintf(szFileName + len, sizeof(szFileName) - 1, "%s", "h265");
    }
    else
    {
        snprintf(szFileName + len, sizeof(szFileName) - 1, "%s", "mjpeg");
    }

    fd = open(szFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
    {
        venc_err("create %s error\n", szFileName);
        close(fd);
        return;
    }

    venc_info("create %s success\n", szFileName);

    vencFd = MI_VENC_GetFd(pConf->venc_chn);
    if(vencFd <= 0)
    {
        venc_err("Unable to get FD:%d for chn:%2d\n", vencFd, pConf->venc_chn);
        close(fd);
        return;
    }
    else
    {
        venc_info("Venc Chn%2d FD:%d\n", pConf->venc_chn, vencFd);
    }

    while(pConf->bStart)
    {
        struct timeval TimeoutVal;
        MI_VENC_ChnStat_t stStat;
        MI_VENC_Stream_t stStream;
        int i;
        TimeoutVal.tv_sec  = 2;
        TimeoutVal.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(vencFd, &read_fds);
        s32Ret = select(vencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            venc_err("select failed\n");
            usleep(10 * 1000);
            continue;
        }
        else if (0 == s32Ret)
        {
            venc_info("select timeout\n");
            usleep(10 * 1000);
            continue;
        }
        else
        {
            if (FD_ISSET(vencFd, &read_fds))
            {
                memset(&stStat, 0, sizeof(MI_VENC_ChnStat_t));
                s32Ret = MI_VENC_Query(pConf->venc_chn, &stStat);
                if (MI_SUCCESS != s32Ret || stStat.u32CurPacks == 0)
                {
                    venc_err("MI_VENC_Query error, %X\n", s32Ret);
                    usleep(10 * 1000);//sleep 10 ms
                    continue;
                }

                //printf("u32CurPacks:%d, u32LeftStreamFrames:%d\n", stStat.u32CurPacks, stStat.u32LeftStreamFrames);

                memset(&stStream, 0, sizeof(MI_VENC_Stream_t));

                stStream.u32PackCount = 1;
                stStream.pstPack = (MI_VENC_Pack_t *)malloc(sizeof(MI_VENC_Pack_t) * stStream.u32PackCount);
                s32Ret = MI_VENC_GetStream(pConf->venc_chn, &stStream, -1);
                if (MI_SUCCESS == s32Ret)
                {
                    printf("u32PackCount:%d, u32Seq:%d, offset:%d, len:%d, type:%d, pts:0x%llx\n", stStream.u32PackCount, stStream.u32Seq,
                        stStream.pstPack[0].u32Offset, stStream.pstPack[0].u32Len, stStream.pstPack[0].stDataType.eH264EType, stStream.pstPack[0].u64PTS);

                    if(pConf->eModType == E_MI_VENC_MODTYPE_H264E)
                    {
                        printf("eRefType:%d\n", stStream.stH264Info.eRefType);
                    }
                    else if(pConf->eModType == E_MI_VENC_MODTYPE_H265E)
                    {
                        printf("eRefType:%d\n", stStream.stH265Info.eRefType);
                    }

                    for (i = 0; i < stStream.u32PackCount; i ++)
                    {
                        write(fd, stStream.pstPack[i].pu8Addr + stStream.pstPack[i].u32Offset,
                            stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset);
                    }

                    MI_VENC_ReleaseStream(pConf->venc_chn, &stStream);
                }
                else
                {
                    venc_err("MI_VENC_GetStream error, %X\n", s32Ret);
                    usleep(10 * 1000);//sleep 10 ms
                }
                //if(g_idr_config && _test_idr_enable == FALSE)
                if(0)
                {
                    g_frameCnt ++;
                    if(g_frameCnt == g_skipFrame)
                    {
                        MI_VENC_ChnAttr_t stAttr;
                        //g_skipFrame += 5;
                        //g_frameCnt = 0;
                        //MI_VENC_RequestIdr(pCtx->vencChn, TRUE);
                        memset(&stAttr, 0, sizeof(MI_VENC_ChnAttr_t));
                        MI_VENC_GetChnAttr(pConf->venc_chn, &stAttr);
                        stAttr.stVeAttr.stAttrH264e.u32PicWidth = 1280;
                        stAttr.stVeAttr.stAttrH264e.u32PicHeight = 720;
                        MI_VENC_SetChnAttr(pConf->venc_chn, &stAttr);
                        printf("change res\n");
                    }
                }
            }
        }
    }

    close(fd);
    s32Ret = MI_VENC_CloseFd(pConf->venc_chn);
    if(s32Ret != 0)
    {
        venc_err("Chn%02d CloseFd error, Ret:%X\n", pConf->venc_chn, s32Ret);
    }
}

void *venc_input_thread(void *args)
{
    VENC_ChnConf_t *conf = (VENC_ChnConf_t *)args;

    MI_SYS_ChnPort_t stVencChnInputPort;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufInfo_t stBufInfo;
    FILE *pFile = NULL;
    MI_U32 u32FrameSize = 0;
    MI_U32 u32YSize = 0;
    MI_U32 u32FilePos = 0;
    struct stat st;
    MI_U32 u32DevId = 0;

    memset(&stVencChnInputPort, 0, sizeof(MI_SYS_ChnPort_t));
    MI_VENC_GetChnDevid(conf->venc_chn, &u32DevId);

    venc_info("chn:%d, dev:%d\n", conf->venc_chn, u32DevId);

    stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnInputPort.u32DevId = u32DevId;
    stVencChnInputPort.u32ChnId = conf->venc_chn;
    stVencChnInputPort.u32PortId = 0;

    memset(&stBufConf, 0, sizeof(MI_SYS_BufConf_t));
    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.stFrameCfg.u16Width = conf->yuv_width;
    stBufConf.stFrameCfg.u16Height = conf->yuv_height;

    u32YSize = conf->yuv_width * conf->yuv_height;
    u32FrameSize = (u32YSize >> 1) + u32YSize;

    if ((s32Ret = stat(conf->yuv_path, &st)) != 0)
    {
        venc_err("stat %s error:%x\n", conf->yuv_path, s32Ret);
    }

    pFile = fopen(conf->yuv_path, "rb");
    if (pFile == NULL)
    {
        venc_err("open %s error\n", conf->yuv_path);
        return NULL;
    }

    if(st.st_size == 0)
    {
        st.st_size = u32FrameSize;
    }

    venc_info("open %s success, total size:%lld bytes\n", conf->yuv_path, (long long)st.st_size);

    venc_info("chn:%d u32YSize:%d,u32FrameSize:%d\n", stVencChnInputPort.u32ChnId,
        u32YSize, u32FrameSize);

    while (conf->bStart)
    {
        memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));

        u32FilePos = ftell(pFile);
        if ((st.st_size - u32FilePos) < u32FrameSize)
        {
            fseek(pFile, 0L, SEEK_SET);
        }

        s32Ret = MI_SYS_ChnInputPortGetBuf(&stVencChnInputPort, &stBufConf, &stBufInfo, &hHandle, 1000);
        if(MI_SUCCESS == s32Ret)
        {
            if (0 >= fread(stBufInfo.stFrameData.pVirAddr[0], 1, u32YSize, pFile))
            {
                fseek(pFile, 0, SEEK_SET);

                stBufInfo.bEndOfStream = TRUE;
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
                if (MI_SUCCESS != s32Ret)
                {
                    venc_err("MI_SYS_ChnInputPortPutBuf error, %X\n", s32Ret);
                }
                continue;
            }

            if (0 >= fread(stBufInfo.stFrameData.pVirAddr[1], 1, u32YSize >> 1, pFile))
            {
                fseek(pFile, 0, SEEK_SET);

                stBufInfo.bEndOfStream = TRUE;
                s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
                if (MI_SUCCESS != s32Ret)
                {
                    venc_err("MI_SYS_ChnInputPortPutBuf error, %X\n", s32Ret);
                }
                continue;
            }

            s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
            if (MI_SUCCESS != s32Ret)
            {
                venc_err("MI_SYS_ChnInputPortPutBuf error, %X\n", s32Ret);
            }
        }
        else
        {
            venc_err("MI_SYS_ChnInputPortGetBuf error, chn:%d, %X\n", conf->venc_chn, s32Ret);
        }
    }

    fclose(pFile);

    return NULL;
}

int venc_start_chn(VENC_ChnConf_t *conf)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VENC_CHN VencChn = conf->venc_chn;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_SYS_ChnPort_t stVencChnOutputPort;

    memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
    memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

    if (E_MI_VENC_MODTYPE_H264E == conf->eModType)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
        stChnAttr.stVeAttr.stAttrH264e.u32PicWidth = conf->pic_width;
        stChnAttr.stVeAttr.stAttrH264e.u32PicHeight = conf->pic_height;
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = conf->pic_width;
        stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = conf->pic_height;
        stChnAttr.stVeAttr.stAttrH264e.bByFrame = true;
        stChnAttr.stVeAttr.stAttrH264e.u32Profile = conf->profile;

        if(conf->eRcMode == E_MI_VENC_RC_MODE_H264FIXQP)
        {
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateNum = conf->fps;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = conf->gop;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = conf->qp;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = conf->qp;
        }
        else if(conf->eRcMode == E_MI_VENC_RC_MODE_H264CBR)
        {
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264CBR;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = conf->bitrate;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = conf->gop;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateNum = conf->fps;
            stChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRateDen = 1;
        }
        else
        {
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264VBR;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = conf->bitrate;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = 48;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp = 12;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = conf->gop;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateNum = conf->fps;
            stChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRateDen = 1;
        }
    }
    else if (E_MI_VENC_MODTYPE_H265E == conf->eModType)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
        stChnAttr.stVeAttr.stAttrH265e.u32PicWidth = conf->pic_width;
        stChnAttr.stVeAttr.stAttrH265e.u32PicHeight = conf->pic_height;
        stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = conf->pic_width;
        stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = conf->pic_height;
        stChnAttr.stVeAttr.stAttrH265e.bByFrame = true;

        if(conf->eRcMode == E_MI_VENC_RC_MODE_H265FIXQP)
        {
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateNum = conf->fps;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32Gop = conf->gop;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32IQp = conf->qp;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32PQp = conf->qp;
        }
        else if(conf->eRcMode == E_MI_VENC_RC_MODE_H265CBR)
        {
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265CBR;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = conf->bitrate;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateNum = conf->fps;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRateDen = 1;
            stChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = conf->gop;
        }
        else
        {
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265VBR;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = conf->bitrate;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = conf->gop;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 48;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 12;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateNum = conf->fps;
            stChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRateDen = 1;
        }
    }
    else if (E_MI_VENC_MODTYPE_JPEGE == conf->eModType)
    {
        stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
        stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
        stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth = conf->pic_width;
        stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight = conf->pic_height;
        stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth = conf->pic_width;
        stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight = conf->pic_height;
        stChnAttr.stVeAttr.stAttrJpeg.bByFrame = true;
        stChnAttr.stRcAttr.stAttrMjpegFixQp.u32Qfactor = conf->qp;
    }

    s32Ret = MI_VENC_CreateChn(VencChn, &stChnAttr);
    if (MI_SUCCESS != s32Ret)
    {
        venc_err("MI_VENC_CreateChn %d error, %X\n", VencChn, s32Ret);
    }
    venc_info(" MI_VENC_CreateChn, vencChn:%d\n", VencChn);

    s32Ret = MI_VENC_SetMaxStreamCnt(VencChn, 4);
    if(MI_SUCCESS != s32Ret)
    {
        venc_err("MI_VENC_SetMaxStreamCnt %d error, %X\n", VencChn, s32Ret);
    }

    venc_set_config(conf);

    conf->vencFd = MI_VENC_GetFd(VencChn);

    venc_info("venc chn:%d\n", VencChn);

    // get es stream
    pthread_create(&conf->pt_output, NULL, (void*)venc_output_thread, (void *)conf);

    // put yuv data to venc
    pthread_create(&conf->pt_input, NULL, (void*)venc_input_thread, (void *)conf);

    s32Ret = MI_VENC_StartRecvPic(VencChn);
    if (MI_SUCCESS != s32Ret)
    {
        venc_err("MI_VENC_StartRecvPic %d error, %X\n", VencChn, s32Ret);
    }

    if(0)
    {
        s32Ret = MI_VENC_SetRoiCfg(0, conf->roi);
        if(s32Ret)
            venc_err("MI_VENC_SetRoiCfg error, %X\n", s32Ret);
        else
            venc_info("MI_VENC_SetRoiCfg Success\n");
    }

    return 0;
}

int venc_end(void)
{
    MI_VENC_CHN VencChn = 0;
    MI_S32 s32Ret = MI_SUCCESS;

    s32Ret = MI_VENC_StopRecvPic(VencChn);
    if(s32Ret)
        venc_err("MI_VENC_StopRecvPic error, %X\n", s32Ret);
    else
        venc_info("MI_VENC_StopRecvPic Success\n");

    s32Ret = MI_VENC_DestroyChn(VencChn);
    if(s32Ret)
        venc_err("MI_VENC_DestroyChn error, %X\n", s32Ret);
    else
        venc_info("MI_VENC_DestroyChn Success\n");

    return 0;
}


int main(int argc, char **argv)
{
    VENC_Ctx_t ctx;
    memset(&ctx, 0, sizeof(VENC_Ctx_t));
    venc_parse_options(&ctx, argc, argv);

    struct sigaction sigAction;
    char szCmd[16];

    sigAction.sa_handler = venc_handle_sig;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags = 0;
    sigaction(SIGINT, &sigAction, NULL);

    if(venc_read_config_file(&ctx))
    {
        return -1;
    }

    init_venc_config(&ctx);

    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    while (!g_bExit)
    {
        if (ctx.conf[0].bStart == FALSE)
        {
            ctx.conf[0].bStart = TRUE;
            venc_start_chn(ctx.conf);
        }

        venc_info("press q to exit:\n");
        scanf("%s", szCmd);
        if(!strcmp(szCmd, "q"))
        {
            ctx.conf[0].bStart = FALSE;
            g_bExit = TRUE;
        }
    }

    pthread_join(ctx.conf[0].pt_input, NULL);
    pthread_join(ctx.conf[0].pt_output, NULL);

    venc_end();
    ExecFunc(MI_SYS_Exit(), MI_SUCCESS);

    return 0;
}

