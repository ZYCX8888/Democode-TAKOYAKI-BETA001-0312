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
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>

#include "st_common.h"
#include "st_panel.h"
//#include "InnoLux_1280x800.h"
//#include "JD9366_800x1280.h"
#include "RM68200_720x1280.h"
//#include "NEW_PANEL_800x480.h"
//-------------------------------------------------------------------------------------------------
// Pirvate Function
//-------------------------------------------------------------------------------------------------

MI_S32 ST_Panel_Init(MI_PANEL_LinkType_e enLinkType)
{
    MI_PANEL_ParamConfig_t stParamCfg;
    MI_PANEL_MipiDsiConfig_t stMipiDsiCfg;

    ExecFunc(MI_PANEL_Init(enLinkType), MI_SUCCESS);

    memset(&stParamCfg, 0, sizeof(MI_PANEL_ParamConfig_t));
    memset(&stMipiDsiCfg, 0, sizeof(MI_PANEL_MipiDsiConfig_t));
#if 0
    if(enLinkType == E_MI_PNL_LINK_LVDS)
    {
        memcpy(&stParamCfg, astPanel_InnovLux1280x800, sizeof(MI_PANEL_ParamConfig_t));
        ExecFunc(MI_PANEL_SetPanelParam(&stParamCfg), MI_SUCCESS);
    }
#endif
    if(enLinkType == E_MI_PNL_LINK_MIPI_DSI)
    {
        memcpy(&stParamCfg, &gstPanelParam, sizeof(MI_PANEL_ParamConfig_t));
        memcpy(&stMipiDsiCfg, &astMipiDsiConfig, sizeof(MI_PANEL_MipiDsiConfig_t));
        ExecFunc(MI_PANEL_SetPanelParam(&stParamCfg), MI_SUCCESS);
        ExecFunc(MI_PANEL_SetMipiDsiConfig(&stMipiDsiCfg), MI_SUCCESS);
    }
    else
    {
        ST_ERR("eLinkType is invalid, type:%d\n", (int)enLinkType);
        return -1;
    }

    return MI_SUCCESS;
}

MI_S32 ST_Panel_DeInit()
{
    ExecFunc(MI_PANEL_DeInit(), MI_SUCCESS);

    return MI_SUCCESS;
}

MI_PANEL_ParamConfig_t *ST_Panel_GetParam(MI_PANEL_LinkType_e enLinkType)
{
    return &gstPanelParam;
}

