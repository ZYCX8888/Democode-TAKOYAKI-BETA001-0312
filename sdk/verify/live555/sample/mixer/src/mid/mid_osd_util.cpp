/*
* mid_osd_util.cpp- Sigmastar
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "module_common.h"
#include "mid_common.h"
#include "miniz_tinfl.h"
#include "mid_video_type.h"
#include "mid_osd.h"


extern MI_S8 g_s8OsdFontPath[128];

static MI_Font_t* gFontInfo[HZ_DOT_NUM];
static pthread_mutex_t g_font_mutex[HZ_DOT_NUM] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};


#define HZ_8P_BIN_SIZE      65424
#define HZ_12P_BIN_SIZE     196272
#define HZ_16P_BIN_SIZE     261696

MI_U32 dilate_total_time;
MI_U32 dilate_count;
MI_U32 dilate_max_time;
BOOL g_osd_dilate_bmp;
BOOL g_osd_dilate_profiling;


MI_U32 mid_Font_Strlen(const MI_S8 *phzstr)
{
    MI_U8 count = 0;
    MI_U8 *str;
    MI_U32 len;
    if(NULL == phzstr)
    {
      return 0;
    }
    len = strlen((char *)phzstr);
    str = (MI_U8 *)phzstr;

    while((len > 0)&& str)
    {
        if((*str) < 0x80)
        {
            count++;
            str += 1;
            len -= 1;
        }
        else
        {
            count++;
            str += 2;
            len -= 2;
        }
    }

    return count;
}


MI_U32 mid_Font_GetCharQW(const MI_S8 *str, MI_U32 *p_qu, MI_U32 *p_wei)
{
    int qu, wei;
    int offset = 0;
    const MI_U8 *pString = (const MI_U8 *)str;

    if(*pString >= 128)
    {
        qu = *pString - 160;
        pString++;

        if(*pString >= 128)
        {
            wei = *pString - 160;

            offset = 2;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        qu = 3;
        wei = *pString - 32;
        offset = 1;
    }

    *p_qu = qu;
    *p_wei = wei;

    return offset;
}


MI_U32 mid_Font_Strcmp(MI_S8* pStr1, MI_S8* pStr2)
{
    MI_U32 idx, offset_1, offset_2;
    MI_U32 hzLen;
    MI_U32 qu1 = 0, wei1 = 0, qu2 = 0, wei2 = 0;

    hzLen = mid_Font_Strlen(pStr1);

    for(idx = 0; idx < hzLen; idx ++)
    {
        offset_1 = mid_Font_GetCharQW(pStr1, &qu1, &wei1);
        offset_2 = mid_Font_GetCharQW(pStr2, &qu2, &wei2);

        if(qu1 != qu2 || wei1 != wei2)
            break;

        pStr1 += offset_1;
        pStr2 += offset_2;
    }

    return idx;
}


static void mid_Font_Open(OsdFontSize_e eFontSize)
{
    MI_U8 *data_file , *data_bin;
    MI_U32 bin_size = 0;
    FILE* fd_zk = NULL;
    MI_S8 hz_path [160] = {0};
    struct stat hz_stat;
    MI_U32 font_size = 0;
    MI_Font_t* pFont;
    MI_FontDot_e hz_dot;

    if((FONT_SIZE_8 == eFontSize)||/*(FONT_SIZE_16 == eFontSize)||*/
       (FONT_SIZE_40 == eFontSize)||(FONT_SIZE_56 == eFontSize)||(FONT_SIZE_80 == eFontSize))
        hz_dot = HZ_DOT_8;
    else if((FONT_SIZE_12 == eFontSize)||(FONT_SIZE_24 == eFontSize)||(FONT_SIZE_36 == eFontSize)||
            (FONT_SIZE_48 == eFontSize)||(FONT_SIZE_60 == eFontSize)||(FONT_SIZE_72 == eFontSize)||(FONT_SIZE_84 == eFontSize))
        hz_dot = HZ_DOT_12;
    else if((FONT_SIZE_16 == eFontSize)||(FONT_SIZE_32 == eFontSize)||(FONT_SIZE_64 == eFontSize)||(FONT_SIZE_96 == eFontSize))
        hz_dot = HZ_DOT_16;
    else
    {
        MIXER_ERR("%s %d font %d exit!\n", __FUNCTION__, __LINE__, eFontSize);;
        return;
    }

    pthread_mutex_lock(&g_font_mutex[hz_dot]);

    if(gFontInfo[hz_dot])
    {
        pthread_mutex_unlock(&g_font_mutex[hz_dot]);
        return;
    }


    if((FONT_SIZE_8 == eFontSize)||/*(FONT_SIZE_16 == eFontSize)||*/
       (FONT_SIZE_40 == eFontSize)||(FONT_SIZE_56 == eFontSize)||(FONT_SIZE_80 == eFontSize))
    {
        sprintf((char *)hz_path, "%s/font/HZ8.mz", g_s8OsdFontPath);
        bin_size = HZ_8P_BIN_SIZE;
        font_size = 8;
    }
    else if((FONT_SIZE_12 == eFontSize)||(FONT_SIZE_24 == eFontSize)||(FONT_SIZE_36 == eFontSize)||
            (FONT_SIZE_48 == eFontSize)||(FONT_SIZE_60 == eFontSize)||(FONT_SIZE_72 == eFontSize)||(FONT_SIZE_84 == eFontSize))
    {
        sprintf((char *)hz_path, "%s/font/HZ12.mz", g_s8OsdFontPath);
        bin_size = HZ_12P_BIN_SIZE;
        font_size = 12;
    }
    else if((FONT_SIZE_16 == eFontSize)||(FONT_SIZE_32 == eFontSize)||(FONT_SIZE_64 == eFontSize)||(FONT_SIZE_96 == eFontSize))
    {
        sprintf((char *)hz_path, "%s/font/HZ16.mz", g_s8OsdFontPath);
        bin_size = HZ_16P_BIN_SIZE;
        font_size = 16;
    }

    if(stat((char *)hz_path, &hz_stat) < 0)
    {
        pthread_mutex_unlock(&g_font_mutex[hz_dot]);
        MIXER_ERR("%s %s file not exit !\n", __FUNCTION__, hz_path);;
        return;
    }

    data_file = (MI_U8 *)MI_SYS_Malloc(hz_stat.st_size);
    if(NULL == data_file)
    {
        pthread_mutex_unlock(&g_font_mutex[hz_dot]);
        MIXER_ERR("%s MI_SYS_Malloc err!\n", __FUNCTION__);
        return;
    }
    fd_zk = fopen((char *)hz_path, "rb");
    if(NULL == fd_zk)
    {
        MIXER_ERR("%s open file \"%s\" error!\n", __FUNCTION__, hz_path);
        if(data_file)
           {
          MI_SYS_Free(data_file);
          data_file = NULL;
           }
        return;
    }

    fread((char *)data_file, hz_stat.st_size, 1, fd_zk);
    fclose(fd_zk);

    data_bin = (MI_U8 *)MI_SYS_Malloc(bin_size);

    if(bin_size != tinfl_decompress_mem_to_mem(data_bin, bin_size, data_file, hz_stat.st_size, TINFL_FLAG_PARSE_ZLIB_HEADER))
    {
        MI_SYS_Free(data_bin);
        MI_SYS_Free(data_file);
        pthread_mutex_unlock(&g_font_mutex[hz_dot]);
        MIXER_ERR("%s decompress err!\n", __FUNCTION__);
        return;
    }

    MI_SYS_Free(data_file);
    pFont = (MI_Font_t *)MI_SYS_Malloc(sizeof(MI_Font_t));
    pFont->nFontSize = font_size;
    pFont->pData     = data_bin;

    gFontInfo[hz_dot] = pFont;

    pthread_mutex_unlock(&g_font_mutex[hz_dot]);
}

MI_Font_t* mid_Font_GetHandle(OsdFontSize_e eFontSize , MI_U32* pMultiple)
{
    mid_Font_Open(eFontSize);

    switch(eFontSize)
    {
        case FONT_SIZE_8:
            *pMultiple = 1; //Original output, must set "1"
            return gFontInfo[HZ_DOT_8];

        case FONT_SIZE_12:
            *pMultiple = 1; //Original output, must set "1"
            return  gFontInfo[HZ_DOT_12];

        case FONT_SIZE_16:
 #if 1
            *pMultiple = 1; //Original output, must set "1"
            return gFontInfo[HZ_DOT_16];
#else
            *pMultiple = 2;
            return gFontInfo[HZ_DOT_8];
#endif
        case FONT_SIZE_24:
            *pMultiple = 2;
            return  gFontInfo[HZ_DOT_12];

        case FONT_SIZE_32:
            *pMultiple = 2;
            return gFontInfo[HZ_DOT_16];

        case FONT_SIZE_36:
            *pMultiple = 3;
            return gFontInfo[HZ_DOT_12];

        case FONT_SIZE_40:
            *pMultiple = 5;
            return gFontInfo[HZ_DOT_8];

        case FONT_SIZE_48:
            *pMultiple = 4;
            return  gFontInfo[HZ_DOT_12];

        case FONT_SIZE_56:
            *pMultiple = 7;
            return gFontInfo[HZ_DOT_8];

        case FONT_SIZE_60:
            *pMultiple = 5;
            return gFontInfo[HZ_DOT_12];

        case FONT_SIZE_64:
            *pMultiple = 4;
            return gFontInfo[HZ_DOT_16];

        case FONT_SIZE_72:
            *pMultiple = 6;
            return  gFontInfo[HZ_DOT_12];

        case FONT_SIZE_80:
            *pMultiple = 10;
            return  gFontInfo[HZ_DOT_8];

        case FONT_SIZE_84:
            *pMultiple = 7;
            return  gFontInfo[HZ_DOT_12];

        case FONT_SIZE_96:
            *pMultiple = 6;
            return gFontInfo[HZ_DOT_16];

        default:
            return NULL;
    }
}


static void MI_Font_Close(MI_Font_t* pFont)
{
    MI_SYS_Free(pFont->pData);
    MI_SYS_Free(pFont);
}

int MI_Font_Uninit()
{
    MI_U32 i;

    for(i = 0 ; i < HZ_DOT_NUM ; i++)
    {
        if(gFontInfo[i])
        {
            MI_Font_Close(gFontInfo[i]);
            gFontInfo[i] = NULL;
        }
    }

    return MI_SUCCESS;
}

void mid_YuvDilate(MI_U8* pYbuffer, MI_U16* pUvbuffer, MI_U32 stride, MI_U32 width, MI_U32 height, YUVColor_t* pYuvColor)
{
    MI_U32 i, j;
    struct timeval start, end;
    MI_U16* pYbufferU16;
    //MI_U8* pYbufferU8;
    MI_U16 Ystroke, UVstroke;

    Ystroke = 255 - pYuvColor->y;
    Ystroke = (Ystroke << 8) | Ystroke;

    UVstroke = 255 - pYuvColor->u;
    UVstroke = (UVstroke << 8) | (255 - pYuvColor->v);

    //printf("stroke = %d\n",stroke);
    //printf("sr=%d sg=%d sb=%d sa=%d\n\n",(stroke&0xF000)>>12,(stroke&0xF00)>>8,(stroke&0xF0)>>4,stroke&0xF);

    //printf("[%s %d] %p %u %u %u\n",__FUNCTION__,__LINE__,pbuffer , width , height , stroke);

    if(g_osd_dilate_profiling)
        gettimeofday(&start, NULL);

    pYbufferU16 = (MI_U16*)pYbuffer + stride ;

    for(i = 1; i < (height - 2) / 2 ; i++)
    {
        for(j = 1; j < (width - 2) / 2; j++)
        {
            if(pYbufferU16[j] > 1)
            {
                //printf("xx %d %d           ",i,j);
                if(pYbufferU16[j - stride] == 0)
                {
                    pYbufferU16[j - stride] = 1;
                    //printf("%d %d   ",i-1,j);
                }

                if(pYbufferU16[j - 1] == 0)
                {
                    pYbufferU16[j - 1] = 1;
                    //printf("%d %d   ",i,j-1);
                }

                if(pYbufferU16[j + stride] == 0)
                {
                    pYbufferU16[j + stride] = 1;
                    //printf("%d %d   ",i+1,j);
                }

                if(pYbufferU16[j + 1] == 0)
                {
                    pYbufferU16[j + 1] = 1;
                    //printf("%d %d",i,j+1);
                }

                //printf("\n");
            }
            else
            {
                //printf(" ");
            }
        }

        pYbufferU16 += stride;
        //printf("\n");
    }

    //printf("stroke = %d\n",stroke);

#if 1
    pYbufferU16 = (MI_U16*)pYbuffer;

    for(i = 0; i < height / 2; i++)
    {
        for(j = 0; j < width / 2; j++)
        {
            if(pYbufferU16[j] == 1)
            {
                pYbufferU16[j] = Ystroke;
                pYbufferU16[j + stride / 2] = Ystroke;
                pUvbuffer[j] = UVstroke;
                //printf("*");
            }
            else if(pYbufferU16[j] > 1)
            {
                //printf("#");
            }
            else
            {
                //printf("-");
            }
        }

        pYbufferU16 += stride;
        pUvbuffer += stride / 2;
        //printf("\n");
    }

    //printf("\n\n");
#endif

    //printf("\n");
    if(g_osd_dilate_profiling)
    {
        gettimeofday(&end, NULL);
        timersub(&end, &start, &end);
        dilate_total_time += end.tv_usec;
        dilate_count ++;

        if((MI_U32)end.tv_usec > dilate_max_time)
        {
            dilate_max_time = end.tv_usec;
            MIXER_INFO("dilate_max_time:%u usec\n", dilate_max_time);
        }
    }
}

void mid_BmpDilate(MI_U16* pbuffer , MI_U32 width , MI_U32 height , MI_U32 stride , MI_U16 rgbColor)
{
    MI_U32 i, j;
    struct timeval start, end;
    MI_U16* pRGBbuffer;
    MI_U16 strokeColor = ((65535 - rgbColor) | 0xF);


    if(g_osd_dilate_profiling)
        gettimeofday(&start, NULL);


    pRGBbuffer = (MI_U16*)pbuffer + stride;

    for(i = 1; i < height - 1; i++)
    {
        for(j = 1; j < width - 1; j++)
        {
            if((pRGBbuffer[j] & 0xF) > 1)
            {
                //printf("%d %d           ",i,j);
                if((pRGBbuffer[j - stride] & 0xF) == 0)
                {
                    pRGBbuffer[j - stride] = 1;
                    //printf("%d %d   ",i-1,j);
                }

                if((pRGBbuffer[j - 1] & 0xF) == 0)
                {
                    pRGBbuffer[j - 1] = 1;
                    //printf("%d %d   ",i,j-1);
                }

                if((pRGBbuffer[j + stride] & 0xF) == 0)
                {
                    pRGBbuffer[j + stride] = 1;
                    //printf("%d %d   ",i+1,j);
                }

                if((pRGBbuffer[j + 1] & 0xF) == 0)
                {
                    pRGBbuffer[j + 1] = 1;
                    //printf("%d %d",i,j+1);
                }

                //printf("\n");
            }
            else
            {
                //printf(" ");
            }
        }

        pRGBbuffer += stride;
        //printf("\n");
    }

    pRGBbuffer = (MI_U16*)pbuffer + stride;

    for(i = 1; i < height - 1; i++)
    {
        for(j = 1; j < width - 1; j++)
        {
            if(pRGBbuffer[j] == 1)
            {
                pRGBbuffer[j] = strokeColor;
                //printf("*");
            }
            else if(pRGBbuffer[j] > 1)
            {
                //printf("#");
            }
            else
            {
                //printf("-");
            }
        }

        pRGBbuffer += stride;
        //printf("\n");
    }


    if(g_osd_dilate_profiling)
    {
        gettimeofday(&end, NULL);
        timersub(&end, &start, &end);
        dilate_total_time += end.tv_usec;
        dilate_count ++;

        if((MI_U32)end.tv_usec > dilate_max_time)
        {
            dilate_max_time = end.tv_usec;
            MIXER_INFO("dilate_max_time:%u usec\n", dilate_max_time);
        }
    }
}

MI_S32 mid_Font_DrawYUV420(MI_Font_t* pFont , MI_U32 qu , MI_U32 wei, MI_U32 nMultiple, ImageData_t* pYuv,
                                        MI_U32 nIndex, YUVColor_t* pfColor, YUVColor_t* pbColor , BOOL bOutline)
{
    MI_U32 nCharWidth = (pFont->nFontSize + 7) / 8;
    MI_U8* pFontBitmap = pFont->pData + ((qu - 1) * 94 + wei - 1) * nCharWidth * pFont->nFontSize;
    MI_U32 i, j, ii, jj;
    MI_U8* yBuffer  = pYuv->buffer + nIndex * pYuv->height;
    MI_U8* uvBuffer = pYuv->buffer + pYuv->width * pYuv->height + nIndex * pYuv->height;

    MI_U32 bDot;
    YUVColor_t* pColor;
    MI_U8 *_yBuffer , *_uvBuffer , *__yBuffer, *__uvBuffer;


    //MIXER_INFO("%s YuvWdg  h:%d w:%d x:%d y:%d buffer:%p\n", __FUNCTION__,pYuvDdg->height,pYuvDdg->width,pYuvDdg->point.x,pYuvDdg->point.y,pYuvDdg->yuvBuff);
    //MIXER_INFO("%s Font size:%d qu:%d wei:%d multiple:%d\n",__FUNCTION__,pFont->nSize,qu,wei,nMultiple);

    //MIXER_INFO("%s fColor y:%d u:%d v:%d t:%d\n",__FUNCTION__,fColor.y,fColor.u,fColor.v,fColor.transparent);
    //MIXER_INFO("%s bColor y:%d u:%d v:%d t:%d\n",__FUNCTION__,bColor.y,bColor.u,bColor.v,bColor.transparent);

    for(j = 0; j < pFont->nFontSize; j++)
    {
        _yBuffer  = yBuffer;
        _uvBuffer = uvBuffer;

        for(i = 0; i < pFont->nFontSize; i++)
        {
            bDot = *(pFontBitmap + (i >> 3)) & (0x80 >> (i & 0x07));

            pColor = bDot ? pfColor : pbColor;

            __yBuffer  = _yBuffer;
            __uvBuffer = _uvBuffer;

            for(jj = 0; jj < nMultiple ; jj++)
            {
                for(ii = 0; ii < nMultiple ; ii++)
                {
                    __yBuffer[ii] = pColor->y;

                    if((!(ii & 1)) && (!(jj & 1)))
                    {
                        __uvBuffer[ii]     = pColor->u;
                        __uvBuffer[ii + 1] = pColor->v;
                    }
                }

                __yBuffer += pYuv->width;

                if(jj & 1)
                    __uvBuffer += pYuv->width;
            }

            _yBuffer  += nMultiple;
            _uvBuffer += nMultiple;
        }

        yBuffer  += nMultiple * pYuv->width;
        uvBuffer += nMultiple * pYuv->width / 2;
        pFontBitmap += nCharWidth;
    }

    yBuffer  = pYuv->buffer + nIndex * pYuv->height;
    uvBuffer = pYuv->buffer + pYuv->width * pYuv->height + nIndex * pYuv->height;

    if(bOutline)
    {
        mid_YuvDilate(yBuffer, (MI_U16*)uvBuffer, pYuv->width, nMultiple * pFont->nFontSize, nMultiple * pFont->nFontSize, pfColor);
    }

    return MI_SUCCESS;
}


MI_S32 mid_Font_DrawRGB4444(MI_Font_t* pFont , MI_U32 qu , MI_U32 wei, MI_U32 nMultiple, ImageData_t* pBmp,
                                          MI_U32 nIndex, Color_t* pfColor, Color_t* pbColor, BOOL bOutline)
{
    MI_U32 nCharWidth = (pFont->nFontSize + 7) / 8;
    MI_U8* pFontBitmap = pFont->pData + ((qu - 1) * 94 + wei - 1) * nCharWidth * pFont->nFontSize;
    MI_U32 i, j, ii, jj;
    MI_U8* pBuffer  = pBmp->buffer + nIndex * pBmp->height * 2;
    MI_U32 bDot;
    MI_U16* _pBuffer = (MI_U16*)pBuffer;
    MI_U16* __pBuffer;
    MI_U16* ___pBuffer;
    MI_U16 r , g , b , a ;
    MI_U16 fcolor4444 , bcolor4444;

    //MIXER_DBG("qu=%d wei=%d nMultiple=%d nIndex=%d nFontSize=%d\n", qu, wei, nMultiple, nIndex, pFont->nFontSize);

    r = pfColor->r;
    g = pfColor->g;
    b = pfColor->b;
    a = pfColor->a;
    //fcolor4444 = ((a & 0xFF) << 8)| ((r & 0xF0) << 4) | (g & 0xF0)| ((b & 0xF0) >> 4);
    fcolor4444 = ((b & 0xF0) << 8) | ((g & 0xF0) << 4) | (r & 0xF0) | ((a & 0xF0) >> 4);
    r = pbColor->r;
    g = pbColor->g;
    b = pbColor->b;
    a = pbColor->a;
    //bcolor4444 = ((a & 0xFF) << 8)| ((r & 0xF0) << 4) | (g & 0xF0)| ((b & 0xF0) >> 4);
    bcolor4444 = ((b & 0xF0) << 8) | ((g & 0xF0) << 4) | (r & 0xF0) | ((a & 0xF0) >> 4);

    //MIXER_DBG("_pBuffer = %p fcolor4444=%x  bcolor4444=%x \n", _pBuffer, fcolor4444, bcolor4444);

    for(j = 0; j < pFont->nFontSize; j++)
    {
        __pBuffer = _pBuffer;

        for(i = 0; i < pFont->nFontSize; i++)
        {
            bDot = *(pFontBitmap + (i >> 3)) & (0x80 >> (i & 0x07));
            //if(bDot)
            //printf("*");
            //else
            //printf(" ");

            ___pBuffer = __pBuffer;

            for(jj = 0; jj < nMultiple ; jj++)
            {
                for(ii = 0; ii < nMultiple ; ii++)
                {
                    ___pBuffer[ii] =   bDot ? fcolor4444 : bcolor4444 ;
                }

                ___pBuffer += pBmp->width;
            }

            __pBuffer += nMultiple;
        }

        //printf("\n");
        _pBuffer += nMultiple * pBmp->width;
        pFontBitmap += nCharWidth;
    }


    if(bOutline)
    {
        mid_BmpDilate((MI_U16*)pBuffer, nMultiple * pFont->nFontSize, nMultiple * pFont->nFontSize, pBmp->width, fcolor4444);
    }

    return MI_SUCCESS;
}

MI_S32 mid_Font_DrawRGB1555(MI_Font_t* pFont , MI_U32 qu , MI_U32 wei, MI_U32 nMultiple, ImageData_t* pBmp,
                                          MI_U32 nIndex, Color_t* pfColor, Color_t* pbColor, BOOL bOutline)
{
    MI_U32 nCharWidth = (pFont->nFontSize + 7) / 8;
    MI_U8* pFontBitmap = pFont->pData + ((qu - 1) * 94 + wei - 1) * nCharWidth * pFont->nFontSize;
    MI_U32 i, j, ii, jj;
    MI_U8* pBuffer  = pBmp->buffer + nIndex * pBmp->height * 2;
    MI_U32 bDot;
    MI_U16* _pBuffer = (MI_U16*)pBuffer;
    MI_U16* __pBuffer;
    MI_U16* ___pBuffer;
    MI_U16 r , g , b , a ;
    MI_U16 fcolor1555 = 0;
    MI_U16 bcolor1555 = 0;

    //MIXER_DBG("qu=%d wei=%d nMultiple=%d nIndex=%d nFontSize=%d\n", qu, wei, nMultiple, nIndex, pFont->nFontSize);

    r = pfColor->r;
    g = pfColor->g;
    b = pfColor->b;
    a = pfColor->a;
    fcolor1555 = ((a & 0x80) << 8) | ((r & 0xF8) << 7) | ((g & 0xF8) >> 2) | ((b & 0xF8) >> 3);

    r = pbColor->r;
    g = pbColor->g;
    b = pbColor->b;
    a = pbColor->a;
    if((a & 0x80))
    {
        bcolor1555 = 0x2323;
    }
    else
    {
        bcolor1555 = ((a & 0x80) << 8) | ((r & 0xF8) << 7) | ((g & 0xF8) >> 2) | ((b & 0xF8) >> 3);
    }

    //printf("=======> _pBuffer = %p fcolor1555=%x  bcolor1555=%x \n", _pBuffer, fcolor1555, bcolor1555);

    for(j = 0; j < pFont->nFontSize; j++)
    {
        __pBuffer = _pBuffer;

        for(i = 0; i < pFont->nFontSize; i++)
        {
            bDot = *(pFontBitmap + (i >> 3)) & (0x80 >> (i & 0x07));
            //if(bDot)
            //printf("*");
            //else
            //printf(" ");

            ___pBuffer = __pBuffer;

            for(jj = 0; jj < nMultiple ; jj++)
            {
                for(ii = 0; ii < nMultiple ; ii++)
                {
                    ___pBuffer[ii] =   bDot ? fcolor1555 : bcolor1555 ;
                }

                ___pBuffer += pBmp->width/2;
            }

            __pBuffer += nMultiple;
        }

        //printf("\n");

        _pBuffer +=  nMultiple *pBmp->width/2;
        pFontBitmap += nCharWidth;
    }

    if(bOutline)
    {
        mid_BmpDilate((MI_U16*)pBuffer, nMultiple * pFont->nFontSize, nMultiple * pFont->nFontSize, pBmp->width, fcolor1555);
    }

    return MI_SUCCESS;
}

MI_S32 mid_Font_DrawPaletteTable_I4(MI_Font_t* pFont , MI_U32 qu , MI_U32 wei, MI_U32 nMultiple, ImageData_t* pBmp,
                                                 MI_U32 nIndex, Color_t* pfColor, Color_t* pbColor, BOOL bOutline)
{
    MI_U32 nCharWidth = (pFont->nFontSize + 7) / 8;
    MI_U8* pFontBitmap = pFont->pData + ((qu - 1) * 94 + wei - 1) * nCharWidth * pFont->nFontSize;
    MI_U32 i = 0, j = 0, ii = 0, jj = 0;
    MI_U8* pBuffer  = pBmp->buffer + (nIndex * pFont->nFontSize * nMultiple) / 2;
    MI_U32 bDot;
    MI_U8* _pBuffer = (MI_U8*)pBuffer;
    MI_U8* __pBuffer;
    MI_U8* ___pBuffer;
    MI_U16 fcolorI4 = 0;
    MI_U16 bcolorI4 = 0;

    #ifdef MIXER_DRAW_TEXT_DEBUG
    #undef MIXER_DRAW_TEXT_DEBUG
    #endif
    #define MIXER_DRAW_TEXT_DEBUG 0
    #define MIXER_OSD_DEBUG_FONT  0


    fcolorI4 = pfColor->r & 0x0F;

    if((pbColor->a & 0x80)) //Set transparency
    {
        bcolorI4 = 0x00;
    }
    else  //Set display background color
    {
        bcolorI4 = pbColor->r & 0x0F;
    }

    if(MIXER_DRAW_TEXT_DEBUG && (MIXER_OSD_DEBUG_FONT == pFont->nFontSize))
    {
        printf("nFontSize=%d, pBmp->buffer=%p, _pBuffer=%p, nMultiple=%x, pBmp->width=%d(0x%x), pBmp->height=%d, nIndex=%d, qu=%d, wei=%d, nCharWidth=%d\n",
                pFont->nFontSize, pBmp->buffer, _pBuffer, nMultiple, pBmp->width, pBmp->width, pBmp->height, nIndex, qu, wei, nCharWidth);
    }

    for(j = 0; j < pFont->nFontSize; j++)
    {
        __pBuffer = _pBuffer;

        for(i = 0; i < pFont->nFontSize; i++)
        {
            bDot = *(pFontBitmap + (i >> 3)) & (0x80 >> (i & 0x07));

            if(0 && MIXER_DRAW_TEXT_DEBUG && (MIXER_OSD_DEBUG_FONT == pFont->nFontSize))
            {
                if(bDot) printf("*");
                else     printf(" ");
            }

            if(1 == nMultiple)
            {
                ___pBuffer = __pBuffer;

                if(bDot)
                {
                    //If it is an even position, it is stored in a lower 4bit position of one byte.
                    //elif it is an odd position, it is stored in the high 4bit position of one byte.
                    if(0 == (i % 2))
                    {
                        ___pBuffer[i/2] = ((bcolorI4 & 0x0F) << 4) | (fcolorI4 & 0x0F);
                    }
                    else if((0 < i) && (0 != (i % 2)))
                    {
                        ___pBuffer[(i-1)/2] |= (fcolorI4 & 0x0F) << 4;
                    }
                }
                else
                {
                    //If it is an even position, it is stored in a lower 4bit position of one byte.
                    //elif it is an odd position, it is stored in the high 4bit position of one byte.
                    if(0 == (i % 2))
                    {
                        ___pBuffer[i/2] = ((bcolorI4 & 0x0F) << 4) | (bcolorI4 & 0x0F);
                    }
                    else if((0 < i) && (0 != (i % 2)))
                    {
                        ___pBuffer[(i-1)/2] |= (bcolorI4 & 0x0F) << 4;
                    }
                }

                if(MIXER_DRAW_TEXT_DEBUG && (MIXER_OSD_DEBUG_FONT == pFont->nFontSize) && (0 < i) && (0 != (i % 2)))
                {
                    printf("%02x",___pBuffer[(i-1)/2]);
                }
            }
            else
            {
                ___pBuffer = __pBuffer;
                //Set the starting coordinate value of the color index value of the current point
                ___pBuffer += (nMultiple / 2) * i;

                for(jj = 0; jj < nMultiple; jj++) //Each column index value of a point needs to be processed separately
                {
                    ___pBuffer = __pBuffer;
                    ___pBuffer += (nMultiple / 2) * i;
                    ___pBuffer += pBmp->width * jj; //Start processing the columns of the enlarged font

                    for(ii = 0; ii < nMultiple / 2; ii++) //nMultiple = 2, 4, 6, 8 ...
                    {
                        if(bDot)
                        {
                            //One byte describing the color index value of a repeated point
                            ___pBuffer[ii] = ((fcolorI4 & 0x0F) << 4) | (fcolorI4 & 0x0F);
                        }
                        else
                        {
                            //One byte describing the color index value of a repeated point
                            ___pBuffer[ii] = ((bcolorI4 & 0x0F) << 4) | (bcolorI4 & 0x0F);
                        }

                        if(MIXER_DRAW_TEXT_DEBUG && (MIXER_OSD_DEBUG_FONT == pFont->nFontSize))
                        {
                            printf("%02x",___pBuffer[ii]);
                        }
                    }
                }
            }
        }


        //Start processing the next line of the font
        _pBuffer += pBmp->width * nMultiple;
        pFontBitmap += nCharWidth;

        if(0 && MIXER_DRAW_TEXT_DEBUG && (MIXER_OSD_DEBUG_FONT == pFont->nFontSize))
        {
            printf("\n");
        }

        if(MIXER_DRAW_TEXT_DEBUG && (MIXER_OSD_DEBUG_FONT == pFont->nFontSize))
        {
            printf("  j=%02d,i=%02d,jj=%02d,ii=%02d, pBuffer=%p,_pBuffer=%p,__pBuffer=%p,___pBuffer=%p\n",
                      j,i,jj,ii,pBuffer,_pBuffer,__pBuffer,___pBuffer);
        }
    }


    if(bOutline)
    {
        //mid_BmpDilate((MI_U16*)pBuffer, nMultiple * pFont->nFontSize, nMultiple * pFont->nFontSize, pBmp->width, fcolorI4);
    }

    if(MIXER_DRAW_TEXT_DEBUG && (MIXER_OSD_DEBUG_FONT == pFont->nFontSize))
    {
        _pBuffer = pBmp->buffer;
        __pBuffer = _pBuffer;
        printf("\npBmp->buffer=%p,_pBuffer=%p,__pBuffer=%p,___pBuffer=%p,pBmp->width=%d(0x%x), pBmp->height=%d\n",
                pBmp->buffer,_pBuffer,__pBuffer,___pBuffer,pBmp->width,pBmp->width,pBmp->height);

        for(j = 0; j < pBmp->height; j++)
        {
            __pBuffer = _pBuffer;

            for(jj = 0; jj < pBmp->width; jj++)
            {
                printf("%02x",__pBuffer[jj]);
            }

            _pBuffer += pBmp->width;
            printf(" j=%02d, __pBuffer=%p, _pBuffer=%p\n",j,__pBuffer,_pBuffer);
        }

        printf("pBmp->buffer=%p, _pBuffer=%p, __pBuffer=%p, pBmp->width=%d, pBmp->height=%d\n\n",
                pBmp->buffer,_pBuffer,__pBuffer,pBmp->width,pBmp->height);
    }

    #undef MIXER_DRAW_TEXT_DEBUG
    return MI_SUCCESS;
}

 MI_S32 mid_Font_DrawPaletteTable_I2(MI_Font_t* pFont , MI_U32 qu , MI_U32 wei, MI_U32 nMultiple, ImageData_t* pBmp,
                                             MI_U32 nIndex, Color_t* pfColor, Color_t* pbColor, BOOL bOutline)
 {
     MI_U32 nCharWidth = (pFont->nFontSize + 7) / 8;
     MI_U8* pFontBitmap = pFont->pData + ((qu - 1) * 94 + wei - 1) * nCharWidth * pFont->nFontSize;
     MI_U32 i, j, ii, jj;
     MI_U8* pBuffer  = pBmp->buffer + nIndex *pBmp->height / 4;
     MI_U32 bDot;
     MI_U8* _pBuffer = (MI_U8*)pBuffer;
     MI_U8* __pBuffer;
     MI_U8* ___pBuffer;
     MI_U16 fcolorI2 = 0;
     MI_U16 bcolorI2 = 0;
     fcolorI2 = pfColor->r & 0x0F;
     if((pbColor->a & 0x80))
     {
        bcolorI2 = 0x00;
     }
     else
     {
        bcolorI2 = pbColor->r & 0x0F;
     }
#define DEBUG_I2_OSD 0
#if DEBUG_I2_OSD
     {
         printf("nFontSize=%d, pBmp->buffer=%p, _pBuffer=%p, nMultiple=%x, pBmp->width=%d(0x%x), pBmp->height=%d, nIndex=%d, qu=%d, wei=%d, nCharWidth=%d\n",
                pFont->nFontSize, pBmp->buffer, _pBuffer, nMultiple, pBmp->width, pBmp->width, pBmp->height, nIndex, qu, wei, nCharWidth);
     }
#endif
     for(j = 0; j < pFont->nFontSize; j++)
     {
         __pBuffer = _pBuffer;
         for(i = 0; i < pFont->nFontSize; i++)
         {
             bDot = *(pFontBitmap + (i >> 3)) & (0x80 >> (i & 0x07));
#if DEBUG_I2_OSD
             if(bDot)
                printf("*");
             else
                printf(".");
#endif
             if(1 == nMultiple)
             {
                 ___pBuffer = __pBuffer;
                 if(bDot)
                 {
                     if(0 == (i % 4))
                     {
                         ___pBuffer[i/4] = ((bcolorI2 & 0x03) << 6) |((bcolorI2 & 0x03)<<4)|((bcolorI2 & 0x03) << 2) | (fcolorI2 & 0x03);
                     }
                     else if((0 < i) && (1 == (i % 4)))
                     {
                          ___pBuffer[i/4] |= ((bcolorI2 & 0x03) << 6) |((bcolorI2 & 0x03)<<4)|((fcolorI2 & 0x03) << 2) ;
                     }
                     else if((0 < i) && (2 == (i % 4)))
                     {
                         ___pBuffer[i/4]  |= ((bcolorI2 & 0x03) << 6) |((fcolorI2 & 0x03)<<4) ;
                     }
                     else if((0 < i) && (3 == (i % 4)))
                     {
                         ___pBuffer[i/4]  |= ((fcolorI2 & 0x03) << 6);
                     }
                 }
                 else
                 {
                     if(0 == (i % 4))
                     {
                         ___pBuffer[i/4] = ((bcolorI2 & 0x03) << 6) |((bcolorI2 & 0x03)<<4)|((bcolorI2 & 0x03) << 2) | (bcolorI2 & 0x03);
                     }
                     else if((0 < i) && (1 == (i % 4)))
                     {
                          ___pBuffer[i/4] |= ((bcolorI2 & 0x03) << 6) |((bcolorI2 & 0x03)<<4)|((bcolorI2 & 0x03) << 2) ;
                     }
                     else if((0 < i) && (2 == (i % 4)))
                     {
                         ___pBuffer[i/4]  |= ((bcolorI2 & 0x03) << 6) |((bcolorI2 & 0x03)<<4) ;
                     }
                     else if((0 < i) && (3 == (i % 4)))
                     {
                         ___pBuffer[i/4]  |= ((bcolorI2 & 0x03) << 6);
                     }
                 }
             }
             else if(nMultiple == 2)
             {
                 ___pBuffer = __pBuffer;
                 for(jj = 0; jj < nMultiple ; jj++)
                 {
                     for(ii = 0; ii < nMultiple/2 ; ii++) //now only support nMultiple 2
                     {
                         if(bDot)
                         {
                              if(0 == (i % 2))
                              {
                                  ___pBuffer[ii/2] |= ((fcolorI2 & 0x03) << 2) | (fcolorI2 & 0x03);
                              }
                              else if((0 < i) && (0 != (i % 2)))
                              {
                                  ___pBuffer[(ii)/2] |=((fcolorI2 & 0x03) << 6) | (fcolorI2 & 0x03)<<4;
                              }
                         }
                         else
                         {
                             if(0 == (i % 2))
                             {
                                 ___pBuffer[ii/2] |= ((bcolorI2 & 0x03) << 2) | (bcolorI2 & 0x03);
                             }
                             else if((0 < i) && (0 != (i % 2)))
                             {
                                 ___pBuffer[(ii)/2]|=((bcolorI2 & 0x03) << 6) | (bcolorI2 & 0x03)<<4;
                             }
                         }
                     }
                     ___pBuffer += pBmp->width*nMultiple/2;
                 }
                  if(i%2 == 1)
                 __pBuffer += nMultiple / 2;
             }
             else if(nMultiple == 6)
             {
                 ___pBuffer = __pBuffer;
                 for(jj = 0; jj < nMultiple ; jj++)
                 {
                     for(ii = 0; ii < nMultiple/6 ; ii++) //now only support nMultiple 6
                     {
                         if(bDot)
                         {
                              if(0 == (i % 2))
                              {
                                  ___pBuffer[ii/2] = ((fcolorI2 & 0x03) << 6) | ((fcolorI2 & 0x03)<<4)|((fcolorI2 & 0x03) << 2) | (fcolorI2 & 0x03);
                                  ___pBuffer[ii/2 + 1] |= ((fcolorI2 & 0x03) << 2) | (fcolorI2 & 0x03);
                              }
                              else if((0 < i) && (0 != (i % 2)))
                              {
                                  ___pBuffer[(ii)/2] |=((fcolorI2 & 0x03) << 6) | (fcolorI2 & 0x03)<<4;
                                  ___pBuffer[ii/2 + 1] = ((fcolorI2 & 0x03) << 6) | ((fcolorI2 & 0x03)<<4)|((fcolorI2 & 0x03) << 2) | (fcolorI2 & 0x03);
                              }
                         }
                         else
                         {
                             if(0 == (i % 2))
                             {
                                 ___pBuffer[ii/2] = ((bcolorI2 & 0x03) << 6) | ((bcolorI2 & 0x03)<<4)|((bcolorI2 & 0x03) << 2) | (bcolorI2 & 0x03);
                                 ___pBuffer[ii/2 + 1] |= ((bcolorI2 & 0x03) << 2) | (bcolorI2 & 0x03);
                             }
                             else if((0 < i) && (0 != (i % 2)))
                             {
                                 ___pBuffer[(ii)/2] |=((bcolorI2 & 0x03) << 6) | (bcolorI2 & 0x03)<<4;
                                 ___pBuffer[ii/2 + 1] = ((bcolorI2 & 0x03) << 6) | ((bcolorI2 & 0x03)<<4)|((bcolorI2 & 0x03) << 2) | (bcolorI2 & 0x03);
                             }
                          }
                        }
                     ___pBuffer += pBmp->width;
                 }
                  if(i%2 == 1)
                     __pBuffer += nMultiple / 2 - 1;
                  else
                     __pBuffer += 1;
             }
         }
         _pBuffer += pBmp->width* nMultiple;
#if DEBUG_I2_OSD
          printf("\n");
#endif
         pFontBitmap += nCharWidth;
     }
     if(bOutline)
     {
     }
     return MI_SUCCESS;
 }
MI_U32 mid_Font_GetSizeByType(OsdFontSize_e eFontSize)
{
    switch(eFontSize)
    {
        case FONT_SIZE_8:  return 8;
        case FONT_SIZE_12: return 12;
        case FONT_SIZE_16: return 16;
        case FONT_SIZE_24: return 24;
        case FONT_SIZE_32: return 32;
        case FONT_SIZE_36: return 36;
        case FONT_SIZE_40: return 40;
        case FONT_SIZE_48: return 48;
        case FONT_SIZE_56: return 56;
        case FONT_SIZE_60: return 60;
        case FONT_SIZE_64: return 64;
        case FONT_SIZE_72: return 72;
        case FONT_SIZE_80: return 80;
        case FONT_SIZE_84: return 84;
        case FONT_SIZE_96: return 96;
        default:           return 0;
    }
}


MI_S32 mid_Font_DrawText(ImageData_t* pImage, const MI_S8 *pStr, MI_U32 idx_start, OsdFontSize_e eFontSize,
                                MI_U32 nSpace, Color_t* pfColor, Color_t* pbColor, BOOL bOutline)
{
   MI_U32 hzLen = mid_Font_Strlen(pStr);
   MI_U32 idx;
   MI_Font_t* pFont;
   MI_U32 nMultiple = 0;
   MI_U32 qu = 0, wei = 0, offset = 0;


   if(NULL == (pFont = mid_Font_GetHandle(eFontSize, &nMultiple)))
   {
        printf("%s:%d cannot get the Font handle and return failed!\n", __func__,__LINE__);
        return E_MI_ERR_FAILED;
   }

   for(idx = 0; idx < hzLen; idx++)
   {
       offset = mid_Font_GetCharQW(pStr, &qu, &wei);
       pStr += offset;

       //if(idx < idx_start)
       //    continue;

       switch(pImage->pmt)
       {
        /*   case COLOR_FormatYUV420SemiPlanar:
               YUVColor_t yuvfColor , yuvbColor;
               Argb2Yuv(pfColor, &yuvfColor);
               Argb2Yuv(pbColor, &yuvbColor);
               mid_Font_DrawYUV420(pFont, qu, wei, nMultiple, pImage, idx, &yuvfColor, &yuvbColor , bOutline);
               break;*/

           case E_MI_RGN_PIXEL_FORMAT_ARGB4444:
               mid_Font_DrawRGB4444(pFont, qu, wei, nMultiple, pImage, idx, pfColor, pbColor, bOutline);
               break;

           case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
               mid_Font_DrawRGB1555(pFont, qu, wei, nMultiple, pImage, idx, pfColor, pbColor, bOutline);
               break;

           case E_MI_RGN_PIXEL_FORMAT_I4:
               mid_Font_DrawPaletteTable_I4(pFont, qu, wei, nMultiple, pImage, idx, pfColor, pbColor, bOutline);
               break;

           case E_MI_RGN_PIXEL_FORMAT_I2:
               mid_Font_DrawPaletteTable_I2(pFont, qu, wei, nMultiple, pImage, idx, pfColor, pbColor, bOutline);
               break;
           default:
               MIXER_ERR("only support argb1555 or I4 or I2now\n");
       }
   }

   return MI_SUCCESS;
}

typedef struct _BITMAP_FILEHEADER
{
    MI_U16 Signature;
    MI_U32 Size;
    MI_U32 Reserved;
    MI_U32 BitsOffset;
} __attribute__((packed)) BITMAP_FILEHEADER;


typedef struct
{
    MI_U32 HeaderSize;
    MI_S32 Width;
    MI_S32 Height;
    MI_U16 Planes;
    MI_U16 BitCount;
    MI_U32 Compression;
    MI_U32 SizeImage;
    MI_S32 PelsPerMeterX;
    MI_S32 PelsPerMeterY;
    MI_U32 ClrUsed;
    MI_U32 ClrImportant;
    MI_U32 RedMask;
    MI_U32 GreenMask;
    MI_U32 BlueMask;
    MI_U32 AlphaMask;
    MI_U32 CsType;
    MI_U32 Endpoints[9]; // see http://msdn2.microsoft.com/en-us/library/ms536569.aspx
    MI_U32 GammaRed;
    MI_U32 GammaGreen;
    MI_U32 GammaBlue;
} __attribute__((packed)) BITMAP_HEADER;



typedef struct
{
    MI_U8 Red;
    MI_U8 Green;
    MI_U8 Blue;
    MI_U8 Alpha;
} __attribute__((packed))RGBA;

typedef struct
{
    MI_U8 Blue;
    MI_U8 Green;
    MI_U8 Red;
    MI_U8 Alpha;
} __attribute__((packed))BGRA;


typedef struct
{
    long maskB;
    long maskG;
    long maskR;
    long maskA;
} __attribute__((packed))ARGBMASK;



//14byte文件头
typedef struct
{
    char cfType[2];//文件类型，"BM"(0x4D42)
    long cfSize;//文件大小（字节）
    long cfReserved;//保留，值为0
    long cfoffBits;//数据区相对于文件头的偏移量（字节）
} __attribute__((packed)) BITMAPFILEHEADER;
//__attribute__((packed))的作用是告诉编译器取消结构在编译过程中的优化对齐

//40byte信息头
typedef struct
{
    char ciSize[4];//BITMAPFILEHEADER所占的字节数
    long ciWidth;//宽度
    long ciHeight;//高度
    char ciPlanes[2];//目标设备的位平面数，值为1
    short ciBitCount;//每个像素的位数
    long ciCompress;//压缩说明
    char ciSizeImage[4];//用字节表示的图像大小，该数据必须是4的倍数
    char ciXPelsPerMeter[4];//目标设备的水平像素数/米
    char ciYPelsPerMeter[4];//目标设备的垂直像素数/米
    char ciClrUsed[4]; //位图使用调色板的颜色数
    char ciClrImportant[4]; //指定重要的颜色数，当该域的值等于颜色数时（或者等于0时），表示所有颜色都一样重要
} __attribute__((packed)) BITMAPINFOHEADER;



MI_S32 MI_Bmp2RGB4444(RGBA4444 *dstData, MI_U8* srcData , MI_U32 srcWidth , MI_U32 srcHeight , PixelFormat_e fmt)
{
    MI_U16 a, r, g, b;
    MI_U32 i, j, Index;
    MI_U32 Color, bitCount , srcSlide;
    MI_U8 *LinePtr;


    Index = 0;

    switch(fmt)
    {
        case COLOR_Format24bitRGB888:

            bitCount = 24;
            srcSlide = srcWidth * bitCount / 8;

            for(i = 0; i < srcHeight; i++)
            {
                LinePtr = srcData;

                for(j = 0; j < srcWidth; j++)
                {
                    b = LinePtr[0];
                    g = LinePtr[1];
                    r = LinePtr[2];
                    a = 0xFF;

                    dstData[Index] = (((b & 0xF0) | (g >> 4)) << 8) | (r & 0xF0) | (a >> 4);
                    Index++;
                    LinePtr += 3;
                }

                srcData += srcSlide;
            }

            break;

        case COLOR_Format16bitARGB1555:

            bitCount = 16;
            srcSlide = srcWidth * bitCount / 8;

            for(i = 0; i < srcHeight; i++)
            {
                LinePtr = srcData;

                for(j = 0; j < srcWidth; j++)
                {
                    Color = *((MI_U16*)LinePtr);
                    b = Color << 3;
                    g = Color >> 2;
                    r = Color >> 7;
                    a = Color & 0x8000 ? 0xFF : 0 ;

                    dstData[Index] = (((b & 0xF0) | (g >> 4)) << 8) | (r & 0xF0) | (a >> 4);
                    Index++;
                    LinePtr += 2;
                }

                srcData += srcSlide;
            }

            break;


        case COLOR_Format16bitBGR565:

            bitCount = 16;
            srcSlide = srcWidth * bitCount / 8;

            for(i = 0; i < srcHeight; i++)
            {
                LinePtr = srcData;

                for(j = 0; j < srcWidth; j++)
                {
                    Color = *((MI_U16*)LinePtr);
                    b = Color << 3;
                    g = Color >> 3;
                    r = Color >> 8;
                    a = 0xFF;

                    dstData[Index] = (((b & 0xF0) | (g >> 4)) << 8) | (r & 0xF0) | (a >> 4);
                    Index++;
                    LinePtr += 2;
                }

                srcData += srcSlide;
            }

            break;

        case COLOR_Format32bitABGR8888:

            bitCount = 32;
            srcSlide = srcWidth * bitCount / 8;

            for(i = 0; i < srcHeight; i++)
            {
                LinePtr = srcData;

                for(j = 0; j < srcWidth; j++)
                {
                    b = LinePtr[0];
                    g = LinePtr[1];
                    r = LinePtr[2];
                    a = LinePtr[3];

                    dstData[Index] = (((b & 0xF0) | (g >> 4)) << 8) | (r & 0xF0) | (a >> 4);
                    Index++;
                    LinePtr += 4;
                }

                srcData += srcSlide;
            }

            break;

        case COLOR_Format16bitARGB4444:
            break;

        default:
            MIXER_ERR("[%s] not support this pixel config:%d\n", __func__, fmt);
            return E_MI_ERR_MAX;
    }


    return MI_SUCCESS;
}

void Argb2Yuv(Color_t* pRgbColor , YUVColor_t* pYuvColor)
{
    if(pRgbColor->a == 0)
    {
        pYuvColor->y = 0;
        pYuvColor->u = 0;
        pYuvColor->v = 0;
        pYuvColor->transparent = TRUE;
    }
    else
    {
        pYuvColor->y = (((66 * pRgbColor->r + 129 * pRgbColor->g +  25 * pRgbColor->b + 128) >> 8) + 16);
        pYuvColor->u = ((-38 * pRgbColor->r -  74 * pRgbColor->g + 112 * pRgbColor->b + 128) >> 8) + 128 ;
        pYuvColor->v = ((112 * pRgbColor->r -  94 * pRgbColor->g -  18 * pRgbColor->b + 128) >> 8) + 128 ;
        pYuvColor->transparent = FALSE;
    }

    //printf("r:%u g:%u b:%u ---> y:%u u:%u v:%u\n",pRgbColor->r,pRgbColor->g,pRgbColor->b,pYuvColor->y,pYuvColor->u,pYuvColor->v);
}


MI_U32 mid_GetPixelBitCount(MI_RGN_PixelFormat_e pixelConfig)
{
    MI_U8 bitCount = 0;

    switch(pixelConfig)
    {
        case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
        case E_MI_RGN_PIXEL_FORMAT_ARGB4444:
        case E_MI_RGN_PIXEL_FORMAT_RGB565:
            bitCount = 16;
            break;

        case E_MI_RGN_PIXEL_FORMAT_I2:
            bitCount = 2;
            break;

        case E_MI_RGN_PIXEL_FORMAT_I4:
            bitCount = 4;
            break;

        case E_MI_RGN_PIXEL_FORMAT_I8:
            bitCount = 8;
            break;

        case E_MI_RGN_PIXEL_FORMAT_ARGB8888:
            bitCount = 32;
            break;

        default:
            MIXER_ERR("[%s] not support this pixel config:%d\n", __func__, pixelConfig);
            break;
    }

    return bitCount;
}


// I4 pt.x % 2 == 0, w % 2 == 0
// I2 pt.x % 4 == 0, w % 4 == 0

void DrawPoint(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t stPt, DrawRgnColor_t stColor)
{
    switch(stColor.ePixelFmt)
    {
        case E_MI_RGN_PIXEL_FORMAT_I4:
            {
                MI_U8 *pDrawBase = (MI_U8*)pBaseAddr;
                if (stPt.u16X % 2)
                    *(pDrawBase+(u32Stride*stPt.u16Y)+stPt.u16X/2) |= stColor.u32Color&0x0f;    // copy 1 byte
                else
                    *(pDrawBase+(u32Stride*stPt.u16Y)+stPt.u16X/2) |= (stColor.u32Color&0x0f) << 4;
            }
            break;
        case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
            {
                MI_U16 *pDrawBase = (MI_U16*)pBaseAddr;
                *(pDrawBase+(u32Stride/2*stPt.u16Y)+stPt.u16X) = stColor.u32Color & 0xffff;      // copy 2 byte, app check alignment
            }
            break;
        case E_MI_RGN_PIXEL_FORMAT_I2:
            {
                MI_U8 *pDrawBase = (MI_U8*)pBaseAddr;
                if (stPt.u16X % 4 == 0)
                    *(pDrawBase+(u32Stride*stPt.u16Y)+stPt.u16X/4) |= stColor.u32Color&0x03;
                else if(stPt.u16X % 4 == 1)
                    *(pDrawBase+(u32Stride*stPt.u16Y)+stPt.u16X/4) |= (stColor.u32Color&0x03) << 2;
                else if(stPt.u16X % 4 == 2)
                    *(pDrawBase+(u32Stride*stPt.u16Y)+stPt.u16X/4) |= (stColor.u32Color&0x03) << 4;
                else if(stPt.u16X % 4 == 3)
                    *(pDrawBase+(u32Stride*stPt.u16Y)+stPt.u16X/4) |= (stColor.u32Color&0x03) << 6;
            }
            break;
        default:
            printf("format not support\n");
    }
}

void DrawLine(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t stStartPt, DrawPoint_t stEndPt, MI_U8 u8BorderWidth, DrawRgnColor_t stColor)
{
    MI_U32 i = 0;
    MI_U32 j = 0;
    DrawPoint_t stPt;
    MI_U32 u32Width = stEndPt.u16X - stStartPt.u16X + 1;
    MI_U32 u32Height = stEndPt.u16Y - stStartPt.u16Y + 1;

    if ( (u8BorderWidth > u32Width/2) || (u8BorderWidth > u32Height/2) )
    {
        printf("invalid border width\n");
        return;
    }

    for (i = 0; i < (stEndPt.u16X - stStartPt.u16X); i++)
    {
        for (j = (1-u8BorderWidth)/2; j <= u8BorderWidth/2; j++)
        {
            stPt.u16X = (stStartPt.u16X+j) + i;
            stPt.u16Y = stStartPt.u16Y + u32Height * (j+i) / u32Width;
            DrawPoint(pBaseAddr, u32Stride, stPt, stColor);
        }
    }
}

void DrawRect(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t stLeftTopPt, DrawPoint_t stRightBottomPt, MI_U8 u8BorderWidth, DrawRgnColor_t stColor)
{
    MI_U32 i = 0;
    MI_U32 j = 0;
    MI_U32 u32Width = stRightBottomPt.u16X - stLeftTopPt.u16X + 1;
    MI_U32 u32Height = stRightBottomPt.u16Y - stLeftTopPt.u16Y + 1;

    if ( (u8BorderWidth > u32Width/2) || (u8BorderWidth > u32Height/2) )
    {
        printf("invalid border width\n");
        return;
    }


    switch(stColor.ePixelFmt)
    {
            case E_MI_RGN_PIXEL_FORMAT_I2:
            {
                MI_U8 *pDrawBase = (MI_U8*)pBaseAddr;
                if (stLeftTopPt.u16X%4 || stRightBottomPt.u16X%4 || (u8BorderWidth > u32Width/4) || (u8BorderWidth > u32Height/4))
                {
                    printf("invalid rect position\n");
                    return;
                }
                for (i = 0; i < u32Width/4; i++)
                {
                    for (j = 0; j < u8BorderWidth && ((stLeftTopPt.u16X/4 + i) < u32Stride); j++)
                    {
                        *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+j)+stLeftTopPt.u16X/4+i) = (stColor.u32Color&0x03) | ((stColor.u32Color&0x03) << 2) | ((stColor.u32Color&0x03) << 4) | ((stColor.u32Color&0x03) << 6);     // copy 1 byte
                        *(pDrawBase+u32Stride*(stRightBottomPt.u16Y-j)+stLeftTopPt.u16X/4+i) = (stColor.u32Color&0x03) | ((stColor.u32Color&0x03) << 2) | ((stColor.u32Color&0x03) << 4) | ((stColor.u32Color&0x03) << 6); // copy 1 byte
                    }
                }
                for (i = 0; i < u32Height; i++)
                {
                    for (j = 0; j < u8BorderWidth/4; j++)
                    {
                        if((stLeftTopPt.u16X/4 + j) < u32Stride)
                        {
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/4+j) = (stColor.u32Color&0x03) | ((stColor.u32Color&0x03) << 2) | ((stColor.u32Color&0x03) << 4) | ((stColor.u32Color&0x03) << 6);    // copy 1 byte
                        }
                        if((stRightBottomPt.u16X/4 - j) < u32Stride)
                        {
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stRightBottomPt.u16X/4-j) = (stColor.u32Color&0x03) | ((stColor.u32Color&0x03) << 2) | ((stColor.u32Color&0x03) << 4) | ((stColor.u32Color&0x03) << 6);// copy 1 byte
                        }
                    }
                    if (u8BorderWidth % 4)
                    {
                        if(((stLeftTopPt.u16X/4 + u8BorderWidth/4) < u32Stride))
                        {
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/4+u8BorderWidth/4) &= 0xf0;
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/4+u8BorderWidth/4) |= (stColor.u32Color&0x03) | ((stColor.u32Color&0x03) << 2) ;
                        }
                        if(((stRightBottomPt.u16X/4 - u8BorderWidth/4) < u32Stride))
                        {
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stRightBottomPt.u16X/4-u8BorderWidth/4) &= 0x0f;
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stRightBottomPt.u16X/4-u8BorderWidth/4) |= ((stColor.u32Color&0x03) | ((stColor.u32Color&0x03) << 2) ) << 4;
                        }
                    }
                }
            }
            break;
        case E_MI_RGN_PIXEL_FORMAT_I4:
            {
                MI_U8 *pDrawBase = (MI_U8*)pBaseAddr;

                if (stLeftTopPt.u16X%2 || stRightBottomPt.u16X%2)
                {
                    printf("invalid rect position\n");
                    return;
                }

                for (i = 0; i < u32Width/2; i++)
                {
                    for (j = 0; j < u8BorderWidth && ((stLeftTopPt.u16X/2 + i) < u32Stride); j++)
                    {

                        *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+j)+stLeftTopPt.u16X/2+i) = (stColor.u32Color&0x0f) | ((stColor.u32Color&0x0f) << 4);     // copy 1 byte
                        *(pDrawBase+u32Stride*(stRightBottomPt.u16Y-j)+stLeftTopPt.u16X/2+i) = (stColor.u32Color&0x0f) | ((stColor.u32Color&0x0f) << 4); // copy 1 byte
                    }
                }

                for (i = 0; i < u32Height; i++)
                {
                    for (j = 0; j < u8BorderWidth/2; j++)
                    {
                        if((stLeftTopPt.u16X/2 + j) < u32Stride)
                        {
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/2+j) = (stColor.u32Color&0x0f) | ((stColor.u32Color&0x0f) << 4);    // copy 1 byte
                        }

                        if((stRightBottomPt.u16X/2 - j) < u32Stride)
                        {
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stRightBottomPt.u16X/2-j) = (stColor.u32Color&0x0f) | ((stColor.u32Color&0x0f) << 4);// copy 1 byte
                        }
                    }

                    if (u8BorderWidth % 2)
                    {
                        if(((stLeftTopPt.u16X/2 + u8BorderWidth/2) < u32Stride))
                        {
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/2+u8BorderWidth/2) &= 0xf0;
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/2+u8BorderWidth/2) |= stColor.u32Color&0x0f;
                        }

                        if(((stRightBottomPt.u16X/2 - u8BorderWidth/2) < u32Stride))
                        {
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stRightBottomPt.u16X/2-u8BorderWidth/2) &= 0x0f;
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stRightBottomPt.u16X/2-u8BorderWidth/2) |= (stColor.u32Color&0x0f) << 4;
                        }
                    }
                }
            }
            break;
        case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
            {
                MI_U16 *pDrawBase = (MI_U16*)pBaseAddr;

                for (i = 0; i < u32Width; i++)
                {
                    for (j = 0; j < u8BorderWidth && (stLeftTopPt.u16X + i) < u32Stride/2; j++)
                    {
                        *(pDrawBase+u32Stride/2*(stLeftTopPt.u16Y+j)+stLeftTopPt.u16X+i) = stColor.u32Color & 0xffff;    // copy 2 byte, app check alignment
                        *(pDrawBase+u32Stride/2*(stRightBottomPt.u16Y-j)+stLeftTopPt.u16X+i) = stColor.u32Color & 0xffff;// copy 2 byte, app check alignment
                    }
                }

                for (i = 0; i < u32Height; i++)
                {
                    for (j = 0; j < u8BorderWidth; j++)
                    {
                        if((stLeftTopPt.u16X + j) < u32Stride/2)
                        {
                            *(pDrawBase+u32Stride/2*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X+j) = stColor.u32Color & 0xffff;    // copy 2 byte, app check alignment
                        }

                        if((stRightBottomPt.u16X - j) < u32Stride/2)
                        {
                            *(pDrawBase+u32Stride/2*(stLeftTopPt.u16Y+i)+stRightBottomPt.u16X-j) = stColor.u32Color & 0xffff;// copy 2 byte, app check alignment
                        }
                    }
                }
            }
            break;
        default:
            printf("format not support\n");
    }
}


void FillRect(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t stLeftTopPt, DrawPoint_t stRightBottomPt, DrawRgnColor_t stColor)
{
    MI_U32 i = 0;
    MI_U32 j = 0;
    MI_U32 u32Width = stRightBottomPt.u16X - stLeftTopPt.u16X + 1;
    MI_U32 u32Height = stRightBottomPt.u16Y - stLeftTopPt.u16Y + 1;

    for (i = 0; i < u32Height; i++)
    {
        switch(stColor.ePixelFmt)
        {
            case E_MI_RGN_PIXEL_FORMAT_I2:
                {
                    MI_U8 *pDrawBase = (MI_U8*)pBaseAddr;
                    if (stLeftTopPt.u16X%4 || stRightBottomPt.u16X%4)
                    {
                        printf("invalid rect position\n");
                        return;
                    }
                    for (j = 0; j < u32Width/4; j++)
                    {
                        *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/4+j) = (stColor.u32Color&0x03) | ((stColor.u32Color&0x03) << 2) | ((stColor.u32Color&0x03) << 4) | ((stColor.u32Color&0x03) << 6);
                    }
                    if (u32Width % 4)
                    {
                        *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/4+u32Width/4) = ((stColor.u32Color&0x03) | ((stColor.u32Color&0x03) << 2)) & 0x0f;
                    }
                }
                break;
            case E_MI_RGN_PIXEL_FORMAT_I4:
                {
                    MI_U8 *pDrawBase = (MI_U8*)pBaseAddr;

                    if (stLeftTopPt.u16X%2 || stRightBottomPt.u16X%2)
                    {
                        printf("invalid rect position\n");
                        return;
                    }

                    for (j = 0; j < u32Width/2; j++)
                    {
                        *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/2+j) = (stColor.u32Color&0x0f) | ((stColor.u32Color&0x0f) << 4);
                    }

                    if (u32Width % 2)
                    {
                        *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/2+u32Width/2) = stColor.u32Color & 0x0f;
                    }
                }
                break;
            case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
                {
                    MI_U16 *pDrawBase = (MI_U16*)pBaseAddr;

                    for (j = 0; j < u32Width; j++)
                    {
                        *(pDrawBase+u32Stride/2*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X+j) = stColor.u32Color & 0xffff;
                    }
                }
                break;
            default:
                printf("format not support\n");
        }
    }
}

void DrawCircular(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t stCenterPt, MI_U32 u32Radius, MI_U8 u8BorderWidth, DrawRgnColor_t stColor)
{

}

void FillCircular(void *pBaseAddr, MI_U32 u32Stride, DrawPoint_t stCenterPt, MI_U32 u32Radius, DrawRgnColor_t stColor)
{

}



