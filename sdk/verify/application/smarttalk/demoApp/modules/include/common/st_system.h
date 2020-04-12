#ifndef _ST_SYSTEM_H_
#define _ST_SYSTEM_H_

#include "st_common.h"

#ifdef  __cplusplus
extern "C"
{
#endif

typedef struct st_SystemNetCfg_s
{
    MI_U32 u32LocalIP;
    MI_U32 u23SubMaskIP;
    MI_U32 u32GateWayIP;
} st_SystemNetCfg_T;

typedef enum
{
    E_ST_IDLE = 0,
    E_ST_BUSY,
    E_ST_CALLING,
    E_ST_MONITOR,
    E_ST_TALKING,
    E_ST_UPGRADE
} ST_SystemStatus_e;

MI_U8 *ST_System_GetLocalID();
MI_S32 ST_System_GetDeviceType(MI_S32 *s32DeviceType);
MI_S32 ST_System_InitCfg();
unsigned long ST_System_GetIPByID(MI_U8 *pu8ID);

#ifdef  __cplusplus
}
#endif

#endif //_ST_SYSTEM_H_
