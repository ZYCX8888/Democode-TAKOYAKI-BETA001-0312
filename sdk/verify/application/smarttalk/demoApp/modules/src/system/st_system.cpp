#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/if.h>
#include <errno.h>

#include "app_config.h"
#include "st_common.h"
#include "st_system.h"

static st_SystemNetCfg_T g_stSysCfg;
static SMART_BD_Machine_CFG_T g_stBD_Machine[2];

MI_U8 *ST_System_GetLocalID()
{
    return NULL;
}

MI_S32 ST_System_GetDeviceType(MI_S32 *s32DeviceType)
{
    
    return MI_SUCCESS; 
}

unsigned long ST_System_GetIPByID(MI_U8 *pu8ID)
{
    MI_S32 i;
    for (i = 0; i < 2; i++)
    {
        if (0 == strncmp((const char *)g_stBD_Machine[i].u8LocalID, (const char *)pu8ID, 8))
        {
            return inet_addr((const char*)g_stBD_Machine[i].u8IPaddr);
        }
    }

    return 0;
}

MI_S32 ST_System_InitCfg()
{
    st_SystemNetCfg_T *pstSysCfg = &g_stSysCfg;

    pstSysCfg->u32LocalIP = 0xAC1318AB;//172.19.24.171
    pstSysCfg->u23SubMaskIP = 0xFFFF0000;
    pstSysCfg->u32GateWayIP = 0xAC1318FE;

    //g_stBD_Machine[0].u8IPaddr = 0xAB1813AC;
    memset(g_stBD_Machine, 0x0, 2*sizeof(SMART_BD_Machine_CFG_T));
    memcpy(g_stBD_Machine[0].u8IPaddr, "172.19.24.171", strlen("172.19.24.171"));
    memcpy(g_stBD_Machine[0].u8LocalID, "01010101", strlen("01010101"));
    memcpy(g_stBD_Machine[0].u8Telphone, "13966668888", strlen("13966668888"));
    g_stBD_Machine[0].s32DeviceType = E_ST_DEV_ROOM;
    g_stBD_Machine[0].s32MainID = 1;
    g_stBD_Machine[0].s32SubID = 1;

    memcpy(g_stBD_Machine[1].u8IPaddr, "172.19.24.172", strlen("172.19.24.172"));
    memcpy(g_stBD_Machine[1].u8LocalID, "01010102", strlen("01010102"));
    memcpy(g_stBD_Machine[1].u8Telphone, "13866668888", strlen("13866668888"));
    g_stBD_Machine[1].s32DeviceType = E_ST_DEV_ROOM;
    g_stBD_Machine[1].s32MainID = 2;
    g_stBD_Machine[1].s32SubID = 1;

    return MI_SUCCESS;
}
