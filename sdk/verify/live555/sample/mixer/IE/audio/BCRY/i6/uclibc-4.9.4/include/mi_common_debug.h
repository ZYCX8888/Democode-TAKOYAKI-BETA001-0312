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
#ifndef __MI_COMMON_DEBUG_H__
#define __MI_COMMON_DEBUG_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Debug level
 */
#define DBG_LEV_NO_OUTPUT  0
#define DBG_LEV_DEBUG      0x01
#define DBG_LEV_INFO       0x02
#define DBG_LEV_WARNING    0x04
#define DBG_LEV_ERROR      0x08
#define DBG_LEV_FATAL      0x10

/**
 * All debug level
 */
#define DBG_LEV_ALL        DBG_LEV_DEBUG


/**
 * Debug module
 */
#define DBG_MODULE_AENC     0x000100
#define DBG_MODULE_ADEC     0x000200
#define DBG_MODULE_AI       0x000400
#define DBG_MODULE_AO       0x000800
#define DBG_MODULE_VI       0x001000
#define DBG_MODULE_VENC     0x002000
#define DBG_MODULE_ISP      0x004000
#define DBG_MODULE_OSD      0x008000
#define DBG_MODULE_SYS      0x010000
#define DBG_MODULE_IE       0x020000
#define DBG_MODULE_SAMPLE   0x040000
#define DBG_MODULE_UT       0x080000
#define DBG_MODULE_WRAPER   0x100000
#define DBG_MODULE_OMX      0x200000
#define DBG_MODULE_BIND       0x400000
#define DBG_MODULE_OS       0x800000
/**
 * All debug module
 */
#define DBG_MODULE_ALL     0xFFFF00


/**
 * Config DEBUG_MODULE and DEBUG_LEVEL to set output message.
 * Debug module setting
 */
#define MI_DEBUG_MODULE DBG_MODULE_ALL

/**
 * Debug level setting
 */
#define MI_DEBUG_LEVEL DBG_LEV_INFO

/**
 * Debug ISP setting
 */
#define MI_ISP_DEBUG_STATUS_CHECK FALSE

#if 1
extern unsigned int g_uDbgLogLevel;
extern unsigned int g_uDbgLogModule;
extern unsigned int g_uDbgISPStatus;

#define LOG_DEBUG(module, fmt, args...) do { if ((g_uDbgLogModule & (module)) && (g_uDbgLogLevel <= DBG_LEV_DEBUG)){fprintf(stderr, "mi-debug:\t" fmt, ##args);} } while (0)
#define LOG_INFO(module, fmt, args...) do { if ((g_uDbgLogModule & (module)) && (g_uDbgLogLevel <= DBG_LEV_INFO)){fprintf(stderr, "mi-info:\t" fmt, ##args);} } while (0)
#define LOG_WARNING(module, fmt, args...) do { if ((g_uDbgLogModule & (module)) && (g_uDbgLogLevel <= DBG_LEV_WARNING)){fprintf(stderr, "mi-warning:\t" fmt, ##args);} } while (0)
#define LOG_ERROR(module, fmt, args...) do { if ((g_uDbgLogModule & (module)) && (g_uDbgLogLevel <= DBG_LEV_ERROR)){fprintf(stderr, "mi-error:\t" fmt, ##args);} } while (0)
#define LOG_FATAL(module, fmt, args...) do { if ((g_uDbgLogModule & (module)) && (g_uDbgLogLevel <= DBG_LEV_FATAL)){fprintf(stderr, "mi-fatal:\t" fmt, ##args);} } while (0)


#else

#define LOG_DEBUG(module, fmt, args...) {}
#define LOG_INFO(module, fmt, args...) {}
#define LOG_WARNING(module, fmt, args...) {}
#define LOG_ERROR(module, fmt, args...) {}
#define LOG_FATAL(module, fmt, args...) {}

#endif


#ifdef __cplusplus
}
#endif

#endif //__MI_COMMON_DEBUG_H__
