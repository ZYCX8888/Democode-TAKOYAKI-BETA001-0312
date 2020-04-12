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
#ifndef __TEST_CIPHER__COMMON_H__
#define __TEST_CIPHER__COMMON_H__


#include "mi_common_datatype.h"


#define CIPHER_DBGLV_NONE           0   //disable all the debug message
#define CIPHER_DBGLV_INFO           1   //information
#define CIPHER_DBGLV_NOTICE         2   //normal but significant condition
#define CIPHER_DBGLV_DEBUG          3   //debug-level messages
#define CIPHER_DBGLV_WARNING        4   //warning conditions
#define CIPHER_DBGLV_ERR            5   //error conditions
#define CIPHER_DBGLV_CRIT           6   //critical conditions
#define CIPHER_DBGLV_ALERT          7   //action must be taken immediately
#define CIPHER_DBGLV_EMERG          8   //system is unusable

extern int g_cipher_dbglevel;

#define COLOR_NONE                 "\033[0m"
#define COLOR_BLACK                "\033[0;30m"
#define COLOR_BLUE                 "\033[0;34m"
#define COLOR_GREEN                "\033[0;32m"
#define COLOR_CYAN                 "\033[0;36m"
#define COLOR_RED                  "\033[0;31m"
#define COLOR_YELLOW               "\033[1;33m"
#define COLOR_WHITE                "\033[1;37m"

#define CIPHER_NOP(fmt, args...)
#define CIPHER_DBG(fmt, args...) \
    if(g_cipher_dbglevel <=  CIPHER_DBGLV_DEBUG)  \
        do { \
            printf(COLOR_GREEN"[DBG]:%s[%d]: "COLOR_NONE, __FUNCTION__,__LINE__); \
            printf(fmt, ##args); \
        }while(0)

#define CIPHER_WARN(fmt, args...) \
    if(g_cipher_dbglevel <=  CIPHER_DBGLV_WARNING)  \
        do { \
            printf(COLOR_YELLOW"[WARN]:%s[%d]: "COLOR_NONE, __FUNCTION__,__LINE__); \
            printf(fmt, ##args); \
        }while(0)

#define CIPHER_INFO(fmt, args...) \
    if(g_cipher_dbglevel <=  CIPHER_DBGLV_INFO)  \
        do { \
            printf("[INFO]:%s[%d]: \n", __FUNCTION__,__LINE__); \
            printf(fmt, ##args); \
        }while(0)

#define CIPHER_ERR(fmt, args...) \
    if(g_cipher_dbglevel <=  CIPHER_DBGLV_ERR)  \
        do { \
            printf(COLOR_RED"[ERR]:%s[%d]: "COLOR_NONE, __FUNCTION__,__LINE__); \
            printf(fmt, ##args); \
        }while(0)

MI_S32 printBuffer(const char *string, const MI_U8 *pu8Input, MI_U32 u32Length);
void test_cipher_Flush(void);
#endif /* __TEST_CIPHER__COMMON_H__ */

