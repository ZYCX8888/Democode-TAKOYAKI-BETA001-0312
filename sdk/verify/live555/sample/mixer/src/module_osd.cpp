/*************************************************
 *
 * Copyright (c) 2006-2015 SigmaStar Technology Inc.
 * All rights reserved.
 *
 **************************************************
 * File name:  module_osd.c
 * Author:     andely.zhou@sigmastar.com.cn
 * Version:    Initial Draft
 * Date:       2018/6/13
 * Description: mixer osd module source file
 *
 *
 *
 * History:
 *
 *    1. Date  :        2018/6/27
 *       Author:        andely.zhou@sigmastar.com.cn
 *       Modification:  Created file
 *
 **************************************************/

#include <time.h>

#include "module_common.h"
#include "mid_common.h"
#include "mid_osd.h"
#include "mid_VideoEncoder.h"


extern MI_U32 g_videoNumber;
extern MI_S32 g_s32OsdFlicker;
extern MI_S32 g_s32OsdTextHandleCnt;
extern MI_RGN_HANDLE g_au32OsdI2Widget[MAX_VIDEO_NUMBER][MAX_RGN_NUMBER_PER_CHN];
extern MI_RGN_HANDLE g_au32OsdI4Widget[MAX_VIDEO_NUMBER][MAX_RGN_NUMBER_PER_CHN];
extern MI_RGN_HANDLE g_au32OsdI8Widget[MAX_VIDEO_NUMBER][MAX_RGN_NUMBER_PER_CHN];
extern MI_RGN_HANDLE g_au32BitmapWidget[MAX_VIDEO_NUMBER][MAX_RGN_NUMBER_PER_CHN];
extern MI_VideoEncoder *g_videoEncoderArray[MAX_VIDEO_NUMBER];
 void mixer_osdInfoLinkListInit(void)
{
   midOsdInfoInit();
}
static void mixer_osd_creathandle(const st_Osd_Attr *tOsdHandle)
{
    if(NULL == tOsdHandle)
    {
        MIXER_ERR("osdhanle is null\n");
        return;
    }
#if TARGET_CHIP_I6B0
    MI_VENC_CHN s32VencChn_tmp = 0;
#endif

    MI_VENC_CHN s32VencChn = 0;
    const MI_S8* s8String = NULL;
    MI_S32 s32Idx = 0;
    MI_S32 s32HandleNumber = -1;
    MI_RGN_HANDLE *pu32RgnHandle = NULL;
    MI_RGN_HANDLE u32RgnHandle = MI_RGN_HANDLE_NULL;
    MI_SYS_WindowRect_t stRect;
    MI_RGN_ChnPort_t stRgnChnPort;
    ST_MIXER_RGN_WIDGET_ATTR stMixerRgnWidgetAttr;

    s32VencChn = tOsdHandle->channel;
    s32HandleNumber = getOsdChnHandleCount(s32VencChn);
#if TARGET_CHIP_I6B0
        if(checkVencInitOsdFull(s32VencChn,&s32VencChn_tmp) == TRUE)
        {
            MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",s32VencChn,s32VencChn_tmp,s32VencChn_tmp,s32VencChn);
            return;
        }
#endif
    if(-1 == s32HandleNumber)
    {
        printf("%s:%d, ch(%d), getOsdChnHandleCount fail!\n", __func__, __LINE__, s32VencChn);
        return;
    }
    else if(MAX_RGN_NUMBER_PER_CHN <= s32HandleNumber)
    {
        printf("%s:%d video Channel-%d have more then %d Rgn widget!\n", __func__, __LINE__, s32VencChn, s32HandleNumber);
        return;
    }

    u32RgnHandle = getOsdChnHandleNumber(s32VencChn, &s32Idx,E_OSD_WIDGET_TYPE_RECT);
    if((MIXER_RGN_OSD_MAX_NUM > u32RgnHandle) && (MAX_RGN_NUMBER_PER_CHN - 1 <= s32Idx))
    {
        getOsdChnHandleFirstNull(s32VencChn, &s32Idx);
        if((MAX_RGN_NUMBER_PER_CHN < s32Idx) || (0 > s32Idx))
        {
            printf("%s:%d video Channel-%d getOsdChnHandleFirstNull() fail!\n", __func__, __LINE__, s32VencChn);
            return;
        }
    }
    else
    {
        s32Idx++;
    }

    stRect.u16X = (MI_U16)tOsdHandle->point.u16X;
    stRect.u16Y = (MI_U16)tOsdHandle->point.u16Y;
    stRect.u16Width = (MI_U16)tOsdHandle->size.u16Width;
    stRect.u16Height = (MI_U16)tOsdHandle->size.u16Height;
    s8String = (const MI_S8*) tOsdHandle->pstr;

    configOsdRgnChnPort(s32VencChn, &stRgnChnPort);
    memset(&stMixerRgnWidgetAttr, 0x00, sizeof(ST_MIXER_RGN_WIDGET_ATTR));
    configRgnWidgetAttr(s32VencChn, s32Idx, &stRect, &stRgnChnPort, &stMixerRgnWidgetAttr,tOsdHandle->format);
    stMixerRgnWidgetAttr.eRgnpixelFormat = tOsdHandle->format;
    printf("%s:%d Create VencChn=%d s32Idx=%d, canvas:x=%d, y=%d, w=%4d, h=%4d, DevID=%d, ChnID=%d, PortID=%d, fmt=%d\n", __func__, __LINE__,
            s32VencChn, s32Idx, stRect.u16X, stRect.u16Y, stRect.u16Width, stRect.u16Height,
            stRgnChnPort.s32DevId, stRgnChnPort.s32ChnId, stRgnChnPort.s32OutputPortId, stMixerRgnWidgetAttr.eRgnpixelFormat);
    pu32RgnHandle = getOsdHandleAddr(s32VencChn, s32Idx, E_OSD_WIDGET_TYPE_TEXT);
    if(0 != createOsdWidget(pu32RgnHandle, &stMixerRgnWidgetAttr))
    {
		MIXER_ERR("createOsdWidget return fail!\n");
	}
    printf("You create osd handle=%d osd string:%s\n",*pu32RgnHandle,s8String);
    if(' ' != s8String[0])
    {
        osdSizePoint_t osdSizePoint;
        MI_S32 retS32 = updateOsdInfomation(*pu32RgnHandle, (const MI_S8 *)s8String,&osdSizePoint);
        if(E_MI_ERR_FAILED != retS32)
        {
        }
    }
}

static void mixer_destroy_osd_handle(const st_osd_destroyhandle *tDosdhandle)
{
    if(NULL == tDosdhandle)
    {
        MIXER_ERR("tDosdhandle is null\n");
        return;
    }
#if TARGET_CHIP_I6B0
    MI_VENC_CHN s32VencChn_tmp = 0;
#endif
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;
    MI_S32 s32IdxTmp = 0;
    MI_RGN_HANDLE u32RgnHandle = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;

    s32VencChn = tDosdhandle->channel;
#if TARGET_CHIP_I6B0
    if(checkVencInitOsdFull(s32VencChn,&s32VencChn_tmp) == TRUE)
    {
        MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",s32VencChn,s32VencChn_tmp,s32VencChn_tmp,s32VencChn);
        return;
    }
#endif

    u32RgnHandle = getOsdChnHandleNumber(s32VencChn, &s32Idx,E_OSD_WIDGET_TYPE_RECT);
    if((-1 == (MI_S32)s32Idx) && (MI_RGN_HANDLE_NULL == (MI_S32)u32RgnHandle))
    {
        printf("%s:%d getOsdChnHandleNumber(%d) error and return\n", __func__, __LINE__, s32VencChn);
        return;
    }

    if(-1 == (MI_S32)s32Idx)
    {
        u32RgnHandle = calcOsdHandle(s32VencChn, s32Idx, E_OSD_WIDGET_TYPE_RECT);
       destroyOsdWidget(s32VencChn, u32RgnHandle);
    }
    else
    {
        s32IdxTmp = (MI_S32)tDosdhandle->iItem;
        if(s32Idx < s32IdxTmp)
        {
            printf("set the index(=%d) of OSD handle you want to destroy is out of range [-1, %d]\n", s32IdxTmp, s32Idx);
            return;
        }
        if(s32IdxTmp < 0)
        {
          u32RgnHandle = getOsdHandle(s32VencChn, s32IdxTmp, E_OSD_WIDGET_TYPE_RECT);
        }
        else
        {
          u32RgnHandle = getOsdHandle(s32VencChn, s32IdxTmp, E_OSD_WIDGET_TYPE_TEXT);
        }
        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != u32RgnHandle)
        {
            destroyOsdWidget(s32VencChn, u32RgnHandle);
        }
        else
        {
          printf("you chice this osd  handle is no create!\n");
        }
    }
}

static void mixer_osd_movebych(const st_osd_movebych *tHandle)
{
    if(NULL == tHandle)
    {
        MIXER_ERR("tHandle is null\n");
        return;
    }
     MI_S32 s32Ret;
    MI_SYS_WindowRect_t stRect;
    MI_RGN_HANDLE u32RgnHandle = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;

    if(tHandle->channel > (MI_VENC_CHN)(g_videoNumber-1))
    {
        MIXER_ERR("channel out of range. [0, %d]. %d\n", (g_videoNumber-1), tHandle->channel);
        return;
    }

    u32RgnHandle = calcOsdHandle(tHandle->channel,tHandle->index, E_OSD_WIDGET_TYPE_TEXT);

    stRect.u16X = (MI_U16)tHandle->point.u16X;
    stRect.u16Y = (MI_U16)tHandle->point.u16Y;

    stRect.u16Width = 0x00;
    stRect.u16Height = 0x00;

    s32Ret = changeOsdPositionByHandle(u32RgnHandle, &stRect);
    if(MI_SUCCESS == s32Ret)
    {
    }
}

static void mixer_osd_changesizebych(const st_Osd_Attr *tHandle)
{
    if(NULL == tHandle)
    {
        MIXER_ERR("tHandle is null\n");
        return;
    }
    const MI_S8 *s8String = NULL;
    MI_S32 s32VencChn = 0;
    MI_SYS_WindowRect_t stRect;
    MI_RGN_HANDLE u32RgnHandle = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;

    for(s32VencChn = 0; (MI_U32)s32VencChn < g_videoNumber; s32VencChn++)
    {
        MI_RGN_HANDLE u32RgnHandleTmp = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;

        u32RgnHandle = calcOsdHandle(s32VencChn, 0, E_OSD_WIDGET_TYPE_TEXT);
        u32RgnHandleTmp = getOsdChnMaxHandle(s32VencChn);
        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32RgnHandleTmp)
        {
            printf("    video-%d OSD handle: not create any OSD handle\n", s32VencChn);
        }
        else
        {
            if(tHandle->handle <= u32RgnHandleTmp && tHandle->handle >= u32RgnHandle)
            {
                break;
            }
            printf("    video-%d OSD handle: [%d, %d]\n", s32VencChn, u32RgnHandle, u32RgnHandleTmp);
        }
    }

    if(s32VencChn == (MI_S32)g_videoNumber)
    {
        MIXER_ERR("can not fine rgn handle\n");
        return;
    }

    stRect.u16X = (MI_U16)tHandle->point.u16X;
    stRect.u16Y = (MI_U16)tHandle->point.u16Y;
    stRect.u16Width = (MI_U16)tHandle->size.u16Width;
    stRect.u16Height = (MI_U16)tHandle->size.u16Height;

    if((0 != stRect.u16Width) && (0 != stRect.u16Height))
    {
        s8String = (const MI_S8*)tHandle->pstr;
    }

    changeOsdPositionAndSizeByHandle(tHandle->handle, &stRect);

    if((0 != stRect.u16Width) && (0 != stRect.u16Height))
    {
       MI_S32 s32Ret = updateOsdInfomation(tHandle->handle, (const MI_S8 *)s8String,NULL);
       if(E_MI_ERR_FAILED != s32Ret)
       {
       }
    }
}

static void mixer_osd_updateinfo(const st_Osd_Info* tHandle)
{
    if(NULL == tHandle)
    {
        MIXER_ERR("tHandle is null\n");
        return;
    }
#if TARGET_CHIP_I6B0
        MI_VENC_CHN s32VencChn_tmp = 0;
#endif
    MI_VENC_CHN s32VencChn = tHandle->channel;
    const MI_S8 *s8String = NULL;
    MI_S32 s32Idx = 0;
    MI_U32 u32HandleNumber = 0;
    MI_RGN_HANDLE *pu32RgnHandle = NULL;

    if((MI_U32)s32VencChn >= g_videoNumber)
    {
        MIXER_ERR("venc ch is out of range.[0, %d]. %d\n", g_videoNumber -1 , s32VencChn);
        return;
    }
#if TARGET_CHIP_I6B0
    if(checkVencInitOsdFull(s32VencChn,&s32VencChn_tmp) == TRUE)
    {
        MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",s32VencChn,s32VencChn_tmp,s32VencChn_tmp,s32VencChn);
        return;
    }
#endif

    u32HandleNumber = getOsdChnHandleNumber(s32VencChn, &s32Idx,E_OSD_WIDGET_TYPE_TEXT);
    if((-1 == (MI_S32)u32HandleNumber) && (-1 == (MI_S32)s32Idx))
    {
        printf("%s:%d the set video stream %d OSD handle number is out of range(%d) and return\n",
                __func__, __LINE__, s32VencChn, u32HandleNumber);
        return;
    }

    if(tHandle->oIndex > (MI_U32)s32Idx)
    {
        MIXER_ERR("index is out of range.[0, %d]. %d\n", s32Idx , tHandle->oIndex);
        return;
    }

    s8String = (const MI_S8*)tHandle->pstr;
    if(NULL == s8String)
    {
        MIXER_ERR("s8String is null\n");
        return;

    }
    pu32RgnHandle = getOsdHandleAddr(s32VencChn, (MI_U32)tHandle->oIndex, E_OSD_WIDGET_TYPE_TEXT);

    if(' ' != s8String[0])
    {
        MI_S32 s32Ret = updateOsdInfomation(*pu32RgnHandle, (const MI_S8 *)s8String,NULL);
        if(E_MI_ERR_FAILED != s32Ret)
        {
        }
    }
    else
    {
      MIXER_WARN("s8String is null\n");
    }
}

static void mixer_osd_alpha(const st_osd_alpha *tHandle)
{
    if(NULL == tHandle)
    {
        MIXER_ERR("tHandle is null\n");
        return;
    }
#if TARGET_CHIP_I6B0
    MI_VENC_CHN s32VencChn_tmp = 0;
#endif
    MI_VENC_CHN s32VencChn = tHandle->channel;
    MI_S32 s32Idx = 0;
    MI_U32 u32HandleNumber = 0;
    MI_S32 s32Ret = -1;
    MI_RGN_ChnPortParam_t stRgnChnPortAttr;
    if((MI_U32)s32VencChn >= g_videoNumber)
    {
        MIXER_ERR("venc ch is out of range.[0, %d]. %d\n", g_videoNumber -1 , s32VencChn);
        return;
    }
#if TARGET_CHIP_I6B0
    if(checkVencInitOsdFull(s32VencChn,&s32VencChn_tmp) == TRUE)
    {
        MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",s32VencChn,s32VencChn_tmp,s32VencChn_tmp,s32VencChn);
        return;
    }
#endif
    u32HandleNumber = getOsdChnHandleNumber(s32VencChn, &s32Idx,E_OSD_WIDGET_TYPE_TEXT);
    if((-1 == (MI_S32)u32HandleNumber) && (-1 == (MI_S32)s32Idx))
    {
        printf("%s:%d the set video stream %d OSD handle number is out of range(%d) and return\n",
                __func__, __LINE__, s32VencChn, u32HandleNumber);
        return;
    }

    if(tHandle->oIndex > (MI_U32)s32Idx)
    {
        MIXER_ERR("index is out of range.[0, %d]. %d\n", s32Idx , tHandle->oIndex);
        return;
    }
    memset(&stRgnChnPortAttr, 0x00, sizeof(MI_RGN_ChnPortParam_t));
    s32Ret = changeOsdAlpha(s32VencChn,tHandle->oIndex,(MI_U8)tHandle->alpha);
    if(MI_SUCCESS == s32Ret)
    {
        printf("WO, good! you change Alpha is successful which belong to venc[%d]\n",s32VencChn);
    }
    else
    {
        printf("Bad! You set opt is failed!\n");
    }
}


static void mixer_osd_testformat(const st_osd_format *tHandle)
{
    if(NULL == tHandle)
    {
        MIXER_ERR("tHandle is null\n");
        return;
    }

    MI_S32 s32Mode = 0;
    MI_U32 start_X = 0;
    MI_U32 start_Y = 0;
    MI_U8 j = 0;
    MI_U8 i = 0;
//    MI_U32  which = 0;
    static MI_S32 s32Format = -1;
    MI_S32 s32Ret = -1;
    MI_S32 closeOsdScope = -1;
    MI_S32  Number_osD = -1;
    static MI_U32  testOsdCount = 0;
    MI_U8  testOsdFlag[MAX_RGN_NUMBER_PER_CHN] = {0};
    MI_U32 minWidth = g_videoEncoderArray[0]->m_width;
    MI_U32 minHeight = g_videoEncoderArray[0]->m_height;
    static MI_S8 NO_OsdArry[MAX_RGN_NUMBER_PER_CHN] = {-1,-1,-1,-1,-1,-1,-1,-1};

    s32Mode = tHandle->enable;

    if(0 == s32Mode)
    {
        if(0 >= testOsdCount)
        {
            printf("not have \"i2,i4 or bitmap format osd\" handle to close!\n");
            testOsdCount = 0;
        }
        else if(0 <= s32Format)
        {
            printf("please chose close osd scope[0->all osd: 1->all bitmap:2->NO. time osd:3->which channel NO.time osd]\n");
            closeOsdScope = tHandle->m1.scope;

            if((0 == closeOsdScope) &&(0 < testOsdCount))
            {
                printf("will close all osd, which have same format!\n");
                s32Ret = uninitOsdFull();
                if((MI_SUCCESS != s32Ret) || (MI_RGN_OK != s32Ret))
                {
                    printf("please check your close scope  errno[%d]\n",s32Ret);
                }
                else
                {
                    testOsdCount = 0;
                    printf("Close all osd successful!\n");
                }
            }
            else if((1 == closeOsdScope)&&(0 < testOsdCount))
            {
                printf("will close all osd, which have same format! g_videoNumber[%d]\n",g_videoNumber);
                for(i = 0; i < g_videoNumber; i++)
                {
                    for(j = 0; j < MAX_RGN_NUMBER_PER_CHN; j++)
                    {
                        s32Ret = -1;
                        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32BitmapWidget[i][j])
                        {
                            s32Ret = destoryOsdHandle(g_au32BitmapWidget[i][j],i,j,E_OSD_WIDGET_TYPE_BITMAP);
                        }
                        else if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI2Widget[i][j])
                        {
                            s32Ret = destoryOsdHandle(g_au32OsdI2Widget[i][j],i,j,E_OSD_WIDGET_TYPE_BITMAP);
                        }
                        else  if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI4Widget[i][j])
                        {
                            s32Ret = destoryOsdHandle(g_au32OsdI4Widget[i][j],i,j,E_OSD_WIDGET_TYPE_BITMAP);
                        }
                        if(MI_SUCCESS == s32Ret)
                        {
                            if(testOsdFlag[j] == 0)
                            {
                             testOsdCount--;
                                testOsdFlag[j] = 1;
                            }
                        }
                    }
                }
                if(0 == testOsdCount)
                {
                    printf("WO, good! all bitmap OSD is closed!\n");
                }
                else
                {
                    printf("channel[%d] NO.%d osd  closed is failed!\n",i,j);
                }
            }
            else if((2 == closeOsdScope)&&(0 < testOsdCount))
            {
                Number_osD = (MI_S32)tHandle->m2.index;
                if(Number_osD >= (MI_S32)testOsdCount || Number_osD >= (MI_S32)(sizeof(NO_OsdArry)/sizeof(NO_OsdArry[0])))
                {
                    MIXER_ERR("Number_osd out of range. %d -%d - %d\n", Number_osD, (testOsdCount), (sizeof(NO_OsdArry)/sizeof(NO_OsdArry[0])));
                    return;
                }

                if(0 <= NO_OsdArry[Number_osD])
                {
                    for(i = 0; (i < g_videoNumber)&&(0 <= NO_OsdArry[Number_osD]); i++)
                    {
                        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32BitmapWidget[i][Number_osD])
                        {
                            s32Ret = destoryOsdHandle(g_au32BitmapWidget[i][Number_osD],i,Number_osD,E_OSD_WIDGET_TYPE_BITMAP);
                        }
                        else if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI2Widget[i][Number_osD])
                        {
                            s32Ret = destoryOsdHandle(g_au32OsdI2Widget[i][Number_osD],i,Number_osD,E_OSD_WIDGET_TYPE_BITMAP);
                        }
                        else  if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI4Widget[i][Number_osD])
                        {
                            s32Ret = destoryOsdHandle(g_au32OsdI4Widget[i][Number_osD],i,Number_osD,E_OSD_WIDGET_TYPE_BITMAP);
                        }
                    }
                    if(MI_SUCCESS == s32Ret)
                    {
                        testOsdCount--;
                         printf("WO, good! NO.%d bitmap OSD is closed!\n",Number_osD);
                    }
                    else
                    {
                        printf("OW, bad! NO.%d bitmap OSD is closed failed!\n",Number_osD);
                    }
                }
                else
                {
                    printf("NOTE: This time osd bitmap have closed or no exist!\n");
                }
                NO_OsdArry[Number_osD] = -1;
            }
            else if(3 == closeOsdScope)
            {
/*                which = tHandle->m2.ch;
                Number_osD = (MI_S32)tHandle->m3.index;

                if(Number_osD >= (testOsdCount) || Number_osD >= (MI_S32)(sizeof(NO_OsdArry)/sizeof(NO_OsdArry[0])))
                {
                    MIXER_ERR("Number_osd out of range. %d -%d - %d\n", Number_osD, (testOsdCount), (sizeof(NO_OsdArry)/sizeof(NO_OsdArry[0])));
                    return;
                }

                if((0 <= Number_osD) && (which <= g_videoNumber) && (0 <= NO_OsdArry[Number_osD]) &&(0 <= which))
                {
                    printf("You have chose Format[%d]  channel[%d] NO.%d osd!\n",s32Format,which,Number_osD);
                    if((0 == s32Format)&&((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL !=g_au32BitmapWidget[which][Number_osD]))
                    {
                        s32Ret = destoryOsdHandle(g_au32BitmapWidget[which][Number_osD],which,Number_osD,E_OSD_WIDGET_TYPE_BITMAP);
                        for(i = 0; i < g_videoNumber; i++)
                        {
                            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32BitmapWidget[i][Number_osD])
                            {
                                break;
                            }
                        }
                        if(i >= g_videoNumber)
                        {
                            testOsdCount--;//NO. time osd all channel  have been closed!
                        }
                    }
                    else if((1 == s32Format)&&((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL !=g_au32OsdI2Widget[which][Number_osD]))
                    {
                        s32Ret =  destoryOsdHandle(g_au32OsdI2Widget[which][Number_osD],which,Number_osD,E_OSD_WIDGET_TYPE_BITMAP);
                        for(i = 0; i < g_videoNumber; i++)
                        {
                            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI2Widget[i][Number_osD])
                            {
                                break;
                            }
                        }
                        if(i >= g_videoNumber)
                        {
                            testOsdCount--;//NO. time osd all channel  have been closed!
                        }
                    }
                    else if((2 == s32Format)&&((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL !=g_au32OsdI4Widget[which][Number_osD]))
                    {
                        s32Ret = destoryOsdHandle(g_au32OsdI4Widget[which][Number_osD],which,Number_osD,E_OSD_WIDGET_TYPE_BITMAP);
                        for(i = 0; i < g_videoNumber; i++)
                        {
                            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI4Widget[i][Number_osD])
                            {
                                break;
                            }
                        }
                        if(i >= g_videoNumber)
                        {
                            testOsdCount--;//NO. time osd all channel  have been closed!
                        }
                    }
                    else
                    {
                        printf("Don't support this formt[%d]\n",s32Format);
                    }
                    if(MI_SUCCESS == s32Ret)
                    {
                        printf("WO, good! channel[%d] NO.%d bitmap OSD is closed!\n",which,Number_osD);
                    }
                    else
                    {
                        printf("OW, bad! NO.%d bitmap OSD is closed failed!\n",Number_osD);
                    }
                }
                else
                {
                    printf("please check input param!channel:[0-%d] NO. time osd [0-%d] maybe have been closed!\n",g_videoNumber,testOsdCount);
                }    */
                printf("delete which channel NO.time osd! is not support\n");
            }
        }
        else
        {
            printf("Don't support this formt[%d] or this osd handle no create!\n",s32Format);
        }
    }
    else if(1 == s32Mode)
    {
        s32Format = tHandle->m1.format;

        for(i = 1; i < g_videoNumber; i++)
        {
            minHeight = (minHeight < g_videoEncoderArray[i]->m_height ? minHeight:g_videoEncoderArray[i]->m_height);
            minWidth  = (minWidth < g_videoEncoderArray[i]->m_width ? minWidth:g_videoEncoderArray[i]->m_width);
        }
        minHeight = (s32Format == 0 ? (minHeight-66):(minHeight-200));
        minWidth =  (s32Format == 0 ? (minWidth-514):(minWidth-200));

        start_X = tHandle->m2.x;
        if(start_X >= minWidth)
            start_X = minWidth;

        start_Y = tHandle->m3.y;
        if(start_Y >= minHeight)
            start_Y = minHeight;

        if((0 > s32Format) || (3 < s32Format) || (start_X > (g_videoEncoderArray[0]->m_width/2))||(start_Y > (g_videoEncoderArray[0]->m_height/2)))
        {
            printf("set OSD format param fail!\n");
            return;
        }
        else
        {
            s32Ret = testOsdFormatBitmap(s32Format,&testOsdCount,start_X,start_Y);
            if(MI_RGN_OK == s32Ret)
            {
              NO_OsdArry[testOsdCount] = testOsdCount;
              printf("WO, good! You create NO.%d OSD handle is successful!\n",testOsdCount);
            }
            else
            {
                printf("Oh, Bad! You create NO.%d OSD handle is falied!\n",testOsdCount);
            }
        }
        testOsdCount++;
    }
}


static void mixer_osd_createcover(const st_cover_attr *tHandle)
{
    if(NULL == tHandle)
    {
        MIXER_ERR("tHandle is null\n");
        return;
    }
#if TARGET_CHIP_I6B0
    MI_VENC_CHN s32VencChn_tmp = 0;
#endif
    MI_VENC_CHN s32VencChn = tHandle->channel;
    MI_S32 s32Idx = -1;
    MI_S32 s32CoverColor = -1;
    MI_S32 s32HandleNumber = -1;
    MI_S32 s32Ret = -1;
    MI_RGN_HANDLE *pu32RgnHandle = NULL;
    MI_RGN_HANDLE u32RgnHandle = MI_RGN_HANDLE_NULL;
    MI_SYS_WindowRect_t stRect;
    MI_RGN_ChnPort_t stRgnChnPort;
    ST_MIXER_RGN_WIDGET_ATTR stMixerRgnWidgetAttr;

    if((MI_U32)s32VencChn >= g_videoNumber)
    {
        MIXER_ERR("venc ch is out of range.[0, %d]. %d\n", g_videoNumber -1 , s32VencChn);
        return;
    }
#if TARGET_CHIP_I6B0
    if(checkVencInitOsdFull(s32VencChn,&s32VencChn_tmp) == TRUE)
    {
        MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",s32VencChn,s32VencChn_tmp,s32VencChn_tmp,s32VencChn);
        return;
    }
#endif

    s32HandleNumber = getCoverHandleCount(s32VencChn);
    if(-1 == s32HandleNumber)
    {
        printf("%s:%d getCoverHandleCount(%d) fail!\n", __func__, __LINE__, s32VencChn);
        return;
    }
    else if(MAX_COVER_NUMBER_PER_CHN <= s32HandleNumber)
    {
        printf("%s:%d video Channel-%d have more then %d Cover widget!\n", __func__, __LINE__, s32VencChn, s32HandleNumber);
        return;
    }

    u32RgnHandle = getOsdChnHandleNumber(s32VencChn, &s32Idx,E_OSD_WIDGET_TYPE_COVER);
    if((MIXER_RGN_OSD_MAX_NUM > u32RgnHandle) && (MAX_COVER_NUMBER_PER_CHN  > s32Idx))
    {
        getCoverChnHandleFirstNull(s32VencChn, &s32Idx);
        if((MAX_COVER_NUMBER_PER_CHN  < s32Idx) || (0 > s32Idx))
        {
            printf("%s:%d video Channel-%d getCoverChnHandleFirstNull() fail!\n", __func__, __LINE__, s32VencChn);
            return;
        }
    }
    else
    {
        s32Idx++;
    }

    stRect.u16X = (MI_U16)tHandle->point.u16X;

    stRect.u16Y = (MI_U16)tHandle->point.u16Y;

    stRect.u16Width = (MI_U16)tHandle->size.u16Width;
    stRect.u16Height = (MI_U16)tHandle->size.u16Height;



    configOsdRgnChnPort(s32VencChn, &stRgnChnPort);

    memset(&stMixerRgnWidgetAttr, 0x00, sizeof(ST_MIXER_RGN_WIDGET_ATTR));
    configRgnWidgetAttr(s32VencChn, s32Idx, &stRect, &stRgnChnPort, &stMixerRgnWidgetAttr,E_MI_RGN_PIXEL_FORMAT_I4 );
    stMixerRgnWidgetAttr.eOsdWidgetType = E_OSD_WIDGET_TYPE_COVER;
    s32CoverColor = tHandle->colorindex;
    switch(s32CoverColor)
    {
        case 0:  stMixerRgnWidgetAttr.u32Color = 0xff801080; break;   //black
        case 1:  stMixerRgnWidgetAttr.u32Color = 0xff6E29F0; break;   //blue
        case 2:  stMixerRgnWidgetAttr.u32Color = 0xffF0525B; break;   //red
        case 3:  stMixerRgnWidgetAttr.u32Color = 0xff239137; break;   //green
        default: stMixerRgnWidgetAttr.u32Color = 0xff801080; break;
    }

    pu32RgnHandle = getOsdHandleAddr(s32VencChn, s32Idx, E_OSD_WIDGET_TYPE_COVER);
    s32Ret = createOsdWidget(pu32RgnHandle, &stMixerRgnWidgetAttr);
    if(MI_SUCCESS == s32Ret)
    {
        printf("Create osd Cover is OK! handle[%d] s32Idx[%d]\n",*pu32RgnHandle,s32Idx);
    }
    else
    {
        printf("Create osd Cover is error! handle[%d]s32Idx[%d]\n",*pu32RgnHandle,s32Idx);
    }



}

static void mixer_osd_destroycover(const osdChnIndex_t *tHandle)
{
    if(NULL == tHandle)
    {
    MIXER_ERR("tHandle is null\n");
    return;
    }
#if TARGET_CHIP_I6B0
        MI_VENC_CHN s32VencChn_tmp = 0;
#endif
    MI_VENC_CHN s32VencChn = tHandle->channel;
    MI_S32 s32Idx = 0;
    MI_S32 s32IdxTmp = 0;
    MI_U32 u32HandleNumber = 0;
    MI_RGN_HANDLE u32RgnHandle = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;

    if((MI_U32)s32VencChn >= g_videoNumber)
    {
        MIXER_ERR("venc ch is out of range.[0, %d]. %d\n", g_videoNumber -1 , s32VencChn);
        return;
    }
#if TARGET_CHIP_I6B0
    if(checkVencInitOsdFull(s32VencChn,&s32VencChn_tmp) == TRUE)
    {
        MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",s32VencChn,s32VencChn_tmp,s32VencChn_tmp,s32VencChn);
        return;
    }
#endif
    u32RgnHandle = getCoverChnHandleNumber(s32VencChn, &s32Idx);
    if((-1 == (MI_S32)u32RgnHandle) || (MI_RGN_MAX_HANDLE <= (MI_S32)u32RgnHandle))
    {
        printf("%s:%d the set video stream %d Cover handle number is out of range(%d) and return\n",
                __func__, __LINE__, s32VencChn, u32HandleNumber);
        return;
    }

    s32IdxTmp = tHandle->index;

    if(s32Idx < s32IdxTmp)
    {
        printf("set the index(=%d) of Cover handle you want to destroy is out of range [0, %d]\n", s32IdxTmp, s32Idx);
        return;
    }

    u32RgnHandle = getOsdHandle(s32VencChn, s32IdxTmp, E_OSD_WIDGET_TYPE_COVER);
    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != u32RgnHandle)
    {
        destroyOsdWidget(s32VencChn, u32RgnHandle);
    }
}

static void mixer_cover_movebyhandle(const st_cover_handle *tHandle)
{
    if(NULL == tHandle)
    {
        MIXER_ERR("tHandle is null\n");
        return;
    }

    MI_S32 s32VencChn = 0;
    MI_SYS_WindowRect_t stRect;
    MI_RGN_HANDLE u32RgnHandle = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;

    for(s32VencChn = 0; (MI_U32)s32VencChn < g_videoNumber; s32VencChn++)
    {
        MI_S32 s32Idx = 0;
        MI_RGN_HANDLE u32RgnHandleTmp = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;

        u32RgnHandle = calcOsdHandle(s32VencChn, 0, E_OSD_WIDGET_TYPE_COVER);
        u32RgnHandleTmp = getCoverChnHandleNumber(s32VencChn, &s32Idx);
        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32RgnHandleTmp)
        {
            printf("    video-%d Cover handle: not create any Cover handle\n", s32VencChn);
        }
        else
        {
            if(tHandle->handle <= u32RgnHandleTmp && tHandle->handle >= u32RgnHandle)
            {
                break;
            }
            printf("    video-%d Cover handle: [%d, %d]\n", s32VencChn, u32RgnHandle, u32RgnHandleTmp);
        }
    }

    if(s32VencChn == (MI_S32)g_videoNumber)
    {
        MIXER_ERR("can not fine rgn handle\n");
        return;
    }

    stRect.u16X = (MI_U16)tHandle->point.u16X;
    stRect.u16Y = (MI_U16)tHandle->point.u16Y;

    stRect.u16Width  = 0x00;
    stRect.u16Height = 0x00;

    MI_S32 s32Ret = changeCoverPositionByHandle(tHandle->handle, &stRect, 0xff801080); //black
    if(s32Ret)
    {

    }
}

static void mixer_cover_changesizebyhandle(const st_cover_handle *tHandle)
{
    if(NULL == tHandle)
    {
        MIXER_ERR("tHandle is null\n");
        return;
    }

    MI_S32 s32VencChn = 0;
    MI_SYS_WindowRect_t stRect;
    MI_S32 s32CoverColor = -1;
    MI_RGN_HANDLE u32RgnHandle = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;

    for(s32VencChn = 0; (MI_U32)s32VencChn < g_videoNumber; s32VencChn++)
    {
        MI_S32 s32Idx = 0;
        MI_RGN_HANDLE u32RgnHandleTmp = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;

        u32RgnHandle = calcOsdHandle(s32VencChn, 0, E_OSD_WIDGET_TYPE_COVER);
        u32RgnHandleTmp = getCoverChnHandleNumber(s32VencChn, &s32Idx);
        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32RgnHandleTmp)
        {
            printf("    video-%d Cover handle: not create any Cover handle\n", s32VencChn);
        }
        else
        {
            printf("    video-%d Cover handle: [%d, %d]\n", s32VencChn, u32RgnHandle, u32RgnHandleTmp);
            if(tHandle->handle <= u32RgnHandleTmp && tHandle->handle >= u32RgnHandle)
            {
                break;
            }

        }
    }
    if(s32VencChn == (MI_S32)g_videoNumber)
    {
        MIXER_ERR("can not find rgn handle\n");
        return;
    }

    stRect.u16X = (MI_U16)tHandle->point.u16X;
    stRect.u16Y = (MI_U16)tHandle->point.u16Y;
    stRect.u16Width = (MI_U16)tHandle->size.u16Width;
    stRect.u16Height = (MI_U16)tHandle->size.u16Width;

    s32CoverColor = tHandle->colorindex;

    switch(s32CoverColor)
    {
        case 0:  changeCoverPositionAndSizeByHandle(tHandle->handle, &stRect, 0xff801080); break;   //black
        case 1:  changeCoverPositionAndSizeByHandle(tHandle->handle, &stRect, 0xff6E29F0); break;   //blue
        case 2:  changeCoverPositionAndSizeByHandle(tHandle->handle, &stRect, 0xffF0525B); break;   //red
        case 3:  changeCoverPositionAndSizeByHandle(tHandle->handle, &stRect, 0xff239137); break;   //green
        default: changeCoverPositionAndSizeByHandle(tHandle->handle, &stRect, 0xff801080); break;
    }
}
static void printOsdInfo(osd_Link_List_t *osdInfo,PRINT_e way)
{
    midFindOsdInfoPrint(osdInfo,way);
}
void setOsdParam(const MI_S8 *buf[], MI_U8 length)
{
    MI_S32 chosen;
    MI_U8 offset = 0x0;
    MI_U8 j = 0x0;
#if TARGET_CHIP_I6B0
    MI_VENC_CHN s32VencChn_tmp = 0;
#endif
    if(NULL == buf ||length <= offset)
    {
        MIXER_ERR("wrong param\n");
        return;
    }

    for(j = 0x0; j <  length; j++)
    {
        if(NULL == buf[j])
        {
            MIXER_ERR("wrong param. len:%d\n", length);
            return;
        }
    }

    chosen = atoi((char *)buf[offset]);
    offset += 1;

    switch(chosen)
    {
        case 0: //create OSD handle
            {
                st_Osd_Attr tmp;
                if(length <= (offset+5))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tmp.channel = (MI_VENC_CHN)atoi((char *)buf[offset]);
                offset += 1;
                tmp.point.u16X = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.point.u16Y = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.size.u16Width = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.size.u16Height = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.pstr = ((const char *)buf[offset]);
                offset += 1;
                if(offset == length-1)
                {
                    tmp.format= (MI_RGN_PixelFormat_e)atoi((char *)buf[offset]);
                    offset += 1;
                }else
                {
                    tmp.format= (MI_RGN_PixelFormat_e)E_MI_RGN_PIXEL_FORMAT_I4;
                    offset += 1;
                }
                if((MI_S32)g_videoNumber <= tmp.channel)
                {
                  MIXER_ERR("your venc channel[%d] is out of range[0-%d]\n",tmp.channel,g_videoNumber-1);
                  break;
                }
#if TARGET_CHIP_I6B0
                if(checkVencInitOsdFull(tmp.channel,&s32VencChn_tmp) == TRUE)
                {
                    MIXER_DBG("[I6B01]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",tmp.channel,s32VencChn_tmp,s32VencChn_tmp,tmp.channel);
                    break;
                }
#endif
                mixer_osd_creathandle(&tmp);
                break;
            }

        case 1: //destroy OSD handle
            {
                st_osd_destroyhandle tDosdhandle;
                if(length <= (offset+1))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tDosdhandle.channel = (MI_VENC_CHN)atoi((char *)buf[offset]);
                offset += 1;
                tDosdhandle.iItem = (MI_U8)atoi((char *)buf[offset]);
                offset += 1;
#if TARGET_CHIP_I6B0
                if(checkVencInitOsdFull(tDosdhandle.channel,&s32VencChn_tmp) == TRUE)
                {
                    MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",tDosdhandle.channel,s32VencChn_tmp,s32VencChn_tmp,tDosdhandle.channel);
                    break;
                }
#endif
                mixer_destroy_osd_handle(&tDosdhandle);
                break;
            }

        case 2: //move OSD position by  channel
            {
                st_osd_movebych thandle;
                if(length <= (offset+2))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                thandle.channel = (MI_VENC_CHN)atoi((char *)buf[offset]);
                offset += 1;
                thandle.index = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                thandle.point.u16X = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                thandle.point.u16Y = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
#if TARGET_CHIP_I6B0
                if(checkVencInitOsdFull(thandle.channel,&s32VencChn_tmp) == TRUE)
                {
                    MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",thandle.channel,s32VencChn_tmp,s32VencChn_tmp,thandle.channel);
                    break;
                }
#endif
                mixer_osd_movebych(&thandle);
                break;
            }

        case 3: //change OSD size by OSD handle
            {
                st_Osd_Attr tmp;
                if(length <= (offset+5))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tmp.handle = (MI_RGN_HANDLE)atoi((char *)buf[offset]);
                offset += 1;
                tmp.point.u16X = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.point.u16Y = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.size.u16Width = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.size.u16Height = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.pstr = ((const char *)buf[offset]);
                offset += 1;

                mixer_osd_changesizebych(&tmp);
                break;
            }

        case 4: //update OSD Info
            {
                st_Osd_Info tmp;
                if(length <= (offset+2))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tmp.channel = (MI_VENC_CHN)atoi((char *)buf[offset]);
                offset += 1;
                tmp.oIndex = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.pstr = ((const char *)buf[offset]);
                offset += 1;
#if TARGET_CHIP_I6B0
                if(checkVencInitOsdFull(tmp.channel,&s32VencChn_tmp) == TRUE)
                {
                    MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",tmp.channel,s32VencChn_tmp,s32VencChn_tmp,tmp.channel);
                    break;
                }
#endif
                mixer_osd_updateinfo(&tmp);

                break;
            }

        case 5: //adjust OSD Alpha
            {
                st_osd_alpha tmp;
                if(length <= (offset+2))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tmp.channel = (MI_VENC_CHN)atoi((char *)buf[offset]);
                offset += 1;
                tmp.oIndex = (MI_U8)atoi((char *)buf[offset]);
                offset += 1;
                tmp.alpha = ( MI_U8 )atoi((char *)buf[offset]);
                offset += 1;
#if TARGET_CHIP_I6B0
                if(checkVencInitOsdFull(tmp.channel,&s32VencChn_tmp) == TRUE)
                {
                    MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",tmp.channel,s32VencChn_tmp,s32VencChn_tmp,tmp.channel);
                    break;
                }
#endif
                mixer_osd_alpha(&tmp);

                break;
            }

        case 6: //test OSD format
            {
                st_osd_format tmp;

                memset(&tmp, 0x0, sizeof(tmp));

                if(length <= (offset))
                {
                    MIXER_ERR("len[%d] < offset[%d] wrong param\n",length,offset);
                    return;
                }
                tmp.enable = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;

                if(TRUE == tmp.enable)
                {
                    if(length > (offset+2))
                    {
                        tmp.m1.format = (MI_BOOL)atoi((char *)buf[offset]);
                        offset += 1;

                        tmp.m2.x = (MI_U16)atoi((char *)buf[offset]);
                        offset += 1;

                        tmp.m3.y = (MI_U16)atoi((char *)buf[offset]);
                    }
                    else
                    {
                        MIXER_ERR("length[%d] <= (offset+2)[%d] wrong param\n",length,offset+2);
                        return;
                    }
                }
                else
                {
                    if(length <= (offset))
                    {
                        MIXER_ERR("wrong param\n");
                        return;
                    }

                    tmp.m1.scope = (MI_U16)atoi((char *)buf[offset]);
                    offset += 1;

                    if(2 == tmp.m1.scope)
                    {
                        if(length <= (offset))
                        {
                            MIXER_ERR("wrong param\n");
                            return;
                        }

                        tmp.m2.index = (MI_U16)atoi((char *)buf[offset]);
                        offset += 1;
                    }
                    else if(3 == tmp.m1.scope)
                    {
                        if(length <= (offset+1))
                        {
                            MIXER_ERR("wrong param\n");
                            return;
                        }

                        tmp.m2.ch = (MI_U16)atoi((char *)buf[offset]);
                        offset += 1;

                        tmp.m3.index = (MI_U16)atoi((char *)buf[offset]);
                        offset += 1;
                    }
                }

                mixer_osd_testformat(&tmp);

                break;
            }
        case 7: //create Cover handle
            {
                st_cover_attr tmp;
                if(length <= (offset+5))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tmp.channel = (MI_VENC_CHN)atoi((char *)buf[offset]);
                offset += 1;
                tmp.point.u16X = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.point.u16Y = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.size.u16Width = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.size.u16Height = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.colorindex = ( MI_U8)atoi((char *)buf[offset]);
                offset += 1;
#if TARGET_CHIP_I6B0
                if(checkVencInitOsdFull(tmp.channel,&s32VencChn_tmp) == TRUE)
                {
                    MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",tmp.channel,s32VencChn_tmp,s32VencChn_tmp,tmp.channel);
                    break;
                }
#endif
                mixer_osd_createcover(&tmp);

                break;
            }

        case 8: //destroy Cover handle
            {
                osdChnIndex_t tmp;

                if(length <= (offset+1))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tmp.channel = (MI_VENC_CHN)atoi((char *)buf[offset]);
                offset += 1;
                tmp.index = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
#if TARGET_CHIP_I6B0
                if(checkVencInitOsdFull(tmp.channel,&s32VencChn_tmp) == TRUE)
                {
                    MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",tmp.channel,s32VencChn_tmp,s32VencChn_tmp,tmp.channel);
                    break;
                }
#endif
                mixer_osd_destroycover(&tmp);

                break;
            }

        case 9: //move Cover position by Cover handle
            {
                 st_cover_handle tmp;
                 if(length <= (offset+2))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tmp.handle = (MI_RGN_HANDLE)atoi((char *)buf[offset]);
                offset += 1;
                tmp.point.u16X = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.point.u16Y = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;

                mixer_cover_movebyhandle(&tmp);

                break;
            }

        case 10: //change Cover size by Cover handle
            {
                st_cover_handle tmp;
                 if(length <= (offset+5))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tmp.handle = (MI_RGN_HANDLE)atoi((char *)buf[offset]);
                offset += 1;
                tmp.point.u16X = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.point.u16Y = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.size.u16Width = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.size.u16Height = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
                tmp.colorindex = ( MI_U8)atoi((char *)buf[offset]);
                offset += 1;

                mixer_cover_changesizebyhandle(&tmp);
                break;
            }
        case 11://print all osd info
            {
              osd_Link_List_t osdInfoNode;
              memset(&osdInfoNode.osdInfo,0,sizeof(osdInfo_t));
              osdInfoNode.osdInfo.osdType = E_OSD_WIDGET_TYPE_MAX;
              printf("osdInfo:\n");
              printOsdInfo(&osdInfoNode,BY_TYPE);
              break;
            }
        default:
            printf("Set \"0\" create OSD handle\n");
            printf("Set \"1\" destroy OSD handle\n");
            printf("Set \"2\" move OSD position by video channel\n");
            printf("Set \"3\" change OSD size by video channel\n");
            printf("Set \"4\" update OSD info\n");
            printf("Set \"5\" adjust OSD Alpha\n");
            printf("Set \"6\" tese OSD format\n");
            printf("Set \"7\" create Cover handle\n");
            printf("Set \"8\" destroy Cover handle\n");
            printf("Set \"9\" move Cover position by cover handle\n");
            printf("Set \"10\" change Cover size by cover handle\n");
            printf("take care: if destroy OSD/Cover handle, will cause move/change OSD/COVER handle exception!\n");
            break;
    }
}

int osd_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen)
{
    switch(id)
    {
        case CMD_OSD_PRE_INIT:
            preInitOsd();
            break;
        case CMD_OSD_OPEN:
            if((NULL != param) && (0 < paramLen))
            {
                initOsdModule((char*)param, paramLen);
            }
            break;

        case CMD_OSD_CLOSE:
            uninitOsdModule();
            break;

        case CMD_OSD_OPEN_DISPLAY_VIDEO_INFO:
            initOsdVideoInfo();
            break;

        case CMD_OSD_CLOSE_DISPLAY_VIDEO_INFO:
            uninitOsdVideoInfo();
            break;

        case CMD_OSD_SET_OD:
        case CMD_OSD_GET_OD:
            break;

        case CMD_OSD_FLICKER:
            memcpy(&g_s32OsdFlicker, param, sizeof(g_s32OsdFlicker));
            break;

        case CMD_OSD_IE_OPEN:
            initOsdIeModule((char *)param, paramLen);
            break;

        case CMD_OSD_IE_CLOSE:
            uninitOsdIeModule();
            break;

        case CMD_OSD_MASK_OPEN:
            {
                MI_S32 OsdParam[2];
                memcpy(&OsdParam, param, paramLen);

                if((OsdParam[0] < 0) || (OsdParam[0] > (MI_S32)g_videoNumber))
                {
                    printf("%s:%d The input video numer(%d) is error!\n", __func__, __LINE__, OsdParam[0]);
                    break;
                }

                if((OsdParam[1] < 0) || (OsdParam[1] > MAX_COVER_NUMBER_PER_CHN))
                {
                    printf("%s:%d Set video Mask numer(%d) is error!\n", __func__, __LINE__, OsdParam[1]);
                    break;
                }

                initOsdMask(OsdParam[0], OsdParam[1]);
            }
            break;

        case CMD_OSD_MASK_CLOSE:
            for(MI_U32 i = 0; i < g_videoNumber; i++)
            {
                uninitOsdMask(i, 0);
            }
            break;

        case CMD_OSD_COLOR_INVERSE_OPEN:
            enableOsdColorInverse();
            break;

        case CMD_OSD_COLOR_INVERSE_CLOSE:
            disableOsdColorInverse();
            break;

        case CMD_OSD_FULL_OPEN:
            if((NULL != param) && (0 < paramLen))
            {
                openOsdModule((char *)param, paramLen);
            }
            break;

        case CMD_OSD_PRIVATEMASK_OPEN:
            //mid_osd_privateMask_init();
            break;

        case CMD_OSD_PRIVATEMASK_CLOSE:
            //mid_osd_privateMask_uninit();
            break;

        case CMD_OSD_PRIVATEMASK_SET:
            if((NULL != param) && (0 < paramLen))
            {
                setOsdPrivateMask((char *)param, paramLen);
            }
            break;

        case CMD_OSD_RESET_RESOLUTION:
            if(g_s32OsdTextHandleCnt && (NULL != param) && (0 < paramLen))
            {
                MI_SYS_WindowSize_t size;
                MI_U32 Osdparam[3];
                MI_U32 veChannel = 0;

                memcpy(Osdparam, param, paramLen);
                veChannel   = Osdparam[0];
                size.u16Width  = Osdparam[1];
                size.u16Height = Osdparam[2];
                resetOsdResolution(veChannel, &size);
            }
            break;

        case CMD_OSD_RESTART_RESOLUTION:
            if(g_s32OsdTextHandleCnt && (NULL != param) && (0 < paramLen))
            {
                MI_SYS_WindowSize_t size;
                MI_U32 Osdparam[3];
                MI_U32 veChannel = 0;

                memcpy(Osdparam, param, paramLen);
                veChannel   = Osdparam[0];
                size.u16Width  = Osdparam[1];
                size.u16Height = Osdparam[2];
                restartOsdResolution(veChannel, &size);
            }
            break;

        case CMD_OSD_PRIVATEMASK_SET_FROM_VI:
            break;

        default:
            break;
    }

    return 0;
}
