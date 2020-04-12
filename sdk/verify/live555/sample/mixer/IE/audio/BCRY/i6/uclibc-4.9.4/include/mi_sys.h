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
#ifndef __MI_SYS_H__
#define __MI_SYS_H__
#ifdef __cplusplus

extern "C"
{
#endif

#include "mi_common.h"

/**
 * Get UUID
 */
typedef struct
{
    unsigned int VerChk_Version;
    unsigned long long udid;
    unsigned int VerChk_Size;
} __attribute__((__packed__)) MSYS_UNIQUEID_INFO;

/**
 * system config info
 */
typedef struct _SysConf_t
{
    /* stride of picture buffer must be aligned with this value.
     * you can choose a value from 1 to 1024, and it must be multiple of 16.
     */
    U32 alignWidth;

} SysConf_t;

typedef struct _CustomID_t
{
    U8 id[5];
} CustomID_t;

#define VERSION_NAME_MAXLEN 64
/**
 * system version
 */
typedef struct _SysVersion_t
{
    S8 version[VERSION_NAME_MAXLEN];
} SysVersion_t;


/**
 * set system config
 */
MI_RET MI_SYS_SetConf(const SysConf_t *pstSysConf);

/**
 * get system config
 */
MI_RET MI_SYS_GetConf(SysConf_t *pstSysConf);

/**
 * system init
 */
MI_RET MI_SYS_Init();

/**
 * system exit
 */
MI_RET MI_SYS_Exit();

/**
 * get mi version
 */
MI_RET MI_SYS_GetVersion(SysVersion_t *pstVersion);

/**
 * printf mallinfo
*/
void MI_SYS_PrintMeminfo(void);

/**
 * bind channel
 **/
MI_RET MI_SYS_Bind(Chn_t *pSrcChn, Chn_t *pDestChn);


/**
 * Unbind channel
 **/
MI_RET MI_SYS_UnBind(Chn_t *pSrcChn, Chn_t *pDestChn);

/**
 * reset system and enter uboot
 **/
MI_RET MI_SYS_ResetToUboot();

MI_RET MI_SYS_SyncAVPts();


/* use neon,but now only for test*/
void *MI_SYS_MemcpyFast(void *dest, const void *src, size_t n);

/**
 *  set custom id, for video anti tamper
 */
MI_RET MI_SYS_SetCustomID(CustomID_t id);

/**
 *  video module init and deinit
 */
MI_RET MI_SYS_Video_Init();
MI_RET MI_SYS_Video_Exit();

/**
 *  Get UUID
 */
int MI_SYS_GetUUID(MSYS_UNIQUEID_INFO *pstUuidInfo);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //__MI_SYS_H__
