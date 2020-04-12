/*
* mid_utils.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef _MID_UTILS_H_
#define _MID_UTILS_H_

#include "mid_common.h"



#define MIXER_CFG_FILE              "mixer.ini"
#define __INIPARSE__                "INIPARSE"
#define MIXER_MD_SECTION_NAME       "MD_CFG"
#define MIXER_OD_SECTION_NAME       "OD_CFG"
#define MIXER_VG_SECTION_NAME       "VG_CFG"

#define ALIGN_2xUP(x)               (((x+1) / 2) * 2)
#define ALIGN_4xUP(x)               (((x+1) / 4) * 4)
#define ALIGN_8xUP(x)               (((x+7) / 8) * 8)
#define ALIGN_16xUP(x)              (((x+15) / 16) * 16)
#define ALIGN_32xUP(x)              (((x+31) / 32) * 32)
#define ALIGN_64xUP(x)              (((x+63) / 64) * 64)
#define ALIGN_256xUP(x)              (((x+255) >>8 ) <<8)
#define ALIGN_4096xUP(x)              (((x+4095) >>12 ) <<12)

#define ALIGN_16xDOWN(x)            (x&0xFFFFFFF0)
#define ALIGN_8xDOWN(x)               (x&0xFFFFFFF8)
#define ALIGN_2xDOWN(x)               (x&0xFFFFFFFE)
#define ALIGN_32xDOWN(x)            (x&0xFFFFFFE0)
#define ISADDRESSALIGNTO(x, y)      (!((x) & ((y) - 1)))



#define ALIGN64(x)                  (((x)+0x3F) & (~0x3F))
#define ALIGN128(x)                 (((x)+0x7F) & (~0x7F))
#define ALIGN256(x)                 (((x)+0xFF) & (~0xFF))

#define _abs(x)  ((x) > 0 ? (x) : (-(x)))

typedef enum _line_status_
{
    LINE_UNPROCESSED,
    LINE_ERROR,
    LINE_EMPTY,
    LINE_COMMENT,
    LINE_SECTION,
    LINE_VALUE
} line_status;

typedef struct _db_file_node
{
    char *key;
    int  value;
    struct _db_file_node *next;
} DB_FILE_NODE;

typedef struct _db_file
{
    char sectionName[20];
    int dbFileNum;
    DB_FILE_NODE *fileNode;
} DB_FILE;

typedef enum
{
    MIXER_MAIN_CHN = 0x0,
    MIXER_SUB_CHN      = 0x1,
    MIXER_THD_CHN      = 0x2,
    MIXER_YUV_CHN   = 0x3
}MIXER_CHN;

DB_FILE * db_file_init(const char *fname, const char *section);
int db_file_free(DB_FILE * db_file);
int db_GetKeyValue(const DB_FILE * db_file, const char * key);
line_status iniparser_line(char * input_line, char * section, char * key, char * value);
char * strstrip(char * s);

MI_U32 mixerStrtok2Int(char *str, const char *delim);
void MySystemDelay(unsigned int  msec);

MI_U32 MI_OS_GetTime(void);

MI_S32 SystemGetCurrentTime (SYSTEM_TIME *pTime);
void __attribute__ ((noinline)) memcpy_neon_pld(void *dest, const void *src, size_t n);

#endif //_MID_UTILS_H_

