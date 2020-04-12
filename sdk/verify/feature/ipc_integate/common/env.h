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
#ifndef __ENV_H__
#define __ENV_H__

#include "mi_common_datatype.h"

#define DEF_QP (25)
#define MAX_CFG_STR_LEN (64)

#define BUILD_YUV_H (288) //0 for none, 240 for 320x240, 288 for 352x288.


#if BUILD_YUV_H == 288
    #define BUILD_YUV_W (352)
#elif BUILD_YUV_H == 240
    #define BUILD_YUV_W (240)
#endif

void dump_cfg(void);
void env_dump_cfg_2_col(void);
void get_cfg_from_env(void);
int find_cfg_idx(char *szCfg);
MI_S32 get_cfg_int(char *szCfg, MI_BOOL *pbError);
char* get_cfg_str(char *szCfg, MI_BOOL *pbError);
MI_BOOL set_cfg_str(char *szCfg, char *szValue);
void set_result_int(char *szCfg, int value);
#endif
