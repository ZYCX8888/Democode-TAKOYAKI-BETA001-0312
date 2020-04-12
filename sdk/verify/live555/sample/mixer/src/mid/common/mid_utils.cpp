/*
* cycle_buffer.cpp- Sigmastar
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

/*************************************************
*
* File name:  mid_utils.c
* Author:     andely.zhou@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/6/13
* Description: mixer utils source file
*
*
*
* History:
*
*    1. Date  :        2018/6/13
*       Author:        andely.zhou@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <time.h>

#include "module_common.h"
#include "mid_utils.h"



#define DEFAULT_PROP          "/vendor/default.prop"
#define BufferSize            127
#define FileTempSize          (1024 * 2)

#if 0
static FILE *pPropfp = NULL;
static char g_propData[FileTempSize];
#endif

#if 1
#if 0
static int mixer_property_match(const char* key, int index)
{
    char* data = (char*)g_propData;

    while (*key == data[index++])
    {
        if (*key++ == '=')
        {
            return(index);
        }
    }

    if ((*key == '\0') && (data[index-1] == '='))
    {
        return(index);
    }

    return(-1);
}
#endif

/*
 *1 ro.XXX can not modify
 *2 persist
 *3 return value is strlen(value)
 *4 size of key and value   //PROP_NAME_MAX
 */
 int mixer_property_get(const char *key, char *value, const char *default_value)
{
#if 0 //INIPARSER_FILE_NODE_GET
    char* data = (char*)g_propData;
    int i, nxt = 0;


    if(NULL == pPropfp)
    {
        mixer_property_init();
    }

    for (i = 0; data[i] != '\0'; i = nxt + 1)
    {
        int val;
        for (nxt = i; data[nxt] != '\0'; ++nxt)
        {
            if (nxt >= (&g_propData[FileTempSize - 1] - data))
                return -1;
        }

        val = mixer_property_match(key, i);
        if (val < 0)
        {
            continue;
        }

        strncpy(value, &data[val], sizeof(value));
        return 0;
    }


    strncpy(value, default_value, strlen(default_value));
#endif
    return -1;
}

int mixer_property_set(const char *key, const char *value)
{
    int ret = -1;
#if 0 //INIPARSER_FILE_NODE_GET
    int len, oldval;
    char *pTmpProp, *nxt = NULL;
    char *pPropData = (char*)g_propData;


    if (NULL == key)
    {
        printf("this input key is NULL!\n");
        return ret;
    }

    if(strncmp(key, "ro.", 3) == 0)  //read only property
    {
        printf("this key is read only\n");
        return ret;
    }

    if(NULL == pPropfp)
    {
        mixer_property_init();
    }

    /*
     * search if variable with this name already exists
     */
    oldval = -1;
    nxt = pPropData;
    for (pTmpProp = pPropData; *pTmpProp != '\0' ; pTmpProp = nxt + 1)
    {
        for (nxt = pTmpProp; *nxt != '\0' ; ++nxt)
        {
            ;
        }

        oldval = mixer_property_match(key, pTmpProp - pPropData);
        if (oldval >= 0)
        {
            break;
        }
    }

    /*
     * Delete any existing definition
     */
    if (oldval >= 0)
    {
        /*
         * Ethernet Address and serial# can be set only once,
         * ver is readonly.
         */
        if (*++nxt == '\0')
        {
            if (pTmpProp > pPropData)
            {
                pTmpProp--;
            }
            else
            {
                *pTmpProp = '\0';
            }
        }
        else
        {
            for (;;)
            {
                *pTmpProp = *nxt++;
                if ((*pTmpProp == '\0') && (*nxt == '\0'))
                {
                    break;
                }
                ++pTmpProp;
            }
        }
        *++pTmpProp = '\0';
    }

    /* Delete only ? */
    if (!value)
    {
        ret = 0;
        return ret;
    }

    /*
     * Append new definition at the end
     */
    for (pTmpProp = pPropData; *pTmpProp || *(pTmpProp + 1); ++pTmpProp)
    {
        ;
    }

    if (pTmpProp > pPropData)
    {
        ++pTmpProp;
    }

    /*
     * Overflow when:
     * "name" + "=" + "val" +"\0\0"  > ENV_SIZE - (env-env_data)
     */
    len = strlen(key) + 2;
    /* add '=' for first arg, ' ' for all others */
    len += strlen(value) + 1;
    if (len > (&g_propData[FileTempSize - 1] - pTmpProp))
    {
        printf("## Error: environment overflow, \"%s\" deleted\n", key);
        return ret;
    }

    while ((*pTmpProp = *key++) != '\0')
    {
        pTmpProp++;
    }

    *pTmpProp =  '=';
    while ((*++pTmpProp = *value++) != '\0')
    {
        ;
    }

    /* end is marked with double '\0' */
    *pTmpProp++ = '\0';

/*
    int i;
    char *pchar = g_propData;

    printf("====>%s:%d Property_SIZE=0x%x, real_size=0x%x\n",__func__,__LINE__, FileTempSize, filesize);
    for(i = 1; i < 1024; i++)
    {
        if((i/40) && (i%40 >= 0) && (*(pchar+i) == '\0')) printf("%c\n", *(pchar + i - 1));
        else printf("%c", *(pchar + i - 1));

        if((*(pchar+i) == '\0') && (*(pchar+i+1) == '\0')) break;
    }
    printf("\n");
*/
    ret = 0;
#endif

    return ret;
}

int mixer_property_init(void)
{
    int ret = -1;
    #if 0
    int filesize = 0;


    if(NULL == pPropfp)
    {
        pPropfp = fopen(DEFAULT_PROP, "rw+");
        if(NULL == pPropfp)
        {
            printf("[%s] open defaullt.prop fail\n", __func__);
            return ret;
        }

        fseek(pPropfp, 0L, SEEK_END);
        filesize = ftell(pPropfp);
        if(FileTempSize < filesize)
        {
            printf("[%s] the defaullt.prop file size(%d) is larger then the buffer\n", __func__, filesize);
            fclose(pPropfp);
            pPropfp = NULL;
        }

        memset(g_propData, 0x00, sizeof(g_propData));
        fseek(pPropfp, 0, SEEK_SET);
        fread(g_propData, filesize, 1, pPropfp);
/*
        char *pchar = g_propData;
        printf("====>%s:%d Property_SIZE=0x%x, real_size=0x%x\n",__func__,__LINE__, FileTempSize, filesize);
        for(i = 1; i < 1024; i++)
        {
            if((i/40) && (i%40 >= 0) && (*(pchar+i) == '\0')) printf("%c\n", *(pchar + i - 1));
            else printf("%c", *(pchar + i - 1));

            if((*(pchar+i) == '\0') && (*(pchar+i+1) == '\0')) break;
        }
        printf("\n");
*/
    }
    #endif
    ret = 0;
    return ret;
}

int mixer_property_uninit(void)
{
    int ret = -1;
    #if 0
    if(NULL != pPropfp)
    {
        int filesize = 0;

        fseek(pPropfp, 0L, SEEK_END);
        filesize = ftell(pPropfp);
/*
        char *pchar = g_propData;

        printf("====>%s:%d Property_SIZE=0x%x, real_size=0x%x\n",__func__,__LINE__, FileTempSize, filesize);
        for(int i = 1; i < 1024; i++)
        {
            if((i/40) && (i%40 >= 0) && (*(pchar+i) == '\0')) printf("%c\n", *(pchar + i - 1));
            else printf("%c", *(pchar + i - 1));

            if((*(pchar+i) == '\0') && (*(pchar+i+1) == '\0')) break;
        }
        printf("\n");
*/

        fclose(pPropfp);
        pPropfp = NULL;

        pPropfp = fopen(DEFAULT_PROP, "w");
        if(NULL == pPropfp)
        {
            printf("[%s] open defaullt.prop fail\n", __func__);
            return ret;
        }

        fseek(pPropfp, 0, SEEK_SET);
        fwrite(g_propData, sizeof(char), filesize * sizeof(char), pPropfp);

        fclose(pPropfp);
        pPropfp = NULL;

        ret = 0;
        return ret;
    }
    #endif
    return ret;
}
#endif


#ifdef INIPARSER_FILE_NODE_GET
DB_FILE * db_file_init(const char *fname,const char *section)
{
    FILE *fp = NULL;
    char tmpsec  [32];
    char tmpkey  [32];
    char tmpvalu [32];
    char line    [BufferSize + 1];
    DB_FILE *pf = NULL;
    DB_FILE_NODE *pdb = NULL;

    int  len;
    int  lineno = 0;
    int  last = 0;  //for multi-line
    int  dbFileMaxNum = 0;

    if((NULL == fname) || (NULL == section))
    {
        goto para_fail;
    }

    if(NULL == (fp = fopen(fname, "r")))
    {
        printf("[%s]Can't open Mixer config file \'%s\'\n", __INIPARSE__, MIXER_CFG_FILE);
        goto para_fail;
    }

    pf = (DB_FILE *)malloc(sizeof(struct _db_file));
    if(NULL == pf)
    {
        printf("[%s]Malloc buff for DB Err!\n", __INIPARSE__);
        goto db_file_fail;
    }

    pdb = (DB_FILE_NODE *)malloc(sizeof(struct _db_file_node));
    if(NULL == pdb)
    {
        printf("[%s]Malloc buff for DB NODE Err!\n", __INIPARSE__);
        goto  db_file_node_fail;
    }

    strcpy(pf->sectionName,section);
    pf->fileNode = pdb;
    pf->dbFileNum = 0;
    pdb->key = NULL;
    pdb->value = 0;
    pdb->next = NULL;

    memset(line,    0, sizeof(line));
    memset(tmpsec,  0, sizeof(tmpsec));
    memset(tmpkey,  0, sizeof(tmpkey));
    memset(tmpvalu, 0, sizeof(tmpvalu));

   //start dealing cfg file
    while(NULL != fgets(line + last, BufferSize - last, fp))
    {
        lineno++;
        len = (int)strlen(line) - 1;

        /* Safety check against buffer overflows */
        if('\n' != line[len])
        {
            printf("[%s]input line too long in %s (%d)\n", __INIPARSE__, fname, lineno);
            goto db_file_node_fail;
        }

        /* Get rid of \n and spaces at end of line */
        while((len >= 0) && (('\n' == line[len]) || (isspace(line[len]))))
        {
            line[len] = 0;
            len--;
        }

        /* Detect multi-line */
        if('\\' == line[len])
        {
            /* Multi-line value */
            last = len;
            continue;
        }
        else
        {
            last = 0;
        }

        switch(iniparser_line(line, tmpsec, tmpkey, tmpvalu))
        {
            case LINE_EMPTY:
            case LINE_COMMENT:
                break;

            case LINE_SECTION:
                if(strcmp(tmpsec, pf->sectionName))
                    break;

                if(NULL == pdb->key)
                {
                    if(NULL == (pdb->key = (char *)malloc(strlen(tmpsec) + 1)))
                    {
                        printf("[%s]Malloc buff for section Err!\n", __INIPARSE__);
                        goto iniparser_file_Err;
                    }
                }

                memset(pdb->key, 0, strlen(tmpsec) + 1);
                strncpy(pdb->key, tmpsec, strlen(tmpsec));
                pdb->value = 0;

                pf->dbFileNum++;

                pdb->next = (DB_FILE_NODE *)malloc(sizeof(struct _db_file_node));

                if(NULL == pdb->next)
                {
                    printf("[%s]Malloc buff for node Err!\n", __INIPARSE__);
                    goto iniparser_file_Err;
                }

                pdb = pdb->next;
                // pdb point to the next struct "_db_file_node_"
                pdb->key = NULL;
                pdb->value = 0;
                pdb->next = NULL;
                break;

            case LINE_VALUE:
                if(!strcmp(tmpsec, section))
                {
                    if(0 == dbFileMaxNum)
                    {
                        dbFileMaxNum = strncmp("maxCfgNum",tmpkey,9)? 0 : atoi(tmpvalu);
                        if(0 == dbFileMaxNum)
                           continue;
                        else
                            lineno = 0;
                    }

                    if((NULL == pdb->key) && (pf->dbFileNum < dbFileMaxNum))
                    {
                        if(NULL == (pdb->key = (char *)malloc(strlen(tmpkey) + 1)))
                        {
                            printf("[%s]Malloc buff for key Err!\n", __INIPARSE__);
                            goto iniparser_file_Err;
                        }

                        memset(pdb->key, 0, strlen(tmpkey) + 1);
                        strncpy(pdb->key, tmpkey, strlen(tmpkey));

                        pdb->value = atoi(tmpvalu);
                        pf->dbFileNum++;

                        pdb->next = (DB_FILE_NODE *)malloc(sizeof(struct _db_file_node));

                        if(NULL == pdb->next)
                        {
                            printf("[%s]Malloc buff for node Err!\n", __INIPARSE__);
                            goto iniparser_file_Err;
                        }

                        pdb = pdb->next;
                        // pdb point to the next struct "_db_file_node_"
                        pdb->key = NULL;
                        pdb->value = 0;
                        pdb->next = NULL;

                        break;
                    }


                    goto iniparser_file_done;
                }
                break;

            case LINE_ERROR:
                printf("[%s]: syntax error in %s (%d):\n", __INIPARSE__, MIXER_CFG_FILE, lineno);
                break;

            default:
                break;
        }

        memset(line, 0, sizeof(line));
        last = 0;
    }

iniparser_file_done:
    if(0 == dbFileMaxNum && !strcmp(tmpsec, section))
    {
       printf("===== Please set mxCfgNum key value of:%s =====\n",section);
       db_file_free(pf);
       pf = NULL;
    }

    fclose(fp);
    return pf;


iniparser_file_Err:
    pdb = pf->fileNode;

    while(NULL != pdb->key)
    {
        free(pdb->key);
        free(pdb);
        pf->fileNode = pdb->next;
        pdb = pf->fileNode;
    }

    if(NULL != pf->fileNode)
        free(pf->fileNode);

db_file_node_fail:
    free(pdb);

db_file_fail:
    free(pf);
    fclose(fp);

para_fail:
    return NULL;

}

int db_file_free(DB_FILE * db_file)
{
    DB_FILE_NODE * pdb = NULL;

    pdb = db_file->fileNode;
    while(NULL != pdb)
    {
        db_file->fileNode = pdb->next;
        free(pdb->key);
        free(pdb);
        pdb = db_file->fileNode;
    }

    if(NULL != db_file->fileNode)
        free(db_file->fileNode);

    if(NULL != db_file)
        free(db_file);

    return 0;
}


int db_GetKeyValue(const DB_FILE * db_file, const char * key)
{
    int i = 0;
    DB_FILE_NODE * pdb = NULL;

    if((NULL == db_file) || (NULL == db_file->sectionName) || (NULL == key))
        return -1;

    pdb = db_file->fileNode;

    while((NULL != pdb->key) && (i++ <= db_file->dbFileNum))
    {
        if(0 == strcmp(pdb->key, key))
            return pdb->value;

        pdb = pdb->next;
    }

    return -1;
}


line_status iniparser_line(char * input_line, char * section, char * key, char * value)
{
    line_status sta;
    char        line[BufferSize + 1];
    int         len;

    strcpy(line, strstrip(input_line));
    len = (int)strlen(line);

    sta = LINE_UNPROCESSED;

    if(len < 1)
    {
        /* Empty line */
        sta = LINE_EMPTY;
    }
    else if('#' == line[0])
    {
        /* Comment line */
        sta = LINE_COMMENT;
    }
    else if(('/' == line[0]) && ('/' == line[1]))
    {
        /* Comment line */
        sta = LINE_COMMENT;
    }
    else if(('[' == line[0]) && (']' == line[len - 1]))
    {
        /* Section name */
        sscanf(line, "[%[^]]", section);
        strcpy(section, strstrip(section));
        sta = LINE_SECTION;
    }
    else if((sscanf(line, "%[^=] = \"%[^\"]\"", key, value) == 2) ||
            (sscanf(line, "%[^=] = '%[^\']'",   key, value) == 2) ||
            (sscanf(line, "%[^=] = %[^;#]",     key, value) == 2))
    {
        /* Usual key=value, with or without comments */
        strcpy(key, strstrip(key));
        strcpy(value, strstrip(value));

        /*
         * sscanf cannot handle '' or "" as empty values
         * this is done here
         */
        if(!strcmp(value, "\"\"") || (!strcmp(value, "''"))) value[0] = 0;

        sta = LINE_VALUE;
    }
    else if((sscanf(line, "%[^=] = %[;#]", key, value) == 2) ||
            (sscanf(line, "%[^=] %[=]", key, value) == 2))
    {
        /*
         * Special cases:
         * key=
         * key=;
         * key=#
         */
        strcpy(key, strstrip(key));
        value[0] = 0;
        sta = LINE_VALUE;
    }
    else
    {
        /* Generate syntax error */
        sta = LINE_ERROR;
    }

    return sta;
}


char * strstrip(char * s)
{
    static char l[BufferSize + 1];
    char * last;

    if(NULL == s) return NULL;

    while(isspace((int)*s) && *s) s++;

    memset(l, 0, BufferSize + 1);
    strcpy(l, s);
    last = l + strlen(l);

    while(last > l)
    {
        if(!isspace((int) * (last - 1))) break;

        last --;
    }

    *last = (char)0;
    return (char*)l;
}
#endif

bool mixerIsHex(char *strNum)
{
    if(NULL == strNum || strlen(strNum) <= 2)
    {
        return false;
    }

    if('0' == strNum[0] && ('x' == strNum[1] || 'X' == strNum[1]))
    {
        return true;
    }

    return false;
}

int mixerStr2Int(char *strNum)
{
    int digit = 0;

    if(mixerIsHex(strNum))
    {
        sscanf(strNum, "%x", &digit);
    }
    else
    {
        sscanf(strNum, "%d", &digit);
    }

    return digit;
}

MI_U32 mixerStrtok2Int(char *str, const char *delim)
{
    MI_U32 _tmp[10] = {0x0};

    char* argv[10] = {NULL};
    char *outer_ptr=NULL;
    int argc = 0x0;
    int i = 0x0;

    if(NULL == str || NULL == delim)
    {
        MIXER_ERR("errm, str %p, delim %p\n", str, delim);
        return 0;
    }

    argv[0] = str;
    MIXER_DBG("str is %s  delim is %s\n", str,delim);
       while((NULL != (argv[argc] = strtok_r(argv[argc], delim, &outer_ptr))) && (argc<2))
        argc++;

    for(i=0; i<argc; i++)
    {
        MIXER_DBG("argv[%d]:%s\n", i, argv[i]);
        _tmp[i] = atoi(argv[i]);
    }
    return (_tmp[1]<<16) | (_tmp[0]);
}

void MySystemDelay(unsigned int  msec)
{
    struct timeval tv;
    tv.tv_sec = msec/1000;
    tv.tv_usec = msec%1000 * 1000;
    select(0, NULL, NULL, NULL, &tv);
}

MI_U32 MI_OS_GetTime(void)
{
    struct timespec t1;

    clock_gettime(CLOCK_MONOTONIC, &t1);

    return 1000000 * (t1.tv_sec) + (t1.tv_nsec) / 1000;
}

MI_S32 SystemGetCurrentTime (SYSTEM_TIME *pTime)
{
    if(NULL == pTime)
    {
        MIXER_ERR("wrong param\n");
        return -1;
    }

    struct tm _time;

    memset(&_time, 0x0, sizeof(struct tm));

    time_t    t = time(NULL);

    if (NULL == localtime_r(&t, &_time))
    {
        MIXER_ERR("SystemGetCurrentTime failed\n");
        return -1;
    }

    pTime->second    = _time.tm_sec;             /* seconds (1 - 60)    */
    pTime->minute    = _time.tm_min;            /* minutes (1 - 60)    */
    pTime->hour        = _time.tm_hour;            /* hours (1 - 24)    */
    pTime->wday    = _time.tm_wday;            /* day of week        */
    pTime->day        = _time.tm_mday;            /* day of the month    */
    pTime->month    = _time.tm_mon+1;        /* month (1-12)        */
    pTime->year        = _time.tm_year + 1900;    /* year                */

    return 0;
}

void __attribute__ ((noinline)) memcpy_neon_pld(void *dest, const void *src, size_t n)
{
    asm(
        "NEONCopyPLD:\n"
        "   pld [r1, #0xC0]\n" //?????u
        "   vldm r1!,{d0-d7}\n" //????@r0?]src?^?[?8*8=64???q?D8????u
        "   vstm r0!,{d0-d7}\n" //?s??b????a?}r1?]dst?^???A?P??O64?8????q?D8????u
        "   subs r2,r2,#0x40\n" //?`???????A?C???64?A??@?`????=row*col*4/64
        "   bgt NEONCopyPLD\n"
    );

}
