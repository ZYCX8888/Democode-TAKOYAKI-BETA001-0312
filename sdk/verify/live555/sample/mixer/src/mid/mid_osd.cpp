/*************************************************
*
* Copyright (c) 2006-2015 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  mid_osd.c
* Author:     andely.zhou@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2018/6/13
* Description: mixer osd middleware source file
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mi_common.h"
#include "mi_venc.h"

#include "module_common.h"
#include "mid_common.h"
#include "mi_sys.h"


#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
#include "ssnn.h"
#include "mi_isp.h"
#elif TARGET_CHIP_I5
#include "mi_fd.h"
#endif

#include "mi_md.h"
#include "mi_od.h"
#include "mi_vg.h"
#include "mi_rgn.h"
#include "mid_md.h"
#include "mi_vdf_datatype.h"
#include "mid_osd.h"
#include "mid_video_type.h"
#include "mid_VideoEncoder.h"
//#include "mid_utils.h"
#include "mid_dla.h"
#include "mid_utils.h"

#define ALIGN_2xUP(x)               (((x+1) / 2) * 2)
#define ALIGN_4xUP(x)               (((x+1) / 4) * 4)

#define _abs(x)  ((x) > 0 ? (x) : (-(x)))

#define OSD_Done                                    0
#define OSD_Draw                                    1

#define OD_DIV_W                                    3
#define OD_DIV_H                                    3
#define MASK_REGION_NUM                             8
//#define MD_REGION_NUM                             8

#define MAX_FD_RECT_NUMBER                          20
#define MAX_FR_RECT_NUMBER                          20

//#define MAX_OD_RECT_NUMBER                        9
//#define MAX_VG_LINE_NUMBER                        2
////////////////////////////////////////////////////////////////////////

#define RGN_OSD_START                               1

#define MIXER_OSD_TEXT_WIDGET_START_X               0
#define MIXER_OSD_TEXT_WIDGET_START_Y               4
#define MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT      20
#define MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT     40
#define MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT      90

#define MIXER_OSD_TEXT_WIDGET_WIDTH_SMALL_FONT      320
#define MIXER_OSD_TEXT_WIDGET_HEIGHT_SMALL_FONT     240
#define MIXER_OSD_TEXT_WIDGET_WIDTH_MEDIUM_FONT     640
#define MIXER_OSD_TEXT_WIDGET_HEIGHT_MEDIUM_FONT    480
#define MIXER_OSD_TEXT_WIDGET_WIDTH_LARGE_FONT      1440
#define MIXER_OSD_TEXT_WIDGET_HEIGHT_LARGE_FONT     960
#define MIXER_OSD_TEXT_SMALL_FONT                   FONT_SIZE_16
#define MIXER_OSD_TEXT_MEDIUM_FONT                  FONT_SIZE_32
#define MIXER_OSD_TEXT_LARGE_FONT                   FONT_SIZE_72


#define RGN_COVER_MAX_WIDTH                         8192
#define RGN_COVER_MAX_HEIGHT                        8192
#define MIXER_RGN_COVER_WIDTH                       (RGN_COVER_MAX_WIDTH/8)
#define MIXER_RGN_COVER_HEIGHT                      (RGN_COVER_MAX_HEIGHT/8)
#define MIXER_OSD_COLOR_INVERSE_THD                 96

#define MIXER_OSD_TEXT_WIDGET_ENABLE                1
#define MIXER_FULL_OSD_TEST_STRING_LENGTH           10
#define MIXER_OSD_COLOR_INVERSE_ACTING_ON_RECT_WIDGET 0

#define MIXER_OSD_MAX_STRING_LENGTH                20
#define MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH     1
#define MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH     6
#define MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH       3
#define MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH     2
#define MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH     1
#define MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH     1
#define MIXER_OSD_USER2_INFO_HEIGHT_MAX_LENGTH     1
#define MIXER_OSD_USER3_INFO_HEIGHT_MAX_LENGTH     1

#define MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX       0x00
#define CHECK_PARAM_IS_X(PARAM,X,RET,errinfo) \
do\
{\
    if((PARAM) == (X))\
    { \
       MIXER_ERR("The input Mixer Rgn Param pointer is NULL!\n");\
       return RET;\
    }\
}while(0);
#define CHECK_PARAM_OPT_X(PARAM,OPT,X,RET,errinfo) \
do\
{\
 if((PARAM) OPT (X))\
  { \
       MIXER_ERR("%s\n",errinfo);\
       return RET;\
  }\
}while(0);

static pthread_mutex_t g_stVideoInfoMutex;
static pthread_mutex_t g_stMutexMixerOsdTextRgnBuf;
static pthread_mutex_t g_stMutexMixerOsdRun[MAX_VIDEO_NUMBER];

MI_RGN_HANDLE g_au32OsdI2Widget[MAX_VIDEO_NUMBER][MAX_RGN_NUMBER_PER_CHN];
MI_RGN_HANDLE g_au32OsdI4Widget[MAX_VIDEO_NUMBER][MAX_RGN_NUMBER_PER_CHN];
MI_RGN_HANDLE g_au32OsdI8Widget[MAX_VIDEO_NUMBER][MAX_RGN_NUMBER_PER_CHN];
MI_RGN_HANDLE g_au32BitmapWidget[MAX_VIDEO_NUMBER][MAX_RGN_NUMBER_PER_CHN];

static MI_RGN_HANDLE g_au32OsdRectWidgetHandle[MAX_VIDEO_NUMBER];
static MI_RGN_HANDLE g_au32OsdTextWidgetHandle[MAX_VIDEO_NUMBER][MAX_RGN_NUMBER_PER_CHN];
static bool g_au32OsdTextWidgetHandleUseFlag[MAX_VIDEO_NUMBER][MAX_RGN_NUMBER_PER_CHN];
static MI_RGN_HANDLE g_au32OsdCoverWidgetHandle[MAX_VIDEO_NUMBER][MAX_RGN_NUMBER_PER_CHN];
//static MI_RGN_HANDLE g_au32OsdCoverWidgetHandle[MAX_VIDEO_NUMBER][MAX_COVER_NUMBER_PER_CHN];
typedef struct _rect{
    struct list_head rectlist;
    MI_S32  tCount;
    MI_U8 *pChar;
}stRectList;

static stRectList g_rectList[10];
static struct list_head g_WorkRectList;
static struct list_head g_EmptyRectList;
static struct MixerOsdTextWidgetOrder_t g_stMixerOsdWidgetOrder;

static MI_U8 *g_pbuffer = NULL;
static unsigned int g_u32AudioDetectOsdStatus = OSD_Done;
static BOOL   g_bOsdSetResulution[MAX_VIDEO_NUMBER];
static BOOL   g_bTimeStamp = FALSE;
static BOOL   g_bVideoInfo = FALSE;
static BOOL   g_bAudioInfo = FALSE;
static BOOL   g_bIspInfo = FALSE;
static BOOL   g_bUser0Info = FALSE;
static BOOL   g_bUser1Info = FALSE;
static BOOL   g_bUser2Info = FALSE;
static BOOL   g_bBabyCry = FALSE;
static BOOL   g_bLoudSound = FALSE;
static volatile BOOL  g_osd_init = false;
static MI_S16 g_s16LoudSounddB = 0;
static char *g_ps8BabyCryString = (char *)"Baby Cry detected";
static char *g_ps8LoudSoundString = (char *)"Loud Sound detected";

void* OSD_Task(void *argu);

static BOOL g_bExit;
static BOOL g_bThreadOsdExit;
static BOOL g_bIEOsdInit = FALSE;
static BOOL g_bMaskOsdInit[MAX_VIDEO_NUMBER];
static MI_S32 g_bMaskOsdCnt[MAX_VIDEO_NUMBER];

static pthread_t g_stPthreadOsd;
#define DEFAULT_OSD_REFRESH_INTERVAL (500)

MI_S32 g_s32OsdRectHandleCnt = 0;
MI_S32 g_s32OsdTextHandleCnt = 0;

MI_BOOL g_bOsdColorInverse = FALSE;
MI_BOOL g_bUserOsdDisplay = FALSE;
//MI_BOOL g_bOsdFlicker = FALSE;
extern MI_S32 g_s32OsdFlicker;


extern MI_S32 g_EnablePIP;
extern MI_VideoEncoder *g_videoEncoderArray[MAX_VIDEO_NUMBER];
extern MI_S32 g_ieWidth;
extern MI_S32 g_ieHeight;
extern MI_S32 g_vgParam;
extern MI_S32 gDebug_OsdTest;
extern MI_U32 g_videoNumber;
extern MI_S32 g_dlaParam;
extern BOOL dla_init; 
extern void MySystemDelay(unsigned int  msec);
static int g_s32VideoStreamNum = g_videoNumber;

pthread_mutex_t g_mutex_UpadteOsdState = PTHREAD_MUTEX_INITIALIZER;  
pthread_cond_t  g_cond_UpadteOsdState  = PTHREAD_COND_INITIALIZER;  
MI_U32 g_UpadteOsdState = 0;

static int g_s32Width  = 1920;
static int g_s32Height = 1080;
static int g_s32OsdInitDone = 0;
static int g_s32OsdDisplayVideoInfo = 0;

static MI_SYS_WindowRect_t g_stFdRect[MAX_FD_RECT_NUMBER];
static MI_SYS_WindowRect_t g_stFrRect[MAX_FR_RECT_NUMBER];
static MI_SYS_WindowRect_t g_stOdRect[OD_DIV_W * OD_DIV_H] = {{0,},};
static MI_SYS_WindowRect_t g_stMdRect[(RAW_W)*(RAW_H)] = {{0,},};
#if TARGET_CHIP_I5 || TARGET_CHIP_I6E
static MI_SYS_WindowRect_t g_stDLARect[MAX_DLA_RECT_NUMBER];
#endif

static MI_RGN_PaletteTable_t g_stPaletteTable = {
    { //index0 ~ index15
     {  0,   0,   0,   0}, {255, 255,   0,   0}, {255,   0, 255,   0}, {255,   0,   0, 255},
     {255, 255, 255,   0}, {255,   0, 112, 255}, {255,   0, 255, 255}, {255, 255, 255, 255},
     {255, 128,   0,   0}, {255, 128, 128,   0}, {255, 128,   0, 128}, {255,   0, 128,   0},
     {255,   0,   0, 128}, {255,   0, 128, 128}, {255, 128, 128, 128}, {255,  64,  64,  64}}
};

//static Color_t g_stTransportColor = {(MI_U8)0,   (MI_U8)0,   (MI_U8)0,  (MI_U8)0};
static Color_t g_stBlackColor     = {(MI_U8)255, (MI_U8)0,   (MI_U8)0,  (MI_U8)0};
static Color_t g_stRedColor       = {(MI_U8)255, (MI_U8)255, (MI_U8)0,   (MI_U8)0};
static Color_t g_stGreenColor     = {(MI_U8)255, (MI_U8)0,   (MI_U8)128, (MI_U8)0};
//static Color_t g_stBlueColor      = {(MI_U8)255, (MI_U8)0,   (MI_U8)0,   (MI_U8)255};
static Color_t g_stYellowColor    = {(MI_U8)255, (MI_U8)255, (MI_U8)255, (MI_U8)0};
//static Color_t g_stCyanColor      = {(MI_U8)255, (MI_U8)0,   (MI_U8)255, (MI_U8)255};
//static Color_t g_stLimeColor      = {(MI_U8)255, (MI_U8)0,   (MI_U8)255, (MI_U8)0};
//static Color_t g_stMagentaColor   = {(MI_U8)255, (MI_U8)255, (MI_U8)0,   (MI_U8)255};
//static Color_t g_stSilverColor    = {(MI_U8)255, (MI_U8)192, (MI_U8)192, (MI_U8)192};
//static Color_t g_stGrayColor      = {(MI_U8)255, (MI_U8)128, (MI_U8)128, (MI_U8)128};
static int g_s32MdFgMode = 0;
static MI_SYS_WindowSize_t g_stVideoSize[MAX_VIDEO_NUMBER];
static struct list_head  osdList;

static BOOL GetOsdInitFullState()
{
    BOOL tmp;

    do{
        tmp = g_osd_init;
    }while(tmp != g_osd_init);

   return tmp;
}

static void SetOsdInitFullState(BOOL state)
{
      g_osd_init = !!state; /*> 0 ? TRUE:FALSE*/;
}
//void getOsdInfoLinkList(struct list_head *osdInfo)
//{
// osdInfo = &osdList;
//}
 void midManageOsdInfoLinkList(osdInfo_t *osdInfo,BOOL State,osdSizePoint_t *osdSizePoint,const char *pstring)
{
    osdInfo_t midOsdInfo;
    memset(&midOsdInfo,0,sizeof(osdInfo_t));
    memcpy(&midOsdInfo,osdInfo,sizeof(osdInfo_t));
    if(osdSizePoint)
    {
      midOsdInfo.size.u16Width = osdSizePoint->size.u16Width;
      midOsdInfo.size.u16Height = osdSizePoint->size.u16Height;
      midOsdInfo.point.u16X = osdSizePoint->point.u16X;
      midOsdInfo.point.u16Y = osdSizePoint->point.u16Y;
    }
    if(TRUE == State)
    {
      midOsdInfoUpdate(&midOsdInfo);
    }
    else
    {
      midOsdInfoDel(&midOsdInfo);
    }
}
void midOsdInfoInit(void)
{
    MI_U32 i = 0x0;
    osd_Link_List_t osdInfoLinkList;
    memset(&osdInfoLinkList,0,sizeof(osdInfoLinkList));
    INIT_LIST_HEAD(&osdList);

    INIT_LIST_HEAD(&g_EmptyRectList);
    INIT_LIST_HEAD(&g_WorkRectList);
    for (; i < 10; i++)
    {
        g_rectList[i].tCount = 0x0;
        g_rectList[i].pChar = NULL;
        INIT_LIST_HEAD(&g_rectList[i].rectlist);

        list_add_tail(&g_rectList[i].rectlist, &g_EmptyRectList);
    }
}
void midOsdInfoUpdate(osdInfo_t *osdInfo)
{
    osd_Link_List_t *tmp = NULL,*NewNode = NULL;
    struct list_head *pHead = NULL, *ptmp = NULL;
    BOOL addNewNodeFlag = false;
    if(list_empty(&osdList) && \
    (E_OSD_WIDGET_TYPE_RECT <= osdInfo->osdType)&&(E_OSD_WIDGET_TYPE_MAX >= osdInfo->osdType))
    {
      NewNode = (osd_Link_List_t*)malloc(sizeof(osd_Link_List_t));
      if(NULL == NewNode)
      {
        printf("NewNode malloc is failed!\n");
        return;
      }
      memset(&NewNode->osdInfo,0,sizeof(osdInfo_t));
      memcpy(&NewNode->osdInfo,osdInfo,sizeof(osdInfo_t));

      list_add_tail(&NewNode->osd_info_list, &osdList);
      printf("NULL linkList Add first OsdNodeInfo: osdType[%d] format[%d] handle[%d]\n",osdInfo->osdType,osdInfo->format,osdInfo->u32RgnHandle);
      return;
    }
    list_for_each_safe(pHead, ptmp, &osdList)
    {
        tmp = list_entry(pHead, osd_Link_List_t, osd_info_list);
        if((NULL != tmp)&&(tmp->osdInfo.osdType == osdInfo->osdType)&& \
        (tmp->osdInfo.u32RgnHandle == osdInfo->u32RgnHandle))
        {
          addNewNodeFlag = FALSE;
          if((E_OSD_WIDGET_TYPE_RECT <= osdInfo->osdType)&&(E_OSD_WIDGET_TYPE_MAX >= osdInfo->osdType))
          {
            memcpy(&tmp->osdInfo,osdInfo,sizeof(tmp->osdInfo));
            printf("Update OsdNodeInfo: osdType[%d] format[%d] handle[%d]\n",tmp->osdInfo.osdType,tmp->osdInfo.format,tmp->osdInfo.u32RgnHandle);
            return;
          }
        }
        else
        {
           addNewNodeFlag = TRUE;
        }
    }
    if(TRUE == addNewNodeFlag)
    {
         addNewNodeFlag = FALSE;
         if((E_OSD_WIDGET_TYPE_RECT <= osdInfo->osdType)&&(E_OSD_WIDGET_TYPE_MAX >= osdInfo->osdType))
         {
           NewNode = (osd_Link_List_t*)malloc(sizeof(osd_Link_List_t));
           if(NULL == NewNode)
           {
              printf("add NewNode malloc is faied!\n");
              return;
           }
           memset(&NewNode->osdInfo,0,sizeof(osdInfo_t));
             memcpy(&NewNode->osdInfo,osdInfo,sizeof(osdInfo_t));
            list_add_tail(&NewNode->osd_info_list, &osdList);
        /*   if(NewNode)
           {
             free(NewNode);
             NewNode = NULL;
           }*/
           printf("Add New OsdNodeInfo:osdType[%d] format[%d] handle[%d]\n",osdInfo->osdType,osdInfo->format,osdInfo->u32RgnHandle);
           return;
         }
    }
}
void midOsdInfoDel(osdInfo_t *osdInfo)
{
    osd_Link_List_t *tmp = NULL;
    BOOL ifDel = TRUE;
    struct list_head *pos = NULL, *ptmp = NULL;
    if(list_empty(&osdList))
    {
      printf("This osdInfo Linklist is empty!\n");
      return;
    }
    list_for_each_safe(pos, ptmp, &osdList)
    {
        tmp = list_entry(pos, osd_Link_List_t, osd_info_list);
        if((NULL != tmp)&&((tmp->osdInfo.u32RgnHandle == osdInfo->u32RgnHandle) /*&& \
        (tmp->osdInfo.format == osdInfo->format)*/))
        {
          list_del(&tmp->osd_info_list);
          ifDel = TRUE;
          printf("Del OsdNodeInfo:osdType[%d] format[%d] handle[%d]\n",osdInfo->osdType,osdInfo->format,osdInfo->u32RgnHandle);
          if(NULL != tmp)
          {
             free(tmp);
          }
        }
        else
        {
          ifDel = FALSE;
        }
     }
     if(FALSE == ifDel)
     {
       printf("NOT find this osdInfo Node:tpye[%d] format[%d] handle[%d]\n",osdInfo->osdType,osdInfo->format,osdInfo->u32RgnHandle);
     }
}
static void osdInfoPrint(osd_Link_List_t tmp,EN_OSD_WIDGET_TYPE type)
{
   if(E_OSD_WIDGET_TYPE_TEXT == type)
   {
     printf("channel:%d osdIndex:%d format:%d handle:%d\n",tmp.osdInfo.channel,tmp.osdInfo.osdIndex,tmp.osdInfo.format,tmp.osdInfo.u32RgnHandle);
     printf(" Width:%d height:%d",tmp.osdInfo.size.u16Width,tmp.osdInfo.size.u16Height);
     printf(" pointX:%d pointY:%d\n",tmp.osdInfo.point.u16X,tmp.osdInfo.point.u16Y);
   }
   else if(E_OSD_WIDGET_TYPE_COVER == type)
   {
     printf("channel:%d osdIndex:%d format:%d handle:%d\n",tmp.osdInfo.channel,tmp.osdInfo.osdIndex,tmp.osdInfo.format,tmp.osdInfo.u32RgnHandle);
     printf(" Width:%d height:%d",tmp.osdInfo.size.u16Width,tmp.osdInfo.size.u16Height);
     printf(" pointX:%d pointY:%d\n",tmp.osdInfo.point.u16X,tmp.osdInfo.point.u16Y);
   }
   else if(E_OSD_WIDGET_TYPE_RECT == type)
   {
     printf("channel:%d osdIndex:%d format:%d handle:%d\n",tmp.osdInfo.channel,tmp.osdInfo.osdIndex,tmp.osdInfo.format,tmp.osdInfo.u32RgnHandle);
     printf(" Width:%d height:%d",tmp.osdInfo.size.u16Width,tmp.osdInfo.size.u16Height);
     printf(" pointX:%d pointY:%d\n",tmp.osdInfo.point.u16X,tmp.osdInfo.point.u16Y);
   }
   else if(E_OSD_WIDGET_TYPE_BITMAP == type)
   {
     printf("channel:%d osdIndex:%d format:%d handle:%d\n",tmp.osdInfo.channel,tmp.osdInfo.osdIndex,tmp.osdInfo.format,tmp.osdInfo.u32RgnHandle);
     printf(" Width:%d height:%d",tmp.osdInfo.size.u16Width,tmp.osdInfo.size.u16Height);
     printf(" pointX:%d pointY:%d\n",tmp.osdInfo.point.u16X,tmp.osdInfo.point.u16Y);
   }
}
MI_RGN_HANDLE midFindOsdHandleByOsdAttr(osdInfo_t *osdInfo,GetOsdATTR_e info)
{
     osd_Link_List_t *tmp = NULL;
     struct list_head *pHead = NULL, *ptmp = NULL;
     if(list_empty(&osdList))
     {
       printf("The osdLinkList is NULL !\n");
     }
     list_for_each_safe(pHead, ptmp, &osdList)
     {
       tmp = list_entry(pHead, osd_Link_List_t, osd_info_list);
       if(NULL != tmp)
       {
         if(E_OSD_WIDGET_TYPE_RECT <= tmp->osdInfo.osdType && \
            tmp->osdInfo.osdType <= E_OSD_WIDGET_TYPE_MAX)
         {
           if(GET_MAX == info)
            {
                osdInfo->point.u16X = tmp->osdInfo.point.u16X;
                osdInfo->point.u16Y = tmp->osdInfo.point.u16Y;
                osdInfo->size.u16Width = tmp->osdInfo.size.u16Width;
                osdInfo->size.u16Height = tmp->osdInfo.size.u16Height;
                osdInfo->format = tmp->osdInfo.format;
                osdInfo->ShowLevel = tmp->osdInfo.ShowLevel;
            }
            else if(GET_FORMAT == info)
            {
                osdInfo->format = tmp->osdInfo.format;
                osdInfo->ShowLevel = tmp->osdInfo.ShowLevel;
            }
            return  tmp->osdInfo.u32RgnHandle;
         }
       }
     }
   return MI_RGN_HANDLE_NULL;
}
void midFindOsdInfoPrint(osd_Link_List_t *osdInfoNode,PRINT_e way)
{
   osd_Link_List_t *tmp = NULL;
   struct list_head *pHead = NULL, *ptmp = NULL;
   if(list_empty(&osdList))
   {
     printf("The osdLinkList is NULL !\n");
   }
   list_for_each_safe(pHead, ptmp, &osdList)
   {
     tmp = list_entry(pHead, osd_Link_List_t, osd_info_list);
     if(NULL != tmp)
     {
       switch(osdInfoNode->osdInfo.osdType)
       {
         case E_OSD_WIDGET_TYPE_RECT:
         {

           if((BY_TYPE == way) && (E_OSD_WIDGET_TYPE_RECT == tmp->osdInfo.osdType))
           {
             osdInfoPrint(*tmp,E_OSD_WIDGET_TYPE_RECT);
           }
           else if((BY_INDEX == way) && (osdInfoNode->osdInfo.osdIndex == tmp->osdInfo.osdIndex))
           {
             osdInfoPrint(*tmp,E_OSD_WIDGET_TYPE_RECT);
           }
           else if((BY_CHANNEL == way) && (osdInfoNode->osdInfo.channel== tmp->osdInfo.channel))
           {
             osdInfoPrint(*tmp,E_OSD_WIDGET_TYPE_RECT);
           }
         }
           break;
         case E_OSD_WIDGET_TYPE_TEXT:
         {
           if(E_OSD_WIDGET_TYPE_TEXT == tmp->osdInfo.osdType)
           {
              if((BY_TYPE == way) && (E_OSD_WIDGET_TYPE_TEXT == tmp->osdInfo.osdType))
               {
                 osdInfoPrint(*tmp,E_OSD_WIDGET_TYPE_TEXT);
               }
               else if((BY_INDEX == way) && (osdInfoNode->osdInfo.osdIndex == tmp->osdInfo.osdIndex))
               {
                 osdInfoPrint(*tmp,E_OSD_WIDGET_TYPE_TEXT);
               }
               else if((BY_CHANNEL == way) && (osdInfoNode->osdInfo.channel== tmp->osdInfo.channel))
               {
                 osdInfoPrint(*tmp,E_OSD_WIDGET_TYPE_TEXT);
               }
           }
         }
          break;
        case E_OSD_WIDGET_TYPE_COVER:
         {
           if(E_OSD_WIDGET_TYPE_COVER == tmp->osdInfo.osdType)
           {
              if((BY_TYPE == way) && (E_OSD_WIDGET_TYPE_COVER == tmp->osdInfo.osdType))
               {
                 osdInfoPrint(*tmp,E_OSD_WIDGET_TYPE_COVER);
               }
               else if((BY_INDEX == way) && (osdInfoNode->osdInfo.osdIndex == tmp->osdInfo.osdIndex))
               {
                 osdInfoPrint(*tmp,E_OSD_WIDGET_TYPE_COVER);
               }
               else if((BY_CHANNEL == way) && (osdInfoNode->osdInfo.channel== tmp->osdInfo.channel))
               {
                 osdInfoPrint(*tmp,E_OSD_WIDGET_TYPE_COVER);
               }
           }
         }
         break;
         case E_OSD_WIDGET_TYPE_BITMAP :
         {
           if(E_OSD_WIDGET_TYPE_BITMAP == tmp->osdInfo.osdType)
           {
              if((BY_TYPE == way) && (E_OSD_WIDGET_TYPE_BITMAP == tmp->osdInfo.osdType))
               {
                 osdInfoPrint(*tmp,E_OSD_WIDGET_TYPE_BITMAP);
               }
               else if((BY_INDEX == way) && (osdInfoNode->osdInfo.osdIndex == tmp->osdInfo.osdIndex))
               {
                 osdInfoPrint(*tmp,E_OSD_WIDGET_TYPE_BITMAP);
               }
               else if((BY_CHANNEL == way) && (osdInfoNode->osdInfo.channel== tmp->osdInfo.channel))
               {
                 osdInfoPrint(*tmp,E_OSD_WIDGET_TYPE_BITMAP);
               }
           }
         }
         break;
         case E_OSD_WIDGET_TYPE_MAX:
         {
             osdInfoPrint(*tmp,tmp->osdInfo.osdType);
         }
          break;
        default:
             break;
      }
     }
   }
}
MI_S32 getVideoSize(MI_VENC_CHN s32VencChn, MI_SYS_WindowSize_t *pstSize)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VENC_ChnAttr_t stVencChnAttr;

    if((s32VencChn < 0) || (s32VencChn > (MAX_VIDEO_NUMBER-1)))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if(NULL == pstSize)
    {
        MIXER_ERR("The input param is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if((0 != g_stVideoSize[s32VencChn].u16Width) && (0 != g_stVideoSize[s32VencChn].u16Height))
    {
        pstSize->u16Width  = g_stVideoSize[s32VencChn].u16Width;
        pstSize->u16Height = g_stVideoSize[s32VencChn].u16Height;
    }
    else
    {
        s32Ret = MI_VENC_GetChnAttr(s32VencChn, &stVencChnAttr);
        if(MI_SUCCESS != s32Ret)
        {
            MIXER_ERR("MI_VENC_GetChnAttr fail, %d\n", s32Ret);
            return s32Ret;
        }

        switch(stVencChnAttr.stVeAttr.eType)
        {
            case E_MI_VENC_MODTYPE_H264E:
                pstSize->u16Width  = stVencChnAttr.stVeAttr.stAttrH264e.u32PicWidth;
                pstSize->u16Height = stVencChnAttr.stVeAttr.stAttrH264e.u32PicHeight;
                break;
            case E_MI_VENC_MODTYPE_H265E:
                pstSize->u16Width  = stVencChnAttr.stVeAttr.stAttrH265e.u32PicWidth;
                pstSize->u16Height = stVencChnAttr.stVeAttr.stAttrH265e.u32PicHeight;
                break;
            case E_MI_VENC_MODTYPE_JPEGE:
                pstSize->u16Width  = stVencChnAttr.stVeAttr.stAttrJpeg.u32PicWidth;
                pstSize->u16Height = stVencChnAttr.stVeAttr.stAttrJpeg.u32PicHeight;
                break;
            default:
                MIXER_ERR("Not support Venc type: %d\n", stVencChnAttr.stVeAttr.eType);
        }

        g_stVideoSize[s32VencChn].u16Width  = pstSize->u16Width;
        g_stVideoSize[s32VencChn].u16Height = pstSize->u16Height;
    }

    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_U16 getVideoWidth(MI_VENC_CHN s32VencChn)
{
    MI_SYS_WindowSize_t size;
    MI_S32 s32Ret = E_MI_ERR_FAILED;

    s32Ret = getVideoSize(s32VencChn, &size);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoSize fail, %d\n", s32Ret);
        return 0;
    }

    return size.u16Width;
}

MI_U16 getVideoHeight(MI_VENC_CHN s32VencChn)
{
    MI_SYS_WindowSize_t size;
    MI_S32 s32Ret = E_MI_ERR_FAILED;

    s32Ret = getVideoSize(s32VencChn, &size);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoSize fail, %d\n", s32Ret);
        return 0;
    }

    return size.u16Height;
}

MI_U32 getFontSizeByType(OsdFontSize_e eFontSize)
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
        default: return 0;
    }
}

MI_U32 getFontOfStrlen(const MI_S8 *phzstr)
{
    MI_U8 count = 0;
    MI_U8 *str;
    MI_U32 len;

    len = strlen((char *)phzstr);
    str = (MI_U8 *)phzstr;

    while(len > 0)
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

MI_S32 getDividedNumber(MI_S32 value)
{
    if(0 == value % 32)       return value / 32;
    else if(0 == value % 30)  return value / 30;
    else if(0 == value % 28)  return value / 28;
    else if(0 == value % 27)  return value / 27;
    else if(0 == value % 25)  return value / 25;
    else if(0 == value % 24)  return value / 24;
    else if(0 == value % 22)  return value / 22;
    else if(0 == value % 21)  return value / 21;
    else if(0 == value % 20)  return value / 20;
    else if(0 == value % 18)  return value / 18;
    else if(0 == value % 16)  return value / 16;
    else if(0 == value % 15)  return value / 15;
    else if(0 == value % 14)  return value / 14;
    else if(0 == value % 12)  return value / 12;
    else if(0 == value % 10)  return value / 10;
    else if(0 == value % 9)   return value / 9;
    else if(0 == value % 8)   return value / 8;
    else if(0 == value % 7)   return value / 7;
    else if(0 == value % 6)   return value / 6;
    else if(0 == value % 5)   return value / 5;
    else if(0 == value % 4)   return value / 4;
    else if(0 == value % 3)   return value / 3;
    else if(0 == value % 2)   return value / 2;
    else return value;
}

MI_RGN_HANDLE calcOsdHandle(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, EN_OSD_WIDGET_TYPE eOsdWidgetType)
{
    MI_RGN_HANDLE u32OsdHdl = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;

    if((0 <= s32VencChn) && (s32VencChn < g_s32VideoStreamNum) && (0 <= s32Idx) && (s32Idx < MAX_RGN_NUMBER_PER_CHN))
    {
         if(E_OSD_WIDGET_TYPE_TEXT == eOsdWidgetType)
        {
            u32OsdHdl = RGN_OSD_START + s32VencChn * MAX_RGN_NUMBER_PER_CHN + s32Idx; //RGN_OSD_START + MAX_VIDEO_NUMBER + s32VencChn * MAX_RGN_NUMBER_PER_CHN + s32Idx;
        }
        else if(E_OSD_WIDGET_TYPE_COVER == eOsdWidgetType)
        {
            u32OsdHdl = RGN_OSD_START + MAX_VIDEO_NUMBER * (MAX_RGN_NUMBER_PER_CHN) + s32VencChn * MAX_RGN_NUMBER_PER_CHN + s32Idx + 1;
        }
        else if(E_OSD_WIDGET_TYPE_BITMAP == eOsdWidgetType)
        {
            u32OsdHdl = RGN_OSD_START + MAX_VIDEO_NUMBER * (MAX_RGN_NUMBER_PER_CHN * 2) + s32VencChn * MAX_RGN_NUMBER_PER_CHN + s32Idx + 1;
        }
        else if(E_OSD_WIDGET_TYPE_RECT == eOsdWidgetType)
        {
            u32OsdHdl = RGN_OSD_START + MAX_VIDEO_NUMBER * (MAX_RGN_NUMBER_PER_CHN * 3)  + s32VencChn * MAX_RGN_NUMBER_PER_CHN + s32Idx + 1; //RGN_OSD_START + s32VencChn;
        }
    }

    return u32OsdHdl;
}

MI_RGN_HANDLE  getOsdHandle(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, EN_OSD_WIDGET_TYPE eOsdWidgetType)
{
    MI_RGN_HANDLE u32RgnHandle = (MI_RGN_HANDLE)-1;


    if((s32VencChn < 0) || (s32VencChn > (MAX_VIDEO_NUMBER-1)))
    {
        MIXER_ERR("The input VenChn(%d) is not support!\n", s32VencChn);
        return u32RgnHandle;
    }

    if((-1 == s32Idx) && (E_OSD_WIDGET_TYPE_RECT == eOsdWidgetType))
    {
        u32RgnHandle = g_au32OsdRectWidgetHandle[s32VencChn];
    }
    else if((0 <= s32Idx) && (MAX_RGN_NUMBER_PER_CHN > s32Idx))
    {
        if(E_OSD_WIDGET_TYPE_TEXT == eOsdWidgetType)
        {
            u32RgnHandle = g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
        }
        else if((E_OSD_WIDGET_TYPE_COVER == eOsdWidgetType) && (s32Idx < MAX_COVER_NUMBER_PER_CHN))
        {
            u32RgnHandle = g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx];
        }
    }
    else
    {
        MIXER_ERR("The input s32Idx(%d) or eOsdWidgetType(%d) is not support!\n", s32Idx, eOsdWidgetType);
        return u32RgnHandle;
    }

    return u32RgnHandle;
}

MI_RGN_HANDLE * getOsdHandleAddr(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, EN_OSD_WIDGET_TYPE eOsdWidgetType)
{
    MI_RGN_HANDLE *pu32RgnHandleAddr = NULL;

    if((s32VencChn < 0) || (s32VencChn > (MAX_VIDEO_NUMBER-1)))
    {
        MIXER_ERR("The input VenChn(%d) is not support!\n", s32VencChn);
        return pu32RgnHandleAddr;
    }

    if((-1 == s32Idx) && (E_OSD_WIDGET_TYPE_RECT == eOsdWidgetType))
    {
        pu32RgnHandleAddr = (MI_RGN_HANDLE *)&g_au32OsdRectWidgetHandle[s32VencChn];
    }
    else if((0 <= s32Idx) && (MAX_RGN_NUMBER_PER_CHN > s32Idx))
    {
        if(E_OSD_WIDGET_TYPE_TEXT == eOsdWidgetType)
        {
            pu32RgnHandleAddr = (MI_RGN_HANDLE *)&g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
        }
        else if((E_OSD_WIDGET_TYPE_COVER == eOsdWidgetType) && (s32Idx < MAX_COVER_NUMBER_PER_CHN))
        {
            pu32RgnHandleAddr = (MI_RGN_HANDLE *)&g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx];
        }
    }
    else
    {
        MIXER_ERR("The input s32Idx(%d) or eOsdWidgetType(%d) is not support!\n", s32Idx, eOsdWidgetType);
        return pu32RgnHandleAddr;
    }

    return pu32RgnHandleAddr;
}

MI_S32 reSetOsdHandle(MI_RGN_HANDLE u32RgnHandle)
{
    MI_S32 s32Idx = 0;
    MI_S32 s32VencChn = 0;


    if(((MI_S32)u32RgnHandle < 0) || (u32RgnHandle > MI_RGN_MAX_HANDLE))
    {
        MIXER_ERR("The input u32RgnHandle(%d) is not out of range!\n", u32RgnHandle);
        return E_MI_ERR_ILLEGAL_PARAM;
    }

    for(s32VencChn = 0; s32VencChn < MAX_VIDEO_NUMBER; s32VencChn++)
    {
        if(u32RgnHandle == g_au32OsdRectWidgetHandle[s32VencChn])
        {
            g_au32OsdRectWidgetHandle[s32VencChn] = MI_RGN_HANDLE_NULL;
            break;
        }

        for(s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
        {
            if(u32RgnHandle == g_au32OsdTextWidgetHandle[s32VencChn][s32Idx])
            {
                g_au32OsdTextWidgetHandle[s32VencChn][s32Idx] = MI_RGN_HANDLE_NULL;
                break;
            }
            else if((u32RgnHandle == g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])&&(s32Idx < MAX_COVER_NUMBER_PER_CHN))
            {
                g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx] = MI_RGN_HANDLE_NULL;
                break;
            }
        }
    }

    return MI_SUCCESS;
}

MI_S32 configOsdRgnChnPort(MI_VENC_CHN s32VencChn, MI_RGN_ChnPort_t *pstRgnChnPort)
{
    MI_S32 s32Ret = MI_SUCCESS;


    if((s32VencChn < 0) || (s32VencChn >= g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_FAILED;
        return s32Ret;
    }

    if(NULL == pstRgnChnPort)
    {
        MIXER_ERR("The input Mixer Rgn chnPort struct pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if((s32VencChn < g_s32VideoStreamNum) && (NULL != g_videoEncoderArray[s32VencChn]))
    {
        if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
        {
            pstRgnChnPort->eModId    = E_MI_RGN_MODID_DIVP;
            pstRgnChnPort->s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpSclDownChnId;
            pstRgnChnPort->s32DevId  = 0;
            pstRgnChnPort->s32OutputPortId = 0;
        }
        else if(g_videoEncoderArray[s32VencChn]->m_bUseDivp)
        {
            //vpe port2 case: vif->vpe->divp->venc
            pstRgnChnPort->eModId    = E_MI_RGN_MODID_DIVP;
            pstRgnChnPort->s32ChnId  = (MI_S32)g_videoEncoderArray[s32VencChn]->m_DivpChnPort.u32ChnId;
            pstRgnChnPort->s32DevId  = (MI_S32)g_videoEncoderArray[s32VencChn]->m_DivpChnPort.u32DevId;
            pstRgnChnPort->s32OutputPortId = (MI_S32)g_videoEncoderArray[s32VencChn]->m_DivpChnPort.u32PortId;
        }
        else
        {
            pstRgnChnPort->eModId   = E_MI_RGN_MODID_VPE;
            pstRgnChnPort->s32DevId = (MI_S32)g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32DevId;
            pstRgnChnPort->s32ChnId = (MI_S32)g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32ChnId;
            pstRgnChnPort->s32OutputPortId = (MI_S32)g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32PortId;
        }
    }

    return s32Ret;
}

MI_S32 configRgnWidgetAttr(MI_VENC_CHN s32VencChn,
                                   MI_S32 s32Idx,
                                   MI_SYS_WindowRect_t *pstRect,
                                   MI_RGN_ChnPort_t *pstRgnChnPort,
                                   ST_MIXER_RGN_WIDGET_ATTR *pstMixerRgnWidgetAttr,MI_RGN_PixelFormat_e eRgnFormat)
{
    MI_S32 s32Ret = MI_SUCCESS;

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_FAILED;
        return s32Ret;
    }

    if((s32Idx < 0) || (s32Idx > MAX_RGN_NUMBER_PER_CHN))
    {
        MIXER_ERR("The input OSD handle index(%d) is out of range!\n", s32Idx);
        s32Ret = E_MI_ERR_FAILED;
        return s32Ret;
    }

    if(NULL == pstMixerRgnWidgetAttr)
    {
        MIXER_ERR("The input pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    //memset(pstMixerRgnWidgetAttr, 0x00, sizeof(ST_MIXER_RGN_WIDGET_ATTR));
    pstMixerRgnWidgetAttr->s32VencChn = s32VencChn;
    pstMixerRgnWidgetAttr->s32Idx = s32Idx;
    pstMixerRgnWidgetAttr->u32Color = 0xff801080; //black
    pstMixerRgnWidgetAttr->eOsdWidgetType = E_OSD_WIDGET_TYPE_TEXT;
    pstMixerRgnWidgetAttr->bShow = TRUE;
    pstMixerRgnWidgetAttr->bOsdColorInverse = g_bOsdColorInverse;
    pstMixerRgnWidgetAttr->eRgnpixelFormat = E_MI_RGN_PIXEL_FORMAT_I4;
    pstMixerRgnWidgetAttr->u16LumaThreshold = MIXER_OSD_COLOR_INVERSE_THD;
    pstMixerRgnWidgetAttr->pstMutexMixerOsdRun = &g_stMutexMixerOsdRun[s32VencChn];
    if(NULL != pstRect)
    {
        memcpy(&pstMixerRgnWidgetAttr->stRect, pstRect, sizeof(MI_SYS_WindowRect_t));
    }

    if(NULL != pstRgnChnPort)
    {
        memcpy(&pstMixerRgnWidgetAttr->stRgnChnPort, pstRgnChnPort, sizeof(MI_RGN_ChnPort_t));
    }
    g_stMixerOsdWidgetOrder.pmt = eRgnFormat;
    return s32Ret;
}

MI_S32 createOsdTextWidget(MI_RGN_HANDLE hHandle, MI_RGN_Attr_t *pstRgnAttr, MI_RGN_ChnPort_t *pstRgnChnPort, MI_RGN_ChnPortParam_t *pstRgnChnPortParam)
{
   MI_S32 s32Ret = E_MI_ERR_FAILED;


 /*  if(0 > hHandle)
   {
       MIXER_ERR("The input Rgn handle(%d) is out of range!\n", hHandle);
       s32Ret = E_MI_ERR_ILLEGAL_PARAM;
       return s32Ret;
   }*/

   if((NULL == pstRgnAttr) || (NULL == pstRgnChnPort) || (NULL == pstRgnChnPortParam))
   {
       MIXER_ERR("createOsdTextWidget() the input pointer is NULL!\n");
       s32Ret = E_MI_ERR_NULL_PTR;
       return s32Ret;
   }

   s32Ret = MI_RGN_Create(hHandle, pstRgnAttr);
   if(MI_RGN_OK != s32Ret)
   {
       MIXER_ERR("MI_RGN_Create error, %X\n", s32Ret);
       printf("%s:%d  Hdl=%d RGN_Attr:Type=%d, Width=%4d, Heitht=%4d, fmt=%d\n", __func__, __LINE__, hHandle, pstRgnAttr->eType,
              pstRgnAttr->stOsdInitParam.stSize.u32Width, pstRgnAttr->stOsdInitParam.stSize.u32Height, pstRgnAttr->stOsdInitParam.ePixelFmt);
       return s32Ret;
   }

   s32Ret = MI_RGN_AttachToChn(hHandle, pstRgnChnPort, pstRgnChnPortParam);
   if(MI_RGN_OK != s32Ret)
   {
   	   MIXER_ERR("MI_RGN_AttachToChn error, %X\n", s32Ret);
       s32Ret = MI_RGN_Destroy(hHandle);
	   if(MI_RGN_OK != s32Ret)
	   	 MIXER_ERR("MI_RGN_Destroy error, %X\n", s32Ret);

       printf("%s:%d  Hdl=%d RGN_Attr:Type=%d, Width=%4d, Heitht=%4d, fmt=%d\n", __func__, __LINE__, hHandle, pstRgnAttr->eType,
              pstRgnAttr->stOsdInitParam.stSize.u32Width, pstRgnAttr->stOsdInitParam.stSize.u32Height, pstRgnAttr->stOsdInitParam.ePixelFmt);
       printf("%s:%d  Hdl=%d RGN:ModId=%d, DevId=%d, ChnId=%d, PortId=%d, fmt=I4\n", __func__, __LINE__, hHandle, pstRgnChnPort->eModId,
                      pstRgnChnPort->s32DevId, pstRgnChnPort->s32ChnId, pstRgnChnPort->s32OutputPortId);
       printf("%s:%d  Hdl=%d canvas:x=%d, y=%d, InvertColorMode=%d, DivNum=%d, DivNum=%d, Threshold=%d\n", __func__, __LINE__, hHandle,
                      pstRgnChnPortParam->stPoint.u32X, pstRgnChnPortParam->stPoint.u32Y,
                      pstRgnChnPortParam->unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode,
                      pstRgnChnPortParam->unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum,
                      pstRgnChnPortParam->unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum,
                      pstRgnChnPortParam->unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold);
       return s32Ret;
   }
   //printf("TextHdl=%d canvas:x=%d, y=%d, w=%4d, h=%4d, fmt=%d\n", hHandle, pstRgnChnPortParam->stPoint.u32X, pstRgnChnPortParam->stPoint.u32Y,
   //        pstRgnAttr->stOsdInitParam.stSize.u32Width, pstRgnAttr->stOsdInitParam.stSize.u32Height, pstRgnAttr->stOsdInitParam.ePixelFmt);

   s32Ret = MI_RGN_OK;
   return s32Ret;
}

MI_S32 updateOsdTextWidget(MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, TextWidgetAttr_t* pstTextWidgetAttr, BOOL visible)
{
   static MI_U32 u32BuffSize = 0;
   MI_S32 s32Ret = E_MI_ERR_FAILED;
   MI_U8 u8Char[1024];
   MI_U32 hzLen = 0;
   MI_U32 cpLen = 0;
   MI_U32 nFontSize = 0;
   DrawRgnColor_t stColor;
   //MI_RGN_CanvasInfo_t stCanvasInfo;
   struct OsdTextWidget_t stOsdTextWidget;
   MI_RGN_CanvasInfo_t *pstCanvasInfo_local = NULL;

/*
   if(0 > hHandle)
   {
       MIXER_ERR("The input Rgn handle(%d) is out of range!\n", hHandle);
       s32Ret = E_MI_ERR_ILLEGAL_PARAM;
       return s32Ret;
   }
*/
   if((NULL == pstCanvasInfo) || (NULL == pstTextWidgetAttr))
   {
       MIXER_ERR("updateOsdTextWidget() the input pointer is NULL!\n");
       s32Ret = E_MI_ERR_NULL_PTR;
       return s32Ret;
   }

   if(NULL == pstTextWidgetAttr)
   {
       MIXER_ERR("The input textwidgetAttr pointer is NULL!\n");
       //pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
       return s32Ret;
   }

   pthread_mutex_lock(&g_stMutexMixerOsdTextRgnBuf);

   //printf("%s:%d pstCanvasInfo=0x%x, VirtAddr=0x%x, phyAddr=0x%llx, width=%d, height=%d, stright=%d\n",__func__,__LINE__,pstCanvasInfo,
   //       pstCanvasInfo->virtAddr, pstCanvasInfo->phyAddr, pstCanvasInfo->stSize.u32Width, pstCanvasInfo->stSize.u32Height, pstCanvasInfo->u32Stride);

   //case 1: (pstCanvasInfo == NULL) && (hHandle != MI_RGN_HANDLE_NULL)
  /* if((NULL == pstCanvasInfo) && MI_RGN_HANDLE_NULL != (MI_S32)hHandle)
   {
       memset(&stCanvasInfo, 0x00, sizeof(MI_RGN_CanvasInfo_t));
       s32Ret = MI_RGN_GetCanvasInfo(hHandle, &stCanvasInfo);
       if(MI_RGN_OK != s32Ret)
       {
           MIXER_ERR("MI_RGN_GetCanvasInfo error, %X\n", s32Ret);
           pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
           return s32Ret;
       }

       pstCanvasInfo_local = &stCanvasInfo;
   }*/
   //case 2: (pstCanvasInfo != NULL) && (hHandle == MI_RGN_HANDLE_NULL)
   //else if((NULL != pstCanvasInfo)/* && (MI_RGN_HANDLE_NULL == (MI_S32)hHandle)*/)
   //{
       pstCanvasInfo_local = pstCanvasInfo;
   //}


   memset(&stOsdTextWidget, 0x00, sizeof(struct OsdTextWidget_t));
   stOsdTextWidget.imageData.pmt = pstTextWidgetAttr->pmt;

   if(pstTextWidgetAttr->string)
   {
       memset(u8Char, 0x00, sizeof(u8Char));
	   cpLen = sizeof(u8Char) > strlen(pstTextWidgetAttr->string) ? strlen(pstTextWidgetAttr->string) : sizeof(u8Char);
       memcpy(u8Char, pstTextWidgetAttr->string, cpLen);
       stOsdTextWidget.string = (MI_S8 *)u8Char;
   }

   switch(stOsdTextWidget.imageData.pmt)
   {
       case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
           if(E_MI_RGN_PIXEL_FORMAT_ARGB1555 != stOsdTextWidget.imageData.pmt)
           {
               MIXER_ERR("config the wrong poxer format, only support ARGB1555 now\n");
               pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
               return s32Ret;
           }

           if(pstTextWidgetAttr->pfColor)
           {
               stOsdTextWidget.fColor.a = pstTextWidgetAttr->pfColor->a;
               stOsdTextWidget.fColor.r = pstTextWidgetAttr->pfColor->r;
               stOsdTextWidget.fColor.g = pstTextWidgetAttr->pfColor->g;
               stOsdTextWidget.fColor.b = pstTextWidgetAttr->pfColor->b;
           }

           if(pstTextWidgetAttr->pbColor)
           {
               stOsdTextWidget.bColor.a = pstTextWidgetAttr->pbColor->a;
               stOsdTextWidget.bColor.r = pstTextWidgetAttr->pbColor->r;
               stOsdTextWidget.bColor.g = pstTextWidgetAttr->pbColor->g;
               stOsdTextWidget.bColor.b = pstTextWidgetAttr->pbColor->b;
           }
           break;

       case E_MI_RGN_PIXEL_FORMAT_I4:
           if(E_MI_RGN_PIXEL_FORMAT_I4 != stOsdTextWidget.imageData.pmt)
           {
               MIXER_ERR("config the wrong poxel format, only support I4 now\n");
               pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
               return s32Ret;
           }

           stColor.ePixelFmt = stOsdTextWidget.imageData.pmt;
           stColor.u32Color = pstTextWidgetAttr->u32Color;
           stOsdTextWidget.fColor.a = 0x00;
           stOsdTextWidget.fColor.r = stColor.u32Color & 0x0F;
           stOsdTextWidget.fColor.g = 0x00;
           stOsdTextWidget.fColor.b = 0x00;

           if(pstTextWidgetAttr->pbColor)
           {
               stOsdTextWidget.bColor.a = pstTextWidgetAttr->pbColor->a;
               stOsdTextWidget.bColor.r = pstTextWidgetAttr->pbColor->r;
               stOsdTextWidget.bColor.g = pstTextWidgetAttr->pbColor->g;
               stOsdTextWidget.bColor.b = pstTextWidgetAttr->pbColor->b;
           }
           break;

       case E_MI_RGN_PIXEL_FORMAT_I2:
           if(E_MI_RGN_PIXEL_FORMAT_I2 != stOsdTextWidget.imageData.pmt)
           {
               MIXER_ERR("config the wrong poxel format, only support I2 now\n");
               pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
               return s32Ret;
           }
           stColor.ePixelFmt = stOsdTextWidget.imageData.pmt;
           stColor.u32Color = pstTextWidgetAttr->u32Color;
           stOsdTextWidget.fColor.a = 0x00;
           stOsdTextWidget.fColor.r = stColor.u32Color & 0x0F;
           stOsdTextWidget.fColor.g = 0x00;
           stOsdTextWidget.fColor.b = 0x00;
           if(pstTextWidgetAttr->pbColor)
           {
               stOsdTextWidget.bColor.a = pstTextWidgetAttr->pbColor->a;
               stOsdTextWidget.bColor.r = pstTextWidgetAttr->pbColor->r;
               stOsdTextWidget.bColor.g = pstTextWidgetAttr->pbColor->g;
               stOsdTextWidget.bColor.b = pstTextWidgetAttr->pbColor->b;
           }
           break;
       default:
           MIXER_ERR("config Text widget pixel format: %d\n", stOsdTextWidget.imageData.pmt);
   }


   hzLen = mid_Font_Strlen(stOsdTextWidget.string);
   //nFontSize = font width *nMultiple, 8, 12, 16, 24, 32, 48...
   nFontSize = mid_Font_GetSizeByType(pstTextWidgetAttr->size);

   if(hzLen == 0)
   {
       pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
       return MI_SUCCESS;
   }


   //Height= font height
   stOsdTextWidget.imageData.height = nFontSize;
   if(E_MI_RGN_PIXEL_FORMAT_ARGB1555 == stOsdTextWidget.imageData.pmt)
   {
       //width = font width  * number of string * 2
       stOsdTextWidget.imageData.width  = nFontSize * hzLen * 2;
   }
   else if(E_MI_RGN_PIXEL_FORMAT_I4 == stOsdTextWidget.imageData.pmt)
   {
       //width = font width  * number of string / 2
       stOsdTextWidget.imageData.width  = nFontSize * hzLen / 2;
   }
   else if(E_MI_RGN_PIXEL_FORMAT_I2 == stOsdTextWidget.imageData.pmt)
   {
       stOsdTextWidget.imageData.width  = nFontSize * hzLen /4;
   }

   if(stOsdTextWidget.imageData.height != 0)
   {
       MI_U32 u32BuffSize_new = stOsdTextWidget.imageData.width * stOsdTextWidget.imageData.height;

       if(NULL == g_pbuffer)
       {
           if(NULL == (g_pbuffer = (MI_U8 *)malloc(u32BuffSize_new)))
           {
               pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
               return s32Ret;
           }
       }
       else if((NULL != g_pbuffer) && (u32BuffSize < u32BuffSize_new))
       {
           if(NULL == (g_pbuffer = (MI_U8 *)realloc(g_pbuffer, u32BuffSize_new)))
           {
               pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
               return s32Ret;
           }
       }

       u32BuffSize = u32BuffSize_new;
       stOsdTextWidget.imageData.buffer = g_pbuffer;
       memset(stOsdTextWidget.imageData.buffer, 0x00, u32BuffSize);
   }

   s32Ret = mid_Font_DrawText(&stOsdTextWidget.imageData, stOsdTextWidget.string, pstTextWidgetAttr->pPoint->x, pstTextWidgetAttr->size,
                              pstTextWidgetAttr->space, &stOsdTextWidget.fColor, &stOsdTextWidget.bColor, pstTextWidgetAttr->bOutline);
   if(MI_SUCCESS != s32Ret)
   {
       MIXER_ERR("mid_Font_DrawText error(0x%X)\n", s32Ret);
       pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
       return s32Ret;
   }

   switch(stOsdTextWidget.imageData.pmt)
   {
       case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
           {
               MI_U16 *dstBuf, *dstBuf1;
               MI_U16 *srcBuf, *srcBuf1;
               MI_U32 dd = 0;
               Point_t stPoint;

               stPoint.x = (pstTextWidgetAttr->pPoint->x % 2) ? (pstTextWidgetAttr->pPoint->x - 1) : pstTextWidgetAttr->pPoint->x;
               stPoint.y = (pstTextWidgetAttr->pPoint->y % 2) ? (pstTextWidgetAttr->pPoint->y - 1) : pstTextWidgetAttr->pPoint->y;

               srcBuf = (MI_U16*)stOsdTextWidget.imageData.buffer;
               dstBuf = (MI_U16*)((pstCanvasInfo_local->virtAddr) + pstCanvasInfo_local->u32Stride * stPoint.y) + stPoint.x;

               for(dd = 0; dd < ((pstCanvasInfo_local->stSize.u32Height > stOsdTextWidget.imageData.height)?stOsdTextWidget.imageData.height:pstCanvasInfo_local->stSize.u32Height-stPoint.y); dd++)
               {
                   srcBuf1 = srcBuf;
                   dstBuf1 = dstBuf;

                   for(int i = 0; i < stOsdTextWidget.imageData.width/2; i++)
                   {
                       *dstBuf1 = *srcBuf1;
#if 0
                       if(*srcBuf1 == 0x2323)
                        printf("*");
                       else
                        printf(".");
#endif
                       srcBuf1++;
                       dstBuf1++;
                   }

                   srcBuf += stOsdTextWidget.imageData.width/2;
                   dstBuf += pstCanvasInfo_local->u32Stride / 2;
               }
           }
           break;

       case E_MI_RGN_PIXEL_FORMAT_I4:
           {
               MI_U8 *dstBuf, *dstBuf1;
               MI_U8 *srcBuf, *srcBuf1;
               Point_t stPoint;

               stPoint.x = (pstTextWidgetAttr->pPoint->x % 2) ? (pstTextWidgetAttr->pPoint->x - 1) : pstTextWidgetAttr->pPoint->x;
               stPoint.y = (pstTextWidgetAttr->pPoint->y % 2) ? (pstTextWidgetAttr->pPoint->y - 1) : pstTextWidgetAttr->pPoint->y;

               srcBuf = (MI_U8*)stOsdTextWidget.imageData.buffer;
               dstBuf = (MI_U8*)((pstCanvasInfo_local->virtAddr) + pstCanvasInfo_local->u32Stride * stPoint.y + stPoint.x / 2);

               //printf("%s:%d srcBuf =%p, dstBuf =0x%x, pPoint->x=%d, pPoint->y=%d, pstCanvasInfo->u32Stride=%d\n",__func__,__LINE__,
               //              srcBuf, dstBuf, pstTextWidgetAttr->pPoint->x, pstTextWidgetAttr->pPoint->y, pstCanvasInfo_local->u32Stride);

               //printf("width=%d, height=%d, x=%d, y=%d\n",stOsdTextWidget.imageData.width,stOsdTextWidget.imageData.height,stPoint.x,stPoint.y);

               for(MI_U32 dd = 0; dd <  ((pstCanvasInfo_local->stSize.u32Height>stOsdTextWidget.imageData.height)?stOsdTextWidget.imageData.height:pstCanvasInfo_local->stSize.u32Height-stPoint.y); dd++)
               {
                   srcBuf1 = srcBuf + stOsdTextWidget.imageData.width * dd;
                   dstBuf1 = dstBuf + pstCanvasInfo_local->u32Stride  * dd;

                   //printf("%s:%d srcBuf1=%p, dstBuf1=%p, imageData.width=0x%x(%d), pstCanvasInfo->u32Stride=0x%x(%d)\n",__func__,__LINE__,srcBuf1, dstBuf1,
                   //        stOsdTextWidget.imageData.width, stOsdTextWidget.imageData.width, pstCanvasInfo_local->u32Stride, pstCanvasInfo_local->u32Stride);

                   memcpy(dstBuf1, srcBuf1, stOsdTextWidget.imageData.width - (stPoint.x / 2));

                   //printf("%s:%d srcBuf =%p, dstBuf =%p, imageData.width=%d, pstCanvasInfo->u32Stride=%d\n\n",__func__,__LINE__,
                   //          srcBuf, dstBuf, stOsdTextWidget.imageData.width, pstCanvasInfo_local->u32Stride);
               }

               if(0 && ((12 == nFontSize) || (16 == nFontSize)))
               {
                   dstBuf1 = dstBuf;
                   printf("dstBuf=%p, dstBuf1=%p, Stride=%d, pBmp->width=%d, pBmp->height=%d\n",dstBuf,dstBuf1,
                           pstCanvasInfo_local->u32Stride,stOsdTextWidget.imageData.width,stOsdTextWidget.imageData.height);

                   for(MI_U32 j = 0; j < ((pstCanvasInfo_local->stSize.u32Height>stOsdTextWidget.imageData.height)?stOsdTextWidget.imageData.height:pstCanvasInfo_local->stSize.u32Height-stPoint.y); j++)
                   {
                       dstBuf1 = dstBuf + pstCanvasInfo_local->u32Stride * j;

                       for(MI_U32 jj = 0; jj < pstCanvasInfo_local->u32Stride; jj++)
                       {
                           printf("%02x",dstBuf1[jj]);
                       }

                       printf("  dstBuf=%p, dstBuf1=%p\n", dstBuf, dstBuf1);
                   }
               }
           }
           break;
         case E_MI_RGN_PIXEL_FORMAT_I2:
           {
               MI_U8 *dstBuf, *dstBuf1;
               MI_U8 *srcBuf, *srcBuf1;
               Point_t stPoint;
               stPoint.x = (pstTextWidgetAttr->pPoint->x % 2) ? (pstTextWidgetAttr->pPoint->x - 1) : pstTextWidgetAttr->pPoint->x;
               stPoint.y = (pstTextWidgetAttr->pPoint->y % 2) ? (pstTextWidgetAttr->pPoint->y - 1) : pstTextWidgetAttr->pPoint->y;
               srcBuf = (MI_U8*)stOsdTextWidget.imageData.buffer;
               dstBuf = (MI_U8*)((pstCanvasInfo_local->virtAddr) + pstCanvasInfo_local->u32Stride * stPoint.y + stPoint.x / 4);
               for(MI_U32 dd = 0; dd < ((pstCanvasInfo_local->stSize.u32Height>stOsdTextWidget.imageData.height)?stOsdTextWidget.imageData.height:pstCanvasInfo_local->stSize.u32Height-stPoint.y); dd++)
               {
                   srcBuf1 = srcBuf;
                   dstBuf1 = dstBuf;
                   for(MI_U32 i = 0; i < stOsdTextWidget.imageData.width; i++)
                   {
                       *dstBuf1 = *srcBuf1;
#if 0
                       if((*srcBuf1&0x03) == 0)
                          printf(".");
                       else
                          printf("*");
                       if((*srcBuf1&0x0C)>>2== 0)
                          printf(".");
                       else
                          printf("*");
                       if((*srcBuf1&0x30 )>>4 == 0)
                          printf(".");
                       else
                          printf("*");
                       if((*srcBuf1&0xC0 )>>6== 0)
                          printf(".");
                       else
                          printf("*");
#endif
                       srcBuf1++;
                       dstBuf1++;
                   }
                   srcBuf += stOsdTextWidget.imageData.width;
                   dstBuf += pstCanvasInfo_local->u32Stride;
               }
           }
           break;

       default:
           MIXER_ERR("config Text widget pixel format: %d\n", stOsdTextWidget.imageData.pmt);
   }
   if((NULL == pstCanvasInfo) && (MI_RGN_HANDLE_NULL != (MI_S32)hHandle) && (visible))
   {
       s32Ret = MI_RGN_UpdateCanvas(hHandle);
       if(MI_RGN_OK != s32Ret)
       {
           MIXER_ERR("MI_RGN_UpdateCanvas error, %X\n", s32Ret);
           pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
           return E_MI_ERR_FAILED;
       }
   }

   pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);

   s32Ret = MI_SUCCESS;
   return s32Ret;
}

MI_S32 drawOsdText2Canvas(MI_RGN_CanvasInfo_t *pstCanvasInfo, ST_Point_T stPoint, MI_S8 *szString,MI_U8 u32Color)
{
    MI_U32 u32BuffSize = 0;
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_U32 hzLen = 0;
    MI_U32 nFontSize = 0;
    OsdFontSize_e eFont = MIXER_OSD_TEXT_MEDIUM_FONT;
    DrawRgnColor_t stColor;
    struct OsdTextWidget_t stOsdTextWidget;

    if(!szString || ('0' == szString[0])) return s32Ret;

    memset(&stOsdTextWidget, 0x00, sizeof(struct OsdTextWidget_t));
    stOsdTextWidget.imageData.pmt = E_MI_RGN_PIXEL_FORMAT_I4;
    stOsdTextWidget.string = szString;
    //while(*szString && *szString != ' ') szString++;
    while(*szString && *szString != 0x0) szString++;
    *szString = 0;

    stColor.ePixelFmt = stOsdTextWidget.imageData.pmt;
    stColor.u32Color = u32Color;   //0:transparent; 1:red; 2:green; 3:blue;
    stOsdTextWidget.fColor.a = 0x00;
    stOsdTextWidget.fColor.r = stColor.u32Color & 0x0F;
    stOsdTextWidget.fColor.g = 0x00;
    stOsdTextWidget.fColor.b = 0x00;

    hzLen = mid_Font_Strlen(stOsdTextWidget.string);
    //nFontSize = font width *nMultiple, 8, 12, 16, 24, 32, 48...
    if(hzLen == 0){
        return E_MI_ERR_FAILED;
    }
    nFontSize = mid_Font_GetSizeByType(eFont);
    //Height= font height
    stOsdTextWidget.imageData.height = nFontSize;
    //width = font width  * number of string / 2
    stOsdTextWidget.imageData.width  = nFontSize * hzLen / 2;
    if(stOsdTextWidget.imageData.height != 0)
    {
        u32BuffSize = stOsdTextWidget.imageData.width * stOsdTextWidget.imageData.height;
        stOsdTextWidget.imageData.buffer = (MI_U8 *)malloc(u32BuffSize);
    }

    if(!stOsdTextWidget.imageData.buffer){
        return E_MI_ERR_FAILED;
    }
    memset(stOsdTextWidget.imageData.buffer, 0x00, u32BuffSize);
    s32Ret = mid_Font_DrawText(&stOsdTextWidget.imageData, stOsdTextWidget.string, stPoint.u32X, eFont,
                               0, &stOsdTextWidget.fColor, &stOsdTextWidget.bColor, FALSE);
    if(MI_SUCCESS != s32Ret)
    {
        if(stOsdTextWidget.imageData.buffer){
            free(stOsdTextWidget.imageData.buffer);
            stOsdTextWidget.imageData.buffer = NULL;
        }

        MIXER_ERR("mid_Font_DrawText error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    MI_U8 *dstBuf, *dstBuf1;
    MI_U8 *srcBuf, *srcBuf1;

    stPoint.u32X = (stPoint.u32X % 2) ? (stPoint.u32X - 1) : stPoint.u32X;
    stPoint.u32Y = (stPoint.u32Y % 2) ? (stPoint.u32Y - 1) : stPoint.u32Y;

    srcBuf = (MI_U8*)stOsdTextWidget.imageData.buffer;
    dstBuf = (MI_U8*)((pstCanvasInfo->virtAddr) + pstCanvasInfo->u32Stride * stPoint.u32Y + stPoint.u32X / 2);

    for(MI_U32 dd = 0; dd < stOsdTextWidget.imageData.height; dd++)
    {
        srcBuf1 = srcBuf + stOsdTextWidget.imageData.width * dd;
        dstBuf1 = dstBuf + pstCanvasInfo->u32Stride  * dd;
        memcpy(dstBuf1, srcBuf1, stOsdTextWidget.imageData.width);
    }

    if(stOsdTextWidget.imageData.buffer){
        free(stOsdTextWidget.imageData.buffer);
        stOsdTextWidget.imageData.buffer = NULL;
    }

    return s32Ret;
}
MI_S32 updateOsdTextWidgetI2_To_I8(MI_RGN_HANDLE hHandle, I2ToI8WidgetAttr_t* pstWidgetAttr, BOOL visible,MI_U8 align)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    if(NULL == pstWidgetAttr->buf)
    {
        printf("%s %d arg fail, buf==NULL\n", __func__,__LINE__);
        return E_MI_ERR_NULL_PTR;
    }

    if(pstWidgetAttr->buf_length != (pstWidgetAttr->size.u16Width * pstWidgetAttr->size.u16Height/align))
    {
        printf("%s %d arg fail,buf_length!=(pstI2WidgetAttr->width * pstI2WidgetAttr->height/%d)\n",__func__,__LINE__,align);
        return E_MI_ERR_FAILED;
    }
    memset(&stCanvasInfo, 0, sizeof(MI_RGN_CanvasInfo_t));
    s32Ret = MI_RGN_GetCanvasInfo(hHandle, &stCanvasInfo);
    if (MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetCanvasInfo() error, %X  hHandle=%d\n", s32Ret,hHandle);
        return s32Ret;
    }

    if(TRUE == visible)
    {
        MI_U8 *dstBuf, *dstBuf1;
        MI_U8 *srcBuf, *srcBuf1;
        Point_t stPoint;

        stPoint.x = (pstWidgetAttr->pPoint.x % align) ? ((pstWidgetAttr->pPoint.x / align)*align) : pstWidgetAttr->pPoint.x;
        stPoint.y = (pstWidgetAttr->pPoint.y % align) ? ((pstWidgetAttr->pPoint.y / align)*align) : pstWidgetAttr->pPoint.y;

        srcBuf = (MI_U8*)pstWidgetAttr->buf;
        dstBuf = (MI_U8*)((stCanvasInfo.virtAddr) + stCanvasInfo.u32Stride * stPoint.y) + stPoint.x/align;

        for(MI_U32 dd = 0; dd < pstWidgetAttr->size.u16Height; dd++)
        {
            srcBuf1 = srcBuf;
            dstBuf1 = dstBuf;

            for(MI_U32 i = 0; i < pstWidgetAttr->size.u16Width / align; i++)
            {
                *dstBuf1 = *srcBuf1;

                srcBuf1++;
                dstBuf1++;
            }

            srcBuf += pstWidgetAttr->size.u16Width/align;
            dstBuf += stCanvasInfo.u32Stride;
        }
    }

    if(MI_RGN_OK != MI_RGN_UpdateCanvas(hHandle))
    {
        MIXER_ERR("MI_RGN_UpdateCanvas() fail\n");
    }
    s32Ret = MI_RGN_OK;
    return s32Ret;
}

MI_S32 createOsdRectWidget(MI_RGN_HANDLE hHandle, MI_RGN_Attr_t *pstRgnAttr, MI_RGN_ChnPort_t *pstRgnChnPort, MI_RGN_ChnPortParam_t *pstRgnChnPortParam)
{
   MI_S32 s32Ret = E_MI_ERR_FAILED;

   if(MI_RGN_MAX_HANDLE < hHandle)
   {
       MIXER_ERR("The input Rgn handle(%d) is out of range!\n", hHandle);
       s32Ret = E_MI_ERR_ILLEGAL_PARAM;
       return s32Ret;
   }

   if((NULL == pstRgnAttr) || (NULL == pstRgnChnPort) || (NULL == pstRgnChnPortParam))
   {
       MIXER_ERR("createOsdRectWidget() the input pointer is NULL!\n");
       s32Ret = E_MI_ERR_NULL_PTR;
       return s32Ret;
   }


   s32Ret = MI_RGN_Create(hHandle, pstRgnAttr);
   if(MI_RGN_OK != s32Ret)
   {
       MIXER_ERR("MI_RGN_Create error(0x%X)\n", s32Ret);
       printf("%s:%d  Hdl=%d RGN_Attr:Type=%d, Width=%4d, Heitht=%4d, fmt=%d\n", __func__, __LINE__, hHandle, pstRgnAttr->eType,
              pstRgnAttr->stOsdInitParam.stSize.u32Width, pstRgnAttr->stOsdInitParam.stSize.u32Height, pstRgnAttr->stOsdInitParam.ePixelFmt);
       return s32Ret;
   }

   s32Ret = MI_RGN_AttachToChn(hHandle, pstRgnChnPort, pstRgnChnPortParam);
   if(MI_RGN_OK != s32Ret)
   {
       MI_RGN_Destroy(hHandle);
       MIXER_ERR("MI_RGN_AttachToChn error(0x%X)\n", s32Ret);
       printf("%s:%d  Hdl=%d RGN_Attr:Type=%d, Width=%4d, Heitht=%4d, fmt=%d\n", __func__, __LINE__, hHandle, pstRgnAttr->eType,
              pstRgnAttr->stOsdInitParam.stSize.u32Width, pstRgnAttr->stOsdInitParam.stSize.u32Height, pstRgnAttr->stOsdInitParam.ePixelFmt);
       printf("%s:%d  Hdl=%d RGN:ModId=%d, DevId=%d, ChnId=%d, PortId=%d\n", __func__, __LINE__, hHandle, pstRgnChnPort->eModId,
                      pstRgnChnPort->s32DevId, pstRgnChnPort->s32ChnId, pstRgnChnPort->s32OutputPortId);
       printf("%s:%d  Hdl=%d canvas:x=%d, y=%d, InvertColorMode=%d, DivNum=%d, DivNum=%d, Threshold=%d\n", __func__, __LINE__, hHandle,
                      pstRgnChnPortParam->stPoint.u32X, pstRgnChnPortParam->stPoint.u32Y,
                      pstRgnChnPortParam->unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode,
                      pstRgnChnPortParam->unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum,
                      pstRgnChnPortParam->unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum,
                      pstRgnChnPortParam->unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold);
       return s32Ret;
   }
   //printf("RectHdl=%d canvas:x=%d, y=%d, w=%4d, h=%4d, fmt=%d\n", hHandle, pstRgnChnPortParam->stPoint.u32X, pstRgnChnPortParam->stPoint.u32Y,
   //       pstRgnAttr->stOsdInitParam.stSize.u32Width, pstRgnAttr->stOsdInitParam.stSize.u32Height, pstRgnAttr->stOsdInitParam.ePixelFmt);
   s32Ret = MI_RGN_OK;
   return s32Ret;
}
#if TARGET_CHIP_I6E
static MI_S32 updateOsdRectWidgetTmp(const MI_RGN_CanvasInfo_t *pRgnInfo, RectWidgetAttr_t *pstRectWidgetAttr)
{
   MI_S32 s32Ret = E_MI_ERR_FAILED;
   MI_S32 i = 0;
   DrawRgnColor_t stColor;
   DrawPoint_t stLefTopPt;
   DrawPoint_t stRightBottomPt;
   MI_RGN_CanvasInfo_t stRgnCanvasInfo;
   MI_SYS_WindowRect_t *pstRectTmp;

   if(NULL == pRgnInfo)
   {
       s32Ret = E_MI_ERR_ILLEGAL_PARAM;
       return s32Ret;
   }

   if(NULL == pstRectWidgetAttr)
   {
       MIXER_ERR("updateOsdRectWidget() the input pointer is NULL!\n");
       s32Ret = E_MI_ERR_NULL_PTR;
       return s32Ret;
   }

 //  pthread_mutex_lock(&g_stMutexMixerOsdTextRgnBuf);

    memcpy(&stRgnCanvasInfo, pRgnInfo, sizeof(MI_RGN_CanvasInfo_t));
   stColor.ePixelFmt = stRgnCanvasInfo.ePixelFmt;

   for (i = 0; i < pstRectWidgetAttr->s32RectCnt; i++)
   {
       pstRectTmp = (MI_SYS_WindowRect_t *)((pstRectWidgetAttr->pstRect) + i);
       if(pstRectTmp->u16Width == 0 || pstRectTmp->u16Height == 0)
           continue;
       stLefTopPt.u16X = pstRectTmp->u16X;
       stLefTopPt.u16Y = pstRectTmp->u16Y;
       stRightBottomPt.u16X = pstRectTmp->u16X + pstRectTmp->u16Width;
       stRightBottomPt.u16Y = pstRectTmp->u16Y + pstRectTmp->u16Height;

     /*  MIXER_DBG("Rect[%d]:x=%d, y=%d, w=%d, h=%d, fmt=%s\n", i, stLefTopPt.u16X, stLefTopPt.u16Y,
                  stRightBottomPt.u16X - stLefTopPt.u16X, stRightBottomPt.u16Y - stLefTopPt.u16Y, (0==pstRectWidgetAttr->pmt)?"ARGB1555":"I4");
    */
       switch(pstRectWidgetAttr->pmt)
       {
           case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
               if(E_MI_RGN_PIXEL_FORMAT_ARGB1555 != stRgnCanvasInfo.ePixelFmt)
               {
                   MIXER_ERR("config the wrong poxer format, only support ARGB1555 now\n");
                 //  pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
                   return s32Ret;
               }

               stColor.u32Color = RGB2PIXEL1555(pstRectWidgetAttr->pfColor->a, pstRectWidgetAttr->pfColor->r,
                                                pstRectWidgetAttr->pfColor->g, pstRectWidgetAttr->pfColor->b);
               break;

           case E_MI_RGN_PIXEL_FORMAT_I4:
               if(E_MI_RGN_PIXEL_FORMAT_I4 != stRgnCanvasInfo.ePixelFmt)
               {
                   MIXER_ERR("config the wrong poxer format, only support I4 now\n");
                  // pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
                   return s32Ret;
               }

               stLefTopPt.u16X = MIXER_ALIGN_DOWN(stLefTopPt.u16X, 2);
               stRightBottomPt.u16X = MIXER_ALIGN_UP(stRightBottomPt.u16X, 2);
               stColor.u32Color = pstRectWidgetAttr->u32Color;
               break;

           case E_MI_RGN_PIXEL_FORMAT_I2:
               if(E_MI_RGN_PIXEL_FORMAT_I2 != stRgnCanvasInfo.ePixelFmt)
               {
                   MIXER_ERR("config the wrong poxer format, only support I2 now\n");
                 //  pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
                   return s32Ret;
               }
               stLefTopPt.u16X = MIXER_ALIGN_DOWN(stLefTopPt.u16X, 4);
               stRightBottomPt.u16X = MIXER_ALIGN_UP(stRightBottomPt.u16X, 4);
               stColor.u32Color = pstRectWidgetAttr->u32Color;
               break;
           default:
               MIXER_ERR("OSD only support %s now\n", (0==pstRectWidgetAttr->pmt)?"ARGB1555":(2==pstRectWidgetAttr->pmt)?"I2":"I4");
       }

       DrawRect((void*)stRgnCanvasInfo.virtAddr, stRgnCanvasInfo.u32Stride, stLefTopPt, stRightBottomPt, pstRectWidgetAttr->u8BorderWidth, stColor);
   }

   s32Ret = MI_RGN_OK;
 //  pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);

   return s32Ret;
}
#endif
MI_S32 updateOsdRectWidget(MI_RGN_HANDLE hHandle, RectWidgetAttr_t *pstRectWidgetAttr, BOOL visible)
{
   MI_S32 s32Ret = E_MI_ERR_FAILED;
   MI_S32 i = 0;
   DrawRgnColor_t stColor;
   DrawPoint_t stLefTopPt;
   DrawPoint_t stRightBottomPt;
   MI_RGN_CanvasInfo_t stRgnCanvasInfo;
   MI_SYS_WindowRect_t *pstRectTmp;


   if(hHandle >= MI_RGN_MAX_HANDLE)
   {
       MIXER_ERR("The input Rgn handle(%d) is out of range[0-%d]!\n", hHandle,MI_RGN_MAX_HANDLE);
       s32Ret = E_MI_ERR_ILLEGAL_PARAM;
       return s32Ret;
   }

   if(NULL == pstRectWidgetAttr)
   {
       MIXER_ERR("updateOsdRectWidget() the input pointer is NULL!\n");
       s32Ret = E_MI_ERR_NULL_PTR;
       return s32Ret;
   }

   pthread_mutex_lock(&g_stMutexMixerOsdTextRgnBuf);

   memset(&stRgnCanvasInfo, 0, sizeof(stRgnCanvasInfo));
   if((s32Ret =MI_RGN_GetCanvasInfo(hHandle, &stRgnCanvasInfo)) != MI_RGN_OK)
   {
       MIXER_ERR("MI_RGN_GetCanvasInfo fail\n");
       pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
       return s32Ret;
   }
   stColor.ePixelFmt = stRgnCanvasInfo.ePixelFmt;

   for (i = 0; i < pstRectWidgetAttr->s32RectCnt; i++)
   {
       pstRectTmp = (MI_SYS_WindowRect_t *)((pstRectWidgetAttr->pstRect) + i);
       if(pstRectTmp->u16Width == 0 || pstRectTmp->u16Height == 0) continue;
       stLefTopPt.u16X = pstRectTmp->u16X;
       stLefTopPt.u16Y = pstRectTmp->u16Y;
       stRightBottomPt.u16X = pstRectTmp->u16X + pstRectTmp->u16Width;
       stRightBottomPt.u16Y = pstRectTmp->u16Y + pstRectTmp->u16Height;

       MIXER_INFO("Rgn %d Rect[%d]:x=%d, y=%d, w=%d, h=%d, fmt=%s\n", hHandle, i, stLefTopPt.u16X, stLefTopPt.u16Y,
                  stRightBottomPt.u16X - stLefTopPt.u16X, stRightBottomPt.u16Y - stLefTopPt.u16Y, (0==pstRectWidgetAttr->pmt)?"ARGB1555":"I4");

       switch(pstRectWidgetAttr->pmt)
       {
           case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
               if(E_MI_RGN_PIXEL_FORMAT_ARGB1555 != stRgnCanvasInfo.ePixelFmt)
               {
                   MIXER_ERR("config the wrong poxer format, only support ARGB1555 now\n");
                   pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
                   return s32Ret;
               }

               stColor.u32Color = RGB2PIXEL1555(pstRectWidgetAttr->pfColor->a, pstRectWidgetAttr->pfColor->r,
                                                pstRectWidgetAttr->pfColor->g, pstRectWidgetAttr->pfColor->b);
               break;

           case E_MI_RGN_PIXEL_FORMAT_I4:
               if(E_MI_RGN_PIXEL_FORMAT_I4 != stRgnCanvasInfo.ePixelFmt)
               {
                   MIXER_ERR("config the wrong poxer format, only support I4 now\n");
                   pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
                   return s32Ret;
               }

               stLefTopPt.u16X = MIXER_ALIGN_DOWN(stLefTopPt.u16X, 2);
               stRightBottomPt.u16X = MIXER_ALIGN_UP(stRightBottomPt.u16X, 2);
               stColor.u32Color = 1 + pstRectWidgetAttr->u32Color;
               break;

           case E_MI_RGN_PIXEL_FORMAT_I2:
               if(E_MI_RGN_PIXEL_FORMAT_I2 != stRgnCanvasInfo.ePixelFmt)
               {
                   MIXER_ERR("config the wrong poxer format, only support I2 now\n");
                   pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
                   return s32Ret;
               }
               stLefTopPt.u16X = MIXER_ALIGN_DOWN(stLefTopPt.u16X, 4);
               stRightBottomPt.u16X = MIXER_ALIGN_UP(stRightBottomPt.u16X, 4);
               stColor.u32Color = 1 + pstRectWidgetAttr->u32Color;
               break;
           default:
               MIXER_ERR("OSD only support %s now\n", (0==pstRectWidgetAttr->pmt)?"ARGB1555":(2==pstRectWidgetAttr->pmt)?"I2":"I4");
       }

       DrawRect((void*)stRgnCanvasInfo.virtAddr, stRgnCanvasInfo.u32Stride, stLefTopPt, stRightBottomPt, pstRectWidgetAttr->u8BorderWidth, stColor);
   }

   if(visible)
   {
      if(MI_RGN_UpdateCanvas(hHandle) != MI_RGN_OK)
      {
           MIXER_ERR("MI_RGN_UpdateCanvas fail\n");
      }
   }

   s32Ret = MI_RGN_OK;
   pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);

   return s32Ret;
}
MI_S32 cleanOsdTextWidgettmp(MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, MI_SYS_WindowRect_t *pstRect, MI_U32 index)
{
   MI_S32 s32Ret = E_MI_ERR_FAILED;
   MI_SYS_WindowRect_t stRect;
   MI_RGN_CanvasInfo_t stRgnCanvasInfo;
   MI_RGN_CanvasInfo_t *pstRgnCanvasInfo = NULL;

   //pthread_mutex_lock(&g_stMutexMixerOsdTextRgnBuf);
   if((NULL != pstCanvasInfo) && ((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == hHandle))
   {
       pstRgnCanvasInfo = pstCanvasInfo;
   }
   else if((NULL == pstCanvasInfo) && ((0 <= hHandle) && (hHandle < MI_RGN_MAX_HANDLE)))
   {
       memset(&stRgnCanvasInfo, 0x00, sizeof(stRgnCanvasInfo));
       s32Ret = MI_RGN_GetCanvasInfo(hHandle, &stRgnCanvasInfo);
       if(MI_RGN_OK != s32Ret)
       {
           printf("%s:%d get the CanvasInfo of the Rgn Handle(%d) is error[%X]!\n", __func__, __LINE__, hHandle,s32Ret);
          // pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
           return s32Ret;
       }

       pstRgnCanvasInfo = &stRgnCanvasInfo;
   }
   else
   {
       printf("%s:%d the Rgn Handle and the CanvasInfo pointer can not be invalid at the same time!\n", __func__, __LINE__);
     //  pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
       return s32Ret;
   }

   if((NULL == pstRect) || ((0 == pstRect->u16Width) || (0 == pstRect->u16Height)))
   {
       stRect.u16X = 0;
       stRect.u16Y = 0;
       stRect.u16Width  = pstRgnCanvasInfo->stSize.u32Width;
       stRect.u16Height = pstRgnCanvasInfo->stSize.u32Height;
   }
   else
   {
       stRect.u16X = pstRect->u16X;
       stRect.u16Y = pstRect->u16Y;
       stRect.u16Width  = pstRect->u16Width;
       stRect.u16Height = pstRect->u16Height;
   }

   switch(pstRgnCanvasInfo->ePixelFmt)
   {
       case E_MI_RGN_PIXEL_FORMAT_I2:
           {
               MI_U32 dd = 0, i = 0;
               MI_U8 *pu8BaseAddr = NULL;
               MIXER_DBG("Rgn %d clean canvas:x=%d, y=%d, w=%d, h=%d, stride=%d, fmt=I4\n", hHandle, stRect.u16X,
                                               stRect.u16Y, stRect.u16Width, stRect.u16Height, pstRgnCanvasInfo->u32Stride);
              if(((stRect.u16X + stRect.u16Width)  <= pstRgnCanvasInfo->stSize.u32Width) && \
                 ((stRect.u16Y + stRect.u16Height) <= pstRgnCanvasInfo->stSize.u32Height))
               {
                   for(dd = 0; dd < stRect.u16Height; dd++)
                   {
                       pu8BaseAddr = (MI_U8*)(pstRgnCanvasInfo->virtAddr + pstRgnCanvasInfo->u32Stride*(dd + stRect.u16Y) + stRect.u16X / 4);
                       for(i = 0; i < stRect.u16Width / 4; i++)
                       {
                           *(pu8BaseAddr + i) = ((index & 0x03) << 6) |((index & 0x03) << 4) |((index & 0x03) << 2) |(index & 0x03);
                       }
                   }
               }
            }
           break;

       case E_MI_RGN_PIXEL_FORMAT_I4:
           {
               MI_U32 dd = 0, i = 0;
               MI_U8 *pu8BaseAddr = NULL;

   //            MIXER_DBG("Rgn %d clean canvas:x=%d, y=%d, w=%d, h=%d, stride=%d, fmt=I4\n", hHandle, stRect.u16X,
   //                                           stRect.u16Y, stRect.u16Width, stRect.u16Height, pstRgnCanvasInfo->u32Stride);

              if(((stRect.u16X + stRect.u16Width)  <= pstRgnCanvasInfo->stSize.u32Width) && \
                 ((stRect.u16Y + stRect.u16Height) <= pstRgnCanvasInfo->stSize.u32Height))
               {
                   for(dd = 0; dd < stRect.u16Height; dd++)
                   {
                       pu8BaseAddr = (MI_U8*)(pstRgnCanvasInfo->virtAddr + pstRgnCanvasInfo->u32Stride*(dd + stRect.u16Y) + stRect.u16X / 2);

                       for(i = 0; i < stRect.u16Width / 2; i++)
                       {
                           *(pu8BaseAddr + i) = ((index & 0x0F) << 4) | (index & 0x0F);
                       }
                   }
               }
           }
           break;

       case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
           {
               MI_U32 dd = 0, i = 0;
               MI_U16 *pu16BaseAddr = NULL;

               MIXER_DBG("Rgn %d clean canvas:x=%d, y=%d, w=%d, h=%d, stride=%d, fmt=ARGB1555\n", hHandle, stRect.u16X,
                                               stRect.u16Y, stRect.u16Width, stRect.u16Height, pstRgnCanvasInfo->u32Stride);

               if(((stRect.u16X + stRect.u16Width)  <= pstRgnCanvasInfo->stSize.u32Width) && \
                  ((stRect.u16Y + stRect.u16Height) <= pstRgnCanvasInfo->stSize.u32Height))
               {
                   for(dd = 0; dd < stRect.u16Height; dd++)
                   {
                       pu16BaseAddr = (MI_U16*)(pstRgnCanvasInfo->virtAddr + pstRgnCanvasInfo->u32Stride*(dd + stRect.u16Y) + stRect.u16X * 2);

                       for(i = 0; i < stRect.u16Width; i++)
                       {
                           *(pu16BaseAddr + i) = index & 0xFFFF;
                       }
                   }
               }
           }
           break;

       default:
           MIXER_ERR("only support %s now\n", (0==pstRgnCanvasInfo->ePixelFmt)?"ARGB1555":(2==pstRgnCanvasInfo->ePixelFmt)?"I2":"I4");
          // pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
           return s32Ret;
   }

   if((NULL == pstCanvasInfo) && ((0 <= hHandle) || (hHandle < MI_RGN_MAX_HANDLE)))
   {
       if(MI_RGN_UpdateCanvas(hHandle) != MI_RGN_OK)
       {
           MIXER_ERR("MI_RGN_UpdateCanvas fail\n");
       }
   }

   s32Ret = MI_RGN_OK;
  // pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);

   return s32Ret;
}

MI_S32 cleanOsdTextWidget(MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, MI_SYS_WindowRect_t *pstRect, MI_U32 index)
{
   MI_S32 s32Ret = E_MI_ERR_FAILED;
   MI_SYS_WindowRect_t stRect;
   MI_RGN_CanvasInfo_t stRgnCanvasInfo;
   MI_RGN_CanvasInfo_t *pstRgnCanvasInfo = NULL;

   pthread_mutex_lock(&g_stMutexMixerOsdTextRgnBuf);
   if((NULL != pstCanvasInfo) && ((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == hHandle))
   {
       pstRgnCanvasInfo = pstCanvasInfo;
   }
   else if((NULL == pstCanvasInfo) && ((0 <= hHandle) && (hHandle < MI_RGN_MAX_HANDLE)))
   {
       memset(&stRgnCanvasInfo, 0x00, sizeof(stRgnCanvasInfo));
       s32Ret = MI_RGN_GetCanvasInfo(hHandle, &stRgnCanvasInfo);
       if(MI_RGN_OK != s32Ret)
       {
           printf("%s:%d get the CanvasInfo of the Rgn Handle(%d) is error[%X]!\n", __func__, __LINE__, hHandle,s32Ret);
           pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
           return s32Ret;
       }

       pstRgnCanvasInfo = &stRgnCanvasInfo;
   }
   else
   {
       printf("%s:%d the Rgn Handle and the CanvasInfo pointer can not be invalid at the same time!\n", __func__, __LINE__);
       pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
       return s32Ret;
   }

   if((NULL == pstRect) || ((0 == pstRect->u16Width) || (0 == pstRect->u16Height)))
   {
       stRect.u16X = 0;
       stRect.u16Y = 0;
       stRect.u16Width  = pstRgnCanvasInfo->stSize.u32Width;
       stRect.u16Height = pstRgnCanvasInfo->stSize.u32Height;
   }
   else
   {
       stRect.u16X = pstRect->u16X;
       stRect.u16Y = pstRect->u16Y;
       stRect.u16Width  = pstRect->u16Width;
       stRect.u16Height = pstRect->u16Height;
   }

   switch(pstRgnCanvasInfo->ePixelFmt)
   {
       case E_MI_RGN_PIXEL_FORMAT_I2:
           {
               MI_U32 dd = 0, i = 0;
               MI_U8 *pu8BaseAddr = NULL;
               MIXER_INFO("Rgn %d clean canvas:x=%d, y=%d, w=%d, h=%d, stride=%d, fmt=I4\n", hHandle, stRect.u16X,
                                               stRect.u16Y, stRect.u16Width, stRect.u16Height, pstRgnCanvasInfo->u32Stride);
              if(((stRect.u16X + stRect.u16Width)  <= pstRgnCanvasInfo->stSize.u32Width) && \
                 ((stRect.u16Y + stRect.u16Height) <= pstRgnCanvasInfo->stSize.u32Height))
               {
                   for(dd = 0; dd < stRect.u16Height; dd++)
                   {
                       pu8BaseAddr = (MI_U8*)(pstRgnCanvasInfo->virtAddr + pstRgnCanvasInfo->u32Stride*(dd + stRect.u16Y) + stRect.u16X / 4);
                       for(i = 0; i < stRect.u16Width / 4; i++)
                       {
                           *(pu8BaseAddr + i) = ((index & 0x03) << 6) |((index & 0x03) << 4) |((index & 0x03) << 2) |(index & 0x03);
                       }
                   }
               }
            }
           break;

       case E_MI_RGN_PIXEL_FORMAT_I4:
           {
               MI_U32 dd = 0, i = 0;
               MI_U8 *pu8BaseAddr = NULL;

               MIXER_INFO("Rgn %d clean canvas:x=%d, y=%d, w=%d, h=%d, stride=%d, fmt=I4\n", hHandle, stRect.u16X,
                                               stRect.u16Y, stRect.u16Width, stRect.u16Height, pstRgnCanvasInfo->u32Stride);

              if(((stRect.u16X + stRect.u16Width)  <= pstRgnCanvasInfo->stSize.u32Width) && \
                 ((stRect.u16Y + stRect.u16Height) <= pstRgnCanvasInfo->stSize.u32Height))
               {
                   for(dd = 0; dd < stRect.u16Height; dd++)
                   {
                       pu8BaseAddr = (MI_U8*)(pstRgnCanvasInfo->virtAddr + pstRgnCanvasInfo->u32Stride*(dd + stRect.u16Y) + stRect.u16X / 2);

                       for(i = 0; i < stRect.u16Width / 2; i++)
                       {
                           *(pu8BaseAddr + i) = ((index & 0x0F) << 4) | (index & 0x0F);
                       }
                   }
               }
           }
           break;

       case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
           {
               MI_U32 dd = 0, i = 0;
               MI_U16 *pu16BaseAddr = NULL;

               MIXER_INFO("Rgn %d clean canvas:x=%d, y=%d, w=%d, h=%d, stride=%d, fmt=ARGB1555\n", hHandle, stRect.u16X,
                                               stRect.u16Y, stRect.u16Width, stRect.u16Height, pstRgnCanvasInfo->u32Stride);

               if(((stRect.u16X + stRect.u16Width)  <= pstRgnCanvasInfo->stSize.u32Width) && \
                  ((stRect.u16Y + stRect.u16Height) <= pstRgnCanvasInfo->stSize.u32Height))
               {
                   for(dd = 0; dd < stRect.u16Height; dd++)
                   {
                       pu16BaseAddr = (MI_U16*)(pstRgnCanvasInfo->virtAddr + pstRgnCanvasInfo->u32Stride*(dd + stRect.u16Y) + stRect.u16X * 2);

                       for(i = 0; i < stRect.u16Width; i++)
                       {
                           *(pu16BaseAddr + i) = index & 0xFFFF;
                       }
                   }
               }
           }
           break;

       default:
           MIXER_ERR("only support %s now\n", (0==pstRgnCanvasInfo->ePixelFmt)?"ARGB1555":(2==pstRgnCanvasInfo->ePixelFmt)?"I2":"I4");
           pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
           return s32Ret;
   }

   if((NULL == pstCanvasInfo) && ((0 <= hHandle) || (hHandle < MI_RGN_MAX_HANDLE)))
   {
       if(MI_RGN_UpdateCanvas(hHandle) != MI_RGN_OK)
       {
           MIXER_ERR("MI_RGN_UpdateCanvas fail\n");
       }
   }

   s32Ret = MI_RGN_OK;
   pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);

   return s32Ret;
}

MI_S32 cleanOsdRectWidget(MI_RGN_HANDLE hHandle, MI_SYS_WindowRect_t *pRect, int rectNum, MI_U8 u8BorderWidth, MI_U32 index)
{
   MI_S32 s32Ret = E_MI_ERR_FAILED;
   int idx = 0;
   MI_RGN_CanvasInfo_t stRgnCanvasInfo;
   MI_U32 i = 0, j = 0;
   MI_SYS_WindowRect_t rect;
   MI_U32 u32Width = 0;
   MI_U32 u32Height = 0;

   if((hHandle >= MI_RGN_MAX_HANDLE) || rectNum <= 0)
   {
       return s32Ret;
   }

   pthread_mutex_lock(&g_stMutexMixerOsdTextRgnBuf);

   memset(&stRgnCanvasInfo, 0x00, sizeof(stRgnCanvasInfo));
   if(MI_RGN_OK != MI_RGN_GetCanvasInfo(hHandle, &stRgnCanvasInfo))
   {
       printf("%s:%d call MI_RGN_GetCanvasInfo() fail!\n", __func__,__LINE__);
       pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
       return s32Ret;
   }

   for(idx = 0; idx < rectNum; idx ++)
   {
       rect = pRect[idx];
       if(rect.u16Width == 0 || rect.u16Height == 0) continue;
       u32Width = rect.u16Width + 1;
       u32Height = rect.u16Height + 1;

       switch(stRgnCanvasInfo.ePixelFmt)
       {
           case E_MI_RGN_PIXEL_FORMAT_I2:
              {
                MI_U8 *pDrawBase = (MI_U8*)stRgnCanvasInfo.virtAddr;
                DrawPoint_t stLeftTopPt, stRightBottomPt;
                stLeftTopPt.u16X = MIXER_ALIGN_DOWN(rect.u16X, 4);
                stLeftTopPt.u16Y = rect.u16Y;
                stRightBottomPt.u16X = MIXER_ALIGN_UP(rect.u16X + rect.u16Width,4);
                stRightBottomPt.u16Y = rect.u16Y + rect.u16Height;
                MI_U32 u32Stride = stRgnCanvasInfo.u32Stride;
                if (stLeftTopPt.u16X%4 || stRightBottomPt.u16X%4 || (u8BorderWidth > u32Width/4) || (u8BorderWidth > u32Height/4))
                {
                    printf("invalid rect position\n");
                    pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
                    return s32Ret;
                }
                for (i = 0; i < u32Width/4; i++)
                {
                    for (j = 0; j < u8BorderWidth && ((stLeftTopPt.u16X/4 + i) < u32Stride); j++)
                    {
                        *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+j)+stLeftTopPt.u16X/4+i) = (index&0x03) | ((index&0x03) << 2) | ((index&0x03) << 4) | ((index&0x03) << 6);     // copy 1 byte
                        *(pDrawBase+u32Stride*(stRightBottomPt.u16Y-j)+stLeftTopPt.u16X/4+i) = (index&0x03) | ((index&0x03) << 2) | ((index&0x03) << 4) | ((index&0x03) << 6); // copy 1 byte
                    }
                }
                for (i = 0; i < u32Height; i++)
                {
                    for (j = 0; j < u8BorderWidth/4; j++)
                    {
                        if((stLeftTopPt.u16X/4 + j) < u32Stride)
                        {
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/4+j) = (index&0x03) | ((index&0x03) << 2) | ((index&0x03) << 4) | ((index&0x03) << 6);    // copy 1 byte
                        }
                        if((stRightBottomPt.u16X/4 - j) < u32Stride)
                        {
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stRightBottomPt.u16X/4-j) = (index&0x03) | ((index&0x03) << 2) | ((index&0x03) << 4) | ((index&0x03) << 6);// copy 1 byte
                        }
                    }
                    if (u8BorderWidth % 4)
                    {
                        if(((stLeftTopPt.u16X/4 + u8BorderWidth/4) < u32Stride))
                        {
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/4+u8BorderWidth/4) &= 0xf0;
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stLeftTopPt.u16X/4+u8BorderWidth/4) |= (index&0x03) | ((index&0x03) << 2) ;
                        }
                        if(((stRightBottomPt.u16X/4 - u8BorderWidth/4) < u32Stride))
                        {
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stRightBottomPt.u16X/4-u8BorderWidth/4) &= 0x0f;
                            *(pDrawBase+u32Stride*(stLeftTopPt.u16Y+i)+stRightBottomPt.u16X/4-u8BorderWidth/4) |= ((index&0x03) | ((index&0x03) << 2) ) << 4;
                        }
                    }
                 }
               }
               break;
           case E_MI_RGN_PIXEL_FORMAT_I4:
               {
                   if(((rect.u16X + rect.u16Width)  < stRgnCanvasInfo.stSize.u32Width) && \
                      ((rect.u16Y + rect.u16Height) < stRgnCanvasInfo.stSize.u32Height))
                   {
                       for (i = 0; i < u32Width/2; i++)
                       {
                           for (j = 0; j < u8BorderWidth; j++)
                           {
                               *(MI_U8*)(stRgnCanvasInfo.virtAddr + stRgnCanvasInfo.u32Stride*(rect.u16Y+j)+rect.u16X/2+i) = (index&0x0f) | ((index&0x0f) << 4);          // copy 1 byte
                               *(MI_U8*)(stRgnCanvasInfo.virtAddr + stRgnCanvasInfo.u32Stride*((rect.u16Y+rect.u16Height)-j)+rect.u16X/2+i) = (index&0x0f) | ((index&0x0f) << 4);      // copy 1 byte
                           }
                       }

                       for (i = 0; i < u32Height; i++)
                       {
                           for (j = 0; j < u8BorderWidth/2; j++)
                           {
                               *(MI_U8*)(stRgnCanvasInfo.virtAddr + stRgnCanvasInfo.u32Stride*(rect.u16Y+i)+rect.u16X/2+j) = (index&0x0f) | ((index&0x0f) << 4);          // copy 1 byte
                               *(MI_U8*)(stRgnCanvasInfo.virtAddr + stRgnCanvasInfo.u32Stride*(rect.u16Y+i)+(rect.u16X+rect.u16Width)/2-j) = (index&0x0f) | ((index&0x0f) << 4);      // copy 1 byte
                           }

                           if(u8BorderWidth % 2)
                           {
                               *(MI_U8*)(stRgnCanvasInfo.virtAddr + stRgnCanvasInfo.u32Stride*(rect.u16Y+i)+rect.u16X/2+u8BorderWidth/2) &= 0xf0;
                               *(MI_U8*)(stRgnCanvasInfo.virtAddr + stRgnCanvasInfo.u32Stride*(rect.u16Y+i)+rect.u16X/2+u8BorderWidth/2) |= (index&0x0f);
                               *(MI_U8*)(stRgnCanvasInfo.virtAddr + stRgnCanvasInfo.u32Stride*(rect.u16Y+i)+(rect.u16X+rect.u16Width)/2-u8BorderWidth/2) &= 0x0f;
                               *(MI_U8*)(stRgnCanvasInfo.virtAddr + stRgnCanvasInfo.u32Stride*(rect.u16Y+i)+(rect.u16X+rect.u16Width)/2-u8BorderWidth/2) |= (index&0x0f) << 4;
                           }
                       }
                   }
               }
               break;

           case E_MI_RGN_PIXEL_FORMAT_ARGB1555:
               {
                   MIXER_INFO("Rgn %d clean canvas:x=%d, y=%d, w=%d, h=%d, stride=%d, fmt=ARGB1555\n", hHandle, rect.u16X,
                                                   rect.u16Y, rect.u16Width, rect.u16Height, stRgnCanvasInfo.u32Stride);

                   if(((rect.u16X + rect.u16Width)  < stRgnCanvasInfo.stSize.u32Width) && \
                      ((rect.u16Y + rect.u16Height) < stRgnCanvasInfo.stSize.u32Height))
                   {
                       for (i = 0; i < u32Width; i++)
                       {
                           for (j = 0; j < u8BorderWidth; j++)
                           {
                               *(MI_U8*)(stRgnCanvasInfo.virtAddr + stRgnCanvasInfo.u32Stride/2*(rect.u16Y+j)+rect.u16X+i) = index&0x0f;          // copy 2 byte, app check alignment
                               *(MI_U8*)(stRgnCanvasInfo.virtAddr + stRgnCanvasInfo.u32Stride/2*(rect.u16Y+rect.u16Height-j)+rect.u16X+i) = index&0x0f;      // copy 2 byte, app check alignment
                           }
                       }

                       for (i = 0; i < u32Height; i++)
                       {
                           for (j = 0; j < u8BorderWidth; j++)
                           {
                               *(MI_U8*)(stRgnCanvasInfo.virtAddr + stRgnCanvasInfo.u32Stride/2*(rect.u16Y+i)+rect.u16X+j) = index&0x0f;          // copy 2 byte, app check alignment
                               *(MI_U8*)(stRgnCanvasInfo.virtAddr + stRgnCanvasInfo.u32Stride/2*(rect.u16Y+i)+(rect.u16X+rect.u16Width)-j) = index&0x0f;         // copy 2 byte, app check alignment
                           }
                       }
                   }
               }
               break;

           default:
               MIXER_ERR("only support %s now\n", (0==stRgnCanvasInfo.ePixelFmt)?"ARGB1555":(2==stRgnCanvasInfo.ePixelFmt)?"I2":"I4");
               pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
               return s32Ret;
       }
   }
    if(MI_RGN_MAX_HANDLE > hHandle)
   {
       s32Ret = MI_RGN_UpdateCanvas(hHandle);
       if(MI_RGN_OK != s32Ret)
       {
           MIXER_ERR("MI_RGN_UpdateCanvas fail\n");
            pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
             return MI_ERR_RGN_INVALID_HANDLE;
       }
   }
   else
   {
     MIXER_ERR("osd handle is out of rang [0-%d]",MI_RGN_MAX_HANDLE);
     pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
     return MI_ERR_RGN_INVALID_HANDLE;
   }
   pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);

   return MI_RGN_OK;
}

MI_S32 createOsdWidget(MI_RGN_HANDLE *pu32RgnHandle, ST_MIXER_RGN_WIDGET_ATTR *pstMixerRgnWidgetAttr)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_S8  errinfo[64] = {0};
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPortParam_t stRgnChnPortParam;
    MI_RGN_HANDLE u32OsdRgnHandleTmp = MI_RGN_HANDLE_NULL;

    CHECK_PARAM_IS_X(pu32RgnHandle,NULL,E_MI_ERR_NULL_PTR,"The input Mixer Rgn Param pointer is NULL!");
    CHECK_PARAM_IS_X(pstMixerRgnWidgetAttr,NULL,E_MI_ERR_NULL_PTR,"The input Mixer Rgn Param pointer is NULL!");
    CHECK_PARAM_IS_X(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun,NULL,E_MI_ERR_NULL_PTR,"The input Mixer Rgn Param pointer is NULL!");

    sprintf((char*)errinfo,"The input VencChn(%d) is out of range!\n",pstMixerRgnWidgetAttr->s32VencChn);
    CHECK_PARAM_OPT_X(pstMixerRgnWidgetAttr->s32VencChn,<,0,E_MI_ERR_ILLEGAL_PARAM,errinfo);
    CHECK_PARAM_OPT_X(pstMixerRgnWidgetAttr->s32VencChn,>,g_s32VideoStreamNum,E_MI_ERR_ILLEGAL_PARAM,errinfo);
    memset(errinfo,0,sizeof(errinfo));
    sprintf((char*)errinfo,"The input OSD handle index(%d) is out of range!\n",pstMixerRgnWidgetAttr->s32Idx);
    CHECK_PARAM_OPT_X(pstMixerRgnWidgetAttr->s32Idx,<,0,E_MI_ERR_ILLEGAL_PARAM,errinfo);
    CHECK_PARAM_OPT_X(pstMixerRgnWidgetAttr->s32Idx,>,MAX_RGN_NUMBER_PER_CHN,E_MI_ERR_ILLEGAL_PARAM,errinfo);

    pthread_mutex_lock(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun);

    if(E_OSD_WIDGET_TYPE_RECT == pstMixerRgnWidgetAttr->eOsdWidgetType)
    {
        u32OsdRgnHandleTmp = calcOsdHandle(pstMixerRgnWidgetAttr->s32VencChn, pstMixerRgnWidgetAttr->s32Idx, E_OSD_WIDGET_TYPE_RECT);
        if(0 > (MI_S32)u32OsdRgnHandleTmp  || MI_RGN_MAX_HANDLE < u32OsdRgnHandleTmp)
        {
            MIXER_ERR("Get OSD handle error(0x%X), VencChn=%d, index=%d, OSD type:%d\n", u32OsdRgnHandleTmp,
                       pstMixerRgnWidgetAttr->s32VencChn, pstMixerRgnWidgetAttr->s32Idx, pstMixerRgnWidgetAttr->eOsdWidgetType);
            *pu32RgnHandle = MI_RGN_HANDLE_NULL;
            pthread_mutex_unlock(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun);
            s32Ret = E_MI_ERR_ILLEGAL_PARAM;
            return s32Ret;
        }

        memset(&stRgnAttr, 0x00, sizeof(MI_RGN_Attr_t));
        stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
        stRgnAttr.stOsdInitParam.ePixelFmt = pstMixerRgnWidgetAttr->eRgnpixelFormat;
        stRgnAttr.stOsdInitParam.stSize.u32Width  = pstMixerRgnWidgetAttr->stRect.u16Width;
        stRgnAttr.stOsdInitParam.stSize.u32Height = pstMixerRgnWidgetAttr->stRect.u16Height;

        memset(&stRgnChnPortParam, 0x00, sizeof(MI_RGN_ChnPortParam_t));
        stRgnChnPortParam.bShow = pstMixerRgnWidgetAttr->bShow;
        stRgnChnPortParam.stPoint.u32X = pstMixerRgnWidgetAttr->stRect.u16X;
        stRgnChnPortParam.stPoint.u32Y = pstMixerRgnWidgetAttr->stRect.u16Y;

        if(((pstMixerRgnWidgetAttr->stRect.u16Width <= 1920) && (pstMixerRgnWidgetAttr->stRect.u16Height <= 1080)) ||
           ((pstMixerRgnWidgetAttr->stRect.u16Width <= 1080) && (pstMixerRgnWidgetAttr->stRect.u16Height <= 1920)))
        {
            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode = E_MI_RGN_ABOVE_LUMA_THRESHOLD;
            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum = getDividedNumber(stRgnAttr.stOsdInitParam.stSize.u32Width);
            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum = getDividedNumber(stRgnAttr.stOsdInitParam.stSize.u32Height);

            if((1920 == stRgnAttr.stOsdInitParam.stSize.u32Width) && (1080 == stRgnAttr.stOsdInitParam.stSize.u32Height))
            {
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum /= 2;
            }
            else if((1080 == stRgnAttr.stOsdInitParam.stSize.u32Width) && (1920 == stRgnAttr.stOsdInitParam.stSize.u32Height))
            {
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum /= 2;
            }

            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold = pstMixerRgnWidgetAttr->u16LumaThreshold;
            stRgnChnPortParam.unPara.stOsdChnPort.u32Layer = (MI_U32)u32OsdRgnHandleTmp;
            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = pstMixerRgnWidgetAttr->bOsdColorInverse;
        }

        s32Ret = createOsdRectWidget(u32OsdRgnHandleTmp, &stRgnAttr, &pstMixerRgnWidgetAttr->stRgnChnPort, &stRgnChnPortParam);
        if(MI_RGN_OK != s32Ret)
        {
            *pu32RgnHandle = MI_RGN_HANDLE_NULL;
            MIXER_ERR("createOsdRectWidget error(0x%X), hdl=%d\n", s32Ret, u32OsdRgnHandleTmp);
            pthread_mutex_unlock(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun);
            return s32Ret;
        }

        printf("%s:%d Create RectHdl=%d canvas:x=%d, y=%d, w=%4d, h=%4d, DevID=%d, ChnID=%d, PortID=%d, fmt=%d\n", __func__, __LINE__,
                          u32OsdRgnHandleTmp, stRgnChnPortParam.stPoint.u32X, stRgnChnPortParam.stPoint.u32Y,
                          stRgnAttr.stOsdInitParam.stSize.u32Width, stRgnAttr.stOsdInitParam.stSize.u32Height,
                          pstMixerRgnWidgetAttr->stRgnChnPort.s32DevId, pstMixerRgnWidgetAttr->stRgnChnPort.s32ChnId,
                          pstMixerRgnWidgetAttr->stRgnChnPort.s32OutputPortId, pstMixerRgnWidgetAttr->eRgnpixelFormat);

        printf("%s:%d Osd_W=%4d, Osd_H=%4d, WDivNum=%d, HDivNum=%d, InvertColorMode=%d, Threshold=%d, Layer=%d\n\n", __func__, __LINE__,
                          pstMixerRgnWidgetAttr->stRect.u16Width, pstMixerRgnWidgetAttr->stRect.u16Height,
                          stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum,
                          stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum,
                          stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode,
                          stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold,
                          stRgnChnPortParam.unPara.stOsdChnPort.u32Layer);

        s32Ret = cleanOsdTextWidget(u32OsdRgnHandleTmp, NULL, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
        if(MI_RGN_OK != s32Ret)
        {
            *pu32RgnHandle = u32OsdRgnHandleTmp;
            MIXER_ERR("cleanOsdTextWidget error(0x%X), hdl=%d\n", s32Ret, u32OsdRgnHandleTmp);
            pthread_mutex_unlock(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun);
            return s32Ret;
        }
    }
    else if(E_OSD_WIDGET_TYPE_TEXT == pstMixerRgnWidgetAttr->eOsdWidgetType)
    {
        u32OsdRgnHandleTmp = calcOsdHandle(pstMixerRgnWidgetAttr->s32VencChn, pstMixerRgnWidgetAttr->s32Idx, E_OSD_WIDGET_TYPE_TEXT);
        if(0 > (MI_S32)u32OsdRgnHandleTmp || MI_RGN_MAX_HANDLE < u32OsdRgnHandleTmp)
        {
            MIXER_ERR("Get OSD handle error(0x%X), VencChn=%d, index=%d, OSD type:%d\n", u32OsdRgnHandleTmp,
                       pstMixerRgnWidgetAttr->s32VencChn, pstMixerRgnWidgetAttr->s32Idx, pstMixerRgnWidgetAttr->eOsdWidgetType);
            *pu32RgnHandle = MI_RGN_HANDLE_NULL;
            pthread_mutex_unlock(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun);
            s32Ret = E_MI_ERR_ILLEGAL_PARAM;
            return s32Ret;
        }

        memset(&stRgnAttr, 0x00, sizeof(MI_RGN_Attr_t));
        stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
        stRgnAttr.stOsdInitParam.ePixelFmt = pstMixerRgnWidgetAttr->eRgnpixelFormat;
        stRgnAttr.stOsdInitParam.stSize.u32Width  = pstMixerRgnWidgetAttr->stRect.u16Width;
        stRgnAttr.stOsdInitParam.stSize.u32Height = pstMixerRgnWidgetAttr->stRect.u16Height;

        memset(&stRgnChnPortParam, 0x00, sizeof(MI_RGN_ChnPortParam_t));
        stRgnChnPortParam.bShow = pstMixerRgnWidgetAttr->bShow;
        stRgnChnPortParam.stPoint.u32X = pstMixerRgnWidgetAttr->stRect.u16X;
        stRgnChnPortParam.stPoint.u32Y = pstMixerRgnWidgetAttr->stRect.u16Y;
        stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode = E_MI_RGN_ABOVE_LUMA_THRESHOLD;
        stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold = pstMixerRgnWidgetAttr->u16LumaThreshold;
        stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum = getDividedNumber(stRgnAttr.stOsdInitParam.stSize.u32Width);
        stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum = getDividedNumber(stRgnAttr.stOsdInitParam.stSize.u32Height);
        stRgnChnPortParam.unPara.stOsdChnPort.u32Layer = (MI_U32)u32OsdRgnHandleTmp;
        stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = pstMixerRgnWidgetAttr->bOsdColorInverse;
        stRgnChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode = E_MI_RGN_PIXEL_ALPHA;
        stRgnChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8BgAlpha = 0x0;
        stRgnChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8FgAlpha = 0xff;

        MIXER_DBG("u32OsdRgnHandleTmp;%d\n", u32OsdRgnHandleTmp);

        s32Ret = createOsdTextWidget(u32OsdRgnHandleTmp, &stRgnAttr, &pstMixerRgnWidgetAttr->stRgnChnPort, &stRgnChnPortParam);
        if(MI_RGN_OK != s32Ret)
        {
            *pu32RgnHandle = MI_RGN_HANDLE_NULL;
            MIXER_ERR("createOsdTextWidget error(0x%X), hdl=%d\n", s32Ret, u32OsdRgnHandleTmp);
            pthread_mutex_unlock(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun);
            return s32Ret;
        }

        printf("%s:%d Create TextHdl=%d canvas:x=%d, y=%d, w=%4d, h=%4d, DevID=%d, ChnID=%d, PortID=%d, fmt=%d\n", __func__, __LINE__,
                          u32OsdRgnHandleTmp, stRgnChnPortParam.stPoint.u32X, stRgnChnPortParam.stPoint.u32Y,
                          stRgnAttr.stOsdInitParam.stSize.u32Width, stRgnAttr.stOsdInitParam.stSize.u32Height,
                          pstMixerRgnWidgetAttr->stRgnChnPort.s32DevId, pstMixerRgnWidgetAttr->stRgnChnPort.s32ChnId,
                          pstMixerRgnWidgetAttr->stRgnChnPort.s32OutputPortId, pstMixerRgnWidgetAttr->eRgnpixelFormat);

        printf("%s:%d Osd_W=%4d, Osd_H=%4d, WDivNum=%d, HDivNum=%d, InvertColorMode=%d, Threshold=%d, Layer=%d\n\n", __func__, __LINE__,
                          pstMixerRgnWidgetAttr->stRect.u16Width, pstMixerRgnWidgetAttr->stRect.u16Height,
                          stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum,
                          stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum,
                          stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode,
                          stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold,
                          stRgnChnPortParam.unPara.stOsdChnPort.u32Layer);

        s32Ret = cleanOsdTextWidget(u32OsdRgnHandleTmp, NULL, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
        if(MI_RGN_OK != s32Ret)
        {
            *pu32RgnHandle = u32OsdRgnHandleTmp;
            MIXER_ERR("cleanOsdTextWidget error(%X), hdl=%d\n", s32Ret, u32OsdRgnHandleTmp);
            pthread_mutex_unlock(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun);
            return s32Ret;
        }
    }
    else if(E_OSD_WIDGET_TYPE_COVER == pstMixerRgnWidgetAttr->eOsdWidgetType)
    {
        MI_U32 u32XNew = 0;
        MI_U32 u32YNew = 0;
        MI_U32 u32WidthNew = 0;
        MI_U32 u32HeightNew = 0;

        u32OsdRgnHandleTmp = calcOsdHandle(pstMixerRgnWidgetAttr->s32VencChn, pstMixerRgnWidgetAttr->s32Idx, E_OSD_WIDGET_TYPE_COVER);
        if(0 > (MI_S32)u32OsdRgnHandleTmp  || MI_RGN_MAX_HANDLE < u32OsdRgnHandleTmp)
        {
            MIXER_ERR("Get OSD handle error(0x%X), VencChn=%d, index=%d, OSD type:%d\n", u32OsdRgnHandleTmp,
                       pstMixerRgnWidgetAttr->s32VencChn, pstMixerRgnWidgetAttr->s32Idx, pstMixerRgnWidgetAttr->eOsdWidgetType);
            *pu32RgnHandle = MI_RGN_HANDLE_NULL;
            pthread_mutex_unlock(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun);
            s32Ret = E_MI_ERR_ILLEGAL_PARAM;
            return s32Ret;
        }

        memset(&stRgnAttr, 0x00, sizeof(MI_RGN_Attr_t));
        stRgnAttr.eType = E_MI_RGN_TYPE_COVER;
        //stRgnAttr.stOsdInitParam.ePixelFmt = pstMixerRgnWidgetAttr->eRgnpixelFormat;
        //stRgnAttr.stOsdInitParam.stSize.u32Width  = (MI_U32)pstMixerRgnWidgetAttr->stRect.u16Width;
        //stRgnAttr.stOsdInitParam.stSize.u32Height = (MI_U32)pstMixerRgnWidgetAttr->stRect.u16Height;

        s32Ret = MI_RGN_Create(u32OsdRgnHandleTmp, &stRgnAttr);
        if(MI_RGN_OK != s32Ret)
        {
            *pu32RgnHandle = MI_RGN_HANDLE_NULL;
            MIXER_ERR("MI_RGN_Create error(0x%X), hdl=%d\n", s32Ret, u32OsdRgnHandleTmp);
            pthread_mutex_unlock(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun);
            return s32Ret;
        }

        //Mapping settings to 8192*8192
        u32XNew = pstMixerRgnWidgetAttr->stRect.u16X * RGN_COVER_MAX_WIDTH;
        u32YNew = pstMixerRgnWidgetAttr->stRect.u16Y * RGN_COVER_MAX_HEIGHT;
        u32WidthNew  = pstMixerRgnWidgetAttr->stRect.u16Width * RGN_COVER_MAX_WIDTH;
        u32HeightNew = pstMixerRgnWidgetAttr->stRect.u16Height * RGN_COVER_MAX_HEIGHT;

        memset(&stRgnChnPortParam, 0x00, sizeof(MI_RGN_ChnPortParam_t));
        stRgnChnPortParam.bShow = pstMixerRgnWidgetAttr->bShow;
        stRgnChnPortParam.unPara.stCoverChnPort.u32Layer = (MI_U32)u32OsdRgnHandleTmp;
        stRgnChnPortParam.unPara.stCoverChnPort.u32Color = pstMixerRgnWidgetAttr->u32Color;
        stRgnChnPortParam.stPoint.u32X = u32XNew / (g_stVideoSize[pstMixerRgnWidgetAttr->s32VencChn].u16Width==0?getVideoWidth(pstMixerRgnWidgetAttr->s32VencChn):g_stVideoSize[pstMixerRgnWidgetAttr->s32VencChn].u16Width);
        stRgnChnPortParam.stPoint.u32Y = u32YNew / (g_stVideoSize[pstMixerRgnWidgetAttr->s32VencChn].u16Height==0?getVideoHeight(pstMixerRgnWidgetAttr->s32VencChn):g_stVideoSize[pstMixerRgnWidgetAttr->s32VencChn].u16Height);
        stRgnChnPortParam.unPara.stCoverChnPort.stSize.u32Width  = u32WidthNew / g_stVideoSize[pstMixerRgnWidgetAttr->s32VencChn].u16Width;
        stRgnChnPortParam.unPara.stCoverChnPort.stSize.u32Height = u32HeightNew / g_stVideoSize[pstMixerRgnWidgetAttr->s32VencChn].u16Height;
        s32Ret = MI_RGN_AttachToChn(u32OsdRgnHandleTmp, &pstMixerRgnWidgetAttr->stRgnChnPort, &stRgnChnPortParam);
        if(MI_RGN_OK != s32Ret)
        {
            MI_RGN_Destroy(u32OsdRgnHandleTmp);
            *pu32RgnHandle = MI_RGN_HANDLE_NULL;
            MIXER_ERR("MI_RGN_AttachToChn error(0x%X), hdl=%d\n", s32Ret, u32OsdRgnHandleTmp);
            pthread_mutex_unlock(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun);
            return s32Ret;
        }
    }
    else
    {
        *pu32RgnHandle = MI_RGN_HANDLE_NULL;
        MIXER_ERR("Set wrong OSD type(%d) and return!\n", pstMixerRgnWidgetAttr->eOsdWidgetType);
        pthread_mutex_unlock(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }
    osdInfo_t osdInfo;
    osdSizePoint_t osdSizePoint;
    memset(&osdSizePoint,0,sizeof(osdSizePoint_t));
    memset(&osdInfo,0,sizeof(osdInfo));
    osdInfo.osdType = pstMixerRgnWidgetAttr->eOsdWidgetType;
    osdInfo.u32RgnHandle = u32OsdRgnHandleTmp;
    osdInfo.channel = pstMixerRgnWidgetAttr->s32VencChn;
    osdInfo.osdIndex =  pstMixerRgnWidgetAttr->s32Idx;

    osdInfo.format = pstMixerRgnWidgetAttr->eRgnpixelFormat;
    osdSizePoint.point.u16X = pstMixerRgnWidgetAttr->stRect.u16X;
    osdSizePoint.point.u16Y = pstMixerRgnWidgetAttr->stRect.u16Y;
    osdSizePoint.size.u16Height = pstMixerRgnWidgetAttr->stRect.u16Width;
    osdSizePoint.size.u16Width = pstMixerRgnWidgetAttr->stRect.u16Height;
    midManageOsdInfoLinkList(&osdInfo,TRUE,&osdSizePoint,NULL);
    *pu32RgnHandle = u32OsdRgnHandleTmp;

    pthread_mutex_unlock(pstMixerRgnWidgetAttr->pstMutexMixerOsdRun);

    s32Ret = MI_SUCCESS;
    return s32Ret;
}
MI_S32 destoryOsdHandle(MI_RGN_HANDLE OsdHandle, MI_U32 osdChn,MI_U32 idex,EN_OSD_WIDGET_TYPE eOsdHandleType)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_S32 s32Idx = 0;
    MI_RGN_HANDLE *pu32OsdRgnHandleTmp = NULL;
    MI_RGN_ChnPort_t stRgnChnPort;
    osdInfo_t osdInfo;
    memset(&osdInfo,0,sizeof(osdInfo_t));
    CHECK_PARAM_IS_X(OsdHandle,(MI_RGN_HANDLE)MI_RGN_HANDLE_NULL,E_MI_ERR_NULL_PTR,"The input osd Handle is NULL!");
    CHECK_PARAM_OPT_X(eOsdHandleType,>=,E_OSD_WIDGET_TYPE_MAX,E_MI_ERR_FAILED,"The input osd Handle Type is non-existent!");

    pthread_mutex_lock(&g_stMutexMixerOsdRun[osdChn]);
    if(OsdHandle == g_au32OsdI2Widget[osdChn][idex])
    {
        pu32OsdRgnHandleTmp = &g_au32OsdI2Widget[osdChn][idex];
    }
    else if(OsdHandle == g_au32OsdI4Widget[osdChn][idex])
    {
       pu32OsdRgnHandleTmp = &g_au32OsdI4Widget[osdChn][idex];
    }
    else if(OsdHandle == g_au32BitmapWidget[osdChn][idex])
    {
       pu32OsdRgnHandleTmp = &g_au32BitmapWidget[osdChn][idex];
    }
    else if(E_OSD_WIDGET_TYPE_RECT == eOsdHandleType)
    {
       if(OsdHandle == g_au32OsdRectWidgetHandle[osdChn])
       {
          pu32OsdRgnHandleTmp = &g_au32OsdRectWidgetHandle[osdChn];
       }
    }
    else if(E_OSD_WIDGET_TYPE_TEXT == eOsdHandleType)
    {
      for(s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
      {
        if(OsdHandle == g_au32OsdTextWidgetHandle[osdChn][s32Idx])
        {
          pu32OsdRgnHandleTmp = &g_au32OsdTextWidgetHandle[osdChn][s32Idx];
          break;
        }
      }
    }
    else if(E_OSD_WIDGET_TYPE_COVER == eOsdHandleType)
    {
      for(s32Idx = 0; s32Idx < MAX_COVER_NUMBER_PER_CHN; s32Idx++)
      {
        if(OsdHandle == g_au32OsdCoverWidgetHandle[osdChn][s32Idx])
        {
          pu32OsdRgnHandleTmp = &g_au32OsdCoverWidgetHandle[osdChn][s32Idx];
          break;
        }
      }
    }
    if((osdChn > (MI_U32)g_s32VideoStreamNum) || (0 > osdChn) \
    ||((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == *pu32OsdRgnHandleTmp)\
    || (MI_RGN_MAX_HANDLE < *pu32OsdRgnHandleTmp))
    {
       printf("no found this osdChn[%d] handle[%d]osd type[%d]!\n",osdChn,OsdHandle,eOsdHandleType);
       pthread_mutex_unlock(&g_stMutexMixerOsdRun[osdChn]);
       return E_MI_ERR_FAILED;
    }
    osdInfo.u32RgnHandle = OsdHandle;
    configOsdRgnChnPort(osdChn, &stRgnChnPort);

  //  if((E_OSD_WIDGET_TYPE_RECT == eOsdHandleType) || (E_OSD_WIDGET_TYPE_TEXT == eOsdHandleType))
    {
        cleanOsdTextWidget(OsdHandle, NULL, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
    }
    midFindOsdHandleByOsdAttr(&osdInfo,GET_FORMAT);
    osdInfo.osdType = eOsdHandleType;
    osdInfo.channel = osdChn;
    osdInfo.osdIndex = s32Idx;
    if(MI_SUCCESS == s32Ret)
    {
      midManageOsdInfoLinkList(&osdInfo,FALSE,NULL,NULL);
    }
    s32Ret = MI_RGN_DetachFromChn(OsdHandle, &stRgnChnPort);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_DetachFromChn error(0x%X), hdl=%d\n", s32Ret, OsdHandle);
        pthread_mutex_unlock(&g_stMutexMixerOsdRun[osdChn]);
        return s32Ret;
    }

    s32Ret = MI_RGN_Destroy(OsdHandle);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_Destroy error(0x%X), hdl=%d\n", s32Ret, OsdHandle);
        pthread_mutex_unlock(&g_stMutexMixerOsdRun[osdChn]);
        return s32Ret;
    }

    *pu32OsdRgnHandleTmp = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[osdChn]);

    s32Ret = MI_SUCCESS;
    return s32Ret;
}
MI_S32 destroyOsdWidget(MI_VENC_CHN s32VencChn, MI_RGN_HANDLE u32OsdHandle)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_S32 s32Idx = 0;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_RGN_HANDLE *pu32OsdRgnHandleTmp = NULL;
    EN_OSD_WIDGET_TYPE eOsdWidgetType = E_OSD_WIDGET_TYPE_MAX;
    osdInfo_t osdInfo;
    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle)
    {
        MIXER_ERR("The input OSD handle(%d) is out of range!\n", u32OsdHandle);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    if(u32OsdHandle == g_au32OsdRectWidgetHandle[s32VencChn])
    {
        pu32OsdRgnHandleTmp = &g_au32OsdRectWidgetHandle[s32VencChn];
        eOsdWidgetType = E_OSD_WIDGET_TYPE_RECT;
    }
    else
    {
        for(s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
        {
            if(u32OsdHandle == g_au32OsdTextWidgetHandle[s32VencChn][s32Idx])
            {
                pu32OsdRgnHandleTmp = &g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
                eOsdWidgetType = E_OSD_WIDGET_TYPE_TEXT;
                break;
            }
            else if((u32OsdHandle == g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx]) && (s32Idx < MAX_COVER_NUMBER_PER_CHN))
            {
                pu32OsdRgnHandleTmp = &g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx];
                eOsdWidgetType = E_OSD_WIDGET_TYPE_COVER;
                break;
            }
        }
    }
    if(NULL == pu32OsdRgnHandleTmp)
    {
        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
        MIXER_ERR("Can not find the OSD handle(%d) on s32VencChn(%d)\n", u32OsdHandle, s32VencChn);
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }
    memset(&osdInfo,0,sizeof(osdInfo_t));
    midFindOsdHandleByOsdAttr(&osdInfo,GET_FORMAT);
    osdInfo.osdType = eOsdWidgetType;
    osdInfo.channel = s32VencChn;
    osdInfo.osdIndex = s32Idx;
    if(MI_SUCCESS == s32Ret)
    {
      midManageOsdInfoLinkList(&osdInfo,FALSE,NULL,NULL);
    }
    configOsdRgnChnPort(s32VencChn, &stRgnChnPort);
    if((E_OSD_WIDGET_TYPE_RECT == eOsdWidgetType) || (E_OSD_WIDGET_TYPE_TEXT == eOsdWidgetType))
    {
        cleanOsdTextWidget(u32OsdHandle, NULL, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
    }
    s32Ret = MI_RGN_DetachFromChn(u32OsdHandle, &stRgnChnPort);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_DetachFromChn error(0x%X), hdl=%d\n", s32Ret, u32OsdHandle);
        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
        return s32Ret;
    }
    s32Ret = MI_RGN_Destroy(u32OsdHandle);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_Destroy error(0x%X), hdl=%d\n", s32Ret, u32OsdHandle);
        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
        return s32Ret;
    }
    *pu32OsdRgnHandleTmp = MI_RGN_HANDLE_NULL;
    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 getOsdChnHandleCount(MI_VENC_CHN s32VencChn)
{
    MI_S32 s32Ret = 0;
    MI_S32 s32Idx = 0;

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum-1))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        s32Ret = -1;
        return s32Ret;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn])
    {
        s32Ret++;
    }

    for(s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
    {
        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdTextWidgetHandle[s32VencChn][s32Idx])
        {
            s32Ret++;
        }
        else if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI2Widget[s32VencChn][s32Idx])
        {
          s32Ret++;
        }
        else if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI4Widget[s32VencChn][s32Idx])
        {
          s32Ret++;
        }
        else if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI8Widget[s32VencChn][s32Idx])
        {
          s32Ret++;
        }
        else if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32BitmapWidget[s32VencChn][s32Idx])
        {
          s32Ret++;
        }
    }

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    return s32Ret;
}

MI_RGN_HANDLE getOsdChnMaxHandle(MI_VENC_CHN s32VencChn)
{
    MI_S32 s32Idx = 0;
    MI_RGN_HANDLE u32OsdRgnHandle = 0;


    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        return (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
    }

    //for(s32VencChn = 0; s32VencChn < g_s32VideoStreamNum; s32VencChn++)
    {
        pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

     //   if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn])
        {
            if(g_au32OsdRectWidgetHandle[s32VencChn] != (MI_RGN_HANDLE)-1
                && u32OsdRgnHandle < g_au32OsdRectWidgetHandle[s32VencChn])
            {
                u32OsdRgnHandle = g_au32OsdRectWidgetHandle[s32VencChn];
            }
        }

        for(s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
        {
        //    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdTextWidgetHandle[s32VencChn][s32Idx])
            {
                if(g_au32OsdTextWidgetHandle[s32VencChn][s32Idx] != (MI_RGN_HANDLE)-1
                    && u32OsdRgnHandle < g_au32OsdTextWidgetHandle[s32VencChn][s32Idx])
                {
                    u32OsdRgnHandle = g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
                }
            }
        }

        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
    }

    return u32OsdRgnHandle;
}

MI_RGN_HANDLE getOsdChnHandleNumber(MI_VENC_CHN s32VencChn, MI_S32 *ps32Idx,EN_OSD_WIDGET_TYPE osdType)
{
    MI_S32 s32Idx = 0;
    MI_RGN_HANDLE u32OsdRgnHandle = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;


    if((s32VencChn < 0) || (s32VencChn > (g_s32VideoStreamNum-1)))
    {
           MIXER_ERR("The input VenChn(%d) is out of range! *ps32Idx=%d\n", s32VencChn,*ps32Idx);
        if(NULL != ps32Idx) *ps32Idx = (MI_U32)-1;
        return (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    if(NULL != ps32Idx) *ps32Idx = (MI_U32)-1;
    if(E_OSD_WIDGET_TYPE_TEXT == osdType || E_OSD_WIDGET_TYPE_RECT == osdType)
    {
        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn])
        {
            if(u32OsdRgnHandle < g_au32OsdRectWidgetHandle[s32VencChn])
            {
                u32OsdRgnHandle = g_au32OsdRectWidgetHandle[s32VencChn];
                if(NULL != ps32Idx) *ps32Idx = (MI_U32)-1;
            }
        }

        for(s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
        {
            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdTextWidgetHandle[s32VencChn][s32Idx])
            {
                //if(u32OsdRgnHandle < g_au32OsdTextWidgetHandle[s32VencChn][s32Idx])
                {
                    u32OsdRgnHandle = g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
                    if(NULL != ps32Idx) *ps32Idx = s32Idx;
                }
            }
        }
    }
    else if(E_OSD_WIDGET_TYPE_COVER == osdType)
    {
        for(s32Idx = 0; s32Idx < MAX_COVER_NUMBER_PER_CHN; s32Idx++)
        {
            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
            {
                u32OsdRgnHandle = g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx];
                if(NULL != ps32Idx) *ps32Idx = s32Idx;
            }
        }
    }
    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    return u32OsdRgnHandle;
}

MI_RGN_HANDLE getOsdChnHandleFirstNull(MI_VENC_CHN s32VencChn, MI_S32 *ps32Idx)
{
    MI_S32 s32Idx = -1;
    MI_RGN_HANDLE u32OsdRgnHandle = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;


    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        if(NULL != ps32Idx) *ps32Idx = (MI_U32)-1;
        return (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    if(NULL != ps32Idx) *ps32Idx = (MI_U32)-1;

    for(s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
    {
        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == g_au32OsdTextWidgetHandle[s32VencChn][s32Idx])
        {
            u32OsdRgnHandle = g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
            if(NULL != ps32Idx) *ps32Idx = s32Idx;
        }
    }

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    return u32OsdRgnHandle;
}

MI_S32 getCoverHandleCount(MI_VENC_CHN s32VencChn)
{
    MI_S32 s32Ret = 0;
    MI_S32 s32Idx = 0;

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        s32Ret = -1;
        return s32Ret;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    for(s32Idx = 0; s32Idx < MAX_COVER_NUMBER_PER_CHN; s32Idx++)
    {
        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
        {
            s32Ret++;
        }
    }

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    return s32Ret;
}

MI_RGN_HANDLE getCoverMaxHandleNumber()
{
    MI_S32 s32Idx = 0;
    MI_VENC_CHN s32VencChn = 0;
    MI_RGN_HANDLE u32OsdRgnHandle = 0;


    for(s32VencChn = 0; s32VencChn < g_s32VideoStreamNum; s32VencChn++)
    {
        pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

        for(s32Idx = 0; s32Idx < MAX_COVER_NUMBER_PER_CHN; s32Idx++)
        {
            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
            {
                //if(u32OsdRgnHandle < g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
                {
                    u32OsdRgnHandle = g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx];
                }
            }
        }

        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
    }

    return u32OsdRgnHandle;
}

MI_RGN_HANDLE getCoverChnHandleNumber(MI_VENC_CHN s32VencChn, MI_S32 *ps32Idx)
{
    MI_S32 s32Idx = 0;
    MI_RGN_HANDLE u32OsdRgnHandle = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;


    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        if(NULL != ps32Idx) *ps32Idx = (MI_U32)-1;
        return (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    if(NULL != ps32Idx) *ps32Idx = -1;

    for(s32Idx = 0; s32Idx < MAX_COVER_NUMBER_PER_CHN; s32Idx++)
    {
        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
        {
            //if(u32OsdRgnHandle < g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
            {
                u32OsdRgnHandle = g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx];
                if(NULL != ps32Idx)
                {
                  *ps32Idx = s32Idx;
                }
            }
        }
    }

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    return u32OsdRgnHandle;
}

MI_RGN_HANDLE getCoverChnHandleFirstNull(MI_VENC_CHN s32VencChn, MI_S32 *ps32Idx)
{
    MI_S32 s32Idx = -1;
    MI_RGN_HANDLE u32OsdRgnHandle = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;


    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        if(NULL != ps32Idx) *ps32Idx = (MI_U32)-1;
        return (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    if(NULL != ps32Idx) *ps32Idx = (MI_U32)-1;

    for(s32Idx = 0; s32Idx < MAX_COVER_NUMBER_PER_CHN; s32Idx++)
    {
        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
        {
            u32OsdRgnHandle = g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx];
             if(NULL != ps32Idx) {
                  *ps32Idx = s32Idx;
                  break;
            }
        }
    }

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    return u32OsdRgnHandle;
}

MI_S32 getVideoChnByOsdHandle(MI_RGN_HANDLE u32OsdHandle, MI_VENC_CHN *ps32VencChn, MI_S32 *ps32Idx)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;


    if((NULL == ps32VencChn) || (NULL == ps32Idx))
    {
        MIXER_ERR("The input Pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if(((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle) || (MI_RGN_MAX_HANDLE < u32OsdHandle))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", u32OsdHandle);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    for(s32VencChn = 0; s32VencChn < g_s32VideoStreamNum; s32VencChn++)
    {
        if(((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn]) &&
           (u32OsdHandle == g_au32OsdRectWidgetHandle[s32VencChn]))
        {
            *ps32VencChn = s32VencChn;
            *ps32Idx = (MI_U32)-1;
            return s32Ret;
        }

        for(s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
        {
            if(((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdTextWidgetHandle[s32VencChn][s32Idx]) &&
               (u32OsdHandle == g_au32OsdTextWidgetHandle[s32VencChn][s32Idx]))
            {
                *ps32VencChn = s32VencChn;
                *ps32Idx = s32Idx;
                return s32Ret;
            }

            if(((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx]) &&
                (u32OsdHandle == g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx]) && (s32Idx < MAX_COVER_NUMBER_PER_CHN))
            {
                *ps32VencChn = s32VencChn;
                *ps32Idx = s32Idx;
                return s32Ret;
            }
        }
    }

    if((MAX_VIDEO_NUMBER == s32VencChn) && (MAX_RGN_NUMBER_PER_CHN == s32Idx))
    {
        *ps32VencChn = -1;
        *ps32Idx = -1;
    }

    s32Ret = E_MI_ERR_FAILED;
    return s32Ret;
}

MI_S32 changeOsdAlpha(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, MI_U8 u8Alpha)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_RGN_ChnPortParam_t stRgnChnPortAttr;
    MI_RGN_HANDLE u32OsdHandle = (MI_RGN_HANDLE)-1;


    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        s32Ret = -1;
        return s32Ret;
    }

    if((0 > s32Idx) ||(s32Idx > MAX_RGN_NUMBER_PER_CHN))
    {
        MIXER_ERR("The input OSD index(%d) is out of range!\n", s32Idx);
        s32Ret = -1;
        return s32Ret;
    }

    u32OsdHandle = getOsdHandle(s32VencChn, s32Idx, E_OSD_WIDGET_TYPE_TEXT);

    if(((MI_RGN_HANDLE)-1 == u32OsdHandle) || (u32OsdHandle > MI_RGN_MAX_HANDLE))
    {
        MIXER_ERR("Get Osd handle(%d) is out of range!\n", u32OsdHandle);
        s32Ret = E_MI_ERR_FAILED;
        return s32Ret;
    }
    if((s32VencChn < g_s32VideoStreamNum) && (NULL != g_videoEncoderArray[s32VencChn]))
    {
        configOsdRgnChnPort(s32VencChn, &stRgnChnPort);
    }

    memset(&stRgnChnPortAttr, 0x00, sizeof(MI_RGN_ChnPortParam_t));
    s32Ret = MI_RGN_GetDisplayAttr(u32OsdHandle, &stRgnChnPort, &stRgnChnPortAttr);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetDisplayAttr(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return s32Ret;
    }

    stRgnChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode = E_MI_RGN_CONSTANT_ALPHA;
    stRgnChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.u8ConstantAlpha = u8Alpha;
    s32Ret = MI_RGN_SetDisplayAttr(u32OsdHandle, &stRgnChnPort, &stRgnChnPortAttr);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("MI_RGN_SetDisplayAttr(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

MI_S32 changeOsdPositionAndSize(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_RGN_HANDLE *pu32RgnHandle = NULL;
    MI_RGN_ChnPortParam_t stRgnChnPortAttr;

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        s32Ret = -1;
        return s32Ret;
    }

    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if(-1 == s32Idx)
    {
        pu32RgnHandle = &g_au32OsdRectWidgetHandle[s32VencChn];
    }
    else
    {
        if(s32Idx > MAX_RGN_NUMBER_PER_CHN)
        {
            MIXER_ERR("The input OSD index(%d) is out of range!\n", s32Idx);
            s32Ret = -1;
            return s32Ret;
        }
        else
        {
            pu32RgnHandle = &g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
        }
    }

    memset(&stRgnAttr, 0x00, sizeof(MI_RGN_Attr_t));
    s32Ret = MI_RGN_GetAttr(*pu32RgnHandle, &stRgnAttr);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetAttr(hdl=%d) error(0x%X)\n", *pu32RgnHandle, s32Ret);
        return s32Ret;
    }

    if((s32VencChn < g_s32VideoStreamNum) && (NULL != g_videoEncoderArray[s32VencChn]))
    {
        configOsdRgnChnPort(s32VencChn, &stRgnChnPort);
    }

    memset(&stRgnChnPortAttr, 0x00, sizeof(MI_RGN_ChnPortParam_t));
    s32Ret = MI_RGN_GetDisplayAttr(*pu32RgnHandle, &stRgnChnPort, &stRgnChnPortAttr);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetDisplayAttr(hdl=%d) error(0x%X)\n", *pu32RgnHandle, s32Ret);
        return s32Ret;
    }

    if((0x00 == pstRect->u16Width) && (0x00 == pstRect->u16Height))
    {
        //if((pstRect->u16X != stRgnChnPortAttr.stPoint.u32X) || (pstRect->u16Y != stRgnChnPortAttr.stPoint.u32Y))
        {
            stRgnChnPortAttr.stPoint.u32X = pstRect->u16X;
            stRgnChnPortAttr.stPoint.u32Y = pstRect->u16Y;

            s32Ret = MI_RGN_SetDisplayAttr(*pu32RgnHandle, &stRgnChnPort, &stRgnChnPortAttr);
            if(MI_SUCCESS != s32Ret)
            {
                MIXER_ERR("MI_RGN_SetDisplayAttr(hdl=%d) error(0x%X)\n", *pu32RgnHandle, s32Ret);
                return s32Ret;
            }
        }
    }
    else if((pstRect->u16X != stRgnChnPortAttr.stPoint.u32X) ||
            (pstRect->u16Y != stRgnChnPortAttr.stPoint.u32Y) ||
            (pstRect->u16Width != stRgnAttr.stOsdInitParam.stSize.u32Width) ||
            (pstRect->u16Height != stRgnAttr.stOsdInitParam.stSize.u32Height))
    {
        ST_MIXER_RGN_WIDGET_ATTR stMixerRgnWidgetAttr;

        s32Ret = destroyOsdWidget(s32VencChn, *pu32RgnHandle);
        if(MI_RGN_OK != s32Ret)
        {
            MIXER_ERR("destroyOsdWidget error(0x%X), hdl=%d\n", s32Ret, *pu32RgnHandle);
            return s32Ret;
        }
        memset(&stMixerRgnWidgetAttr, 0x00, sizeof(ST_MIXER_RGN_WIDGET_ATTR));
        configRgnWidgetAttr(s32VencChn, s32Idx, pstRect, &stRgnChnPort, &stMixerRgnWidgetAttr,g_stMixerOsdWidgetOrder.pmt );
        stMixerRgnWidgetAttr.u32Color = 0xff801080; //black
        stMixerRgnWidgetAttr.bShow = TRUE;
        stMixerRgnWidgetAttr.eRgnpixelFormat = stRgnAttr.stOsdInitParam.ePixelFmt; //E_MI_RGN_PIXEL_FORMAT_I4;

        if((-1 == s32Idx) && (E_MI_RGN_TYPE_OSD == stRgnAttr.eType))
        {
            stMixerRgnWidgetAttr.eOsdWidgetType = E_OSD_WIDGET_TYPE_RECT;
        }
        else if((-1 != s32Idx) && (E_MI_RGN_TYPE_OSD == stRgnAttr.eType))
        {
            stMixerRgnWidgetAttr.eOsdWidgetType = E_OSD_WIDGET_TYPE_TEXT;
        }

        s32Ret = createOsdWidget(pu32RgnHandle, &stMixerRgnWidgetAttr);
        if(MI_SUCCESS != s32Ret)
        {
            MIXER_ERR("createOsdWidget() error(0x%X)\n", s32Ret);
            return s32Ret;
        }

    }
    else
    {
        printf("%s:%d error!  Old window size:%d, %d, %d, %d;   set New window size:%d, %d, %d, %d\n",
                              __func__, __LINE__, stRgnChnPortAttr.stPoint.u32X, stRgnChnPortAttr.stPoint.u32Y,
                              stRgnAttr.stOsdInitParam.stSize.u32Width, stRgnAttr.stOsdInitParam.stSize.u32Height,
                              pstRect->u16X, pstRect->u16Y, pstRect->u16Width, pstRect->u16Height);
    }


    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 changeOsdPositionByHandle(MI_RGN_HANDLE u32OsdHandle, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;
       MI_RGN_Attr_t pstRegion;
    memset(&pstRegion,0,sizeof(MI_RGN_Attr_t));
    if((u32OsdHandle < 0) || (u32OsdHandle > MI_RGN_MAX_HANDLE))
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    s32Ret = getVideoChnByOsdHandle(u32OsdHandle, &s32VencChn, &s32Idx);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoChnByOsdHandle(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return s32Ret;
    }

    s32Ret = changeOsdPositionAndSize(s32VencChn, s32Idx, pstRect);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("changeOsdPosition(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return s32Ret;
    }
    printf("osd move u32OsdHandle=%d ok! to new_x:%d new_y:%d\n",u32OsdHandle,pstRect->u16X,pstRect->u16Y);
    s32Ret = MI_RGN_GetAttr(u32OsdHandle, &pstRegion);
    if(MI_SUCCESS != s32Ret)
     {
        printf("line[%d] MI_RGN_GetAttr is failed [0x%x]!\n",__LINE__,s32Ret);
        return s32Ret;
     }
    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 changeOsdPositionAndSizeByHandle(MI_RGN_HANDLE u32OsdHandle, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;


    if((MI_RGN_HANDLE_NULL == (MI_S32)u32OsdHandle) || (u32OsdHandle > MI_RGN_MAX_HANDLE))
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    s32Ret = getVideoChnByOsdHandle(u32OsdHandle, &s32VencChn, &s32Idx);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoChnByOsdHandle(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return s32Ret;
    }

    s32Ret = changeOsdPositionAndSize(s32VencChn, s32Idx, pstRect);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("changeOsdPosition(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return s32Ret;
    }

    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 changeCoverPositionAndSize(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, MI_SYS_WindowRect_t *pstRect, MI_U32 u32Color)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U32 u32XNew = 0;
    MI_U32 u32YNew = 0;
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_RGN_HANDLE *pu32RgnHandle = NULL;
    MI_RGN_ChnPortParam_t stRgnChnPortAttr;


    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VenChn(%d) is out of range!\n", s32VencChn);
        s32Ret = -1;
        return s32Ret;
    }

    if((0 > s32Idx) ||(s32Idx > MAX_RGN_NUMBER_PER_CHN))
    {
        MIXER_ERR("The input OSD index(%d) is out of range!\n", s32Idx);
        s32Ret = -1;
        return s32Ret;
    }

    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    pu32RgnHandle = &g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx];
    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == *pu32RgnHandle)
    {
      return  E_MI_ERR_NULL_PTR;
    }
    printf("changeCoverPositionAndSize handle =%d\n",*pu32RgnHandle);
    memset(&stRgnAttr, 0x00, sizeof(MI_RGN_Attr_t));
    s32Ret = MI_RGN_GetAttr(*pu32RgnHandle, &stRgnAttr);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetAttr(hdl=%d) error(0x%X)\n", *pu32RgnHandle, s32Ret);
        return s32Ret;
    }

    if((s32VencChn < g_s32VideoStreamNum) && (NULL != g_videoEncoderArray[s32VencChn]))
    {
        configOsdRgnChnPort(s32VencChn, &stRgnChnPort);
    }

    memset(&stRgnChnPortAttr, 0x00, sizeof(MI_RGN_ChnPortParam_t));
    s32Ret = MI_RGN_GetDisplayAttr(*pu32RgnHandle, &stRgnChnPort, &stRgnChnPortAttr);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetDisplayAttr(hdl=%d) error(0x%X)\n", *pu32RgnHandle, s32Ret);
        return s32Ret;
    }

    u32XNew = pstRect->u16X * RGN_COVER_MAX_WIDTH / g_stVideoSize[s32VencChn].u16Width;
    u32YNew = pstRect->u16Y * RGN_COVER_MAX_HEIGHT / g_stVideoSize[s32VencChn].u16Height;

    if((0x00 == pstRect->u16Width) && (0x00 == pstRect->u16Height))
    {
        if((u32XNew != stRgnChnPortAttr.stPoint.u32X) || (u32YNew != stRgnChnPortAttr.stPoint.u32Y))
        {
            stRgnChnPortAttr.stPoint.u32X = u32XNew;
            stRgnChnPortAttr.stPoint.u32Y = u32YNew;

            s32Ret = MI_RGN_SetDisplayAttr(*pu32RgnHandle, &stRgnChnPort, &stRgnChnPortAttr);
            if(MI_SUCCESS != s32Ret)
            {
                MIXER_ERR("MI_RGN_GetDisplayAttr(hdl=%d) error(0x%X)\n", *pu32RgnHandle, s32Ret);
                return s32Ret;
            }
        }
    }
    else if((u32XNew != stRgnChnPortAttr.stPoint.u32X) || (u32YNew != stRgnChnPortAttr.stPoint.u32Y) ||
            (pstRect->u16Width != stRgnAttr.stOsdInitParam.stSize.u32Height) ||
            (pstRect->u16Height != stRgnAttr.stOsdInitParam.stSize.u32Height))
    {
        ST_MIXER_RGN_WIDGET_ATTR stMixerRgnWidgetAttr;

        s32Ret = destroyOsdWidget(s32VencChn, *pu32RgnHandle);
        if(MI_RGN_OK != s32Ret)
        {
            MIXER_ERR("destroyOsdWidget error(0x%X), hdl=%d\n", s32Ret, *pu32RgnHandle);
            return s32Ret;
        }

        memset(&stMixerRgnWidgetAttr, 0x00, sizeof(ST_MIXER_RGN_WIDGET_ATTR));
        configRgnWidgetAttr(s32VencChn, s32Idx, pstRect, &stRgnChnPort, &stMixerRgnWidgetAttr,g_stMixerOsdWidgetOrder.pmt);
        stMixerRgnWidgetAttr.u32Color = u32Color;
        stMixerRgnWidgetAttr.eOsdWidgetType = E_OSD_WIDGET_TYPE_COVER;
        stMixerRgnWidgetAttr.bShow = TRUE;
        stMixerRgnWidgetAttr.eRgnpixelFormat = E_MI_RGN_PIXEL_FORMAT_I4;


        s32Ret = createOsdWidget(pu32RgnHandle, &stMixerRgnWidgetAttr);
        if(MI_SUCCESS != s32Ret)
        {
            MIXER_ERR("createOsdWidget() error(0x%X)\n", s32Ret);
            return s32Ret;
        }
    }

    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 changeCoverPositionByHandle(MI_RGN_HANDLE u32OsdHandle, MI_SYS_WindowRect_t *pstRect, MI_U32 u32Color)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;


    if((0 > u32OsdHandle) || (u32OsdHandle > (MI_RGN_HANDLE)MI_RGN_MAX_HANDLE))
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    s32Ret = getVideoChnByOsdHandle(u32OsdHandle, &s32VencChn, &s32Idx);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoChnByOsdHandle(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return s32Ret;
    }

    s32Ret = changeCoverPositionAndSize(s32VencChn, s32Idx, pstRect, u32Color);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("changeOsdPosition(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return s32Ret;
    }
    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 changeCoverPositionAndSizeByHandle(MI_RGN_HANDLE u32OsdHandle, MI_SYS_WindowRect_t *pstRect, MI_U32 u32Color)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;


    if((u32OsdHandle < 0) || (u32OsdHandle > MI_RGN_MAX_HANDLE))
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    s32Ret = getVideoChnByOsdHandle(u32OsdHandle, &s32VencChn, &s32Idx);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoChnByOsdHandle(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return s32Ret;
    }

    s32Ret = changeCoverPositionAndSize(s32VencChn, s32Idx, pstRect, u32Color);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("changeOsdPosition(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return s32Ret;
    }

    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 configOsdTimeStampCanvasSize(MI_VENC_CHN s32VencChn, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = MI_SUCCESS;


    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VencChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    pstRect->u16X = 0;
    pstRect->u16Y = 0;

    if(NULL != g_videoEncoderArray[s32VencChn] )
    {
        if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_pipRectW) && (1080 < g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (720 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (480 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
            else
            {
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
        }
        else
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
            else
            {
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
        }
    }
    return s32Ret;
}

MI_S32 configOsdVideoInfoCanvasSize(MI_VENC_CHN s32VencChn, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = MI_SUCCESS;


    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VencChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if( NULL != g_videoEncoderArray[s32VencChn] )
    {
        if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_pipRectW) && (1080 < g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (720 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (480 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH <= g_videoEncoderArray[s32VencChn]->m_pipRectW)
                     && (MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = g_videoEncoderArray[s32VencChn]->m_pipRectW;
                pstRect->u16Height = g_videoEncoderArray[s32VencChn]->m_pipRectH;
            }
        }
        else
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH <= g_videoEncoderArray[s32VencChn]->m_width)
                     && (MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = g_videoEncoderArray[s32VencChn]->m_width;
                pstRect->u16Height = g_videoEncoderArray[s32VencChn]->m_height;
            }
        }
    }
    return s32Ret;
}

MI_S32 configOsdIspInfoCanvasSize(MI_VENC_CHN s32VencChn, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = MI_SUCCESS;


    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VencChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if(NULL != g_videoEncoderArray[s32VencChn] )
    {
        if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_pipRectW) && (1080 < g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (720 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (480 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
            }
        }
        else
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
            }
        }
    }
    return s32Ret;
}

MI_S32 configOsdAudioInfoCanvasSize(MI_VENC_CHN s32VencChn, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = MI_SUCCESS;


    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VencChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if(NULL != g_videoEncoderArray[s32VencChn] )
    {
        if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_pipRectW) && (1080 < g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (720 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (480 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
            }
        }
        else
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
            }
        }
    }
    return s32Ret;
}

MI_S32 configOsdUser0InfoCanvasSize(MI_VENC_CHN s32VencChn, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = MI_SUCCESS;


    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VencChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if(NULL != g_videoEncoderArray[s32VencChn] )
    {
        if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_pipRectW) && (1080 < g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (720 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (480 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
        }
        else
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
        }
    }
    return s32Ret;
}

MI_S32 configOsdUser1InfoCanvasSize(MI_VENC_CHN s32VencChn, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = MI_SUCCESS;


    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VencChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if(NULL != g_videoEncoderArray[s32VencChn] )
    {
        if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_pipRectW) && (1080 < g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (720 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (480 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
        }
        else
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
        }
    }
    return s32Ret;
}

MI_S32 configOsdUser2InfoCanvasSize(MI_VENC_CHN s32VencChn, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = MI_SUCCESS;


    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VencChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if(NULL != g_videoEncoderArray[s32VencChn] )
    {
        if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_pipRectW) && (1080 < g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (720 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (480 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
        }
        else
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
        }
    }
    return s32Ret;
}

MI_S32 configOsdUser3InfoCanvasSize(MI_VENC_CHN s32VencChn, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = MI_SUCCESS;


    if(NULL == pstRect)
    {
        MIXER_ERR("The input Rgn windowRect pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VencChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if(NULL != g_videoEncoderArray[s32VencChn] )
    {
        if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_pipRectW) && (1080 < g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_USER2_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (720 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_USER2_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (480 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER2_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER2_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
        }
        else
        {
            if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * MIXER_OSD_USER2_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * MIXER_OSD_USER2_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER2_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
            else
            {
                pstRect->u16X = 0;
                pstRect->u16Y =   MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_TIME_STAMP_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_VIDEO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_ISP_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_AUDIO_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER0_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER1_INFO_HEIGHT_MAX_LENGTH
                                + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * MIXER_OSD_USER2_INFO_HEIGHT_MAX_LENGTH;
                pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_OSD_MAX_STRING_LENGTH;
                pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
            }
        }
    }
    return s32Ret;
}

MI_S32 initOsdMask(MI_VENC_CHN s32VencChn, MI_S32 s32Idx)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_Attr_t stRgnAttr;
#if TARGET_CHIP_I6B0
    MI_VENC_CHN s32VencChn_tmp = 0;
#endif

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VencChn(%d) is out of range!\n", s32VencChn);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if((0 > s32Idx) ||(s32Idx >= MAX_COVER_NUMBER_PER_CHN))
    {
        MIXER_ERR("The input COVER index(%d) is out of range!\n", s32Idx);
        s32Ret = -1;
        return s32Ret;
    }
#if TARGET_CHIP_I6B0
    if(checkVencInitOsdFull(s32VencChn,&s32VencChn_tmp) == TRUE)
    {
        MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",s32VencChn,s32VencChn_tmp,s32VencChn_tmp,s32VencChn);
        return 0;
    }
#endif

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    if(NULL != g_videoEncoderArray[s32VencChn])
    {
        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
        {
            MIXER_ERR("g_au32OsdCoverWidgetHandle[%d][%d] = %d, continue initial the next RGN handle!\n",
                                               s32VencChn,s32Idx, g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx]);
            pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
            return E_MI_ERR_FAILED;
        }
        else
        {
            g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx] = calcOsdHandle(s32VencChn, s32Idx, E_OSD_WIDGET_TYPE_COVER);
        }

        memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
        stRgnAttr.eType = E_MI_RGN_TYPE_COVER;

        s32Ret = MI_RGN_Create(g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx], &stRgnAttr);
        if(MI_RGN_OK != s32Ret)
        {
            MIXER_ERR("MI_RGN_Create error(hdl=%d), %X\n", g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx], s32Ret);
            pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
            return s32Ret;
        }

        printf("%s:%d Create Osd cover hdl=%d\n", __func__, __LINE__, g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx]);


        g_bMaskOsdCnt[s32VencChn]++;
        g_bMaskOsdInit[s32VencChn] = TRUE;
    }

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
    return 0;
}

MI_S32 uninitOsdMask(MI_VENC_CHN s32VencChn, MI_S32 s32Idx)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
#if TARGET_CHIP_I6B0
        MI_VENC_CHN s32VencChn_tmp = 0;
#endif
    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        MIXER_ERR("The input VencChn(%d) is out of range!g_s32VideoStreamNum=%d\n", s32VencChn,g_s32VideoStreamNum);
        s32Ret = E_MI_ERR_ILLEGAL_PARAM;
        return s32Ret;
    }

    if((0 > s32Idx) ||(s32Idx > MAX_COVER_NUMBER_PER_CHN))
    {
        MIXER_ERR("The input COVER index(%d) is out of range!\n", s32Idx);
        s32Ret = -1;
        return s32Ret;
    }
#if TARGET_CHIP_I6B0
    if(checkVencInitOsdFull(s32VencChn,&s32VencChn_tmp) == TRUE)
    {
        MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",s32VencChn,s32VencChn_tmp,s32VencChn_tmp,s32VencChn);
        return 0;
    }
#endif

    if(g_bMaskOsdInit[s32VencChn] == TRUE)
    {
        pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

        //for(MI_U32 i = 0; i < g_bMaskOsdCnt[s32VencChn]; i++)
        {
            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
            {
                s32Ret = MI_RGN_Destroy(g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx]);
                if(MI_RGN_OK != s32Ret)
                {
                    MIXER_ERR("MI_RGN_Destroy error(hdl=%d), %X\n", g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx], s32Ret);
                    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
                    return E_MI_ERR_FAILED;
                }

                g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx] = MI_RGN_HANDLE_NULL;
            }
        }

        g_bMaskOsdCnt[s32VencChn]--;

        if(0 > g_bMaskOsdCnt[s32VencChn])
        {
            g_bMaskOsdCnt[s32VencChn] = 0;
        }

        if(0 == g_bMaskOsdCnt[s32VencChn])
        {
            g_bMaskOsdInit[s32VencChn] = FALSE;
        }

        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
    }
    return 0;
}

int setCoverData(MI_VENC_CHN s32VencChn, MI_S32 s32Idx, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_RGN_ChnPortParam_t stRgnChnPortParam;
    MI_RGN_HANDLE OsdCoverWidgetHdl = MI_RGN_HANDLE_NULL;


    if(FALSE == g_bMaskOsdInit[s32VencChn])
    {
        printf("%s:%d please do mask initial first!\n", __func__, __LINE__);
        return s32Ret;
    }

    if(0 >= g_bMaskOsdCnt[s32VencChn])
    {
        printf("%s:%d Video channel-%d initial mask handle %d\n", __func__, __LINE__, s32VencChn, g_bMaskOsdCnt[s32VencChn]);
        return s32Ret;
    }

    if(g_bMaskOsdCnt[s32VencChn] < (MI_S32)(s32Idx - 1))
    {
        printf("%s:%d set MaskNum(%d) is large than initial mask handle %d\n", __func__, __LINE__, g_bMaskOsdCnt[s32VencChn], s32Idx);
        return s32Ret;
    }

    //for(MI_U32 i = 0; i < s32MaskNum; i++)
    {
        OsdCoverWidgetHdl = g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx];

        if((TRUE == g_bMaskOsdInit[s32VencChn]) && ((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != OsdCoverWidgetHdl) && (NULL != g_videoEncoderArray[s32VencChn]))
        {
            configOsdRgnChnPort(s32VencChn, &stRgnChnPort);

            memset(&stRgnChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
            stRgnChnPortParam.bShow = TRUE;
            stRgnChnPortParam.unPara.stCoverChnPort.u32Layer = OsdCoverWidgetHdl;
            stRgnChnPortParam.unPara.stCoverChnPort.u32Color = 0xff801080; //black

            if(NULL != pstRect)
            {
                stRgnChnPortParam.stPoint.u32X = pstRect->u16X;
                stRgnChnPortParam.stPoint.u32Y = pstRect->u16Y;
                stRgnChnPortParam.unPara.stCoverChnPort.stSize.u32Width  = pstRect->u16Width;
                stRgnChnPortParam.unPara.stCoverChnPort.stSize.u32Height = pstRect->u16Height;
                stRgnChnPortParam.unPara.stCoverChnPort.u32Color = 0xff6E29F0; //blue
            }
            else
            {
                if(0 == s32Idx)
                {
                    stRgnChnPortParam.stPoint.u32X = 0;
                    stRgnChnPortParam.stPoint.u32Y = 0;
                    stRgnChnPortParam.unPara.stCoverChnPort.stSize.u32Width  = MIXER_RGN_COVER_WIDTH;
                    stRgnChnPortParam.unPara.stCoverChnPort.stSize.u32Height = MIXER_RGN_COVER_HEIGHT;
                    stRgnChnPortParam.unPara.stCoverChnPort.u32Color = 0xff6E29F0; //blue
                }
                else if(1 == s32Idx)
                {
                    stRgnChnPortParam.stPoint.u32X = RGN_COVER_MAX_WIDTH - MIXER_RGN_COVER_WIDTH;
                    stRgnChnPortParam.stPoint.u32Y = 0;
                    stRgnChnPortParam.unPara.stCoverChnPort.stSize.u32Width  = MIXER_RGN_COVER_WIDTH;
                    stRgnChnPortParam.unPara.stCoverChnPort.stSize.u32Height = MIXER_RGN_COVER_HEIGHT;
                    stRgnChnPortParam.unPara.stCoverChnPort.u32Color = 0xffF0525B; //red
                }
                else if(2 == s32Idx)
                {
                    stRgnChnPortParam.stPoint.u32X = 0;
                    stRgnChnPortParam.stPoint.u32Y = RGN_COVER_MAX_HEIGHT - MIXER_RGN_COVER_WIDTH;
                    stRgnChnPortParam.unPara.stCoverChnPort.stSize.u32Width  = MIXER_RGN_COVER_WIDTH;
                    stRgnChnPortParam.unPara.stCoverChnPort.stSize.u32Height = MIXER_RGN_COVER_HEIGHT;
                    stRgnChnPortParam.unPara.stCoverChnPort.u32Color = 0xff239137; //green
                }
                else if(3 == s32Idx)
                {
                    stRgnChnPortParam.stPoint.u32X = RGN_COVER_MAX_WIDTH - MIXER_RGN_COVER_WIDTH;
                    stRgnChnPortParam.stPoint.u32Y = RGN_COVER_MAX_HEIGHT - MIXER_RGN_COVER_HEIGHT;
                    stRgnChnPortParam.unPara.stCoverChnPort.stSize.u32Width  = MIXER_RGN_COVER_WIDTH;
                    stRgnChnPortParam.unPara.stCoverChnPort.stSize.u32Height = MIXER_RGN_COVER_HEIGHT;
                    stRgnChnPortParam.unPara.stCoverChnPort.u32Color = 0xff801080; // black
                }
            }

            s32Ret = MI_RGN_AttachToChn(OsdCoverWidgetHdl, &stRgnChnPort, &stRgnChnPortParam);
            if(MI_RGN_OK != s32Ret)
            {
                MIXER_ERR("MI_RGN_AttachToChn error(hdl=%d), %X\n", OsdCoverWidgetHdl, s32Ret);
                return s32Ret;
            }
        }
    }

    return 0;
}

void setOsdPrivateMask(char *param, int paramLen)
{
    MI_SYS_WindowRect_t maskRect;
    MI_S32 s32VencChn = 0;
    MI_S32 s32Idx = 0;
    int iqparam[6];
#if TARGET_CHIP_I6B0
    MI_S32 s32VencChn_tmp = 0;
#endif
    memcpy(iqparam, param, sizeof(iqparam));
    s32VencChn = iqparam[0];
    s32Idx = iqparam[1];
    maskRect.u16X = iqparam[2];
    maskRect.u16Y = iqparam[3];
    maskRect.u16Width = iqparam[4];
    maskRect.u16Height = iqparam[5];

    if((s32VencChn < 0) || (s32VencChn > g_s32VideoStreamNum))
    {
        printf("%s:%d return for s32VencChn(=%d) error!\n", __func__, __LINE__, s32VencChn);
        return;
    }
    if(g_videoEncoderArray[s32VencChn]->m_width < ( maskRect.u16Width + maskRect.u16X ) || \
     g_videoEncoderArray[s32VencChn]->m_height < ( maskRect.u16Height +  maskRect.u16Y))
    {
      MIXER_DBG("param is error,out of current video resultion!\n");
      return;
    }
    if((s32Idx < 0) || (s32Idx > MAX_COVER_NUMBER_PER_CHN))
    {
        printf("%s:%d return for MaskIdx Num(=%d) error!\n", __func__, __LINE__, s32Idx);
        return;
    }
#if TARGET_CHIP_I6B0
    if(checkVencInitOsdFull(s32VencChn,&s32VencChn_tmp) == TRUE)
    {
        MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",s32VencChn,s32VencChn_tmp,s32VencChn_tmp,s32VencChn);
        return ;
    }
#endif
    //if((-1 == g_bMaskOsdCnt[s32VencChn]) && (FALSE == g_bMaskOsdInit[s32VencChn]))
    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
    {
        initOsdMask(s32VencChn, s32Idx);
    }
    else if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
    {
        uninitOsdMask(s32VencChn, s32Idx);
        initOsdMask(s32VencChn, s32Idx);
    }

    printf("%s:%d s32VencChn=%d, MaskIdx=%d, Cover hdl=%d\n", __func__,__LINE__, s32VencChn, s32Idx,
                            g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx]);

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    if(0 != setCoverData(s32VencChn, s32Idx, &maskRect))
    {
        printf("%s:%d create s32VencChn(%d) Cover data fail!\n", __func__, __LINE__, s32VencChn);
        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
        return;
    }

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    return ;
}

int MasktoRect(MI_VENC_CHN s32VencChn, MI_S32 s32MaskNum, MI_SYS_WindowRect_t *pstRect)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_S32 s32Idx = 0;
    bool  okFlag = false;
    if(NULL != pstRect)
    {
        if(g_videoEncoderArray[s32VencChn]->m_width < (pstRect->u16Width + pstRect->u16X ) || \
         g_videoEncoderArray[s32VencChn]->m_height < ( pstRect->u16Height + pstRect->u16Y))
        {
          MIXER_DBG("param is error,out of current video resultion!\n");
          return -1;
        }
    }
    if((0 > s32VencChn) || (s32VencChn > g_s32VideoStreamNum))
    {
        printf("%s:%d return for s32VencChn(=%d) error!\n", __func__, __LINE__, s32VencChn);
        return s32Ret;
    }

    if((0 > s32MaskNum) || (MAX_COVER_NUMBER_PER_CHN < s32MaskNum))
    {
        printf("%s:%d return for MaskNum(=%d) error!\n", __func__, __LINE__, s32MaskNum);
        return s32Ret;
    }

    for(s32Idx = 0; s32Idx < MAX_COVER_NUMBER_PER_CHN; s32Idx++)
    {
        if(s32Idx < s32MaskNum)
        {
        //if((-1 == g_bMaskOsdCnt[s32VencChn]) && (FALSE == g_bMaskOsdInit[s32VencChn]))
            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
            {
                if(0 != initOsdMask(s32VencChn, s32Idx))
                {
                    continue;
                }
                okFlag = true;
            }
            else if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
            {
                uninitOsdMask(s32VencChn, s32Idx);
                if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])
                {
                      if(0 != initOsdMask(s32VencChn, s32Idx))
                      {
                        continue;
                      }
                      okFlag = true;
                    }
                else
                {
                      continue;
                }
            }
        }
        else if(g_bMaskOsdCnt[s32VencChn] != s32MaskNum)
        {
            uninitOsdMask(s32VencChn, s32Idx);
        }
    }
    if(false == okFlag)
    {
      printf("All osd Handle is vailed! Maybe not call initOsdModule(..) to init osd\n");
      return s32Ret;
    }
    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    for(s32Idx = 0; s32Idx < s32MaskNum; s32Idx++)
    {
        if((0 < s32MaskNum) && (0 != setCoverData(s32VencChn, s32Idx, pstRect)))
        {
            okFlag = false;
            printf("%s:%d create s32VencChn(%d) Cover data fail!\n", __func__, __LINE__, s32VencChn);
            pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
            return s32Ret;
        }
        okFlag = false;
        MIXER_INFO("%s:%d Cover hdl=%d, MaskCnt=%d, MaskOsdInit=%d g_bIEOsdInit=%d s32VencChn=%d, s32MaskNum=%d\n", __func__,__LINE__,
                    g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx], g_bMaskOsdCnt[s32VencChn], g_bMaskOsdInit[s32VencChn], g_bIEOsdInit, s32VencChn, s32MaskNum);
    }

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
    return MI_SUCCESS;
}

#if TARGET_CHIP_I5
int FDtoRECT(MI_VENC_CHN s32VencChn, int FaceNum_Detected, FacePos_t* face_position, FRFaceInfo_t*  frInfo, int show)
{
    int i;
    static MI_S32 fd_detect_cnt = 0;
    static MI_S32 fr_detect_cnt = 0;
    RectWidgetAttr_t stRectWidgetAttr;
    MI_RGN_HANDLE hHandle = g_au32OsdRectWidgetHandle[s32VencChn];

    if((0 >= g_s32OsdRectHandleCnt) || (TRUE == g_bOsdSetResulution[s32VencChn]))
    {
        return 0;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    for(i = 0;  i < MAX_FD_RECT_NUMBER; i++)
    {
        /*clear fd OSD */
        if((0 != g_stFdRect[i].u16Width) && (0 != g_stFdRect[i].u16Height) && (i < fd_detect_cnt))
        {
            cleanOsdRectWidget(hHandle, g_stFdRect, fd_detect_cnt, 1, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
        }

        /*clear fr OSD */
        if((0 != g_stFrRect[i].u16Width) && (0 != g_stFrRect[i].u16Height) && (i < fr_detect_cnt))
        {
            cleanOsdTextWidget(hHandle, NULL, &g_stFrRect[i], MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
        }
    }

    memset(&g_stFdRect[0], 0x00, sizeof(MI_SYS_WindowRect_t) * MAX_FD_RECT_NUMBER);
    memset(&g_stFrRect[0], 0x00, sizeof(MI_SYS_WindowRect_t) * MAX_FD_RECT_NUMBER);
    fd_detect_cnt = 0;
    fr_detect_cnt = 0;

    if(!show){
        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
        return 0;
    }

    for(i = 0;  i < FaceNum_Detected && i < MAX_FD_RECT_NUMBER; i++)
    {
        /*show fd coordinates*/
        g_stFdRect[fd_detect_cnt].u16X = (getVideoWidth(s32VencChn)* face_position[i].posx / g_ieWidth) & 0xFFFE;
        g_stFdRect[fd_detect_cnt].u16Y = (getVideoHeight(s32VencChn) * face_position[i].posy / g_ieHeight) & 0xFFFE;
        g_stFdRect[fd_detect_cnt].u16Width = (getVideoWidth(s32VencChn) * face_position[i].posw / g_ieWidth) & 0xFFFE;
        g_stFdRect[fd_detect_cnt].u16Height = (getVideoHeight(s32VencChn) * face_position[i].posh / g_ieHeight) & 0xFFFE;
        /*
        printf("hand:%d i=%d posx=%d posy=%d posw=%d posh=%d\n",
                (int)hHandle, i, face_position[i].posx, face_position[i].posy, face_position[i].posw, face_position[i].posh);
        */
        fd_detect_cnt++;
        /*show fr names*/
        if(frInfo[i].store_idx != -1)
        {
            Point_t point = {0};
            TextWidgetAttr_t stTextWidgetAttr;

            point.x = g_stFdRect[i].u16X;
            point.y = g_stFdRect[i].u16Y;

            memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
            stTextWidgetAttr.string  = (char *)(frInfo[i].name);
            stTextWidgetAttr.pPoint  = &point;
            stTextWidgetAttr.size    = FONT_SIZE_32;
            stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
            stTextWidgetAttr.pfColor = &g_stGreenColor;
            stTextWidgetAttr.pbColor = &g_stBlackColor;

            stTextWidgetAttr.space = 0;
            //stTextWidgetAttr.bHard = FALSE;
            //stTextWidgetAttr.bRle  = FALSE;
            stTextWidgetAttr.bOutline = FALSE;
          //  osdInfo.osd_union.osdRectTxt.point.u16X = pstTextWidgetAttr;
            updateOsdTextWidget(hHandle, NULL, &stTextWidgetAttr, TRUE);

#if 0
            osdInfo_t osdInfo;
            memset(&osdInfo,0,sizeof(osdInfo_t));
            osdInfo.osdType = E_OSD_WIDGET_TYPE_RECT;
            osdInfo.u32RgnHandle = hHandle;
            osdInfo.osd_union.osdRectTxt.point.u16X = point.x ;
            osdInfo.osd_union.osdRectTxt.point.u16Y = point.y;
            midOsdInfoUpdate(&osdInfo);
#endif

            g_stFrRect[fr_detect_cnt].u16X = point.x;
            g_stFrRect[fr_detect_cnt].u16Y = point.y;
            if(FONT_SIZE_16 == stTextWidgetAttr.size)
            {
                g_stFrRect[fr_detect_cnt].u16Width  = 16;
                g_stFrRect[fr_detect_cnt].u16Height = 16;
            }
            else if(FONT_SIZE_24 == stTextWidgetAttr.size)
            {
                g_stFrRect[fr_detect_cnt].u16Width  = 24;
                g_stFrRect[fr_detect_cnt].u16Height = 24;
            }
            else if(FONT_SIZE_32 == stTextWidgetAttr.size)
            {
                g_stFrRect[fr_detect_cnt].u16Width  = 32;
                g_stFrRect[fr_detect_cnt].u16Height = 32;
            }
            else if(FONT_SIZE_64 == stTextWidgetAttr.size)
            {
                g_stFrRect[fr_detect_cnt].u16Width  = 64;
                g_stFrRect[fr_detect_cnt].u16Height = 64;
            }
            else
            {
                g_stFrRect[fr_detect_cnt].u16Width  = 32;
                g_stFrRect[fr_detect_cnt].u16Height = 32;
            }

            fr_detect_cnt++;
        }
    }

    stRectWidgetAttr.pstRect = &g_stFdRect[0];
    stRectWidgetAttr.s32RectCnt = fd_detect_cnt;
    stRectWidgetAttr.u8BorderWidth = 1;
    stRectWidgetAttr.pmt = g_stMixerOsdWidgetOrder.pmt;
    stRectWidgetAttr.pfColor = &g_stRedColor;
    stRectWidgetAttr.pbColor = &g_stBlackColor;
    stRectWidgetAttr.bFill = FALSE;

    updateOsdRectWidget(hHandle, &stRectWidgetAttr, TRUE);

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    return 0;
}
#endif

int ODtoRECT(MI_VENC_CHN s32VencChn, MI_OD_Result_t stOdResult)
{
    int i, j;
    static MI_S32 od_detect_cnt = 0;
    RectWidgetAttr_t stRectWidgetAttr;
    MI_RGN_HANDLE hHandle = g_au32OsdRectWidgetHandle[s32VencChn];
    MI_U8 u8BorderWidth = 1;
    MI_SYS_WindowRect_t od_rect[OD_DIV_W * OD_DIV_H] = {{0,},};
    MI_S32 od_cnt = 0;
    int idx;

    if((0 >= g_s32OsdRectHandleCnt) || (TRUE == g_bOsdSetResulution[s32VencChn]))
    {
        return 0;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    // draw rect
    if(stOdResult.u8Enable > 0)
    {
        for(i = 0; i < OD_DIV_W; i++)
        {
            for(j = 0; j < OD_DIV_H; j++)
            {
                if(1 == stOdResult.u8RgnAlarm[i][j])
                {
                    od_rect[od_cnt].u16X = i * getVideoWidth(s32VencChn) / OD_DIV_W  + 32;
                    od_rect[od_cnt].u16Y = j * getVideoHeight(s32VencChn) / OD_DIV_H + 32;
                    od_rect[od_cnt].u16Width =  getVideoWidth(s32VencChn) / OD_DIV_W - 64 ;
                    od_rect[od_cnt].u16Height = getVideoHeight(s32VencChn) / OD_DIV_H - 64;

                    od_rect[od_cnt].u16X &= 0xFFFE;
                    od_rect[od_cnt].u16Y &= 0xFFFE;
                    od_rect[od_cnt].u16Width   &= 0xFFFE;
                    od_rect[od_cnt].u16Height  &= 0xFFFE;

                    for(idx = 0; idx < od_detect_cnt; idx++)
                    {
                        if( g_stOdRect[idx].u16X == od_rect[od_cnt].u16X &&
                            g_stOdRect[idx].u16Y == od_rect[od_cnt].u16Y &&
                            g_stOdRect[idx].u16Width == od_rect[od_cnt].u16Width &&
                            g_stOdRect[idx].u16Height == od_rect[od_cnt].u16Height )
                        {
                            g_stOdRect[idx].u16Width = 0;
                            g_stOdRect[idx].u16Height = 0;
                        }
                    }

                    //printf("osd idx %d, x %d,y %d,w %d,h %d\n",od_detect_cnt, g_stOdRect[od_detect_cnt].x, g_stOdRect[od_detect_cnt].y,
                    //                              g_stOdRect[od_detect_cnt].width,g_stOdRect[od_detect_cnt].height);

                    od_cnt++;
                }
            }
        }
        // cls prev rect
        cleanOsdRectWidget(hHandle, g_stOdRect, od_detect_cnt, u8BorderWidth, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

        stRectWidgetAttr.pstRect = &od_rect[0];
        stRectWidgetAttr.s32RectCnt = od_cnt;
        stRectWidgetAttr.u8BorderWidth = u8BorderWidth;
        stRectWidgetAttr.u32Color = 0;   //0:red; 1:green; 2:blue;
        stRectWidgetAttr.pmt = g_stMixerOsdWidgetOrder.pmt;
        stRectWidgetAttr.pfColor = &g_stGreenColor;
        stRectWidgetAttr.pbColor = &g_stBlackColor;
        stRectWidgetAttr.bFill = FALSE;
        //stRectWidgetAttr.bHard = FALSE;
        //stRectWidgetAttr.bOutline = FALSE;
        updateOsdRectWidget(hHandle, &stRectWidgetAttr, TRUE);
        memcpy(g_stOdRect, od_rect, sizeof(od_rect));
        od_detect_cnt = od_cnt;
    }
    else
    {
        if(od_detect_cnt > 0)
        {
            cleanOsdRectWidget(hHandle, g_stOdRect, od_detect_cnt, u8BorderWidth, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
            od_detect_cnt = 0;
        }
    }
    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    return 0;
}

#if TARGET_CHIP_I5
int DLAtoRECT(MI_VENC_CHN s32VencChn, int recCnt, ST_DlaRectInfo_T* pRecInfo, int show)
{
    int i;
    static MI_S32 dla_detect_cnt = 0;
    RectWidgetAttr_t stRectWidgetAttr;
    MI_RGN_HANDLE hHandle = g_au32OsdRectWidgetHandle[s32VencChn];
    MI_SYS_WindowRect_t stOsdRect;
    ST_Point_T stPoint;
    MI_RGN_CanvasInfo_t stRgnCanvasInfo;

    if((0 >= g_s32OsdRectHandleCnt) || (TRUE == g_bOsdSetResulution[s32VencChn]))
    {
         MIXER_ERR("param err.  osdhandlecnt:%d,   %d\n", g_s32OsdRectHandleCnt, g_bOsdSetResulution[s32VencChn]);
        return 0;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    // cls osd
    stOsdRect.u16X = 0;
    stOsdRect.u16Y = 0;
    stOsdRect.u16Width = getVideoWidth(s32VencChn);
    stOsdRect.u16Height = getVideoHeight(s32VencChn);
    cleanOsdTextWidget(hHandle, NULL, &stOsdRect, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

    memset(&g_stDLARect[0], 0x00, sizeof(MI_SYS_WindowRect_t) * MAX_DLA_RECT_NUMBER);
    dla_detect_cnt = 0;

    if(!show){
        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
        return 0;
    }

    // osd name
    pthread_mutex_lock(&g_stMutexMixerOsdTextRgnBuf);
    if(MI_RGN_OK != MI_RGN_GetCanvasInfo(hHandle, &stRgnCanvasInfo))
    {
        printf("%s:%d call MI_RGN_GetCanvasInfo() fail!\n", __func__,__LINE__);
        pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
        return 0;
    }
    for(i = 0;  i < recCnt && i < MAX_DLA_RECT_NUMBER; i++)
    {
       /*show dla coordinates*/
        g_stDLARect[dla_detect_cnt].u16X = (getVideoWidth(s32VencChn)* pRecInfo[i].rect.u32X / g_ieWidth) & 0xFFFE;
        g_stDLARect[dla_detect_cnt].u16Y = (getVideoHeight(s32VencChn) * pRecInfo[i].rect.u32Y / g_ieHeight) & 0xFFFE;
        g_stDLARect[dla_detect_cnt].u16Width = (getVideoWidth(s32VencChn) * pRecInfo[i].rect.u16PicW / g_ieWidth) & 0xFFFE;
        g_stDLARect[dla_detect_cnt].u16Height = (getVideoHeight(s32VencChn) * pRecInfo[i].rect.u16PicH / g_ieHeight) & 0xFFFE;
        stPoint.u32X = g_stDLARect[dla_detect_cnt].u16X;
        stPoint.u32Y = g_stDLARect[dla_detect_cnt].u16Y;
        MIXER_DBG("%d x=%d, y=%d, szString=%s\n",dla_detect_cnt, stPoint.u32X, stPoint.u32Y, pRecInfo[i].szObjName);
        drawOsdText2Canvas(&stRgnCanvasInfo, stPoint, (MI_S8 *)pRecInfo[i].szObjName,2);
        dla_detect_cnt++;
    }
    if(MI_RGN_UpdateCanvas(hHandle) != MI_RGN_OK)
    {
        MIXER_ERR("MI_RGN_UpdateCanvas fail\n");
    }
    pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);

    stRectWidgetAttr.pstRect = &g_stDLARect[0];
    stRectWidgetAttr.s32RectCnt = dla_detect_cnt;
    stRectWidgetAttr.u8BorderWidth = 1;
    stRectWidgetAttr.pmt = g_stMixerOsdWidgetOrder.pmt;
    stRectWidgetAttr.pfColor = &g_stRedColor;
    stRectWidgetAttr.pbColor = &g_stBlackColor;
    stRectWidgetAttr.bFill = FALSE;
    updateOsdRectWidget(hHandle, &stRectWidgetAttr, TRUE);

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    return 0;
}

#elif TARGET_CHIP_I6E
int DLAtoRECT(MI_VENC_CHN s32VencChn, int recCnt, ST_DlaRectInfo_T* pRecInfo, MI_BOOL bShow, MI_BOOL bShowBorder)
{
#if 0
    int i;
    static MI_S32 dla_detect_cnt = 0;
    RectWidgetAttr_t stRectWidgetAttr;
    MI_RGN_HANDLE hHandle = g_au32OsdRectWidgetHandle[s32VencChn];
    MI_SYS_WindowRect_t stOsdRect;
    ST_Point_T stPoint;
    MI_RGN_CanvasInfo_t stRgnCanvasInfo;

    if((0 >= g_s32OsdRectHandleCnt) || (TRUE == g_bOsdSetResulution[s32VencChn]))
    {
         MIXER_ERR("param err.  osdhandlecnt:%d,   %d\n", g_s32OsdRectHandleCnt, g_bOsdSetResulution[s32VencChn]);
        return 0;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    // cls osd
    stOsdRect.u16X = 0;
    stOsdRect.u16Y = 0;
    stOsdRect.u16Width = getVideoWidth(s32VencChn);
    stOsdRect.u16Height = getVideoHeight(s32VencChn);
    cleanOsdTextWidget(hHandle, NULL, &stOsdRect, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

    memset(&g_stDLARect[0], 0x00, sizeof(MI_SYS_WindowRect_t) * MAX_DLA_RECT_NUMBER);
    dla_detect_cnt = 0;

    if(!bShow){
        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
        return 0;
    }

    // osd name
    pthread_mutex_lock(&g_stMutexMixerOsdTextRgnBuf);
    if(MI_RGN_OK != MI_RGN_GetCanvasInfo(hHandle, &stRgnCanvasInfo))
    {
        printf("%s:%d call MI_RGN_GetCanvasInfo() fail!\n", __func__,__LINE__);
        pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
        return 0;
    }
    for(i = 0;  i < recCnt && i < MAX_DLA_RECT_NUMBER; i++)
    {
       /*show dla coordinates*/
        g_stDLARect[dla_detect_cnt].u16X = (getVideoWidth(s32VencChn)* pRecInfo[i].rect.u32X / 1920) & 0xFFFE;
        g_stDLARect[dla_detect_cnt].u16Y = (getVideoHeight(s32VencChn) * pRecInfo[i].rect.u32Y / 1080) & 0xFFFE;
        g_stDLARect[dla_detect_cnt].u16Width = (getVideoWidth(s32VencChn) * pRecInfo[i].rect.u16PicW / 1920) & 0xFFFE;
        g_stDLARect[dla_detect_cnt].u16Height = (getVideoHeight(s32VencChn) * pRecInfo[i].rect.u16PicH / 1080) & 0xFFFE;
        stPoint.u32X = g_stDLARect[dla_detect_cnt].u16X;
        stPoint.u32Y = g_stDLARect[dla_detect_cnt].u16Y;

        #if 0
        MIXER_DBG("\ni:%d, x:%d, y:%d, w:%d, h:%d, name:%s\n", i, g_stDLARect[dla_detect_cnt].u16X,\
                                                        g_stDLARect[dla_detect_cnt].u16Y,\
                                                        g_stDLARect[dla_detect_cnt].u16Width,\
                                                        g_stDLARect[dla_detect_cnt].u16Height,\
                                                        pRecInfo[i].szObjName);
        #endif

        drawOsdText2Canvas(&stRgnCanvasInfo, stPoint, (MI_S8 *)pRecInfo[i].szObjName);
        dla_detect_cnt++;
    }
    if(MI_RGN_UpdateCanvas(hHandle) != MI_RGN_OK)
    {
        MIXER_ERR("MI_RGN_UpdateCanvas fail\n");
    }

    pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
    if (bShowBorder)
    {
        stRectWidgetAttr.pstRect = &g_stDLARect[0];
        stRectWidgetAttr.s32RectCnt = dla_detect_cnt;
        stRectWidgetAttr.u8BorderWidth = 4;
        stRectWidgetAttr.pmt = g_stMixerOsdWidgetOrder.pmt;
        stRectWidgetAttr.pfColor = &g_stRedColor;
        stRectWidgetAttr.pbColor = &g_stBlackColor;
        stRectWidgetAttr.bFill = FALSE;
        updateOsdRectWidget(hHandle, &stRectWidgetAttr, TRUE);
    }
    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
#else
    MI_U32 size = 0x0;
    stRectList *mRectList;

    if(recCnt <= 0x0)
    {
    //    MIXER_ERR("param err, recCnt should not be zero.\n");
        return -1;
    }

    if(list_empty(&g_EmptyRectList))
    {
    //    MIXER_ERR("no empty rect list.\n");
        return 0;
    }

    mRectList = list_entry(g_EmptyRectList.next, stRectList, rectlist);
    if(NULL == mRectList)
    {
        //MIXER_ERR("no find mRectList.\n");
        return -1;
    }

    size = recCnt * sizeof(ST_DlaRectInfo_T);

    mRectList->pChar= (MI_U8 *)malloc(size);
    if(NULL == mRectList->pChar)
    {
        MIXER_ERR("can not malloc size:%d.\n", size);
        return -1;
    }

    mRectList->tCount = recCnt;

    memcpy((MI_S8 *)mRectList->pChar, (MI_S8 *)pRecInfo, recCnt * sizeof(ST_DlaRectInfo_T));

    pthread_mutex_lock(&g_stMutexMixerOsdTextRgnBuf);
    //delete from empty list
    list_del(&mRectList->rectlist);

    //add this rect to work list
    list_add_tail(&mRectList->rectlist, &g_WorkRectList);
//    MIXER_DBG("mRectList: %p.\n", &mRectList->rectlist);
    pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);

#endif

    return 0;
}


#endif

int _MDSadMdNumCal(MI_U8* pu8MdRstData,int i, int j, int col, int row, int cusCol, int cusRow)
{
    int c,r;
    int rowIdx = 0;
    int sad8BitThr = 20;
    int mdNum = 0;

    if(g_s32MdFgMode) sad8BitThr = 0;

    for(r = 0; r < cusRow; r++)
    {
        rowIdx = (i+r)*col+j;
        for(c = 0; c < cusCol; c++)
        {
            if(pu8MdRstData[rowIdx+c] > sad8BitThr) mdNum ++;
        }
    }

    return mdNum;
}

int MultiMDtoRECT_SAD(MI_VENC_CHN s32VencChn, MI_U8* pu8MdRstData, int col, int row, int enable)
{
    int i, j;
    static MI_S32 md_detect_cnt = 0;
    RectWidgetAttr_t stRectWidgetAttr;
    MI_RGN_HANDLE hHandle = g_au32OsdRectWidgetHandle[s32VencChn];
    MI_U8 u8BorderWidth = 1;
    //MI_SYS_WindowRect_t md_rect[(RAW_W/4)*(RAW_H/4)] = {{0,},};
    MI_S32 md_cnt = 0;

    MI_SYS_WindowRect_t *md_rect = (MI_SYS_WindowRect_t *)malloc(col * row * sizeof(MI_SYS_WindowRect_t) );
    if(NULL == md_rect)
    {
        MIXER_ERR("can not alloc md_rect buf.\n");
        return -1;
    }

    if((0 >= g_s32OsdRectHandleCnt) || (TRUE == g_bOsdSetResulution[s32VencChn]))
    {
        if(NULL != md_rect)
        {
            free(md_rect);
            md_rect = NULL;
        }
        return 0;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);
    hHandle = g_au32OsdRectWidgetHandle[s32VencChn];

    int cusCol = 4;     // 4 macro block Horizontal
    int cusRow = 2;     // 3 macro block vertical
    int idx = 0;

    // draw rect
    if(enable && pu8MdRstData)
    {
        for(i = 0; i < row; i+=cusRow)
        {
            for(j = 0; j < col; j+=cusCol)
            {
                // clac all macro block result
                if(_MDSadMdNumCal(pu8MdRstData,i,j,col,row,cusCol,cusRow) > 1)
                {
                    md_rect[md_cnt].u16X = (j * getVideoWidth(s32VencChn)/col) & 0xFFFE;
                    md_rect[md_cnt].u16Y = (i * getVideoHeight(s32VencChn)/row) & 0xFFFE;
                    md_rect[md_cnt].u16Width = (cusCol*getVideoWidth(s32VencChn)/col) & 0xFFFE;
                    md_rect[md_cnt].u16Height = (cusRow*getVideoHeight(s32VencChn)/row) & 0xFFFE;
                    while(md_rect[md_cnt].u16X + md_rect[md_cnt].u16Width >= getVideoWidth(s32VencChn))
                        md_rect[md_cnt].u16Width -= 2;
                    while(md_rect[md_cnt].u16Y + md_rect[md_cnt].u16Height >= getVideoHeight(s32VencChn))
                        md_rect[md_cnt].u16Height -= 2;

//                  printf("osd idx %d, x %d,y %d,w %d,h %d\n",md_cnt, md_rect[md_cnt].u16X, md_rect[md_cnt].u16Y,
//                                                  md_rect[md_cnt].u16Width,md_rect[md_cnt].u16Height);
                    for(idx = 0; idx < md_detect_cnt; idx++)
                    {
                        if( g_stMdRect[idx].u16X == md_rect[md_cnt].u16X &&
                            g_stMdRect[idx].u16Y == md_rect[md_cnt].u16Y &&
                            g_stMdRect[idx].u16Width == md_rect[md_cnt].u16Width &&
                            g_stMdRect[idx].u16Height == md_rect[md_cnt].u16Height )
                        {
                            g_stMdRect[idx].u16Width = 0;
                            g_stMdRect[idx].u16Height = 0;
                        }
                    }
                    md_cnt++;
                }
            }
        }
        // cls prev rect
        cleanOsdRectWidget(hHandle, g_stMdRect, md_detect_cnt, u8BorderWidth, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

        stRectWidgetAttr.pstRect = &md_rect[0];
        stRectWidgetAttr.s32RectCnt = md_cnt;
        stRectWidgetAttr.u8BorderWidth = u8BorderWidth;
        stRectWidgetAttr.u32Color = 2;   //0:red; 1:green; 2:blue;
        stRectWidgetAttr.pmt = g_stMixerOsdWidgetOrder.pmt;
        stRectWidgetAttr.pfColor = &g_stGreenColor;
        stRectWidgetAttr.pbColor = &g_stBlackColor;
        stRectWidgetAttr.bFill = FALSE;
        //stRectWidgetAttr.bHard = FALSE;
        //stRectWidgetAttr.bOutline = FALSE;
        updateOsdRectWidget(hHandle, &stRectWidgetAttr, TRUE);
        memcpy(g_stMdRect, md_rect,  sizeof(g_stMdRect) > (col * row * sizeof(MI_SYS_WindowRect_t)) ? \
                        (col * row * sizeof(MI_SYS_WindowRect_t)) : sizeof(g_stMdRect));
        md_detect_cnt = md_cnt;
    }
    else
    {
        if(md_detect_cnt > 0)
        {
            cleanOsdRectWidget(hHandle, g_stMdRect, md_detect_cnt, u8BorderWidth, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
            md_detect_cnt = 0;
        }
    }

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    if(NULL != md_rect)
    {
        free(md_rect);
        md_rect = NULL;
    }

    return 0;
}

int MultiMDtoRECT_FG(MI_VENC_CHN s32VencChn, MDOBJ_t* pstRegion, int regionNum, int enable)
{
    int j;
    MI_SYS_WindowRect_t rect;
    static MI_S32 md_detect_cnt = 0;
    RectWidgetAttr_t stRectWidgetAttr;
    MI_RGN_HANDLE hHandle = g_au32OsdRectWidgetHandle[s32VencChn];
    MI_U8 u8BorderWidth = 1;
    MI_SYS_WindowRect_t md_rect[(RAW_W/4)*(RAW_H/4)] = {{0,},};
    MI_S32 md_cnt = 0;
    int idx = 0;

    if((0 >= g_s32OsdRectHandleCnt) || (TRUE == g_bOsdSetResulution[s32VencChn]))
    {
        return 0;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    // draw rect
    if(enable && regionNum > 0)
    {
        for(j=0; j < regionNum; j++)
        {
            rect.u16X = pstRegion[j].u16Left;
            rect.u16Y = pstRegion[j].u16Top;
            rect.u16Width = pstRegion[j].u16Right - rect.u16X;
            rect.u16Height = pstRegion[j].u16Bottom - rect.u16Y;
            md_rect[md_cnt].u16X = (rect.u16X * getVideoWidth(s32VencChn)/g_ieWidth) & 0xFFFE;
            md_rect[md_cnt].u16Y = (rect.u16Y * getVideoHeight(s32VencChn)/g_ieHeight) & 0xFFFE;
            md_rect[md_cnt].u16Width = (rect.u16Width*getVideoWidth(s32VencChn)/g_ieWidth) & 0xFFFE;
            md_rect[md_cnt].u16Height = (rect.u16Height*getVideoHeight(s32VencChn)/g_ieHeight) & 0xFFFE;
//            printf("osd idx %d(%d), x %d,y %d,w %d,h %d\n",md_cnt,regionNum, md_rect[md_cnt].u16X, md_rect[md_cnt].u16Y,
//                                          md_rect[md_cnt].u16Width,md_rect[md_cnt].u16Height);
            for(idx = 0; idx < md_detect_cnt; idx++)
            {
                if( g_stMdRect[idx].u16X == md_rect[md_cnt].u16X &&
                    g_stMdRect[idx].u16Y == md_rect[md_cnt].u16Y &&
                    g_stMdRect[idx].u16Width == md_rect[md_cnt].u16Width &&
                    g_stMdRect[idx].u16Height == md_rect[md_cnt].u16Height )
                {
                    g_stMdRect[idx].u16Width = 0;
                    g_stMdRect[idx].u16Height = 0;
                }
            }
            md_cnt++;
        }
        // cls prev rect
        cleanOsdRectWidget(hHandle, g_stMdRect, md_detect_cnt, u8BorderWidth, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

        stRectWidgetAttr.pstRect = &md_rect[0];
        stRectWidgetAttr.s32RectCnt = md_cnt;
        stRectWidgetAttr.u8BorderWidth = u8BorderWidth;
        stRectWidgetAttr.u32Color = 2;   //0:red; 1:green; 2:blue;
        stRectWidgetAttr.pmt = g_stMixerOsdWidgetOrder.pmt;
        stRectWidgetAttr.pfColor = &g_stGreenColor;
        stRectWidgetAttr.pbColor = &g_stBlackColor;
        stRectWidgetAttr.bFill = FALSE;
        //stRectWidgetAttr.bHard = FALSE;
        //stRectWidgetAttr.bOutline = FALSE;
        updateOsdRectWidget(hHandle, &stRectWidgetAttr, TRUE);
        memcpy(g_stMdRect, md_rect, sizeof(md_rect));
        md_detect_cnt = md_cnt;
    }
    else
    {
        if(md_detect_cnt > 0)
        {
            cleanOsdRectWidget(hHandle, g_stMdRect, md_detect_cnt, u8BorderWidth, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
            md_detect_cnt = 0;
        }
    }

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    return 0;
}

int MultiMDtoRECT_FG2(MI_VENC_CHN s32VencChn, MI_U8* pu8MdRstData, int col, int row, int enable)
{
    g_s32MdFgMode = 1;
    MultiMDtoRECT_SAD(s32VencChn, pu8MdRstData, col, row, enable);
    g_s32MdFgMode = 0;

    return 0x0;
}

////////////////////////////////////////////////////////////////////////
static int _I4_osd_draw_Point(Point_t point,int color,MI_RGN_CanvasInfo_t *ptCanvasinfo)
{
    int ret = 0;
    int x,y,n;
    unsigned char *pDst = (unsigned char *)ptCanvasinfo->virtAddr;
    x = MIN((unsigned int)point.x,ptCanvasinfo->stSize.u32Width-1);
    y = MIN((unsigned int)point.y,ptCanvasinfo->stSize.u32Height-1);
   
    n = x/2+y*ptCanvasinfo->u32Stride;	//I4 format

    switch(x%2){
    case 0:pDst[n]&=0xF0;pDst[n]|=color<<0;break;
    case 1:pDst[n]&=0x0F;pDst[n]|=color<<4;break;
    default:break;
    }
    return ret;
}


static int _I4_osd_draw_line(Point_t point0,Point_t point1,int borderwidth, int color,MI_RGN_CanvasInfo_t *ptCanvasinfo)
{
    int x, y, i, j;
	float k, dx, dy;
    Point_t point;

    if (point0.x == point1.x){
        x = point0.x;   
        if (point1.y > point0.y){
            dy = point1.y - point0.y;
            y = point0.y;
        }else{
            dy = point0.y - point1.y;
            y = point1.y;
        }
        
	    for (j = 0; j < borderwidth; j ++){
            for (i = 0; i < dy; i ++){
    		    point.x = x + j;
                point.y = y + i;
    			_I4_osd_draw_Point(point,color,ptCanvasinfo);
    		}
        }
		return 0;
    }

    if (point0.y == point1.y){
        y = point0.y;
        if (point1.x > point0.x){
            dx = point1.x - point0.x;
            x = point0.x;
        }else{
            dx = point0.x - point1.x;
            x = point1.x;
        }
        for (j = 0; j < borderwidth; j ++){
            for (i = 0; i < dx; i ++){
                point.x = x + i;
                point.y = y + j;
    			_I4_osd_draw_Point(point,color,ptCanvasinfo);
            }
        }
        return 0;
    }

//    EDBG("point0 %d:%d point1 %d:%d borderwidth:%d \n",point0.u32X,point0.u32Y,point1.u32X,point1.u32Y,borderwidth);
    if (point1.x > point0.x){
        dx = point1.x - point0.x;
        x = point0.x;
        y = point0.y;
        if (point1.y > point0.y){
//            EDBG("X+ Y+ \n");
            dy = point1.y - point0.y;
            if (dy > dx){
                k = (float)(dx /dy);
                for (i = 0; i < dy; i ++){
            	    point.x = x+i*k;
                    point.y = y+i;  
                    for (j = 0; j < borderwidth; j ++){  
            		    _I4_osd_draw_Point(point,color,ptCanvasinfo);
                        point.x++;
                	} 
                }
            }else{
                k = (float)(dy /dx);
                for (i = 0; i < dx; i ++){
            	    point.x = x+i;
                    point.y = y+i*k;  
                    for (j = 0; j < borderwidth; j ++){  
            		    _I4_osd_draw_Point(point,color,ptCanvasinfo);
                        point.y++;
                	} 
                }
            }
            
        }else{
//            EDBG("X+ Y- \n");
            dy = point0.y - point1.y;
            if (dy > dx){
                k = (float)(dx /dy);
                for (i = 0; i < dy; i ++){
            	    point.x = x+i*k;
                    point.y = y-i;  
                    for (j = 0; j < borderwidth; j ++){  
            		    _I4_osd_draw_Point(point,color,ptCanvasinfo);
                        point.x++;
                	} 
                }
            }else{
                k = (float)(dy /dx);
                for (i = 0; i < dx; i ++){
            	    point.x = x+i;
                    point.y = y-i*k;  
                    for (j = 0; j < borderwidth; j ++){  
            		    _I4_osd_draw_Point(point,color,ptCanvasinfo);
                        point.y++;
                	} 
                }
            }   
        }   
    }else{
        dx = point0.x - point1.x;
        x = point1.x;
        y = point1.y;
        if (point0.y > point1.y){
//            EDBG("X- Y- \n");
            dy = point0.y - point1.y;
            if (dy > dx){
                k = (float)(dx /dy);
                for (i = 0; i < dy; i ++){
            	    point.x = x+i*k;
                    point.y = y+i;  
                    for (j = 0; j < borderwidth; j ++){  
            		    _I4_osd_draw_Point(point,color,ptCanvasinfo);
                        point.x++;
                	} 
                }
            }else{
                k = (float)(dy /dx);
                for (i = 0; i < dx; i ++){
            	    point.x = x+i;
                    point.y = y+i*k;  
                    for (j = 0; j < borderwidth; j ++){  
            		    _I4_osd_draw_Point(point,color,ptCanvasinfo);
                        point.y++;
                	} 
                }
            }
            
        }else{
//            EDBG("X- Y+ \n");
            dy = point1.y - point0.y;
            if (dy > dx){
                k = (float)(dx /dy);
                for (i = 0; i < dy; i ++){
            	    point.x = x+i*k;
                    point.y = y-i;  
                    for (j = 0; j < borderwidth; j ++){  
            		    _I4_osd_draw_Point(point,color,ptCanvasinfo);
                        point.x++;
                	} 
                }
            }else{
                k = (float)(dy /dx);
                for (i = 0; i < dx; i ++){
            	    point.x = x+i;
                    point.y = y-i*k;  
                    for (j = 0; j < borderwidth; j ++){  
            		    _I4_osd_draw_Point(point,color,ptCanvasinfo);
                        point.y++;
                	} 
                }
            }
        }   
    }
    
    return 0;
}
static int _I4_osd_draw_Triangle(Point_t point,MI_RGN_CanvasInfo_t *ptCanvasinfo)
{
	Point_t Point1,Point2,Point3;
	int borderwidth = 8;
	int color = 4;//1:red 2:greed 3:blue 4:yellow
	Point3 = point;
	Point1.x = Point3.x>50?(Point3.x - 50):0;
	Point1.y = Point3.y>65?(Point3.y - 65):0;
	Point2.x = Point3.x + 50;
	Point2.y = Point3.y>65?(Point3.y - 65):0;
	if((MI_U32)Point2.x >= ptCanvasinfo->stSize.u32Width || (MI_U32)Point3.y >= ptCanvasinfo->stSize.u32Height)
	{
		MIXER_ERR("draw triangle out of range\n");
	}
	
	_I4_osd_draw_line(Point1, Point2, borderwidth, color, ptCanvasinfo);
	_I4_osd_draw_line(Point3, Point2, borderwidth, color, ptCanvasinfo);
	_I4_osd_draw_line(Point1, Point3, borderwidth, color, ptCanvasinfo);
	return 0;
}

int HCHDtoTRIANGLE(MI_VENC_CHN s32VencChn,MI_SYS_WindowRect_t *drawRect,unsigned char count)
{
	MI_S32 s32Ret = E_MI_ERR_FAILED;
	Point_t point;
	MI_SYS_WindowRect_t mainStream_Rect={0};
	MI_RGN_CanvasInfo_t stRgnCanvasInfo;
    MI_RGN_HANDLE hHandle = g_au32OsdRectWidgetHandle[s32VencChn];
	int g_s32Width  = getVideoWidth(s32VencChn);
    int g_s32Height = getVideoHeight(s32VencChn);
	
    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == hHandle)
    {
        MIXER_ERR("MI_RGN_HANDLE hHandle is NULL\n");
        return 0;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);
	cleanOsdTextWidget(hHandle, NULL, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
	memset(&stRgnCanvasInfo, 0, sizeof(stRgnCanvasInfo));
	if((s32Ret =MI_RGN_GetCanvasInfo(hHandle, &stRgnCanvasInfo)) != MI_RGN_OK)
	{
	   MIXER_ERR("MI_RGN_GetCanvasInfo fail\n");
	   pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
	   return s32Ret;
	}
	if(E_MI_RGN_PIXEL_FORMAT_I4 != stRgnCanvasInfo.ePixelFmt)
	{
	   MIXER_ERR("config the wrong poxer format, only support I4 now\n");
	   pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
	   return s32Ret;
	}
	for(unsigned char i=0; i<count; i++){
		mainStream_Rect.u16X = (int)((float)drawRect[i].u16X * g_s32Width / g_ieWidth );
        mainStream_Rect.u16Y = (int)((float)drawRect[i].u16Y * g_s32Height / g_ieHeight);
        mainStream_Rect.u16Width = (int)((float)drawRect[i].u16Width * g_s32Width/ g_ieWidth);
        mainStream_Rect.u16Height = (int)((float)drawRect[i].u16Height * g_s32Height/ g_ieHeight);
		if(mainStream_Rect.u16Height!=0 && mainStream_Rect.u16Width!=0)
		{
			point.x = (mainStream_Rect.u16X + mainStream_Rect.u16Width/2);
			point.y = mainStream_Rect.u16Y;
			_I4_osd_draw_Triangle(point,&stRgnCanvasInfo);
		}
	}
    if(MI_RGN_UpdateCanvas(hHandle) != MI_RGN_OK)
	{
	   MIXER_ERR("MI_RGN_UpdateCanvas fail\n");
	}
    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
    return 0;
}


int HCHDtoRECT(MI_VENC_CHN s32VencChn,MI_SYS_WindowRect_t *drawRect,unsigned char count)
{
    RectWidgetAttr_t stRectWidgetAttr;
    MI_U8 u8BorderWidth = 3;
    MI_SYS_WindowRect_t mainStream_Rect={0};
    static MI_RGN_HANDLE hHandle = g_au32OsdRectWidgetHandle[s32VencChn];
    int g_s32Width  = getVideoWidth(s32VencChn);
    int g_s32Height = getVideoHeight(s32VencChn);

    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == hHandle)
    {
        printf("[%s %d]MI_RGN_HANDLE hHandle is NULL\n",__func__,__LINE__);
        return 0;
    }


    if(NULL == drawRect)
    {
        printf("[%s %d]MI_SYS_WindowRect_t point is NULL\n",__func__,__LINE__);
        return 0;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);
    cleanOsdTextWidget(hHandle, NULL, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
    for(unsigned char i=0; i<count; i++){
        mainStream_Rect.u16X = (int)((float)drawRect[i].u16X * g_s32Width / g_ieWidth );
        mainStream_Rect.u16Y = (int)((float)drawRect[i].u16Y * g_s32Height / g_ieHeight);
        mainStream_Rect.u16Width = (int)((float)drawRect[i].u16Width * g_s32Width/ g_ieWidth);
        mainStream_Rect.u16Height = (int)((float)drawRect[i].u16Height * g_s32Height/ g_ieHeight);


        if(mainStream_Rect.u16Height!=0 && mainStream_Rect.u16Width!=0){

            stRectWidgetAttr.pstRect = &mainStream_Rect;
            stRectWidgetAttr.s32RectCnt = 1;
            stRectWidgetAttr.u8BorderWidth = u8BorderWidth;
            stRectWidgetAttr.u32Color = 0;     //0:red; 1:green; 2:blue;
            stRectWidgetAttr.pmt = E_MI_RGN_PIXEL_FORMAT_I4;//g_stMixerOsdWidgetOrder.pmt;
            stRectWidgetAttr.pfColor = &g_stGreenColor;
            stRectWidgetAttr.pbColor = &g_stBlackColor;
            stRectWidgetAttr.bFill = FALSE;
            //printf("draw REcthandle %d %d %d %d %d \n",hHandle,mainStream_Rect.u16X,mainStream_Rect.u16Y,mainStream_Rect.u16Width,mainStream_Rect.u16Height);

            updateOsdRectWidget(hHandle, &stRectWidgetAttr, TRUE);
        }
    }

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

    return 0;
}


int VGtoRECT(MI_VENC_CHN s32VencChn, MI_VDF_VgAttr_t *pVgAttr, MI_VG_Result_t *pVgResult)
{
    int i = 0, iCount = 0;
    RectWidgetAttr_t stRectWidgetAttr;
    MI_SYS_WindowRect_t vg_rect[4] = {{0,},};
    MI_RGN_HANDLE hHandle = g_au32OsdRectWidgetHandle[s32VencChn];
    static MI_S32 vg_detect_cnt = 0;
    static MI_SYS_WindowRect_t vg_rect_pre[4] = {{0,},};

    if((0 >= g_s32OsdRectHandleCnt) || (TRUE == g_bOsdSetResulution[s32VencChn]))
    {
        return 0;
    }

    if(pVgAttr == NULL || pVgResult == NULL)
    {
        if(vg_detect_cnt > 0)
        {
            pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);
            stRectWidgetAttr.pstRect = vg_rect_pre;
            stRectWidgetAttr.s32RectCnt = vg_detect_cnt;
            stRectWidgetAttr.u8BorderWidth = 1;
            stRectWidgetAttr.u32Color = 2;   //0:red; 1:green; 2:blue;
            stRectWidgetAttr.pmt = g_stMixerOsdWidgetOrder.pmt;
            stRectWidgetAttr.pfColor = &g_stGreenColor;
            stRectWidgetAttr.pbColor = &g_stBlackColor;
            stRectWidgetAttr.bFill = TRUE;
            updateOsdRectWidget(hHandle, &stRectWidgetAttr, TRUE);
            //printf("u32Color=%d,iCount=%d\n", alarmColor,iCount);
            vg_detect_cnt = 0;
            memset(vg_rect_pre, 0, sizeof(vg_rect_pre));
            pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
        }
        return 0;
    }

    for(i = 0; i < 4; i ++)
    {
        if(1 != pVgResult->alarm[i]) continue;
        vg_rect[iCount].u16X = pVgAttr->line[i].px.x;
        vg_rect[iCount].u16Y = pVgAttr->line[i].px.y;
        vg_rect[iCount].u16Width = pVgAttr->line[i].py.x - vg_rect[iCount].u16X + 1;
        vg_rect[iCount].u16Height = 2;
        vg_rect[iCount].u16X = (getVideoWidth(s32VencChn)* vg_rect[iCount].u16X / g_ieWidth) & 0xFFFE;
        vg_rect[iCount].u16Y = (getVideoHeight(s32VencChn) * vg_rect[iCount].u16Y / g_ieHeight) & 0xFFFE;
        vg_rect[iCount].u16Width = (getVideoWidth(s32VencChn) * vg_rect[iCount].u16Width / g_ieWidth) & 0xFFFE;
        vg_rect[iCount].u16Height = vg_rect[iCount].u16Height & 0xFFFE;
        printf("vg%d x=%d, y=%d, width=%d, height=%d, pVgAttr->line[i].py.x: %d, pVgAttr->line[i].px.x:%d\n", i, vg_rect[i].u16X, vg_rect[i].u16Y,vg_rect[i].u16Width,vg_rect[i].u16Height, pVgAttr->line[i].py.x, pVgAttr->line[i].px.x);
        iCount ++;
    }


    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);
    stRectWidgetAttr.pstRect = vg_rect;
    stRectWidgetAttr.s32RectCnt = iCount;
    stRectWidgetAttr.u8BorderWidth = 1;
    stRectWidgetAttr.u32Color = 0;   //0:red; 1:green; 2:blue;
    stRectWidgetAttr.pmt = g_stMixerOsdWidgetOrder.pmt;
    stRectWidgetAttr.pfColor = &g_stGreenColor;
    stRectWidgetAttr.pbColor = &g_stBlackColor;
    stRectWidgetAttr.bFill = TRUE;
    updateOsdRectWidget(hHandle, &stRectWidgetAttr, TRUE);
   // printf("=============AlarmCnt=%d, u32Color=%d,iCount=%d\n", *pAlarmCnt,alarmColor,iCount);
    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
    vg_detect_cnt = iCount;
    memcpy(vg_rect_pre, vg_rect, sizeof(vg_rect_pre));

    return 0;
}

////////////////////////////////////////////////////////////////////////


int AudioDetectToRECT(BOOL bBabyCry, BOOL bLoudSound, short LoudSounddB)
{
    printf("AudioDetectToRECT Draw %d %d %d \n", bBabyCry , bLoudSound, (int)LoudSounddB);

    if(g_bIEOsdInit == FALSE)
    {
        return 0;
    }

    if(g_u32AudioDetectOsdStatus == OSD_Done)
    {
        g_bBabyCry = bBabyCry;
        g_bLoudSound = bLoudSound;
        g_s16LoudSounddB = LoudSounddB;
        g_u32AudioDetectOsdStatus = OSD_Draw;
    }
    else
    {
        return -1;
    }

    return 0;
}


MI_S32 openOsdModule(char *param, int paramLen)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_S32 s32Idx = 0;


    if(NULL == param)
    {
        printf("%s:%d you must set Osd handle count!\n", __func__, __LINE__);
        return s32Ret;
    }
    for(s32Idx = 0; s32Idx < 32; s32Idx++)
    {
        if((*param)&(0x01<<s32Idx))
        {
            g_s32OsdTextHandleCnt += 1;
            switch(s32Idx)
            {
                case 0: g_bTimeStamp = TRUE; break;
                case 1: g_bVideoInfo = TRUE; break;
                case 2: g_bIspInfo   = TRUE; break;
                case 3: g_bAudioInfo = TRUE; break;
                case 4: g_bUser0Info = TRUE; break;
                case 5: g_bUser1Info = TRUE; break;
                case 6: g_bUser2Info = TRUE; break;
                default: break;
            }
        }
    }

    printf("%s:%d ====>Get Osd Text handle count = %d! g_bTimeStamp=%d\n", __func__, __LINE__, g_s32OsdTextHandleCnt,g_bTimeStamp);

    s32Ret = MI_SUCCESS;
    return s32Ret;
}


MI_S32 resetOsdResolution(MI_VENC_CHN s32VencChn, MI_SYS_WindowSize_t *pstSize)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_ChnPort_t stRgnChnPort;
    EN_OSD_WIDGET_TYPE    osdType;
    osdInfo_t osdInfo;
    MI_S32 s32Idx = 0;
    if(s32VencChn >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[s32VencChn] || s32VencChn != g_videoEncoderArray[s32VencChn]->m_veChn)
    {
        MIXER_ERR("venc channel [%d] error\n", s32VencChn);
        return s32Ret;
    }

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    g_bOsdSetResulution[s32VencChn] = TRUE;

    if(s32VencChn == g_videoEncoderArray[s32VencChn]->m_veChn)
    {
        configOsdRgnChnPort(s32VencChn, &stRgnChnPort);
    }
    else
    {
       pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
       return -1;
    }

    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn])
    {
        cleanOsdTextWidget(g_au32OsdRectWidgetHandle[s32VencChn], NULL, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

        s32Ret = MI_RGN_DetachFromChn(g_au32OsdRectWidgetHandle[s32VencChn], &stRgnChnPort);
        if(MI_RGN_OK != s32Ret)
        {
            MIXER_ERR("MI_RGN_DetachFromChn[%d] error(hdl=%d), %X\n",s32VencChn, g_au32OsdRectWidgetHandle[s32VencChn], s32Ret);
            pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
            return s32Ret;
        }

        s32Ret = MI_RGN_Destroy(g_au32OsdRectWidgetHandle[s32VencChn]);
        if(s32Ret != MI_RGN_OK)
        {
            MIXER_ERR("MI_RGN_Destroy chn[%d] error(hdl=%d), %X\n",s32VencChn,g_au32OsdRectWidgetHandle[s32VencChn], s32Ret);
            pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
            return s32Ret;
        }
        osdType = E_OSD_WIDGET_TYPE_RECT;
    }
   for(s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
   {
    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdTextWidgetHandle[s32VencChn][s32Idx])
    {
        cleanOsdTextWidget(g_au32OsdTextWidgetHandle[s32VencChn][s32Idx], NULL, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

        s32Ret = MI_RGN_DetachFromChn(g_au32OsdTextWidgetHandle[s32VencChn][s32Idx], &stRgnChnPort);
        if(MI_RGN_OK != s32Ret)
        {
            MIXER_ERR("MI_RGN_DetachFromChn[%d] error(hdl=%d), %X\n",s32VencChn,g_au32OsdTextWidgetHandle[s32VencChn][s32Idx], s32Ret);
            pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
            return s32Ret;
        }

        s32Ret = MI_RGN_Destroy(g_au32OsdTextWidgetHandle[s32VencChn][s32Idx]);
        if(s32Ret != MI_RGN_OK)
        {
            MIXER_ERR("MI_RGN_Destroy chn[%d] error(hdl=%d), %X\n",s32VencChn,g_au32OsdTextWidgetHandle[s32VencChn][s32Idx], s32Ret);
            pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
            return s32Ret;
        }
        osdType = E_OSD_WIDGET_TYPE_TEXT;
    }

    if((s32Idx < MAX_COVER_NUMBER_PER_CHN)&&((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx])/* && (TRUE == g_bMaskOsdInit[s32VencChn])*/)
    {
        s32Ret = MI_RGN_DetachFromChn(g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx], &stRgnChnPort);
        if(MI_RGN_OK != s32Ret)
        {
            MIXER_ERR("MI_RGN_DetachFromChn error(hdl=%d), %X\n", g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx], s32Ret);
            pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
            return s32Ret;
        }

        s32Ret = MI_RGN_Destroy(g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx]);
        if(s32Ret != MI_RGN_OK)
        {
            MIXER_ERR("MI_RGN_Destroy error(hdl=%d), %X\n", g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx], s32Ret);
            pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
            return s32Ret;
        }
        g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx] = MI_RGN_HANDLE_NULL;
        osdType = E_OSD_WIDGET_TYPE_COVER;
    }
   }

    /*if((MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[MAX_RGN_NUMBER_PER_CHN * s32VencChn]) && (TRUE == g_bMaskOsdInit[s32VencChn]))
    {
        g_bMaskOsdInit[s32VencChn] = FALSE;
    }*/
    memset(&osdInfo,0,sizeof(osdInfo_t));
    midFindOsdHandleByOsdAttr(&osdInfo,GET_FORMAT);
    osdInfo.osdType = osdType;
    osdInfo.channel = s32VencChn;
    osdInfo.osdIndex = s32Idx;
    if(MI_SUCCESS == s32Ret)
    {
      midManageOsdInfoLinkList(&osdInfo,FALSE,NULL,NULL);
    }
    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);


    s32Ret = MI_RGN_OK;
    return s32Ret;
}


MI_S32 restartOsdResolution(MI_VENC_CHN s32VencChn, MI_SYS_WindowSize_t *pstSize)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_RGN_ChnPortParam_t stRgnChnPortParam;
    MI_SYS_WindowRect_t pstRect;
#if TARGET_CHIP_I6B0
    MI_VENC_CHN s32VencChn_tmp = 0;
#endif

    memset(&pstRect,0,sizeof(MI_SYS_WindowRect_t));
    if(s32VencChn >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[s32VencChn] || s32VencChn != g_videoEncoderArray[s32VencChn]->m_veChn)
    {
        MIXER_ERR("venc channel [%d] error\n", s32VencChn);
        return s32Ret;
    }
#if TARGET_CHIP_I6B0
    if(checkVencInitOsdFull(s32VencChn,&s32VencChn_tmp) == TRUE)
    {
        MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",s32VencChn,s32VencChn_tmp,s32VencChn_tmp,s32VencChn);
        s32Ret = MI_RGN_OK;
        g_bOsdSetResulution[s32VencChn] = FALSE;
        return s32Ret ;
    }
#endif

    pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

    g_stVideoSize[s32VencChn].u16Width  = g_videoEncoderArray[s32VencChn]->m_width;
    g_stVideoSize[s32VencChn].u16Height = g_videoEncoderArray[s32VencChn]->m_height;

#if MIXER_OSD_TEXT_WIDGET_ENABLE
    if(((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn]))
#else
    if((g_bIEOsdInit || gDebug_OsdTest) && (0 == s32VencChn) && (MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn]))
#endif
    {
        memset(&stRgnAttr, 0x00, sizeof(MI_RGN_Attr_t));
        stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
        stRgnAttr.stOsdInitParam.ePixelFmt = g_stMixerOsdWidgetOrder.pmt;
        stRgnAttr.stOsdInitParam.stSize.u32Width  = g_videoEncoderArray[s32VencChn]->m_width;
        stRgnAttr.stOsdInitParam.stSize.u32Height = g_videoEncoderArray[s32VencChn]->m_height;

        if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
        {
            stRgnChnPort.eModId    = E_MI_RGN_MODID_DIVP;
            stRgnChnPort.s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpSclUpChnId;
            stRgnChnPort.s32DevId  = 0;
            stRgnChnPort.s32OutputPortId = 0;
        }
        else if(g_videoEncoderArray[s32VencChn]->m_bUseDivp)
        {
            stRgnChnPort.eModId    = E_MI_RGN_MODID_DIVP;
            stRgnChnPort.s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpChnPort.u32ChnId;
            stRgnChnPort.s32DevId  = 0;
            stRgnChnPort.s32OutputPortId = 0;
        }
        else
        {
            stRgnChnPort.eModId   = E_MI_RGN_MODID_VPE;
            stRgnChnPort.s32DevId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32DevId;
            stRgnChnPort.s32ChnId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32ChnId;
            stRgnChnPort.s32OutputPortId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32PortId;
        }

        memset(&stRgnChnPortParam, 0x00, sizeof(MI_RGN_ChnPortParam_t));
        stRgnChnPortParam.bShow = TRUE;
        stRgnChnPortParam.stPoint.u32X = 0;
        stRgnChnPortParam.stPoint.u32Y = 0;

        if(((g_videoEncoderArray[s32VencChn]->m_width <= 1920) && (g_videoEncoderArray[s32VencChn]->m_height <= 1080)) ||
           ((g_videoEncoderArray[s32VencChn]->m_width <= 1080) && (g_videoEncoderArray[s32VencChn]->m_height <= 1920)))
        {
            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode = E_MI_RGN_ABOVE_LUMA_THRESHOLD;
            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum = getDividedNumber(stRgnAttr.stOsdInitParam.stSize.u32Width);
            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum = getDividedNumber(stRgnAttr.stOsdInitParam.stSize.u32Height);

            if((1920 == stRgnAttr.stOsdInitParam.stSize.u32Width) && (1080 == stRgnAttr.stOsdInitParam.stSize.u32Height))
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum /= 2;
            else if((1080 == stRgnAttr.stOsdInitParam.stSize.u32Width) && (1920 == stRgnAttr.stOsdInitParam.stSize.u32Height))
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum /= 2;

            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold = MIXER_OSD_COLOR_INVERSE_THD;
            stRgnChnPortParam.unPara.stOsdChnPort.u32Layer = g_au32OsdRectWidgetHandle[s32VencChn];
#if MIXER_OSD_COLOR_INVERSE_ACTING_ON_RECT_WIDGET
            if(TRUE == g_bOsdColorInverse)
            {
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = TRUE;
            }
            else
            {
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = FALSE;
            }
#else
            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = FALSE;
#endif
        }

        createOsdRectWidget(g_au32OsdRectWidgetHandle[s32VencChn], &stRgnAttr, &stRgnChnPort, &stRgnChnPortParam);

        MIXER_DBG("Create RectHdl=%d canvas:x=%d, y=%d, w=%4d, h=%4d, DevID=%d, ChnID=%d, PortID=%d, fmt=I4\n", g_au32OsdRectWidgetHandle[s32VencChn],
                   stRgnChnPortParam.stPoint.u32X, stRgnChnPortParam.stPoint.u32Y, stRgnAttr.stOsdInitParam.stSize.u32Width, stRgnAttr.stOsdInitParam.stSize.u32Height,
                   g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32DevId, g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32ChnId, g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32PortId);

        if(((g_videoEncoderArray[s32VencChn]->m_width <= 1920) && (g_videoEncoderArray[s32VencChn]->m_height <= 1080)) ||
           ((g_videoEncoderArray[s32VencChn]->m_width <= 1080) && (g_videoEncoderArray[s32VencChn]->m_height <= 1920)))
        {
            printf("%s:%d Rect Hdl=%d canvas:x=%d, y=%d, w=%4d, h=%4d, WDivNum=%d, HDivNum=%d, InvertColorMode=%d, Threshold=%d, Layer=%d\n", __func__, __LINE__,
                    g_au32OsdRectWidgetHandle[s32VencChn], stRgnChnPortParam.stPoint.u32X, stRgnChnPortParam.stPoint.u32Y,
                    stRgnAttr.stOsdInitParam.stSize.u32Width, stRgnAttr.stOsdInitParam.stSize.u32Height, stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum,
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum, stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode,
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold, stRgnChnPortParam.unPara.stOsdChnPort.u32Layer);
        }

        cleanOsdTextWidget(g_au32OsdRectWidgetHandle[s32VencChn], NULL, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
        osdInfo_t osdInfo;
        memset(&osdInfo,0,sizeof(osdInfo));
        osdInfo.osdType = E_OSD_WIDGET_TYPE_RECT;
        osdInfo.u32RgnHandle = g_au32OsdRectWidgetHandle[s32VencChn];
        osdInfo.channel = s32VencChn;
        osdInfo.osdIndex = -1;
        osdInfo.format = stRgnAttr.stOsdInitParam.ePixelFmt;
        osdInfo.point.u16X = stRgnChnPortParam.stPoint.u32X;
        osdInfo.point.u16Y = stRgnChnPortParam.stPoint.u32Y;
        osdInfo.size.u16Height = stRgnAttr.stOsdInitParam.stSize.u32Width;
        osdInfo.size.u16Width = stRgnAttr.stOsdInitParam.stSize.u32Height;
        midManageOsdInfoLinkList(&osdInfo,true,NULL,NULL);
    }


    for(MI_S32 s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
    {
#if MIXER_OSD_TEXT_WIDGET_ENABLE
        if(((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdTextWidgetHandle[s32VencChn][s32Idx]))
        {
            g_au32OsdTextWidgetHandle[s32VencChn][s32Idx] = calcOsdHandle(s32VencChn, s32Idx, E_OSD_WIDGET_TYPE_TEXT);

            memset(&stRgnAttr, 0x00, sizeof(MI_RGN_Attr_t));
            stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
            stRgnAttr.stOsdInitParam.ePixelFmt = g_stMixerOsdWidgetOrder.pmt;

            if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
            {
                stRgnAttr.stOsdInitParam.stSize.u32Width  = MIXER_OSD_TEXT_WIDGET_WIDTH_LARGE_FONT;
                stRgnAttr.stOsdInitParam.stSize.u32Height = MIXER_OSD_TEXT_WIDGET_HEIGHT_LARGE_FONT;
            }
            else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                stRgnAttr.stOsdInitParam.stSize.u32Width  = MIXER_OSD_TEXT_WIDGET_WIDTH_MEDIUM_FONT;
                stRgnAttr.stOsdInitParam.stSize.u32Height = MIXER_OSD_TEXT_WIDGET_HEIGHT_MEDIUM_FONT;
            }
            else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
            {
                stRgnAttr.stOsdInitParam.stSize.u32Width  = MIXER_OSD_TEXT_WIDGET_WIDTH_SMALL_FONT;
                stRgnAttr.stOsdInitParam.stSize.u32Height = MIXER_OSD_TEXT_WIDGET_HEIGHT_SMALL_FONT;
            }
            else
            {
                stRgnAttr.stOsdInitParam.stSize.u32Width  = g_videoEncoderArray[s32VencChn]->m_width;
                stRgnAttr.stOsdInitParam.stSize.u32Height = g_videoEncoderArray[s32VencChn]->m_height;
            }

            if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
            {
                stRgnChnPort.eModId    = E_MI_RGN_MODID_DIVP;
                stRgnChnPort.s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpSclDownChnId;
                stRgnChnPort.s32DevId  = 0;
                stRgnChnPort.s32OutputPortId = 0;
            }
            else if(g_videoEncoderArray[s32VencChn]->m_bUseDivp) //vpe port2 case: vif->vpe->divp->venc
            {
                stRgnChnPort.eModId    = E_MI_RGN_MODID_DIVP;
                stRgnChnPort.s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpChnPort.u32ChnId;
                stRgnChnPort.s32DevId  = 0;
                stRgnChnPort.s32OutputPortId = 0;
            }
            else
            {
                stRgnChnPort.eModId   = E_MI_RGN_MODID_VPE;
                stRgnChnPort.s32DevId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32DevId;
                stRgnChnPort.s32ChnId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32ChnId;
                stRgnChnPort.s32OutputPortId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32PortId;
            }

            memset(&stRgnChnPortParam, 0x00, sizeof(MI_RGN_ChnPortParam_t));
            stRgnChnPortParam.bShow = TRUE;
            stRgnChnPortParam.stPoint.u32X = 0;
            stRgnChnPortParam.stPoint.u32Y = 0;


            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum = getDividedNumber(stRgnAttr.stOsdInitParam.stSize.u32Width);
            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum = getDividedNumber(stRgnAttr.stOsdInitParam.stSize.u32Height);
            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold = MIXER_OSD_COLOR_INVERSE_THD;
            stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode = E_MI_RGN_ABOVE_LUMA_THRESHOLD;
            stRgnChnPortParam.unPara.stOsdChnPort.u32Layer = g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
            stRgnChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode = E_MI_RGN_PIXEL_ALPHA;
            stRgnChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8BgAlpha = 0x0;
            stRgnChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8FgAlpha = 0xff;
            if(TRUE == g_bOsdColorInverse)
            {
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = TRUE;
            }
            else
            {
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = FALSE;
            }
            switch(s32Idx)
            {
                case 0:
                    s32Ret = configOsdTimeStampCanvasSize(s32VencChn, &pstRect);
                    break;
                case 1:
                    s32Ret = configOsdVideoInfoCanvasSize(s32VencChn, &pstRect);
                    break;
                case 2:
                    s32Ret = configOsdIspInfoCanvasSize(s32VencChn, &pstRect);
                    break;
                case 3:
                    s32Ret = configOsdAudioInfoCanvasSize(s32VencChn, &pstRect);
                    break;
                case 4:
                    s32Ret = configOsdUser0InfoCanvasSize(s32VencChn, &pstRect);
                    break;
                case 5:
                    s32Ret = configOsdUser1InfoCanvasSize(s32VencChn, &pstRect);
                    break;
                case 6:
                    s32Ret = configOsdUser2InfoCanvasSize(s32VencChn, &pstRect);
                    break;
                case 7:
                    s32Ret = configOsdUser3InfoCanvasSize(s32VencChn, &pstRect);
                    break;
                default:
                    break;
            }
             stRgnChnPortParam.stPoint.u32X = pstRect.u16X;
            stRgnChnPortParam.stPoint.u32Y = pstRect.u16Y;
            stRgnAttr.stOsdInitParam.stSize.u32Width = pstRect.u16Width;
            stRgnAttr.stOsdInitParam.stSize.u32Height = pstRect.u16Height;
            if(MI_SUCCESS != s32Ret)
            {
                MIXER_ERR("configOsdXXXCanvasSize() error(0x%X), VencChn=%d, s32Idx=%d\n", s32Ret, s32VencChn, s32Idx);
                pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
                return s32Ret;
            }
            createOsdTextWidget(g_au32OsdTextWidgetHandle[s32VencChn][s32Idx], &stRgnAttr, &stRgnChnPort, &stRgnChnPortParam);

            MIXER_DBG("Create Text Hdl=%d canvas:x=%d, y=%d, w=%4d, h=%4d, DevID=%d, ChnID=%d, PortID=%d, fmt=I4\n", g_au32OsdTextWidgetHandle[s32VencChn][s32Idx],
                       stRgnChnPortParam.stPoint.u32X, stRgnChnPortParam.stPoint.u32Y, stRgnAttr.stOsdInitParam.stSize.u32Width, stRgnAttr.stOsdInitParam.stSize.u32Height,
                       g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32DevId, g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32ChnId, g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32PortId);

            printf("%s:%d Text Hdl=%d canvas:x=%d, y=%d, w=%4d, h=%4d, WDivNum=%d, HDivNum=%d, InvertColorMode=%d, Threshold=%d, Layer=%d\n", __func__, __LINE__,
                      g_au32OsdTextWidgetHandle[s32VencChn][s32Idx], stRgnChnPortParam.stPoint.u32X, stRgnChnPortParam.stPoint.u32Y,
                      stRgnAttr.stOsdInitParam.stSize.u32Width, stRgnAttr.stOsdInitParam.stSize.u32Height, stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum,
                      stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum, stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode,
                      stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold, stRgnChnPortParam.unPara.stOsdChnPort.u32Layer);

             cleanOsdTextWidget(g_au32OsdTextWidgetHandle[s32VencChn][s32Idx], NULL, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);
             osdInfo_t osdInfo;
             memset(&osdInfo,0,sizeof(osdInfo));
             osdInfo.osdType = E_OSD_WIDGET_TYPE_RECT;
             osdInfo.u32RgnHandle = g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
             osdInfo.channel = s32VencChn;
             osdInfo.osdIndex = s32Idx;
             osdInfo.format = stRgnAttr.stOsdInitParam.ePixelFmt;
             osdInfo.point.u16X = stRgnChnPortParam.stPoint.u32X;
             osdInfo.point.u16Y = stRgnChnPortParam.stPoint.u32Y;
             osdInfo.size.u16Height = stRgnAttr.stOsdInitParam.stSize.u32Width;
             osdInfo.size.u16Width = stRgnAttr.stOsdInitParam.stSize.u32Height;
             midManageOsdInfoLinkList(&osdInfo,true,NULL,NULL);
        }
#endif


     /*   if((s32Idx < MAX_COVER_NUMBER_PER_CHN)&&((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx]))
        {
            g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx] = calcOsdHandle(s32VencChn, s32Idx, E_OSD_WIDGET_TYPE_COVER);

            memset(&stRgnAttr, 0, sizeof(MI_RGN_Attr_t));
            stRgnAttr.eType = E_MI_RGN_TYPE_COVER;

            s32Ret = MI_RGN_Create(g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx], &stRgnAttr);
            if(MI_RGN_OK != s32Ret)
            {
                MIXER_ERR("MI_RGN_Create error(hdl=%d), %X\n", g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx], s32Ret);
                pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
                return s32Ret;
            }
            s32Ret = MI_RGN_AttachToChn(g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx],&stRgnChnPort,&stRgnChnPortParam);
            if(MI_RGN_OK != s32Ret)
            {
              MIXER_ERR("MI_RGN_AttachToChn error(hdl=%d), %X\n", g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx], s32Ret);
              pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
              return s32Ret;
            }
            printf("%s:%d Create Osd cover handle=%d\n", __func__, __LINE__, g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx]);
        }*/
    }

    g_bOsdSetResulution[s32VencChn] = FALSE;

    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);


    if(gDebug_OsdTest)
    {
       // _osd_apitest_init(s32VencChn);
    }

    SetOsdInitFullState(TRUE);
    s32Ret = MI_RGN_OK;
    return s32Ret;
}


int initOsdIeModule(char *param, int paramLen)
{
    MI_S32 s32VencChn = 0;

    g_s32OsdRectHandleCnt = *param;
    MIXER_DBG("%s:%d ====>Get Osd Rect handle count = %d!, state:%d, handle:%d\n", __func__, __LINE__, g_s32OsdRectHandleCnt, \
                GetOsdInitFullState(), g_au32OsdRectWidgetHandle[s32VencChn]);

    if(GetOsdInitFullState() && NULL != g_videoEncoderArray[s32VencChn] && (MI_RGN_HANDLE_NULL == (MI_S32)g_au32OsdRectWidgetHandle[s32VencChn]))
    {
        MI_S32 s32Ret = MI_SUCCESS;
        MI_RGN_HANDLE * pu32RgnHandle = NULL;
        ST_MIXER_RGN_WIDGET_ATTR stMixerRgnWidgetAttr;

        memset(&stMixerRgnWidgetAttr, 0x00, sizeof(ST_MIXER_RGN_WIDGET_ATTR));
        stMixerRgnWidgetAttr.s32VencChn = s32VencChn;
        stMixerRgnWidgetAttr.s32Idx = 0;
        stMixerRgnWidgetAttr.eOsdWidgetType = E_OSD_WIDGET_TYPE_RECT;
        stMixerRgnWidgetAttr.bShow = TRUE;
        stMixerRgnWidgetAttr.bOsdColorInverse = g_bOsdColorInverse;
        stMixerRgnWidgetAttr.eRgnpixelFormat = E_MI_RGN_PIXEL_FORMAT_I4;
        stMixerRgnWidgetAttr.u32Color = 0xff801080; //black
        stMixerRgnWidgetAttr.u16LumaThreshold = MIXER_OSD_COLOR_INVERSE_THD;
        stMixerRgnWidgetAttr.pstMutexMixerOsdRun = &g_stMutexMixerOsdRun[s32VencChn];

        stMixerRgnWidgetAttr.stRect.u16X = 0;
        stMixerRgnWidgetAttr.stRect.u16Y = 0;
        stMixerRgnWidgetAttr.stRect.u16Width  = g_videoEncoderArray[s32VencChn]->m_width;
        stMixerRgnWidgetAttr.stRect.u16Height = g_videoEncoderArray[s32VencChn]->m_height;
        configOsdRgnChnPort(s32VencChn, &stMixerRgnWidgetAttr.stRgnChnPort);

        pu32RgnHandle = &g_au32OsdRectWidgetHandle[s32VencChn];

        s32Ret = createOsdWidget(pu32RgnHandle, &stMixerRgnWidgetAttr);
        if(MI_SUCCESS != s32Ret)
        {
            MIXER_ERR("createOsdWidget() error(0x%X)\n", s32Ret);
            return s32Ret;
        }
    }
    return MI_SUCCESS;
}

int uninitOsdIeModule()
{
    g_bIEOsdInit = FALSE;
    MI_S32 s32Ret = MI_SUCCESS;
    osdInfo_t osdInfo;
     memset(&osdInfo,0,sizeof(osdInfo_t));
    osdInfo.u32RgnHandle = g_au32OsdRectWidgetHandle[0];
    if((MI_RGN_HANDLE_NULL != (MI_S32)g_au32OsdRectWidgetHandle[0]))
    {
         s32Ret = destroyOsdWidget(0 , g_au32OsdRectWidgetHandle[0]);
         if(MI_SUCCESS != s32Ret)
         {
                MIXER_ERR("destroyOsdWidget() error(0x%X)\n", s32Ret);
                return s32Ret;
         }
        if(MI_SUCCESS == s32Ret)
        {
#if 0
               osdInfo.osdType = E_OSD_WIDGET_TYPE_RECT;
              osdInfo.osd_union.osdRect.channel = 0;
              osdInfo.osd_union.osdRect.osdIndex = -1;
               midManageOsdInfoLinkList(&osdInfo,FALSE,NULL,NULL);
#endif
        }
    }

    return MI_SUCCESS;
}

#if TARGET_CHIP_I6B0
//because I6B0 vpe port2 have GOP.different venc can bind to vpe port 2 without divp.different venc and bind to real mode divp,too.
//so, some venc chn from same port does not need to create and draw RGN double time.
//return BOOL: does not need to createRGN(TRUE)
//if return TRUE:vencChn is the same port with me.
MI_BOOL checkVencInitOsdFull(MI_S32 vencChn1,MI_S32 *vencChn2)
{
    MI_U32 j;
    static MI_BOOL port2_RGN =  FALSE;
    static MI_S32 port2_VencChn = 0xff;
    if((MI_U32)vencChn1 >= g_videoNumber)
        MIXER_ERR("param error!\n");
    for(j=0; (j<g_videoNumber) && (FALSE == port2_RGN);j++)
    {
        if(g_videoEncoderArray[j]->m_VpeChnPort.u32PortId == 2 && g_videoEncoderArray[j]->m_DivpChnPort.eModId == E_MI_MODULE_ID_MAX)
        {
            port2_RGN = TRUE;
            port2_VencChn= j;
        }

    }
    if(port2_RGN==TRUE && g_videoEncoderArray[vencChn1]->m_DivpChnPort.eModId == E_MI_MODULE_ID_DIVP && g_videoEncoderArray[vencChn1]->m_VpeChnPort.u32PortId==2)
    {
        if(vencChn2!= NULL)*vencChn2 = port2_VencChn;
        return TRUE;
    }
    for(j=0; j<(MI_U32)vencChn1; j++)
    {
        if(g_videoEncoderArray[j]->m_DivpChnPort.eModId == E_MI_MODULE_ID_DIVP && g_videoEncoderArray[vencChn1]->m_DivpChnPort.eModId == E_MI_MODULE_ID_DIVP \
            && g_videoEncoderArray[j]->m_DivpChnPort.u32DevId ==g_videoEncoderArray[vencChn1]->m_DivpChnPort.u32DevId
            && g_videoEncoderArray[j]->m_DivpChnPort.u32ChnId ==g_videoEncoderArray[vencChn1]->m_DivpChnPort.u32ChnId \
            && g_videoEncoderArray[j]->m_DivpChnPort.u32PortId ==g_videoEncoderArray[vencChn1]->m_DivpChnPort.u32PortId)
            break;
        else if(g_videoEncoderArray[j]->m_VpeChnPort.u32DevId ==g_videoEncoderArray[vencChn1]->m_VpeChnPort.u32DevId
            && g_videoEncoderArray[j]->m_VpeChnPort.u32ChnId ==g_videoEncoderArray[vencChn1]->m_VpeChnPort.u32ChnId \
            && g_videoEncoderArray[j]->m_VpeChnPort.u32PortId ==g_videoEncoderArray[vencChn1]->m_VpeChnPort.u32PortId\
            && g_videoEncoderArray[j]->m_DivpChnPort.eModId == E_MI_MODULE_ID_MAX && g_videoEncoderArray[vencChn1]->m_DivpChnPort.eModId == E_MI_MODULE_ID_MAX)
            break;
    }
    if(j == (MI_U32)vencChn1 )
    {
        return FALSE;
    }
    else if(vencChn2!= NULL)
    {
        *vencChn2 = j;
        return TRUE;
    }
    return TRUE;
}
#endif

void preInitOsd()
{
	MI_S32 s32Ret = E_MI_ERR_FAILED;
    s32Ret = MI_RGN_Init(&g_stPaletteTable);
	if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_Init error(%X)\n", s32Ret);
        SetOsdInitFullState(FALSE);
        return;
    }
    
}

MI_S32 initOsdFull()
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_S32 s32Idx = 0;
    MI_VENC_CHN s32VencChn = 0;
    MI_SYS_WindowRect_t *pstRect = NULL;
    MI_RGN_HANDLE *pu32RgnHandle = NULL;
    MI_RGN_ChnPort_t *pstRgnChnPort = NULL;
    ST_MIXER_RGN_WIDGET_ATTR stMixerRgnWidgetAttr; 
    for(s32VencChn = 0; ((s32VencChn < g_s32VideoStreamNum) && (NULL != g_videoEncoderArray[s32VencChn])); s32VencChn++)
    {
        g_stVideoSize[s32VencChn].u16Width  = g_videoEncoderArray[s32VencChn]->m_width;
        g_stVideoSize[s32VencChn].u16Height = g_videoEncoderArray[s32VencChn]->m_height;

        memset(&stMixerRgnWidgetAttr, 0x00, sizeof(stMixerRgnWidgetAttr));
        stMixerRgnWidgetAttr.s32VencChn = s32VencChn;
        stMixerRgnWidgetAttr.s32Idx = 0;
        stMixerRgnWidgetAttr.eOsdWidgetType = E_OSD_WIDGET_TYPE_MAX;
        stMixerRgnWidgetAttr.bShow = TRUE;
        stMixerRgnWidgetAttr.bOsdColorInverse = g_bOsdColorInverse;
        stMixerRgnWidgetAttr.eRgnpixelFormat = E_MI_RGN_PIXEL_FORMAT_I4;
        stMixerRgnWidgetAttr.u32Color = 0xff801080; //black
        stMixerRgnWidgetAttr.u16LumaThreshold = MIXER_OSD_COLOR_INVERSE_THD;
        stMixerRgnWidgetAttr.pstMutexMixerOsdRun = &g_stMutexMixerOsdRun[s32VencChn];

        pstRect = &stMixerRgnWidgetAttr.stRect;
        pstRgnChnPort = &stMixerRgnWidgetAttr.stRgnChnPort;

        if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
        {
            pstRgnChnPort->eModId    = E_MI_RGN_MODID_DIVP;
            pstRgnChnPort->s32ChnId  = (MI_S32)g_videoEncoderArray[s32VencChn]->m_DivpSclDownChnId;
            pstRgnChnPort->s32DevId  = 0;
            pstRgnChnPort->s32OutputPortId = 0;

            pstRect->u16Width  = g_videoEncoderArray[s32VencChn]->m_pipRectW;
            pstRect->u16Height = g_videoEncoderArray[s32VencChn]->m_pipRectH;
        }
        else if(g_videoEncoderArray[s32VencChn]->m_bUseDivp)
        {
            //vpe port2 case: vif->vpe->divp->venc
            pstRgnChnPort->eModId    = E_MI_RGN_MODID_DIVP;
            pstRgnChnPort->s32ChnId  = (MI_S32)g_videoEncoderArray[s32VencChn]->m_DivpChnPort.u32ChnId;
            pstRgnChnPort->s32DevId  = (MI_S32)g_videoEncoderArray[s32VencChn]->m_DivpChnPort.u32DevId;
            pstRgnChnPort->s32OutputPortId = (MI_S32)g_videoEncoderArray[s32VencChn]->m_DivpChnPort.u32PortId;
        }
        else
        {
            pstRgnChnPort->eModId   = E_MI_RGN_MODID_VPE;
            pstRgnChnPort->s32DevId = (MI_S32)g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32DevId;
            pstRgnChnPort->s32ChnId = (MI_S32)g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32ChnId;
            pstRgnChnPort->s32OutputPortId = (MI_S32)g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32PortId;
        }

        if(s32VencChn < g_s32OsdRectHandleCnt)
        {
            stMixerRgnWidgetAttr.s32VencChn = s32VencChn;
            stMixerRgnWidgetAttr.s32Idx = 0;
            stMixerRgnWidgetAttr.eOsdWidgetType = E_OSD_WIDGET_TYPE_RECT;

            pstRect->u16X  = 0;
            pstRect->u16Y  = 0;
            pstRect->u16Width  = g_videoEncoderArray[s32VencChn]->m_width;
            pstRect->u16Height = g_videoEncoderArray[s32VencChn]->m_height;

            pu32RgnHandle = &g_au32OsdRectWidgetHandle[s32VencChn];

            s32Ret = createOsdWidget(pu32RgnHandle, &stMixerRgnWidgetAttr);
            if(MI_SUCCESS != s32Ret)
            {
                MIXER_ERR("createOsdWidget() error(0x%X)\n", s32Ret);
                return s32Ret;
            }
        }
        for(s32Idx = 0; s32Idx < g_s32OsdTextHandleCnt; s32Idx++)
        {
            stMixerRgnWidgetAttr.s32VencChn = s32VencChn;
            stMixerRgnWidgetAttr.s32Idx = s32Idx;
            stMixerRgnWidgetAttr.eOsdWidgetType = E_OSD_WIDGET_TYPE_TEXT;
            stMixerRgnWidgetAttr.bShow = TRUE;
            stMixerRgnWidgetAttr.bOsdColorInverse = g_bOsdColorInverse;
            stMixerRgnWidgetAttr.eRgnpixelFormat = E_MI_RGN_PIXEL_FORMAT_I4;
            stMixerRgnWidgetAttr.u16LumaThreshold = MIXER_OSD_COLOR_INVERSE_THD;
            stMixerRgnWidgetAttr.pstMutexMixerOsdRun = &g_stMutexMixerOsdRun[s32VencChn];
            switch(s32Idx)
            {
                case 0:
                    stMixerRgnWidgetAttr.u32Color = 0xff6E29F0; //blue
                    s32Ret = configOsdTimeStampCanvasSize(s32VencChn, pstRect);
                    break;
                case 1:
                    stMixerRgnWidgetAttr.u32Color = 0xff801080; //black
                    s32Ret = configOsdVideoInfoCanvasSize(s32VencChn, pstRect);
                    break;
                case 2:
                    stMixerRgnWidgetAttr.u32Color = 0xffF0525B; //red
                    s32Ret = configOsdIspInfoCanvasSize(s32VencChn, pstRect);
                    break;
                case 3:
                    stMixerRgnWidgetAttr.u32Color = 0xff239137; //green
                    s32Ret = configOsdAudioInfoCanvasSize(s32VencChn, pstRect);
                    break;
                case 4:
                    stMixerRgnWidgetAttr.u32Color = 0xff6E29F0; //blue
                    s32Ret = configOsdUser0InfoCanvasSize(s32VencChn, pstRect);
                    break;
                case 5:
                    stMixerRgnWidgetAttr.u32Color = 0xff801080; //black
                    s32Ret = configOsdUser1InfoCanvasSize(s32VencChn, pstRect);
                    break;
                case 6:
                    stMixerRgnWidgetAttr.u32Color = 0xffF0525B; //red
                    s32Ret = configOsdUser2InfoCanvasSize(s32VencChn, pstRect);
                    break;
                case 7:
                    stMixerRgnWidgetAttr.u32Color = 0xff239137; //green
                    s32Ret = configOsdUser3InfoCanvasSize(s32VencChn, pstRect);
                    break;
                default: stMixerRgnWidgetAttr.u32Color = 0xff801080; break; //black
            }

            if(MI_SUCCESS != s32Ret)
            {
                MIXER_ERR("configOsdXXXCanvasSize() error(0x%X), VencChn=%d, s32Idx=%d\n", s32Ret, s32VencChn, s32Idx);
                return s32Ret;
            }
#if 0
            if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
            {
                if(0 == s32Idx)
                {
                    if((1920 < g_videoEncoderArray[s32VencChn]->m_pipRectW) && (1080 < g_videoEncoderArray[s32VencChn]->m_pipRectH))
                    {
                        pstRect->u16Width  = MIXER_OSD_TEXT_WIDGET_WIDTH_LARGE_FONT;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_HEIGHT_LARGE_FONT;
                    }
                    else if((1280 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (720 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
                    {
                        pstRect->u16Width  = MIXER_OSD_TEXT_WIDGET_WIDTH_MEDIUM_FONT;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_HEIGHT_MEDIUM_FONT;
                    }
                    else if((640 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (480 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
                    {
                        pstRect->u16Width  = MIXER_OSD_TEXT_WIDGET_WIDTH_SMALL_FONT;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_HEIGHT_SMALL_FONT;
                    }
                    else
                    {
                        pstRect->u16Width  = g_videoEncoderArray[s32VencChn]->m_pipRectW;
                        pstRect->u16Height = g_videoEncoderArray[s32VencChn]->m_pipRectH;
                    }
                }
                else
                {
                    if((1920 < g_videoEncoderArray[s32VencChn]->m_pipRectW) && (1080 < g_videoEncoderArray[s32VencChn]->m_pipRectH))
                    {
                        pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_FULL_OSD_TEST_STRING_LENGTH;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT;
                    }
                    else if((1280 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (720 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
                    {
                        pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_FULL_OSD_TEST_STRING_LENGTH;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT;
                    }
                    else if((640 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (480 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
                    {
                        pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_FULL_OSD_TEST_STRING_LENGTH;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
                    }
                    else
                    {
                        pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_FULL_OSD_TEST_STRING_LENGTH;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
                    }
                }
            }
            else
            {
                if(0 == s32Idx)
                {
                    if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
                    {
                        pstRect->u16Width  = MIXER_OSD_TEXT_WIDGET_WIDTH_LARGE_FONT;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_HEIGHT_LARGE_FONT;
                    }
                    else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
                    {
                        pstRect->u16Width  = MIXER_OSD_TEXT_WIDGET_WIDTH_MEDIUM_FONT;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_HEIGHT_MEDIUM_FONT;
                    }
                    else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
                    {
                        pstRect->u16Width  = MIXER_OSD_TEXT_WIDGET_WIDTH_SMALL_FONT;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_HEIGHT_SMALL_FONT;
                    }
                    else
                    {
                        pstRect->u16Width  = g_videoEncoderArray[s32VencChn]->m_width;
                        pstRect->u16Height = g_videoEncoderArray[s32VencChn]->m_height;
                    }
                }
                else
                {
                    if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
                    {
                        pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_LARGE_FONT) * MIXER_FULL_OSD_TEST_STRING_LENGTH;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT;
                    }
                    else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
                    {
                        pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_MEDIUM_FONT) * MIXER_FULL_OSD_TEST_STRING_LENGTH;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT;
                    }
                    else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
                    {
                        pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_FULL_OSD_TEST_STRING_LENGTH;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
                    }
                    else
                    {
                        pstRect->u16Width  = mid_Font_GetSizeByType(MIXER_OSD_TEXT_SMALL_FONT) * MIXER_FULL_OSD_TEST_STRING_LENGTH;
                        pstRect->u16Height = MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT;
                    }
                }
            }


            if(0 == s32Idx)
            {
                pstRect->u16X = 0;
                pstRect->u16Y = 0;
            }
            else
            {
                if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
                {
                    pstRect->u16X = (g_videoEncoderArray[s32VencChn]->m_width / 3) * ((s32Idx - 1) % 3);
                    pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_HEIGHT_LARGE_FONT + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT +
                                    ((g_videoEncoderArray[s32VencChn]->m_height - MIXER_OSD_TEXT_WIDGET_HEIGHT_LARGE_FONT) / 3) * ((s32Idx - 1) / 3);
                }
                else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
                {
                    pstRect->u16X = (g_videoEncoderArray[s32VencChn]->m_width / 3) * ((s32Idx - 1) % 3);
                    pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_HEIGHT_MEDIUM_FONT + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT +
                                    ((g_videoEncoderArray[s32VencChn]->m_height - MIXER_OSD_TEXT_WIDGET_HEIGHT_MEDIUM_FONT) / 3) * ((s32Idx - 1) / 3);
                }
                else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
                {
                    pstRect->u16X = (g_videoEncoderArray[s32VencChn]->m_width / 3) * ((s32Idx - 1) % 3);
                    pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_HEIGHT_SMALL_FONT + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT +
                                    ((g_videoEncoderArray[s32VencChn]->m_height - MIXER_OSD_TEXT_WIDGET_HEIGHT_SMALL_FONT) / 3) * ((s32Idx - 1) / 3);
                }
                else
                {
                    pstRect->u16X = (g_videoEncoderArray[s32VencChn]->m_width / 3) * ((s32Idx - 1) % 3);
                    pstRect->u16Y = MIXER_OSD_TEXT_WIDGET_HEIGHT_SMALL_FONT + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT +
                                    ((g_videoEncoderArray[s32VencChn]->m_height - MIXER_OSD_TEXT_WIDGET_HEIGHT_SMALL_FONT) / 3) * ((s32Idx - 1) / 3);
                }
            }
#endif
#if TARGET_CHIP_I6B0
            MI_S32 s32VencChn2 = 0;
            if(checkVencInitOsdFull(s32VencChn,&s32VencChn2)==TRUE)
            {
                g_au32OsdTextWidgetHandle[s32VencChn][s32Idx] = MI_RGN_HANDLE_NULL;
            }
            else
#endif
            {
                pu32RgnHandle = &g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
                s32Ret = createOsdWidget(pu32RgnHandle, &stMixerRgnWidgetAttr);
                if(MI_SUCCESS != s32Ret)
                {
                    MIXER_ERR("createOsdWidget() error(0x%X)\n", s32Ret);
                    return s32Ret;
                }
            }

        }
    }

    SetOsdInitFullState(TRUE);
    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 uninitOsdFull()
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_S32 s32Idx = 0;
    MI_VENC_CHN s32VencChn = 0;
     for(s32VencChn = 0; (s32VencChn < g_s32VideoStreamNum) && (NULL != g_videoEncoderArray[s32VencChn]); s32VencChn++)
     {
         if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn])
         {
             s32Ret = destroyOsdWidget(s32VencChn, g_au32OsdRectWidgetHandle[s32VencChn]);
             if(MI_SUCCESS != s32Ret)
             {
                MIXER_ERR("destroyOsdWidget() error(0x%X)\n", s32Ret);
             }
        }
        for(s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
        {
            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdTextWidgetHandle[s32VencChn][s32Idx])
            {
              s32Ret = destroyOsdWidget(s32VencChn, g_au32OsdTextWidgetHandle[s32VencChn][s32Idx]);
              if(MI_SUCCESS != s32Ret)
              {
                MIXER_ERR("destroyOsdWidget() error(0x%X)\n", s32Ret);
              }
               }
            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx] && (s32Idx < MAX_COVER_NUMBER_PER_CHN))
            {
                s32Ret = destroyOsdWidget(s32VencChn, g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx]);
                if(MI_SUCCESS != s32Ret)
                {
                    MIXER_ERR("destroyOsdWidget() error(0x%X)\n", s32Ret);
                 //   continue;
                }
            }
            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI2Widget[s32VencChn][s32Idx])
            {
                s32Ret = destoryOsdHandle(g_au32OsdI2Widget[s32VencChn][s32Idx],s32VencChn,s32Idx,E_OSD_WIDGET_TYPE_BITMAP);
                if(MI_SUCCESS != s32Ret)
                {
                    MIXER_ERR("destroyOsdWidget() error(0x%X)\n", s32Ret);
                 //   continue;
                }
            }
            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI4Widget[s32VencChn][s32Idx])
            {
                s32Ret = destoryOsdHandle(g_au32OsdI4Widget[s32VencChn][s32Idx],s32VencChn,s32Idx,E_OSD_WIDGET_TYPE_BITMAP);
                if(MI_SUCCESS != s32Ret)
                {
                    MIXER_ERR("destroyOsdWidget() error(0x%X)\n", s32Ret);
                  //  continue;
                }
            }
            if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32BitmapWidget[s32VencChn][s32Idx])
            {
                s32Ret = destoryOsdHandle(g_au32BitmapWidget[s32VencChn][s32Idx],s32VencChn,s32Idx,E_OSD_WIDGET_TYPE_BITMAP);
                if(MI_SUCCESS != s32Ret)
                {
                    MIXER_ERR("destroyOsdWidget() error(0x%X)\n", s32Ret);
                   // continue;
                }
            }
        }

        g_stVideoSize[s32VencChn].u16Width  = 0;
        g_stVideoSize[s32VencChn].u16Height = 0;
     }
     if((MI_SUCCESS == s32Ret) && (TRUE == GetOsdInitFullState()))
     {
        g_s32OsdRectHandleCnt = 0;
        g_bOsdColorInverse = FALSE;
     }

    return s32Ret;
}
int  initOsdModule(char *param, int paramLen)
{
    //char strProp[128];
    //MI_S32 iqparam[4];
    MI_S32 s32Idx = 0;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Ret = E_MI_ERR_FAILED;

    if((NULL == param) || (paramLen <= 0))
    {
        return s32Ret;
    }

    g_s32VideoStreamNum = *param;
    printf("%s:%d ====>OSD get video stream Num=%d!\n", __func__, __LINE__, g_s32VideoStreamNum);

    for(s32VencChn = 0; s32VencChn < MAX_VIDEO_NUMBER; s32VencChn++)
    {
        pthread_mutex_init(&g_stMutexMixerOsdRun[s32VencChn], NULL);
        g_bOsdSetResulution[s32VencChn] = FALSE;
        g_bMaskOsdInit[s32VencChn] = FALSE;
        g_bMaskOsdCnt[s32VencChn] = 0;

        g_stVideoSize[s32VencChn].u16Width  = 0;
        g_stVideoSize[s32VencChn].u16Height = 0;

        g_au32OsdRectWidgetHandle[s32VencChn] = MI_RGN_HANDLE_NULL;
        for (s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
        {
           g_au32OsdI2Widget[s32VencChn][s32Idx] = MI_RGN_HANDLE_NULL;
           g_au32OsdI4Widget[s32VencChn][s32Idx] = MI_RGN_HANDLE_NULL;
           g_au32OsdI8Widget[s32VencChn][s32Idx] = MI_RGN_HANDLE_NULL;
           g_au32BitmapWidget[s32VencChn][s32Idx] = MI_RGN_HANDLE_NULL;
           g_au32OsdTextWidgetHandle[s32VencChn][s32Idx] = MI_RGN_HANDLE_NULL;
           g_au32OsdTextWidgetHandleUseFlag[s32VencChn][s32Idx] = TRUE;
           if(s32Idx < MAX_COVER_NUMBER_PER_CHN)
            g_au32OsdCoverWidgetHandle[s32VencChn][s32Idx] = MI_RGN_HANDLE_NULL;
        }
    }

    pthread_mutex_init(&g_stMutexMixerOsdTextRgnBuf, NULL);


    if(NULL != g_videoEncoderArray[0] )
    {
        g_s32Width  = g_videoEncoderArray[0]->m_width;
        g_s32Height = g_videoEncoderArray[0]->m_height;
    }

    for(s32VencChn = 0; s32VencChn < MAX_VIDEO_NUMBER; s32VencChn++)
    {
        g_stMixerOsdWidgetOrder.size[s32VencChn] = MIXER_OSD_TEXT_MEDIUM_FONT;
    }
    g_stMixerOsdWidgetOrder.pmt  = E_MI_RGN_PIXEL_FORMAT_I4;
    //g_stMixerOsdWidgetOrder.pmt  = E_MI_RGN_PIXEL_FORMAT_ARGB1555;
    //g_stMixerOsdWidgetOrder.stPointTime.x = MIXER_OSD_TEXT_WIDGET_START_X;
    //g_stMixerOsdWidgetOrder.stPointTime.y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * 0;
    g_stMixerOsdWidgetOrder.stPointFps.x = MIXER_OSD_TEXT_WIDGET_START_X;
    g_stMixerOsdWidgetOrder.stPointFps.y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * 0;
    g_stMixerOsdWidgetOrder.stPointBitRate.x = MIXER_OSD_TEXT_WIDGET_START_X;
    g_stMixerOsdWidgetOrder.stPointBitRate.y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * 1;
    g_stMixerOsdWidgetOrder.stPointGop.x = MIXER_OSD_TEXT_WIDGET_START_X;
    g_stMixerOsdWidgetOrder.stPointGop.y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * 2;
    g_stMixerOsdWidgetOrder.stPointResolution.x = MIXER_OSD_TEXT_WIDGET_START_X;
    g_stMixerOsdWidgetOrder.stPointResolution.y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * 3;
    g_stMixerOsdWidgetOrder.stPointTemp.x = MIXER_OSD_TEXT_WIDGET_START_X;
    g_stMixerOsdWidgetOrder.stPointTemp.y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * 4;
    g_stMixerOsdWidgetOrder.stPointTotalGain.x = MIXER_OSD_TEXT_WIDGET_START_X;
    g_stMixerOsdWidgetOrder.stPointTotalGain.y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * 5;
    g_stMixerOsdWidgetOrder.stPointIspExpo.x = MIXER_OSD_TEXT_WIDGET_START_X;
    g_stMixerOsdWidgetOrder.stPointIspExpo.y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * 0;
    g_stMixerOsdWidgetOrder.stPointIspWb.x = MIXER_OSD_TEXT_WIDGET_START_X;
    g_stMixerOsdWidgetOrder.stPointIspWb.y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * 1;
    g_stMixerOsdWidgetOrder.stPointIspExpoInfo.x = MIXER_OSD_TEXT_WIDGET_START_X;
    g_stMixerOsdWidgetOrder.stPointIspExpoInfo.y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * 2;
    g_stMixerOsdWidgetOrder.stPointUser.x = MIXER_OSD_TEXT_WIDGET_START_X;
    g_stMixerOsdWidgetOrder.stPointUser.y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * 0;
    g_stMixerOsdWidgetOrder.stPointUser1.x = MIXER_OSD_TEXT_WIDGET_START_X;
    g_stMixerOsdWidgetOrder.stPointUser1.y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * 0;

    memset(&g_stFdRect, 0x00, sizeof(MI_SYS_WindowRect_t) * MAX_FD_RECT_NUMBER);
    memset(&g_stFrRect, 0x00, sizeof(MI_SYS_WindowRect_t) * MAX_FR_RECT_NUMBER);
    memset(&g_stOdRect, 0x00, sizeof(MI_SYS_WindowRect_t) * OD_DIV_W * OD_DIV_H);

    g_bExit = FALSE;
    g_bThreadOsdExit = FALSE;

    initOsdFull();

    switch(g_stMixerOsdWidgetOrder.size[s32VencChn])
    {
        case FONT_SIZE_8:  printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_8");  break;
        case FONT_SIZE_12: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_12"); break;
        case FONT_SIZE_16: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_16"); break;
        case FONT_SIZE_24: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_24"); break;
        case FONT_SIZE_32: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_32"); break;
        case FONT_SIZE_36: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_36"); break;
        case FONT_SIZE_40: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_40"); break;
        case FONT_SIZE_48: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_48"); break;
        case FONT_SIZE_56: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_56"); break;
        case FONT_SIZE_60: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_60"); break;
        case FONT_SIZE_64: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_64"); break;
        case FONT_SIZE_72: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_72"); break;
        case FONT_SIZE_80: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_80"); break;
        case FONT_SIZE_84: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_84"); break;
        case FONT_SIZE_96: printf("set OSD pixer size:    %d (%s)\n", g_stMixerOsdWidgetOrder.size[s32VencChn], "FONT_SIZE_96"); break;
        default:           printf("set OSD pixer size:    %d (Set wrong param)\n", g_stMixerOsdWidgetOrder.size[s32VencChn]);
    }

    switch(g_stMixerOsdWidgetOrder.pmt)
    {
        case 0: printf("set OSD pixer format:  %d (%s)\n", g_stMixerOsdWidgetOrder.pmt, "E_MI_RGN_PIXEL_FORMAT_ARGB1555"); break;
        case 1: printf("set OSD pixer format:  %d (%s)\n", g_stMixerOsdWidgetOrder.pmt, "E_MI_RGN_PIXEL_FORMAT_ARGB4444"); break;
        case 2: printf("set OSD pixer format:  %d (%s)\n", g_stMixerOsdWidgetOrder.pmt, "E_MI_RGN_PIXEL_FORMAT_I2"); break;
        case 3: printf("set OSD pixer format:  %d (%s)\n", g_stMixerOsdWidgetOrder.pmt, "E_MI_RGN_PIXEL_FORMAT_I4"); break;
        default:printf("set OSD pixer format:  %d (Set wrong param)\n", g_stMixerOsdWidgetOrder.pmt);
    }
    pthread_attr_t attr; 
	MI_S32 policy = 0;
    pthread_attr_init(&attr);
	policy = SCHED_FIFO;
    Mixer_set_thread_policy(&attr,policy);
	struct sched_param s_parm;
    s_parm.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &s_parm);
	policy = Mixer_get_thread_policy(&attr);
    Mixer_show_thread_priority(&attr, policy);
    Mixer_get_thread_priority(&attr);
    pthread_create(&g_stPthreadOsd, &attr, OSD_Task, (void*)NULL);
    pthread_setname_np(g_stPthreadOsd , "OSD_Task");
    g_s32OsdInitDone = 1;
    MIXER_INFO("initOsdModule()\n");

    s32Ret = MI_RGN_OK;
    return s32Ret;
}


int  uninitOsdModule()
{
 //   MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_VENC_CHN s32VencChn = 0;

    if(1 != g_s32OsdInitDone)
    {
        return 0;
    }

    g_bExit = TRUE;

    while(FALSE == g_bThreadOsdExit)
    {
        sleep(1);
        MIXER_INFO("uninitOsdModule() wait for exit\n");
    }
    pthread_join(g_stPthreadOsd, NULL);
    g_s32OsdInitDone = 0;
    uninitOsdFull();
	//if(g_dlaParam)
	{
  	   pthread_mutex_destroy(&g_mutex_UpadteOsdState);  
       pthread_cond_destroy(&g_cond_UpadteOsdState);
	}
    //no need RGN_DeInit when exitting mixer.
   /* s32Ret = MI_RGN_DeInit();
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_DeInit error(0x%X)\n", s32Ret);
        return s32Ret;
    }*/
    SetOsdInitFullState(FALSE);

    for(s32VencChn = 0; s32VencChn < MAX_VIDEO_NUMBER; s32VencChn++)
    {
        pthread_mutex_destroy(&g_stMutexMixerOsdRun[s32VencChn]);
        g_bMaskOsdInit[s32VencChn] = FALSE;
        g_bMaskOsdCnt[s32VencChn] = 0;
    }
    if(NULL != g_pbuffer)
    {
        free(g_pbuffer);
    }
    return MI_SUCCESS;
}

MI_S32 setOsdDisplayLocation(VENC_CHN s32VencChn, MI_S32 s32Idx, OsdFontSize_e *peSize, Point_t *pstPoint)
{
    MI_S32 s32Ret = MI_SUCCESS;

    if(0 > s32VencChn || s32VencChn >= MAX_VIDEO_NUMBER || NULL == g_videoEncoderArray[s32VencChn])
    {
        if((s32VencChn < 0) || (s32VencChn >= MAX_VIDEO_NUMBER))
        {
            MIXER_ERR("The input VencChn(%d) is out of range!\n", s32VencChn);
            s32Ret = E_MI_ERR_ILLEGAL_PARAM;
            return s32Ret;
        }
        MIXER_WARN("g_videoEncoderArray[s32VencChn] == NULL. return.\n");
        return  s32Ret;
    }

    if((NULL == peSize) || (NULL == pstPoint))
    {
        MIXER_ERR("The input Rgn pointer is NULL!\n");
        s32Ret = E_MI_ERR_NULL_PTR;
        return s32Ret;
    }

    if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
    {
        if((1920 < g_videoEncoderArray[s32VencChn]->m_pipRectW) && (1080 < g_videoEncoderArray[s32VencChn]->m_pipRectH))
        {
            *peSize = MIXER_OSD_TEXT_LARGE_FONT;
            pstPoint->x = MIXER_OSD_TEXT_WIDGET_START_X;
            pstPoint->y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * s32Idx;
        }
        else if((1280 <= g_videoEncoderArray[s32VencChn]->m_pipRectW) && (720 <= g_videoEncoderArray[s32VencChn]->m_pipRectH))
        {
            *peSize = MIXER_OSD_TEXT_MEDIUM_FONT;
            pstPoint->x = MIXER_OSD_TEXT_WIDGET_START_X;
            pstPoint->y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * s32Idx;
        }
        else
        {
            *peSize = MIXER_OSD_TEXT_SMALL_FONT;
            pstPoint->x = MIXER_OSD_TEXT_WIDGET_START_X;
            pstPoint->y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * s32Idx;
        }
    }
    else
    {
        if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
        {
            *peSize = MIXER_OSD_TEXT_LARGE_FONT;
            pstPoint->x = MIXER_OSD_TEXT_WIDGET_START_X;
            pstPoint->y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * s32Idx;
        }
        else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
        {
            *peSize = MIXER_OSD_TEXT_MEDIUM_FONT;
            pstPoint->x = MIXER_OSD_TEXT_WIDGET_START_X;
            pstPoint->y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_MEDIUM_FONT * s32Idx;
        }
        else
        {
            *peSize = MIXER_OSD_TEXT_SMALL_FONT;
            pstPoint->x = MIXER_OSD_TEXT_WIDGET_START_X;
            pstPoint->y = MIXER_OSD_TEXT_WIDGET_START_Y + MIXER_OSD_TEXT_WIDGET_SPACE_SMALL_FONT * s32Idx;
        }
    }

    return s32Ret;
}


static MI_S32 displayOsdFps(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    //stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stRedColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 0, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return s32Ret;
}

static MI_S32 displayOsdBitRate(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    //stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stRedColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 1, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return s32Ret;
}

static MI_S32 displayOsdGop(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    //stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stRedColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 2, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return s32Ret;
}

static MI_S32 displayOsdResolution(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    //stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stRedColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 3, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return s32Ret;
}

static int getCpuTemp(void)
{
    int fd = -1;
    int temp = 0x0;

    fd = open("/dev/msys", O_RDONLY);

    if(fd >= 0)
    {
#define MSYS_IOCTL_MAGIC               'S'
#define IOCTL_MSYS_READ_PM_TSENSOR     _IO(MSYS_IOCTL_MAGIC, 0x78)
        ioctl(fd, IOCTL_MSYS_READ_PM_TSENSOR, &temp);
        close(fd);
    }

    return temp;
}

static MI_S32 displayOsdTemp(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    //stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stRedColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 4, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return s32Ret;
}

static MI_S32 displayOsdBabyCry(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    //stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stRedColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 0, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    stTextWidgetAttr.size = FONT_SIZE_12;
    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return s32Ret;
}

static MI_S32 displayOsdLsd(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    //stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stRedColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 1, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    stTextWidgetAttr.size = FONT_SIZE_12;
    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return s32Ret;
}

static MI_S32 getAeTotalGain(MI_U32 nChannel)
{
    MI_S32 s32Ret = MI_ISP_FAILURE;
    MI_U32 total_gain = 0;
    MI_ISP_AE_EXPO_INFO_TYPE_t stAeExpInfo;

    if(MI_ISP_OK != (s32Ret = MI_ISP_AE_QueryExposureInfo(nChannel, &stAeExpInfo))){
        MIXER_ERR("MI_ISP_AE_QueryExposureInfo error = %d\n", s32Ret);
        return s32Ret;
    }

    total_gain = (stAeExpInfo.stExpoValueLong.u32SensorGain * stAeExpInfo.stExpoValueLong.u32ISPGain) / (1024 * 1024);
    return total_gain;
}

static MI_S32 displayOsdTotalGain(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    //stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stRedColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 5, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return s32Ret;
}

static MI_ISP_AE_EXPO_INFO_TYPE_t* getAeExpoAttr(MI_U32 nChannel)
{
   // MI_S32 s32Ret = MI_ISP_FAILURE;
    static MI_ISP_AE_EXPO_INFO_TYPE_t stAeExpInfo;

    memset(&stAeExpInfo, 0x00, sizeof(MI_ISP_AE_EXPO_INFO_TYPE_t));
/*
    if(MI_ISP_FAILURE != (s32Ret = MI_ISP_AE_QueryExposureInfo(nChannel, &stAeExpInfo))){
        MIXER_ERR("MI_ISP_AE_QueryExposureInfo error = %d\n", s32Ret);
        return NULL;
    }
*/
    return &stAeExpInfo;
}

static MI_S32 displayOsdAeExpoAttr(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    //stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stYellowColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 0, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    stTextWidgetAttr.size = FONT_SIZE_12;
    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return 0;
}

#if TARGET_CHIP_I5 || TARGET_CHIP_I6
//static MI_ISP_AWB_ATTR_TYPE_t* getAwbAttr(MI_U32 nChannel)
//{
    //MI_S32 s32Ret = MI_ISP_FAILURE;
 //   static MI_ISP_AWB_ATTR_TYPE_t stAwbAttr;

 //   memset(&stAwbAttr, 0x00, sizeof(MI_ISP_AWB_ATTR_TYPE_t));
/*
    if(MI_ISP_OK != (s32Ret = MI_ISP_AWB_GetAttr(nChannel, &stAwbAttr))){
        MIXER_ERR("MI_ISP_GetWhiteBalanceAttr error = %d\n", s32Ret);
        return NULL;
    }
*/
//    return &stAwbAttr;
//}

static MI_S32 displayOsdAwbAttr(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    //stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stYellowColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 1, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    stTextWidgetAttr.size = FONT_SIZE_12;
    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return 0;
}
#endif

static MI_ISP_AE_EXPO_INFO_TYPE_t* queryAeExpoInfo(MI_U32 nChannel)
{
//    MI_S32 s32Ret = MI_ISP_FAILURE;
    static MI_ISP_AE_EXPO_INFO_TYPE_t stAeExpInfo;

    memset(&stAeExpInfo, 0x00, sizeof(MI_ISP_AE_EXPO_INFO_TYPE_t));
/*
    if(MI_ISP_FAILURE != (s32Ret = MI_ISP_AE_QueryExposureInfo(nChannel, &stAeExpInfo))){
        MIXER_ERR("MI_ISP_AE_QueryExposureInfo error = %d\n", s32Ret);
        return NULL;
    }
*/
    return &stAeExpInfo;
}

static MI_S32 displayOsdExpoInfo(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    //stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stYellowColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 2, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    stTextWidgetAttr.size = FONT_SIZE_12;
    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return 0;
}

static MI_S32 displayOsdUserInfo(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stRedColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 0, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return 0;
}

#if 0
static MI_S32 displayOsdUserInfo1(VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo, char *string)
{
    MI_S32 s32Ret = MI_SUCCESS;
    Point_t stPoint;
    TextWidgetAttr_t stTextWidgetAttr;

    memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));
    stTextWidgetAttr.string  = string;
    stTextWidgetAttr.pPoint  = &stPoint;
    //stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
    stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
    stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
    stTextWidgetAttr.pfColor = &g_stRedColor;
    stTextWidgetAttr.pbColor = &g_stBlackColor;

    stTextWidgetAttr.space = 0;
    //stTextWidgetAttr.bHard = FALSE;
    //stTextWidgetAttr.bRle  = FALSE;
    stTextWidgetAttr.bOutline = FALSE;

    s32Ret = setOsdDisplayLocation(s32VencChn, 0, &stTextWidgetAttr.size, &stPoint);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
        return s32Ret;
    }

    stTextWidgetAttr.size = FONT_SIZE_8;
    updateOsdTextWidget(hHandle, pstCanvasInfo, &stTextWidgetAttr, TRUE);

    return 0;
}
#endif

void initOsdVideoInfo()
{
    pthread_mutex_init(&g_stVideoInfoMutex, NULL);
    pthread_mutex_lock(&g_stVideoInfoMutex);

    g_s32OsdDisplayVideoInfo = TRUE;

    pthread_mutex_unlock(&g_stVideoInfoMutex);
}

void uninitOsdVideoInfo()
{
    pthread_mutex_lock(&g_stVideoInfoMutex);

    g_s32OsdDisplayVideoInfo = FALSE;

    pthread_mutex_unlock(&g_stVideoInfoMutex);
    pthread_mutex_destroy(&g_stVideoInfoMutex);
}

void displayOsdVideoInfomation(MI_VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo)
{
    char msg[16];
    char text[128];

    if(NULL == g_videoEncoderArray[s32VencChn] || NULL == g_videoEncoderArray[s32VencChn]->m_videoInfoCaculator)
    {
        MIXER_WARN("g_videoEncoderArray[s32VencChn] == NULL. return.\n");

        return ;
    }

    memset(msg, 0x00, sizeof(msg));
    memset(text, 0x00, sizeof(text));
    switch(g_videoEncoderArray[s32VencChn]->m_encoderType)
    {
        case VE_AVC:   strcpy(msg, "H264");  break;
        case VE_H265:  strcpy(msg, "H265");  break;
        case VE_MJPEG: strcpy(msg, "MJpeg"); break;
        case VE_JPG:   strcpy(msg, "Jpeg");  break;
        default:       break;
    }
    sprintf(text, "%s  fps:%.02f  ", msg, g_videoEncoderArray[s32VencChn]->m_videoInfoCaculator->GetFps());
    displayOsdFps(s32VencChn, hHandle, pstCanvasInfo, text);

    sprintf(text, "bitrate:%dk  ", g_videoEncoderArray[s32VencChn]->m_videoInfoCaculator->GetBitrate() / 1000);
    displayOsdBitRate(s32VencChn, hHandle, pstCanvasInfo, text);

    sprintf(text, "gop:%d  ", g_videoEncoderArray[s32VencChn]->m_videoInfoCaculator->GetGop());
    displayOsdGop(s32VencChn, hHandle, pstCanvasInfo, text);

    sprintf(text, "resolution:%dx%d  ", g_videoEncoderArray[s32VencChn]->m_width, g_videoEncoderArray[s32VencChn]->m_height);
    displayOsdResolution(s32VencChn, hHandle, pstCanvasInfo, text);

    sprintf(text, "temp:%d  ", getCpuTemp());
    displayOsdTemp(s32VencChn, hHandle, pstCanvasInfo, text);

    sprintf(text, "totalgain:%d", getAeTotalGain(0));
    displayOsdTotalGain(s32VencChn, hHandle, pstCanvasInfo, text);
}

void displayOsdIspInfomation(MI_VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo)
{
    char text[128];

    //#if TARGET_CHIP_I6 || TARGET_CHIP_I5
   // MI_ISP_AWB_ATTR_TYPE_t* pwbAttr = NULL;
   // #endif

    MI_ISP_AE_EXPO_INFO_TYPE_t* pExpInfo = NULL;
    MI_ISP_AE_EXPO_INFO_TYPE_t * pexpoAttr = NULL;

    pexpoAttr = getAeExpoAttr(0);
    sprintf(text, "expo_info:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                   pexpoAttr->bIsStable,
                   pexpoAttr->stExpoValueLong.u32FNx10,
                   pexpoAttr->stExpoValueLong.u32SensorGain,
                   pexpoAttr->stExpoValueLong.u32ISPGain,
                   pexpoAttr->stExpoValueLong.u32US,
                   pexpoAttr->stExpoValueShort.u32FNx10,
                   pexpoAttr->stExpoValueShort.u32SensorGain,
                   pexpoAttr->stExpoValueShort.u32ISPGain,
                   pexpoAttr->stExpoValueShort.u32US,
                   pexpoAttr->u32LVx10,
                   pexpoAttr->s32BV,
                   pexpoAttr->u32SceneTarget);
    displayOsdAeExpoAttr(s32VencChn, hHandle, pstCanvasInfo, text);

#if TARGET_CHIP_I5 || TARGET_CHIP_I6
   // pwbAttr = getAwbAttr(0);
    #if TARGET_OS_DUAL
    /*sprintf(text, "awb_attr:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                   pwbAttr->eOpType,
                   pwbAttr->stManualParaAPI.u16Rgain,
                   pwbAttr->stManualParaAPI.u16Grgain,
                   pwbAttr->stManualParaAPI.u16Gbgain,
                   pwbAttr->stManualParaAPI.u16Bgain,
                   pwbAttr->stAutoParaAPI.u8Speed,
                   pwbAttr->stAutoParaAPI.u8ConvInThd,
                   pwbAttr->stAutoParaAPI.u8ConvOutThd,
                   pwbAttr->stAutoParaAPI.u16ZoneSel,
                   pwbAttr->stAutoParaAPI.eAlgType,
                   pwbAttr->stAutoParaAPI.u8RGStrength,
                   pwbAttr->stAutoParaAPI.u8BGStrength);*/
    #else
    /*sprintf(text, "awb_attr:%d,%d,%d,%d,%d,%d,%d,%d",
                   pwbAttr->eOpType,
                   pwbAttr->stManualParaAPI.u16Rgain,
                   pwbAttr->stManualParaAPI.u16Grgain,
                   pwbAttr->stManualParaAPI.u16Gbgain,
                   pwbAttr->stManualParaAPI.u16Bgain,
                   pwbAttr->stAutoParaAPI.u8Speed,
                   //pwbAttr->stAutoParaAPI.u8Tolerance,      //build master_i6
                   //pwbAttr->stAutoParaAPI.u16RefColorTemp,  //build master_i6
                   //pwbAttr->stAutoParaAPI.u16RefRgain,      //build master_i6
                   //pwbAttr->stAutoParaAPI.u16RefBgain,      //build master_i6
                   pwbAttr->stAutoParaAPI.eAdvType,
                   pwbAttr->stAutoParaAPI.eAlgType);*/
    #endif
    displayOsdAwbAttr(s32VencChn, hHandle, pstCanvasInfo, text);
#endif

    pExpInfo = queryAeExpoInfo(0);
    sprintf(text, "expo_info:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
            pExpInfo->bIsStable,
            pExpInfo->bIsReachBoundary,
            pExpInfo->stExpoValueLong.u32FNx10,
            pExpInfo->stExpoValueLong.u32SensorGain,
            pExpInfo->stExpoValueLong.u32ISPGain,
            pExpInfo->stExpoValueLong.u32US,
            pExpInfo->stExpoValueShort.u32FNx10,
            pExpInfo->stExpoValueShort.u32SensorGain,
            pExpInfo->stExpoValueShort.u32ISPGain,
            pExpInfo->stExpoValueShort.u32US,
            pExpInfo->u32LVx10,
            pExpInfo->s32BV,
            pExpInfo->u32SceneTarget);
    displayOsdExpoInfo(s32VencChn, hHandle, pstCanvasInfo, text);
}

void displayOsdAudioInfomation(MI_VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo)
{
    char text[128];


    if(TRUE == g_bBabyCry)
    {
        sprintf(text, "%s", g_ps8BabyCryString);
        displayOsdBabyCry(s32VencChn, hHandle, pstCanvasInfo, text);
    }
    else
    {
        memset(text, 0x00, sizeof(text));
        displayOsdBabyCry(s32VencChn, hHandle, pstCanvasInfo, text);
    }

    if(TRUE == g_bLoudSound)
    {
        sprintf(text, "%s, SounddB:%d", g_ps8LoudSoundString, g_s16LoudSounddB);
        displayOsdLsd(s32VencChn, hHandle, pstCanvasInfo, text);
    }
    else
    {
        memset(text, 0x00, sizeof(text));
        displayOsdLsd(s32VencChn, hHandle, pstCanvasInfo, text);
    }
}


void displayOsdUser0Infomation(MI_VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo)
{
    char text[128];

    sprintf(text, "userinfo0: %s", "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    displayOsdUserInfo(s32VencChn, hHandle, pstCanvasInfo, text);
}

void displayOsdUser1Infomation(MI_VENC_CHN s32VencChn, MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t *pstCanvasInfo)
{
    char text[128];

    sprintf(text, "userinfo1: %s", "0123456789abcdefghijklmnopqrstuvwxyz");
    displayOsdUserInfo(s32VencChn, hHandle, pstCanvasInfo, text);
}

MI_S32 enableOsdColorInverse()
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_S32 s32Idx = 0;
    MI_VENC_CHN s32VencChn = 0;
    //MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_RGN_ChnPortParam_t stRgnChnPortParam;
#if TARGET_CHIP_I6B0
    MI_VENC_CHN s32VencChn_tmp = 0;
#endif

    printf("%s:%d g_bOsdColorInverse=%d\n",__func__,__LINE__,g_bOsdColorInverse);

    if((FALSE == g_s32OsdRectHandleCnt) || (TRUE == g_bOsdColorInverse))
    {
        g_bOsdColorInverse = TRUE;
        printf("%s:%d g_bOsdColorInverse=%d\n",__func__,__LINE__,g_bOsdColorInverse);
        s32Ret = MI_SUCCESS;
        return s32Ret;
    }
    for(s32VencChn = 0; s32VencChn < g_s32VideoStreamNum; s32VencChn++)
    {
        if(NULL == g_videoEncoderArray[s32VencChn])
        {
            printf("%s:%d not initial g_videoEncoderArray[%d] and continue!\n", __func__, __LINE__, s32VencChn);
            continue;
        }
#if TARGET_CHIP_I6B0
        if(checkVencInitOsdFull(s32VencChn,&s32VencChn_tmp) == TRUE)
        {
            MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",s32VencChn,s32VencChn_tmp,s32VencChn_tmp,s32VencChn);
            continue;
        }
#endif

        pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

        for(s32Idx = 0; s32Idx < g_s32OsdTextHandleCnt; s32Idx++)
        {
#if MIXER_OSD_COLOR_INVERSE_ACTING_ON_RECT_WIDGET
#if MIXER_OSD_TEXT_WIDGET_ENABLE
            if((MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn]) &&
               (((g_videoEncoderArray[s32VencChn]->m_width <= 1920) && (g_videoEncoderArray[s32VencChn]->m_height <= 1080)) ||
                ((g_videoEncoderArray[s32VencChn]->m_width <= 1080) && (g_videoEncoderArray[s32VencChn]->m_height <= 1920))))
#else
            if((g_bIEOsdInit || gDebug_OsdTest) && (MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn]) &&
              (((g_videoEncoderArray[s32VencChn]->m_width <= 1920) && (g_videoEncoderArray[s32VencChn]->m_height <= 1080)) ||
               ((g_videoEncoderArray[s32VencChn]->m_width <= 1080) && (g_videoEncoderArray[s32VencChn]->m_height <= 1920))))
#endif
            {
                memset(&stRgnChnPortParam, 0x00, sizeof(MI_RGN_ChnPortParam_t));

                if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
                {
                    stRgnChnPort.eModId    = E_MI_RGN_MODID_DIVP;
                    stRgnChnPort.s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpSclUpChnId;
                    stRgnChnPort.s32DevId  = 0;
                    stRgnChnPort.s32OutputPortId = 0;
                }
                else if(g_videoEncoderArray[s32VencChn]->m_bUseDivp) //vpe port2 case: vif->vpe->divp->venc
                {
                    stRgnChnPort.eModId    = E_MI_RGN_MODID_DIVP;
                    stRgnChnPort.s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpChnPort.u32ChnId;
                    stRgnChnPort.s32DevId  = 0;
                    stRgnChnPort.s32OutputPortId = 0;
                }
                else
                {
                    stRgnChnPort.eModId   = E_MI_RGN_MODID_VPE;
                    stRgnChnPort.s32DevId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32DevId;
                    stRgnChnPort.s32ChnId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32ChnId;
                    stRgnChnPort.s32OutputPortId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32PortId;
                }

                if(MI_SUCCESS != MI_RGN_GetDisplayAttr(g_au32OsdRectWidgetHandle[s32VencChn], &stRgnChnPort, &stRgnChnPortParam))
                {
                    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
                    printf("Get osd Rect Widget hand=%d Display attribute error!\n", g_au32OsdRectWidgetHandle[s32VencChn]);
                    return s32Ret;
                }

                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode = E_MI_RGN_ABOVE_LUMA_THRESHOLD;
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum = getDividedNumber(g_videoEncoderArray[s32VencChn]->m_width);
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum = getDividedNumber(g_videoEncoderArray[s32VencChn]->m_height);

                if((1920 == g_videoEncoderArray[s32VencChn]->m_width) && (1080 == g_videoEncoderArray[s32VencChn]->m_height))
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum /= 2;
                else if((1080 == g_videoEncoderArray[s32VencChn]->m_width) && (1920 == g_videoEncoderArray[s32VencChn]->m_height))
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum /= 2;

                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold = MIXER_OSD_COLOR_INVERSE_THD;
                stRgnChnPortParam.unPara.stOsdChnPort.u32Layer = g_au32OsdRectWidgetHandle[s32VencChn];
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = TRUE;

                if(MI_SUCCESS != MI_RGN_SetDisplayAttr(g_au32OsdRectWidgetHandle[s32VencChn], &stRgnChnPort, &stRgnChnPortParam))
                {
                    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
                    printf("Set osd Rect Widget hand=%d color inverse error!\n", g_au32OsdRectWidgetHandle[s32VencChn]);
                    return s32Ret;
                }

                printf("%s:%d Osd_W=%4d, Osd_H=%4d, WDivNum=%d, HDivNum=%d, InvertColorMode=%d, Threshold=%d, Layer=%d\n", __func__, __LINE__,
                        g_videoEncoderArray[s32VencChn]->m_width, g_videoEncoderArray[s32VencChn]->m_height, stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum,
                        stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum, stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode,
                        stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold, stRgnChnPortParam.unPara.stOsdChnPort.u32Layer);
            }
#endif

#if MIXER_OSD_TEXT_WIDGET_ENABLE
            if(((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdTextWidgetHandle[s32VencChn][s32Idx]))
            {
                g_au32OsdTextWidgetHandle[s32VencChn][s32Idx] = calcOsdHandle(s32VencChn, s32Idx, E_OSD_WIDGET_TYPE_TEXT);

                memset(&stRgnChnPortParam, 0x00, sizeof(MI_RGN_ChnPortParam_t));

                if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
                {
                    stRgnChnPort.eModId    = E_MI_RGN_MODID_DIVP;
                    stRgnChnPort.s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpSclUpChnId;
                    stRgnChnPort.s32DevId  = 0;
                    stRgnChnPort.s32OutputPortId = 0;
                }
                else if(g_videoEncoderArray[s32VencChn]->m_bUseDivp) //vpe port2 case: vif->vpe->divp->venc
                {
                    stRgnChnPort.eModId    = E_MI_RGN_MODID_DIVP;
                    stRgnChnPort.s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpChnPort.u32ChnId;
                    stRgnChnPort.s32DevId  = 0;
                    stRgnChnPort.s32OutputPortId = 0;
                }
                else
                {
                    stRgnChnPort.eModId   = E_MI_RGN_MODID_VPE;
                    stRgnChnPort.s32DevId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32DevId;
                    stRgnChnPort.s32ChnId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32ChnId;
                    stRgnChnPort.s32OutputPortId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32PortId;
                }

                if(MI_SUCCESS != MI_RGN_GetDisplayAttr(g_au32OsdTextWidgetHandle[s32VencChn][s32Idx], &stRgnChnPort, &stRgnChnPortParam))
                {
                    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
                    printf("Get osd Text Widget hand=%d Display attribute error!\n", g_au32OsdTextWidgetHandle[s32VencChn][s32Idx]);
                    return s32Ret;
                }

                if((1920 < g_videoEncoderArray[s32VencChn]->m_width) && (1080 < g_videoEncoderArray[s32VencChn]->m_height))
                {
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum = getDividedNumber(MIXER_OSD_TEXT_WIDGET_WIDTH_LARGE_FONT);
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum = getDividedNumber(MIXER_OSD_TEXT_WIDGET_HEIGHT_LARGE_FONT);
                }
                else if((1280 <= g_videoEncoderArray[s32VencChn]->m_width) && (720 <= g_videoEncoderArray[s32VencChn]->m_height))
                {
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum = getDividedNumber(MIXER_OSD_TEXT_WIDGET_WIDTH_MEDIUM_FONT);
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum = getDividedNumber(MIXER_OSD_TEXT_WIDGET_HEIGHT_MEDIUM_FONT);
                }
                else if((640 <= g_videoEncoderArray[s32VencChn]->m_width) && (480 <= g_videoEncoderArray[s32VencChn]->m_height))
                {
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum = getDividedNumber(MIXER_OSD_TEXT_WIDGET_WIDTH_SMALL_FONT);
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum = getDividedNumber(MIXER_OSD_TEXT_WIDGET_HEIGHT_SMALL_FONT);
                }
                else
                {
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum = getDividedNumber(g_videoEncoderArray[s32VencChn]->m_width);
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum = getDividedNumber(g_videoEncoderArray[s32VencChn]->m_height);
                }

                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode = E_MI_RGN_ABOVE_LUMA_THRESHOLD;
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold = MIXER_OSD_COLOR_INVERSE_THD;
                stRgnChnPortParam.unPara.stOsdChnPort.u32Layer = g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
                stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = TRUE;

                if(MI_SUCCESS != MI_RGN_SetDisplayAttr(g_au32OsdTextWidgetHandle[s32VencChn][s32Idx], &stRgnChnPort, &stRgnChnPortParam))
                {
                    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
                    printf("Set osd Text Widget hand=%d color inverse error!\n", g_au32OsdTextWidgetHandle[s32VencChn][s32Idx]);
                    return s32Ret;
                }

                printf("%s:%d Osd_W=%4d, Osd_H=%4d, WDivNum=%d, HDivNum=%d, InvertColorMode=%d, Threshold=%d, Layer=%d\n", __func__, __LINE__,
                        g_videoEncoderArray[s32VencChn]->m_width, g_videoEncoderArray[s32VencChn]->m_height, stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum,
                        stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum, stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode,
                        stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold, stRgnChnPortParam.unPara.stOsdChnPort.u32Layer);
            }
#endif
        }

        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
    }

    g_bOsdColorInverse = TRUE;
    printf("%s:%d Enable OSD color inverse!\n", __func__, __LINE__);

    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 disableOsdColorInverse()
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_S32 s32Idx = 0;
    MI_VENC_CHN s32VencChn = 0;
    //MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_RGN_ChnPortParam_t stRgnChnPortParam;
#if TARGET_CHIP_I6B0
    MI_VENC_CHN s32VencChn_tmp = 0;
#endif

    if((FALSE == g_s32OsdRectHandleCnt) || (FALSE == g_bOsdColorInverse))
    {
        g_bOsdColorInverse = FALSE;
        s32Ret = MI_SUCCESS;
        return s32Ret;
    }

    for(s32VencChn = 0; s32VencChn < g_s32VideoStreamNum; s32VencChn++)
    {
        if(NULL == g_videoEncoderArray[s32VencChn])
        {
            printf("%s:%d not initial g_videoEncoderArray[%d] and continue!\n", __func__, __LINE__, s32VencChn);
            continue;
        }
#if TARGET_CHIP_I6B0
        if(checkVencInitOsdFull(s32VencChn,&s32VencChn_tmp) == TRUE)
        {
            MIXER_DBG("[I6B0]Because venc%d use the same RGN source with venc%d,you should operate venc%d install of venc%d\n",s32VencChn,s32VencChn_tmp,s32VencChn_tmp,s32VencChn);
            continue ;
        }
#endif

        pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

        for(s32Idx = 0; s32Idx < g_s32OsdTextHandleCnt; s32Idx++)
        {
#if MIXER_OSD_COLOR_INVERSE_ACTING_ON_RECT_WIDGET
#if MIXER_OSD_TEXT_WIDGET_ENABLE
            if((MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn]))
#else
            if((g_bIEOsdInit || gDebug_OsdTest) && (MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn]))
#endif
            {
                memset(&stRgnChnPortParam, 0x00, sizeof(MI_RGN_ChnPortParam_t));

                if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
                {
                    stRgnChnPort.eModId    = E_MI_RGN_MODID_DIVP;
                    stRgnChnPort.s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpSclUpChnId;
                    stRgnChnPort.s32DevId  = 0;
                    stRgnChnPort.s32OutputPortId = 0;
                }
                else if(g_videoEncoderArray[s32VencChn]->m_bUseDivp) //vpe port2 case: vif->vpe->divp->venc
                {
                    stRgnChnPort.eModId    = E_MI_RGN_MODID_DIVP;
                    stRgnChnPort.s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpChnPort.u32ChnId;
                    stRgnChnPort.s32DevId  = 0;
                    stRgnChnPort.s32OutputPortId = 0;
                }
                else
                {
                    stRgnChnPort.eModId   = E_MI_RGN_MODID_VPE;
                    stRgnChnPort.s32DevId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32DevId;
                    stRgnChnPort.s32ChnId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32ChnId;
                    stRgnChnPort.s32OutputPortId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32PortId;
                }

                if(MI_SUCCESS != MI_RGN_GetDisplayAttr(g_au32OsdRectWidgetHandle[s32VencChn], &stRgnChnPort, &stRgnChnPortParam))
                {
                    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
                    printf("Get osd Rect Widget hand=%d Display attribute error!\n", g_au32OsdRectWidgetHandle[s32VencChn]);
                    return s32Ret;
                }

                if(TRUE == stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv)
                {
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = FALSE;
                }

                if(MI_SUCCESS != MI_RGN_SetDisplayAttr(g_au32OsdRectWidgetHandle[s32VencChn], &stRgnChnPort, &stRgnChnPortParam))
                {
                    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
                    printf("Set osd Rect Widget hand=%d color inverse error!\n", g_au32OsdRectWidgetHandle[s32VencChn]);
                    return s32Ret;
                }
            }
#endif

#if MIXER_OSD_TEXT_WIDGET_ENABLE
            if(((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdTextWidgetHandle[s32VencChn][s32Idx]))
            {
                g_au32OsdTextWidgetHandle[s32VencChn][s32Idx] = calcOsdHandle(s32VencChn, s32Idx, E_OSD_WIDGET_TYPE_TEXT);

                memset(&stRgnChnPortParam, 0x00, sizeof(MI_RGN_ChnPortParam_t));

                if((g_EnablePIP) && (g_videoEncoderArray[s32VencChn]->m_pipCfg))
                {
                    stRgnChnPort.eModId    = E_MI_RGN_MODID_DIVP;
                    stRgnChnPort.s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpSclUpChnId;
                    stRgnChnPort.s32DevId  = 0;
                    stRgnChnPort.s32OutputPortId = 0;
                }
                else if(g_videoEncoderArray[s32VencChn]->m_bUseDivp) //vpe port2 case: vif->vpe->divp->venc
                {
                    stRgnChnPort.eModId    = E_MI_RGN_MODID_DIVP;
                    stRgnChnPort.s32ChnId  = g_videoEncoderArray[s32VencChn]->m_DivpChnPort.u32ChnId;
                    stRgnChnPort.s32DevId  = 0;
                    stRgnChnPort.s32OutputPortId = 0;
                }
                else
                {
                    stRgnChnPort.eModId   = E_MI_RGN_MODID_VPE;
                    stRgnChnPort.s32DevId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32DevId;
                    stRgnChnPort.s32ChnId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32ChnId;
                    stRgnChnPort.s32OutputPortId = g_videoEncoderArray[s32VencChn]->m_VpeChnPort.u32PortId;
                }


                if(MI_SUCCESS != MI_RGN_GetDisplayAttr(g_au32OsdTextWidgetHandle[s32VencChn][s32Idx], &stRgnChnPort, &stRgnChnPortParam))
                {
                    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
                    printf("Get osd Text Widget hand=%d Display attribute error!\n", g_au32OsdTextWidgetHandle[s32VencChn][s32Idx]);
                    return s32Ret;
                }

                if(TRUE == stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv)
                {
                    stRgnChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = FALSE;
                }

                if(MI_SUCCESS != MI_RGN_SetDisplayAttr(g_au32OsdTextWidgetHandle[s32VencChn][s32Idx], &stRgnChnPort, &stRgnChnPortParam))
                {
                    pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
                    printf("Set osd Text Widget hand=%d color inverse error!\n", g_au32OsdTextWidgetHandle[s32VencChn][s32Idx]);
                    return s32Ret;
                }
            }
#endif
        }

        pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
    }

    g_bOsdColorInverse = FALSE;
    printf("Disable OSD color inverse!\n");

    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 testOsdFormatBitmap(MI_S32  format,MI_U32 *idex,MI_U32 startX, MI_U32 startY)
{
    MI_VENC_CHN s32VencChn = 0;
    //static MI_U32 OsdFormatBitmapCount = 0;
    MI_U8 *picFileBuffer = NULL;
    MI_S32 picFileSize = 0;
    MI_RGN_Bitmap_t stBitmap;
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_RGN_ChnPortParam_t stChnPortParam;
    I2ToI8WidgetAttr_t pstWidgetAttr;
    FILE *pfile = NULL;
    MI_U8 align = 0;
    MI_S32 s32Ret = -1;
    static MI_U8 i2_num = 0;
    static MI_U8 i4_num = 0;
    static MI_U8 i8_num = 0;
    static MI_U8 bitMap_num = 0;
    static BOOL flag = true;
    static MI_S32 lastFormat;
    MI_U8 u8Idx = 0;
    MI_RGN_Attr_t stRegion;
    MI_S32 rgnAttrRet = MI_RGN_OK;
    MI_RGN_HANDLE hHandle = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
    osdInfo_t osdInfo;
    osdSizePoint_t osdSizePoint;
    if(flag)
    {
      lastFormat = format;
      flag = FALSE;
    }
	if(bitMap_num >= MAX_RGN_NUMBER_PER_CHN)
    {
      MIXER_DBG("You create osd num is  more than %d, will is error!\n",MAX_RGN_NUMBER_PER_CHN);
      return -1; 
    }
    memset(&osdSizePoint,0,sizeof(osdSizePoint_t));
    memset(&osdInfo,0,sizeof(osdInfo_t));

    memset(&stRgnAttr, 0x00, sizeof(MI_RGN_Attr_t));
    memset(&stRegion, 0x00, sizeof(MI_RGN_Attr_t));
    stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
    if(0 == format)
    {
      pfile = fopen("nohead.bmp", "rb");
      stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_ARGB1555;
      stRgnAttr.stOsdInitParam.stSize.u32Width  = 514; //g_videoEncoderArray[i]->m_width;
      stRgnAttr.stOsdInitParam.stSize.u32Height = 66;  //g_videoEncoderArray[i]->m_height;
    }
    else if(1 == format) //i2
    {
      align = 4;
      pfile = fopen("200X200.i2", "rb");
      stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_I2;
      stRgnAttr.stOsdInitParam.stSize.u32Width  = 200;
      stRgnAttr.stOsdInitParam.stSize.u32Height = 200;
    }
    else if(2 == format)//i4
    {
      align = 2;
      pfile = fopen("200X200.i4", "rb");
      stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_I4;
      stRgnAttr.stOsdInitParam.stSize.u32Width  = 200;
      stRgnAttr.stOsdInitParam.stSize.u32Height = 200;
    }
    else if(3 == format)//i8
    {
      align = 1;
      pfile = fopen("200X200.i8", "rb");
      stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_I8;
      stRgnAttr.stOsdInitParam.stSize.u32Width  = 200;
      stRgnAttr.stOsdInitParam.stSize.u32Height = 200;
    }
    if(pfile)
    {
        fseek(pfile, 0, SEEK_END);
        picFileSize = ftell(pfile);
        fseek(pfile, 0, SEEK_SET);
        picFileBuffer = (unsigned char *)malloc(picFileSize);
        if(NULL == picFileBuffer)
        {
          fclose(pfile);
        pfile = NULL;
          return (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
        }
        fread(picFileBuffer, 1, picFileSize, pfile);
        fclose(pfile);
        pfile = NULL;
    }
    else
    {
        printf("fopen pfile is error!\n");
        return (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
    }
    printf("fopen pfile is ok!\n");
    for(s32VencChn = 0; (TRUE == GetOsdInitFullState())&&(s32VencChn < g_s32VideoStreamNum) && (NULL != g_videoEncoderArray[s32VencChn]); s32VencChn++)
    {
        if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdRectWidgetHandle[s32VencChn])
        {
          rgnAttrRet = MI_RGN_GetAttr(g_au32OsdRectWidgetHandle[s32VencChn], &stRegion);
          if(MI_RGN_OK  == rgnAttrRet)
          {
             break;
          }
        }
        for(u8Idx = 0; u8Idx < MAX_RGN_NUMBER_PER_CHN; u8Idx++)
        {
          if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdTextWidgetHandle[s32VencChn][u8Idx])
          {
            rgnAttrRet = MI_RGN_GetAttr(g_au32OsdTextWidgetHandle[s32VencChn][u8Idx], &stRegion);
            if(MI_RGN_OK  == rgnAttrRet)
            {
              break;
            }
          }
          if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI2Widget[s32VencChn][u8Idx])
          {
                rgnAttrRet = MI_RGN_GetAttr(g_au32OsdI2Widget[s32VencChn][u8Idx], &stRegion);
                if(MI_RGN_OK  == rgnAttrRet)
                {
                   break;
                }
          }
          if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32OsdI4Widget[s32VencChn][u8Idx])
          {
                rgnAttrRet = MI_RGN_GetAttr(g_au32OsdI4Widget[s32VencChn][u8Idx], &stRegion);
                if(MI_RGN_OK  == rgnAttrRet)
                {
                   break;
                }
          }
          if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL != g_au32BitmapWidget[s32VencChn][u8Idx])
          {
            rgnAttrRet = MI_RGN_GetAttr(g_au32BitmapWidget[s32VencChn][u8Idx], &stRegion);
            if(MI_RGN_OK  == rgnAttrRet)
            {
               break;
            }
          }
        }
    }
    if((MI_RGN_OK  == rgnAttrRet)&&(TRUE == GetOsdInitFullState()))
    {
      if(stRgnAttr.stOsdInitParam.ePixelFmt != stRegion.stOsdInitParam.ePixelFmt)
      {
        uninitOsdFull();    //one port can not bind two format RGN.
      }
    }

/*    if(FALSE == GetOsdInitFullState())
    {
      s32Ret = MI_RGN_Init(&g_stPaletteTable);
      if(MI_RGN_OK != s32Ret)
      {
        MIXER_ERR("MI_RGN_Init error(%X)\n", s32Ret);
        SetOsdInitFullState(FALSE);
        return s32Ret;
      }
       SetOsdInitFullState(TRUE);
  }*/
    for(s32VencChn = 0; s32VencChn < g_s32VideoStreamNum; s32VencChn++)
    {
#if TARGET_CHIP_I6B0
        if(checkVencInitOsdFull(s32VencChn,NULL)==TRUE)continue;
#endif
        if(format != lastFormat)
        {
           if(0 == lastFormat)
           {
             for(MI_U8 i=0; i < bitMap_num; i++)
              {
                g_au32BitmapWidget[s32VencChn][i] = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
              }
              bitMap_num = 0;
           }
           else if(1 == lastFormat)
           {
              for(MI_U8 i=0; i < i2_num; i++)
              {
                g_au32OsdI2Widget[s32VencChn][i] = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
              }
              i2_num = 0;
           }
           else if(2 == lastFormat)
           {
             for(MI_U8 i=0; i < i4_num; i++)
             {
               g_au32OsdI4Widget[s32VencChn][i] = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
             }
              i4_num = 0;
           }
           else if(3 == lastFormat)
           {
             for(MI_U8 i=0; i < i8_num; i++)
             {
               g_au32OsdI8Widget[s32VencChn][i] = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
             }
             i8_num = 0;
           }
        }
        if(0 == format)    //bitmap
        {
            if(format != lastFormat)
            {
              bitMap_num = 0;
              *idex = 0;
            }
            bitMap_num = *idex;
            if(bitMap_num >= MAX_RGN_NUMBER_PER_CHN)
            {
              MIXER_DBG("You create osd num is  more than %d, will is error!",MAX_RGN_NUMBER_PER_CHN);
              s32Ret = -1;
              break;
            }
            g_au32BitmapWidget[s32VencChn][bitMap_num] = calcOsdHandle(s32VencChn,bitMap_num, E_OSD_WIDGET_TYPE_BITMAP);
            hHandle = g_au32BitmapWidget[s32VencChn][bitMap_num];
            if(((MI_S32)hHandle > MI_RGN_HANDLE_NULL )&&(hHandle < MI_RGN_MAX_HANDLE))
            {
                configOsdRgnChnPort(s32VencChn, &stRgnChnPort);
                memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
                stChnPortParam.bShow = TRUE;
                stChnPortParam.stPoint.u32X = startX;
                stChnPortParam.stPoint.u32Y = startY;
                stChnPortParam.unPara.stOsdChnPort.u32Layer = g_au32BitmapWidget[s32VencChn][bitMap_num];
                stChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = FALSE;
                stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode = E_MI_RGN_PIXEL_ALPHA;
                stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8BgAlpha = 0x0;
                stChnPortParam.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8FgAlpha = 0xff;

                s32Ret = createOsdRectWidget(g_au32BitmapWidget[s32VencChn][bitMap_num], &stRgnAttr, &stRgnChnPort, &stChnPortParam);
                memset(&stBitmap, 0x00, sizeof(MI_RGN_Bitmap_t));
                stBitmap.stSize.u32Width  = 514; //the width of nohead.bmp
                stBitmap.stSize.u32Height = 66;  //the height of nohead.bmp

                stBitmap.ePixelFormat = E_MI_RGN_PIXEL_FORMAT_ARGB1555;
                stBitmap.pData = picFileBuffer;

                if(MI_SUCCESS != MI_RGN_SetBitMap(g_au32BitmapWidget[s32VencChn][bitMap_num], &stBitmap))
                {
                    printf("MI_RGN_SetBitMap() fail\n");
                }
                else
                {
                  memset(&osdSizePoint,0,sizeof(osdSizePoint_t));
                  memset(&osdInfo,0,sizeof(osdInfo_t));
                  osdSizePoint.size.u16Height = stBitmap.stSize.u32Width;
                  osdSizePoint.size.u16Width = stBitmap.stSize.u32Height;
                  osdSizePoint.point.u16X = startX;
                     osdSizePoint.point.u16Y = startY;
                }
            }
            else
            {
               MIXER_DBG("osd handle[%d] is out of range[0-%d] \n",hHandle,MI_RGN_MAX_HANDLE);
            }
        }
        else if(1 == format || 2 == format || 3 == format)//I2.i4.i8
        {
            memset(&stChnPortParam, 0, sizeof(MI_RGN_ChnPortParam_t));
            if(1 == format)
            {
                if(format != lastFormat)
                {
                  i2_num = 0;
                  *idex = 0;
                }
                i2_num =  *idex;
              if(i2_num >= MAX_RGN_NUMBER_PER_CHN)
              {
                MIXER_DBG("You create osd num is  more than %d, will is error!",MAX_RGN_NUMBER_PER_CHN);
                s32Ret = -1;
                break;
              }
              stChnPortParam.stPoint.u32X = ALIGN_4xUP(startX);
              stChnPortParam.stPoint.u32Y = ALIGN_4xUP(startY);
              g_au32OsdI2Widget[s32VencChn][i2_num] = calcOsdHandle(s32VencChn, i2_num, E_OSD_WIDGET_TYPE_BITMAP);
              hHandle = g_au32OsdI2Widget[s32VencChn][i2_num];
               }
            else if(2 == format)
            {
                if(format != lastFormat)
                {
                  i4_num = 0;
                  *idex = 0;
                }
                i4_num = *idex;
               if(i4_num >= MAX_RGN_NUMBER_PER_CHN)
               {
                 printf("You create osd num is  more than %d, will is error!",MAX_RGN_NUMBER_PER_CHN);
                 s32Ret = -1;
                 break;
               }
              stChnPortParam.stPoint.u32X = ALIGN_2xUP(startX);
              stChnPortParam.stPoint.u32Y = ALIGN_2xUP(startY);
              g_au32OsdI4Widget[s32VencChn][i4_num] = calcOsdHandle(s32VencChn, i4_num, E_OSD_WIDGET_TYPE_BITMAP);
              hHandle = g_au32OsdI4Widget[s32VencChn][i4_num];
            }
            else if(3 == format)
            {
               if(format != lastFormat)
                {
                  i8_num = 0;
                  *idex = 0;
                }
                i8_num = *idex;
              if(i8_num >= MAX_RGN_NUMBER_PER_CHN)
              {
                printf("You create osd num is  more than %d, will is error!",MAX_RGN_NUMBER_PER_CHN);
                s32Ret = -1;
                break;
              }
              g_au32OsdI8Widget[s32VencChn][i8_num] = calcOsdHandle(s32VencChn, i8_num, E_OSD_WIDGET_TYPE_BITMAP);
              hHandle = g_au32OsdI8Widget[s32VencChn][i8_num];
            }
            if(((MI_S32)hHandle > MI_RGN_HANDLE_NULL)&&(hHandle < MI_RGN_MAX_HANDLE))
            {
                configOsdRgnChnPort(s32VencChn, &stRgnChnPort);

                stChnPortParam.bShow = TRUE;
                stChnPortParam.unPara.stOsdChnPort.u32Layer = hHandle;
                stChnPortParam.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = FALSE;
                s32Ret = createOsdRectWidget(hHandle, &stRgnAttr, &stRgnChnPort, &stChnPortParam);
                memset(&pstWidgetAttr , 0 ,sizeof(I2ToI8WidgetAttr_t));
                pstWidgetAttr.buf=picFileBuffer;
                pstWidgetAttr.buf_length = picFileSize;
                pstWidgetAttr.size.u16Width  = 200;
                pstWidgetAttr.size.u16Height = 200;
                pstWidgetAttr.pPoint.x = 0;
                pstWidgetAttr.pPoint.y = 0;
				pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);
                MI_S32 s32Ret= updateOsdTextWidgetI2_To_I8(hHandle, &pstWidgetAttr, TRUE,align);
				pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
                if(MI_RGN_OK == s32Ret)
                {
                  memset(&osdSizePoint,0,sizeof(osdSizePoint_t));
                  memset(&osdInfo,0,sizeof(osdInfo_t));
                  osdSizePoint.size.u16Height = stBitmap.stSize.u32Width;
                  osdSizePoint.size.u16Width = stBitmap.stSize.u32Height;
                  osdSizePoint.point.u16X = stChnPortParam.stPoint.u32X;
                  osdSizePoint.point.u16Y =  stChnPortParam.stPoint.u32Y;
                }
                else
                {
                   MIXER_DBG("buf_length = %d hHandle=%d [%d][%d] is err 0x%x!\n",pstWidgetAttr.buf_length,hHandle,s32VencChn,*idex,s32Ret);
                }
           }
           else
           {
               MIXER_DBG("osd handle[%d] is out of range[0-%d] \n",hHandle,MI_RGN_MAX_HANDLE);
           }
        }
        if(MI_RGN_OK == s32Ret)
        {
            osdInfo.osdIndex =  *idex;
            osdInfo.u32RgnHandle = hHandle;
            osdInfo.channel = s32VencChn;
            osdInfo.osdType = E_OSD_WIDGET_TYPE_BITMAP;
            osdInfo.format =  stRgnAttr.stOsdInitParam.ePixelFmt;
            midManageOsdInfoLinkList(&osdInfo,TRUE,&osdSizePoint,NULL);
        }
    }
    if(MI_RGN_OK == s32Ret)
    {
        if(0 == format)
        {
           bitMap_num++;
        }
        else if(1 == format)
        {
          i2_num++;
        }
        else if(2 == format)
        {
          i4_num++;
        }
        else if(3 == format)
        {
          i8_num++;
        }
        lastFormat = format;
    }
    hHandle = (MI_RGN_HANDLE)MI_RGN_HANDLE_NULL;
    //if(NULL != pfile)
    {
    //    fclose(pfile);
        if(NULL != picFileBuffer)
            free(picFileBuffer);
    }
    return s32Ret;
}

static struct timeval lastTm;
inline void updateOsdTimestampInfomation(MI_RGN_HANDLE u32OsdHandle, MI_BOOL& bVisible)
{
    time_t t;
    struct tm *local;
    MI_S32 s32Idx = 0;
    MI_VENC_CHN s32VencChn = 0;
    MI_U8 u8OsdTimeString[128]={0x0};
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_S32 diffTime;
    struct timeval      tv;
    struct timezone     tz;


    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle)
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        return;
    }

    s32Ret = getVideoChnByOsdHandle(u32OsdHandle, &s32VencChn, &s32Idx);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoChnByOsdHandle(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    t = time(NULL);
    local = localtime(&t);
    sprintf((char *)u8OsdTimeString, "%d-%02d-%02d %02d:%02d:%02d", local->tm_year + 1900, \
                                                        local->tm_mon + 1, \
                                                        local->tm_mday,\
                                                        local->tm_hour, \
                                                        local->tm_min, \
                                                        local->tm_sec);
    gettimeofday(&tv, &tz);
    diffTime = (tv.tv_sec * 1000 + tv.tv_usec / 1000) - (lastTm.tv_sec * 1000 + lastTm.tv_usec / 1000);
    if(g_s32OsdFlicker && (diffTime > 500 || diffTime < -500))
    {
        bVisible = (BOOL)!bVisible;
        lastTm = tv;
    }

    memset(&stCanvasInfo, 0x00, sizeof(MI_RGN_CanvasInfo_t));

    s32Ret = MI_RGN_GetCanvasInfo(u32OsdHandle, &stCanvasInfo);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetCanvasInfo(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    cleanOsdTextWidget(MI_RGN_HANDLE_NULL, &stCanvasInfo, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

   // if(bVisible)
    {
        Point_t stPoint;
        TextWidgetAttr_t stTextWidgetAttr;

        stPoint.x = MIXER_OSD_TEXT_WIDGET_START_X;
        stPoint.y = MIXER_OSD_TEXT_WIDGET_START_Y/* + MIXER_OSD_TEXT_WIDGET_SPACE_LARGE_FONT * 0*/;

        memset(&stTextWidgetAttr, 0x00, sizeof(TextWidgetAttr_t));

        if(!bVisible)
            memset(u8OsdTimeString, 0x00, 128);

        stTextWidgetAttr.string  = (char *)u8OsdTimeString;
        stTextWidgetAttr.pPoint  = &stPoint;
        stTextWidgetAttr.size    = g_stMixerOsdWidgetOrder.size[s32VencChn];
        stTextWidgetAttr.pmt     = g_stMixerOsdWidgetOrder.pmt;
        stTextWidgetAttr.u32Color= 1; //0:transparent; 1:red; 2:green; 3:blue;
        stTextWidgetAttr.pfColor = &g_stRedColor;
        stTextWidgetAttr.pbColor = &g_stBlackColor;
        stTextWidgetAttr.space = 0;
        //stTextWidgetAttr.bHard = FALSE;
        //stTextWidgetAttr.bRle  = FALSE;
        stTextWidgetAttr.bOutline = FALSE;
        //MIXER_ERR("The input Osd handle=%d, VenChn=%d, Idx=%d\n", u32OsdHandle, s32VencChn, s32Idx);

        s32Ret = setOsdDisplayLocation(s32VencChn, 0, &stTextWidgetAttr.size, &stPoint);
        if(MI_SUCCESS != s32Ret)
        {
            MIXER_ERR("setOsdDisplayLocation error(0x%X)\n", s32Ret);
            return;
        }

        updateOsdTextWidget(MI_RGN_HANDLE_NULL, &stCanvasInfo, &stTextWidgetAttr, FALSE);
    }

    s32Ret = MI_RGN_UpdateCanvas(u32OsdHandle);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_UpdateCanvas error(0x%X)\n", s32Ret);
        return;
    }

    return;
}

inline void updateOsdVideoInfomation(MI_RGN_HANDLE u32OsdHandle)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;
    //MI_SYS_WindowRect_t stRect;
    //Point_t stPoint;
    //MI_U8 u8OsdTestBuf[64];
    //Point_t stOsdPoint;


    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle)
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        return;
    }

    s32Ret = getVideoChnByOsdHandle(u32OsdHandle, &s32VencChn, &s32Idx);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoChnByOsdHandle(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }


    memset(&stCanvasInfo, 0x00, sizeof(MI_RGN_CanvasInfo_t));
    s32Ret = MI_RGN_GetCanvasInfo(u32OsdHandle, &stCanvasInfo);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetCanvasInfo(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }
    
    cleanOsdTextWidget(MI_RGN_HANDLE_NULL, &stCanvasInfo, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

    pthread_mutex_lock(&g_stVideoInfoMutex);
    displayOsdVideoInfomation(s32VencChn, u32OsdHandle, &stCanvasInfo);
    pthread_mutex_unlock(&g_stVideoInfoMutex);

    s32Ret = MI_RGN_UpdateCanvas(u32OsdHandle);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_UpdateCanvas error(0x%X)\n", s32Ret);
        return;
    }

    return;
}

inline void updateOsdIspInfomation(MI_RGN_HANDLE u32OsdHandle)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;


    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle)
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        return;
    }

    s32Ret = getVideoChnByOsdHandle(u32OsdHandle, &s32VencChn, &s32Idx);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoChnByOsdHandle(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    memset(&stCanvasInfo, 0x00, sizeof(MI_RGN_CanvasInfo_t));

    s32Ret = MI_RGN_GetCanvasInfo(u32OsdHandle, &stCanvasInfo);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetCanvasInfo(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    cleanOsdTextWidget(MI_RGN_HANDLE_NULL, &stCanvasInfo, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

    displayOsdIspInfomation(s32VencChn, u32OsdHandle, &stCanvasInfo);

    s32Ret = MI_RGN_UpdateCanvas(u32OsdHandle);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_UpdateCanvas error(0x%X)\n", s32Ret);
        return;
    }

    return;
}

inline void updateOsdAudioInfomation(MI_RGN_HANDLE u32OsdHandle)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;


    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle)
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        return;
    }

    s32Ret = getVideoChnByOsdHandle(u32OsdHandle, &s32VencChn, &s32Idx);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoChnByOsdHandle(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    memset(&stCanvasInfo, 0x00, sizeof(MI_RGN_CanvasInfo_t));

    s32Ret = MI_RGN_GetCanvasInfo(u32OsdHandle, &stCanvasInfo);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetCanvasInfo(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    cleanOsdTextWidget(MI_RGN_HANDLE_NULL, &stCanvasInfo, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

    displayOsdAudioInfomation(s32VencChn, u32OsdHandle, &stCanvasInfo);

    s32Ret = MI_RGN_UpdateCanvas(u32OsdHandle);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_UpdateCanvas error(0x%X)\n", s32Ret);
        return;
    }

    return;
}

inline void updateOsdUser0Infomation(MI_RGN_HANDLE u32OsdHandle)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    //TextWidgetAttr_t stTextWidgetAttr;
    //MI_U8 u8OsdTestBuf[64];
    //Point_t stOsdPoint;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;


    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle)
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        return;
    }

    s32Ret = getVideoChnByOsdHandle(u32OsdHandle, &s32VencChn, &s32Idx);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoChnByOsdHandle(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    memset(&stCanvasInfo, 0x00, sizeof(MI_RGN_CanvasInfo_t));

    s32Ret = MI_RGN_GetCanvasInfo(u32OsdHandle, &stCanvasInfo);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetCanvasInfo(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    cleanOsdTextWidget(MI_RGN_HANDLE_NULL, &stCanvasInfo, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

    displayOsdUser0Infomation(s32VencChn, u32OsdHandle, &stCanvasInfo);

    s32Ret = MI_RGN_UpdateCanvas(u32OsdHandle);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_UpdateCanvas error(0x%X)\n", s32Ret);
        return;
    }

    return;
}

inline void updateOsdUser1Infomation(MI_RGN_HANDLE u32OsdHandle)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_VENC_CHN s32VencChn = 0;
    //MI_S32 s32Idx = 0;


    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle)
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        return;
    }

    memset(&stCanvasInfo, 0x00, sizeof(MI_RGN_CanvasInfo_t));

    s32Ret = MI_RGN_GetCanvasInfo(u32OsdHandle, &stCanvasInfo);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetCanvasInfo(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    cleanOsdTextWidget(MI_RGN_HANDLE_NULL, &stCanvasInfo, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

    displayOsdUser1Infomation(s32VencChn, u32OsdHandle, &stCanvasInfo);

    s32Ret = MI_RGN_UpdateCanvas(u32OsdHandle);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_UpdateCanvas error(0x%X)\n", s32Ret);
        return;
    }

    return;
}

inline void updateOsdUser2Infomation(MI_RGN_HANDLE u32OsdHandle)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;


    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle)
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        return;
    }

    s32Ret = getVideoChnByOsdHandle(u32OsdHandle, &s32VencChn, &s32Idx);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoChnByOsdHandle(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    memset(&stCanvasInfo, 0x00, sizeof(MI_RGN_CanvasInfo_t));

    s32Ret = MI_RGN_GetCanvasInfo(u32OsdHandle, &stCanvasInfo);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetCanvasInfo(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    cleanOsdTextWidget(MI_RGN_HANDLE_NULL, &stCanvasInfo, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

    //displayOsdUser1Infomation(s32VencChn, u32OsdHandle, &stCanvasInfo);

    s32Ret = MI_RGN_UpdateCanvas(u32OsdHandle);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_UpdateCanvas error(0x%X)\n", s32Ret);
        return;
    }

    return;
}

inline void updateOsdUser3Infomation(MI_RGN_HANDLE u32OsdHandle)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;


    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle)
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        return;
    }

    s32Ret = getVideoChnByOsdHandle(u32OsdHandle, &s32VencChn, &s32Idx);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoChnByOsdHandle(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    memset(&stCanvasInfo, 0x00, sizeof(MI_RGN_CanvasInfo_t));

    s32Ret = MI_RGN_GetCanvasInfo(u32OsdHandle, &stCanvasInfo);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetCanvasInfo(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return;
    }

    cleanOsdTextWidget(MI_RGN_HANDLE_NULL, &stCanvasInfo, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

    s32Ret = MI_RGN_UpdateCanvas(u32OsdHandle);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_UpdateCanvas error(0x%X)\n", s32Ret);
        return;
    }

    return;
}

MI_S32 updateOsdInfomation(MI_RGN_HANDLE u32OsdHandle, const MI_S8 *ps8String,osdSizePoint_t *osdSizePoint)
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_VENC_CHN s32VencChn = 0;
    MI_S32 s32Idx = 0;


    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle)
    {
        MIXER_ERR("The input Osd handle(%d) is out of range!\n", u32OsdHandle);
        return s32Ret;
    }

    s32Ret = getVideoChnByOsdHandle(u32OsdHandle, &s32VencChn, &s32Idx);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("getVideoChnByOsdHandle(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return s32Ret;
    }

    memset(&stCanvasInfo, 0x00, sizeof(MI_RGN_CanvasInfo_t));

    s32Ret = MI_RGN_GetCanvasInfo(u32OsdHandle, &stCanvasInfo);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_GetCanvasInfo(hdl=%d) error(0x%X)\n", u32OsdHandle, s32Ret);
        return s32Ret;
    }

    cleanOsdTextWidget(MI_RGN_HANDLE_NULL, &stCanvasInfo, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

    displayOsdUserInfo(s32VencChn, u32OsdHandle, &stCanvasInfo, (char*)ps8String);

    s32Ret = MI_RGN_UpdateCanvas(u32OsdHandle);
    if(MI_RGN_OK != s32Ret)
    {
        MIXER_ERR("MI_RGN_UpdateCanvas error(0x%X)\n", s32Ret);
        return s32Ret;
    }
    printf("updateOsdInfomation is ok new_osdString[%s] format[%d]\n",ps8String,stCanvasInfo.ePixelFmt);
    return s32Ret;
}


static int UpdateDLARect()
{
#if TARGET_CHIP_I6E
    MI_S32 s32VencChn = 0x0;
    MI_RGN_HANDLE hHandle = g_au32OsdRectWidgetHandle[s32VencChn];
    RectWidgetAttr_t stRectWidgetAttr;
    ST_Point_T stPoint;
    MI_RGN_CanvasInfo_t stRgnCanvasInfo;
    ST_DlaRectInfo_T pstIpuRectInfo;
    stRectList *tRectList = NULL;
    static MI_BOOL mBeCurRect = FALSE;

    MI_BOOL beFine = FALSE;
    int i;
    MI_U8 clolor = 2;


    //clean rect
    if(FALSE == mBeCurRect)
    {
        //no osd needed to be show.  and no need to clean osd.
        if(TRUE == list_empty(&g_WorkRectList))
        {
           // MIXER_DBG("param is error,list is empty or handle is out of range[0-%d] handle=%d\n",hHandle,MI_RGN_MAX_HANDLE);
             return 0;
        }
    }

    if(((MI_S32)hHandle <= MI_RGN_HANDLE_NULL  || hHandle >= MI_RGN_MAX_HANDLE))
    {

		MIXER_DBG(" mBeCurRect %d.	beFine=%d  hHandle=%d\n", mBeCurRect,beFine,hHandle);
        return 0;
    }

    mBeCurRect = FALSE;
    pthread_mutex_lock(&g_stMutexMixerOsdTextRgnBuf);
    if(MI_RGN_OK != MI_RGN_GetCanvasInfo(hHandle, &stRgnCanvasInfo))
    {
        printf("%s:%d call MI_RGN_GetCanvasInfo() fail.. handle:%d\n", __func__,__LINE__, hHandle);
        pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
        return 0;
    }

    cleanOsdTextWidgettmp(MI_RGN_HANDLE_NULL, &stRgnCanvasInfo, NULL, MIXER_RGN_PALETTEL_TABLE_ALPHA_INDEX);

    memset(&g_stDLARect[0], 0x00, sizeof(MI_SYS_WindowRect_t) * MAX_DLA_RECT_NUMBER);
    //end



    tRectList = list_entry(g_WorkRectList.next, stRectList, rectlist);
    //MIXER_DBG("tRectList: tRectList->tCount= %d.  beFine=%d\n", tRectList->tCount,beFine);
    if(NULL != tRectList && 0x0 != tRectList->tCount && NULL != tRectList->pChar)
    {
        list_del(&tRectList->rectlist);
        beFine = TRUE;
    }

    if(TRUE == beFine)
    {
        MI_S32 dla_detect_cnt = 0;
        if((0 >= g_s32OsdRectHandleCnt) || (TRUE == g_bOsdSetResulution[s32VencChn]))
        {
            MIXER_ERR("param err.  osdhandlecnt:%d,   %d\n", g_s32OsdRectHandleCnt, g_bOsdSetResulution[s32VencChn]);
                goto exit;
        }

        // osd name
        for(i = 0;  (i < tRectList->tCount) && (i < MAX_DLA_RECT_NUMBER); i++)
        {
            /*show dla coordinates*/
            memset(&pstIpuRectInfo,0x00,sizeof(ST_DlaRectInfo_T));
            memcpy(&pstIpuRectInfo,tRectList->pChar+i*sizeof(ST_DlaRectInfo_T),sizeof(ST_DlaRectInfo_T));

            g_stDLARect[dla_detect_cnt].u16X = (getVideoWidth(s32VencChn)* pstIpuRectInfo.rect.u32X / 1920) & 0xFFFE;
            g_stDLARect[dla_detect_cnt].u16Y = (getVideoHeight(s32VencChn) * pstIpuRectInfo.rect.u32Y / 1080) & 0xFFFE;
            g_stDLARect[dla_detect_cnt].u16Width = (getVideoWidth(s32VencChn) * pstIpuRectInfo.rect.u16PicW / 1920) & 0xFFFE;
            g_stDLARect[dla_detect_cnt].u16Height = (getVideoHeight(s32VencChn) * pstIpuRectInfo.rect.u16PicH / 1080) & 0xFFFE;
            stPoint.u32X = g_stDLARect[dla_detect_cnt].u16X;
            stPoint.u32Y = g_stDLARect[dla_detect_cnt].u16Y;
            if(g_stVideoSize[s32VencChn].u16Width < g_stDLARect[dla_detect_cnt].u16Width)
            {
              g_stDLARect[dla_detect_cnt].u16Width = g_stVideoSize[s32VencChn].u16Width;
            }
            if(g_stVideoSize[s32VencChn].u16Height < g_stDLARect[dla_detect_cnt].u16Height)
            {
                g_stDLARect[dla_detect_cnt].u16Height = g_stVideoSize[s32VencChn].u16Height;
            }
            if(g_stVideoSize[s32VencChn].u16Width < (stPoint.u32X + g_stDLARect[dla_detect_cnt].u16Width))
            {
                stPoint.u32X = g_stVideoSize[s32VencChn].u16Width - g_stDLARect[dla_detect_cnt].u16Width;
                g_stDLARect[dla_detect_cnt].u16X = stPoint.u32X;
            }
            if(g_stVideoSize[s32VencChn].u16Height < (stPoint.u32Y + g_stDLARect[dla_detect_cnt].u16Height))
            {
               stPoint.u32Y = g_stVideoSize[s32VencChn].u16Height - g_stDLARect[dla_detect_cnt].u16Height;
               g_stDLARect[dla_detect_cnt].u16Y = stPoint.u32Y;
            }
		    if(0 == strncmp(pstIpuRectInfo.szObjName,"person",6))
		     {
		         clolor = 2;
				 dla_detect_cnt++;
		         drawOsdText2Canvas(&stRgnCanvasInfo, stPoint, (MI_S8 *)pstIpuRectInfo.szObjName,clolor);
		     }
			 else if(0 == strncmp(pstIpuRectInfo.szObjName,"motorbike",9))
		      {
		         clolor = 1;
				 dla_detect_cnt++;
		         drawOsdText2Canvas(&stRgnCanvasInfo, stPoint, (MI_S8 *)pstIpuRectInfo.szObjName,clolor);
		      }
			 else if(0 == strncmp(pstIpuRectInfo.szObjName,"bicycle",7))
		      {
		         clolor = 0;
				 dla_detect_cnt++;
		         drawOsdText2Canvas(&stRgnCanvasInfo, stPoint, (MI_S8 *)pstIpuRectInfo.szObjName,clolor);
		      }
			 else if(0 == strncmp(pstIpuRectInfo.szObjName,"car",3))
		     {
		         clolor = 3;
				 dla_detect_cnt++;
		         drawOsdText2Canvas(&stRgnCanvasInfo, stPoint, (MI_S8 *)pstIpuRectInfo.szObjName,clolor);
		     }
			 else if(0 == strncmp(pstIpuRectInfo.szObjName,"bus",3))
		      {
		         clolor = 3;
				 dla_detect_cnt++;
		         drawOsdText2Canvas(&stRgnCanvasInfo, stPoint, (MI_S8 *)pstIpuRectInfo.szObjName,clolor);
		      }
			 else if(0 == strncmp(pstIpuRectInfo.szObjName,"unknownID",9))
		      {
		         clolor = 3;
				 dla_detect_cnt++;
		         drawOsdText2Canvas(&stRgnCanvasInfo, stPoint, (MI_S8 *)pstIpuRectInfo.szObjName,clolor);
		      }
		     else //fdfr
		     {
		         clolor = 1;
				 dla_detect_cnt++;
		         drawOsdText2Canvas(&stRgnCanvasInfo, stPoint, (MI_S8 *)pstIpuRectInfo.szObjName,clolor);
		     }
			 if(0x0 != dla_detect_cnt)
			 {
				stRectWidgetAttr.pstRect = &g_stDLARect[0];
				stRectWidgetAttr.s32RectCnt = 1;
				dla_detect_cnt = 0;
				stRectWidgetAttr.u8BorderWidth = 4;
				stRectWidgetAttr.pmt = g_stMixerOsdWidgetOrder.pmt;
				stRectWidgetAttr.pfColor = &g_stRedColor;
				stRectWidgetAttr.pbColor = &g_stBlackColor;
				stRectWidgetAttr.bFill = FALSE;
				stRectWidgetAttr.u32Color = clolor;
				updateOsdRectWidgetTmp(&stRgnCanvasInfo, &stRectWidgetAttr);
			 }
        }
        mBeCurRect = TRUE;
        //pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);

        tRectList->tCount = 0x0;
        free(tRectList->pChar);
        tRectList->pChar = NULL;
        list_add_tail(&tRectList->rectlist, &g_EmptyRectList);

    }

exit:

    if(MI_RGN_UpdateCanvas(hHandle) != MI_RGN_OK)
    {
        MIXER_ERR("MI_RGN_UpdateCanvas fail\n");
    }
    pthread_mutex_unlock(&g_stMutexMixerOsdTextRgnBuf);
#endif
    return 0;
}

void* OSD_Task(void *argu)
{
 //   struct timeval stTimeVal;
 //   MI_U32 u32TimeVal = 0;
 //   MI_U32 u32TimeValOld = 0;
    MI_U32 tick = 0x0;
    MI_BOOL bVisible = TRUE;
    MI_S32  s32Idx = 0;
    MI_VENC_CHN s32VencChn = 0;
    MI_RGN_HANDLE u32OsdHandle = MI_RGN_HANDLE_NULL;
	struct timeval now;
	struct timespec outtime;


    g_bThreadOsdExit = FALSE;

    printf("OSD_Task +\n");


    if(gDebug_OsdTest)
    {
        for(s32VencChn = 0; s32VencChn < g_s32VideoStreamNum; s32VencChn++)
        {
           // _osd_apitest_init(s32VencChn);
        }
    }


    while(FALSE == g_bExit)
    {
#if 0
        gettimeofday(&stTimeVal, NULL);

        u32TimeVal = stTimeVal.tv_sec*1000 + stTimeVal.tv_usec/1000;
        if(0 == u32TimeValOld)
        {
            u32TimeValOld  = u32TimeVal;
        }

        usleep(1000 * g_s32OsdRefreshIntervalMs - _abs(u32TimeVal - u32TimeValOld));
        u32TimeValOld  = u32TimeVal;


        for(s32VencChn = 0; (s32VencChn < g_s32VideoStreamNum) && (FALSE == g_bOsdSetResulution[s32VencChn]); s32VencChn++)
        {
            if(NULL == g_videoEncoderArray[s32VencChn])
            {
                continue;
            }
#if TARGET_CHIP_I6B0
            if(checkVencInitOsdFull(s32VencChn,NULL) == TRUE)
            {
                //MIXER_DBG("Because venc%d use the same RGN with venc%d,no need to do something......\n",s32VencChn,s32VencChn_tmp);
                continue ;
            }
#endif

#if MIXER_AED_ENABLE
            if(0 == ((u32TimeVal&0xFFFFF400) / 0x1400))
            {
                g_bBabyCry = TRUE;//FALSE;
                g_bLoudSound = TRUE;//FALSE;
                g_s16LoudSounddB = u32TimeVal & 0xFF;
            }

            if(0 == ((u32TimeVal&0xFFFFF400) / 0x1400))
            {
                g_bBabyCry = FALSE;
                g_bLoudSound = FALSE;
                g_s16LoudSounddB = u32TimeVal & 0xFF;
            }
#endif
            pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

            for(s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
            {
                u32OsdHandle = g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
                if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle || g_au32OsdTextWidgetHandleUseFlag[s32VencChn][s32Idx] == FALSE)
                {
                    g_au32OsdTextWidgetHandleUseFlag[s32VencChn][s32Idx] = FALSE;
                    continue;
                }
                switch(s32Idx)
                {
                    case 0 : if(g_bTimeStamp) updateOsdTimestampInfomation(u32OsdHandle, bVisible); break;
                    case 1 : if(g_bVideoInfo) updateOsdVideoInfomation(u32OsdHandle); break;
                    case 2 : if(g_bIspInfo)   updateOsdIspInfomation(u32OsdHandle); break;
                    case 3 : if(g_bAudioInfo) updateOsdAudioInfomation(u32OsdHandle); break;
                    case 4 : if(g_bUser0Info) updateOsdUser0Infomation(u32OsdHandle); break;
                    case 5 : if(g_bUser1Info) updateOsdUser1Infomation(u32OsdHandle); break;
                    case 6 : if(g_bUser2Info) updateOsdUser2Infomation(u32OsdHandle); break;
                    default: /*updateOsdUser3Infomation(u32OsdHandle);*/ break;
                }
            }

            pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
        }

        if(g_s32OsdFlicker)
        {
            bVisible = (BOOL)!bVisible;
        }
#else
      tick++;
      if((1 == g_dlaParam))
      {  
        if(dla_init)
        {
         pthread_mutex_lock(&g_mutex_UpadteOsdState);  
         if(0 == g_UpadteOsdState)
         {
		   gettimeofday(&now, NULL);
		   outtime.tv_sec = now.tv_sec + 2;
		   outtime.tv_nsec = now.tv_usec * 1000;		 
		   pthread_cond_timedwait(&g_cond_UpadteOsdState, &g_mutex_UpadteOsdState, &outtime);
          // pthread_cond_wait(&g_cond_UpadteOsdState, &g_mutex_UpadteOsdState);  
         }
		 g_UpadteOsdState--;
		 UpdateDLARect();
	     pthread_mutex_unlock(&g_mutex_UpadteOsdState); 
        }
      }
      else 
      {
        tick++; 
		MySystemDelay(10);  
		if(0x0 == tick%9)    // update osd every 1second
       	{	
       		tick = 0;
          //  MIXER_DBG("g_dlaParam=%d\n",g_dlaParam);
            for(s32VencChn = 0; (s32VencChn < g_s32VideoStreamNum) && (FALSE == g_bOsdSetResulution[s32VencChn]); s32VencChn++)
            {
                if(NULL == g_videoEncoderArray[s32VencChn])
                {
                    continue;
                }

#if TARGET_CHIP_I6B0
                if(checkVencInitOsdFull(s32VencChn,NULL) == TRUE)
                {
                    //MIXER_DBG("Because venc%d use the same RGN with venc%d,no need to do something......\n",s32VencChn,s32VencChn_tmp);
                    continue ;
                }
#endif

#if MIXER_AED_ENABLE
                if(0 == ((u32TimeVal&0xFFFFF400) / 0x1400))
                {
                    g_bBabyCry = TRUE;//FALSE;
                    g_bLoudSound = TRUE;//FALSE;
                    g_s16LoudSounddB = u32TimeVal & 0xFF;
                }

                if(0 == ((u32TimeVal&0xFFFFF400) / 0x1400))
                {
                    g_bBabyCry = FALSE;
                    g_bLoudSound = FALSE;
                    g_s16LoudSounddB = u32TimeVal & 0xFF;
                }
#endif
                pthread_mutex_lock(&g_stMutexMixerOsdRun[s32VencChn]);

                for(s32Idx = 0; s32Idx < MAX_RGN_NUMBER_PER_CHN; s32Idx++)
                {
                    u32OsdHandle = g_au32OsdTextWidgetHandle[s32VencChn][s32Idx];
                    if((MI_RGN_HANDLE)MI_RGN_HANDLE_NULL == u32OsdHandle || g_au32OsdTextWidgetHandleUseFlag[s32VencChn][s32Idx] == FALSE)
                    {
                        g_au32OsdTextWidgetHandleUseFlag[s32VencChn][s32Idx] = FALSE;
                        continue;
                    }
                    MIXER_INFO("s32VencChn:%d Index:%d. u32OsdHandle:%d. g_bTimeStamp:%d. g_bVideoInfo:%d\n", \
                            s32VencChn,s32Idx, u32OsdHandle, g_bTimeStamp, g_bVideoInfo);
                    switch(s32Idx)
                    {
                        case 0 : if(g_bTimeStamp) updateOsdTimestampInfomation(u32OsdHandle, bVisible); break;
                        case 1 : if(g_bVideoInfo) updateOsdVideoInfomation(u32OsdHandle); break;
                        case 2 : if(g_bIspInfo)   updateOsdIspInfomation(u32OsdHandle); break;
                        case 3 : if(g_bAudioInfo) updateOsdAudioInfomation(u32OsdHandle); break;
                        case 4 : if(g_bUser0Info) updateOsdUser0Infomation(u32OsdHandle); break;
                        case 5 : if(g_bUser1Info) updateOsdUser1Infomation(u32OsdHandle); break;
                        case 6 : if(g_bUser2Info) updateOsdUser2Infomation(u32OsdHandle); break;
                        default: /*updateOsdUser3Infomation(u32OsdHandle);*/ break;
                    }
                }

                pthread_mutex_unlock(&g_stMutexMixerOsdRun[s32VencChn]);
             }
		   }
        }
#endif
    }
//osd_task_exit:
    g_bThreadOsdExit = TRUE;
    MIXER_DBG("OSD_Task -\n");
    pthread_exit(NULL);

    return NULL;
}
