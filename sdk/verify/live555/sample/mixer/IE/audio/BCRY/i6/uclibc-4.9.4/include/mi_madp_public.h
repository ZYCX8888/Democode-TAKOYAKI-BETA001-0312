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
#ifndef _MI_MADP_PUBLIC_
#define _MI_MADP_PUBLIC_

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <fcntl.h>     //named semaphore
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include "dlfcn.h"
#include "mi_common.h"
#include "mi_osd.h"
#include "mi_vi_type.h"


#define MADP_OSD_ENABLE                       1
#define MADP_OSD_MESSAGE_ENABLE               1

#define MADP_AUDIO_ENABLE                     1

#define DIV_W_FULL_REGION                     10
#define DIV_H_FULL_REGION                     12
#define DIV_W_MULTI_REGION                    1
#define DIV_H_MULTI_REGION                    1
#define Region_Num_MAX                        8
#define Region_Num                            8
#define MAX_FUNC_TEXT_NUMBER                  10

#define SHM_MSGSND_FLAG                       0    //IPC_NOWAIT
#define SHM_READ_LOOP_MAX                     3
#define SHM_SRV_OSD_ENABLE                    0

#define MADP_VIDEO_SHM_READABLE               0x00000001
#define MADP_VIDEO_SHM_WRITEABLE              0x00000010

#define MADP_MSG_VIDEO_MODE                   0x21001000
#define MADP_MSG_OSD_MODE                     0x21002000
#define MADP_MSG_AUDIO_AO_MODE                0x21003000
#define MADP_MSG_AUDIO_AI_MODE                0x21003100

#define MADP_MSG_VIDEO_SET_DEV_ATTR           0x21001001
#define MADP_MSG_VIDEO_GET_DEV_ATTR           0x21001002
#define MADP_MSG_VIDEO_SET_CHN_ATTR           0x21001003
#define MADP_MSG_VIDEO_GET_CHN_ATTR           0x21001004
#define MADP_MSG_VIDEO_ENABLE_CHN             0x21001005
#define MADP_MSG_VIDEO_DISABLE_CHN            0x21001006
#define MADP_MSG_VIDEO_GET_IE_CHN             0x21001007
#define MADP_MSG_VIDEO_GET_VICHN_NUMBER       0x21001008
#define MADP_MSG_VIDEO_GET_IMAGE_TIMEOUT      0x21001009
#define MADP_MSG_VIDEO_GET_IMAGE_SANDBOX      0x2100100A
#define MADP_MSG_VIDEO_GET_RESULUTION         0x2100100B
#define MADP_MSG_VIDEO_GET_DEV_ATTR_VAL       0x21001022
#define MADP_MSG_VIDEO_GET_CHN_ATTR_VAL       0x21001024
#define MADP_MSG_VIDEO_GET_VICHN_NUMBER_VAL   0x21001028
#define MADP_MSG_VIDEO_THREAD_EXIT            0x210010FF

#define MADP_MSG_OSD_INIT                     0x21002001
#define MADP_MSG_OSD_CREATE_RECT_WIDGET       0x21002002
#define MADP_MSG_OSD_CREATE_RECT_WIDGET_EX    0x21002003
#define MADP_MSG_OSD_CREATE_TEXT_WIDGET       0x21002004
#define MADP_MSG_OSD_UPDATE_RECT_WIDGET       0x21002015
#define MADP_MSG_OSD_UPDATE_RECT_WIDGET_EX    0x21002016
#define MADP_MSG_OSD_UPDATE_TEXT_WIDGET       0x21002017
#define MADP_MSG_OSD_SET_WIDGET_VISIBLE       0x21002021
#define MADP_MSG_OSD_DESTROY_WIDGET           0x21002031
#define MADP_MSG_OSD_DESTROY_WIDGET_EX        0x21002032
#define MADP_MSG_OSD_THREAD_EXIT              0x210020FF

#define MADP_MSG_AUDIO_AO_INIT                0x21003001
#define MADP_MSG_AUDIO_AO_DEINIT              0x21003002
#define MADP_MSG_AUDIO_AO_SETDEVATTR          0x21003003
#define MADP_MSG_AUDIO_AO_GETDEVATTR          0x21003004
#define MADP_MSG_AUDIO_AO_SETCHNATTR          0x21003005
#define MADP_MSG_AUDIO_AO_GETCHNATTR          0x21003006
#define MADP_MSG_AUDIO_AO_ENABLECHN           0x21003007
#define MADP_MSG_AUDIO_AO_DISABLECHN          0x21003008
#define MADP_MSG_AUDIO_AO_ENABLE              0x21003009
#define MADP_MSG_AUDIO_AO_DISABLE             0x2100300A
#define MADP_MSG_AUDIO_AO_SETVOLUME           0x2100300B
#define MADP_MSG_AUDIO_AO_GETVOLUME           0x2100300C
#define MADP_MSG_AUDIO_AO_SETMUTE             0x2100300D
#define MADP_MSG_AUDIO_AO_GETMUTE             0x2100300E
#define MADP_MSG_AUDIO_AO_SENDFRAME           0x2100300F
#define MADP_MSG_AUDIO_AO_START               0x21003010
#define MADP_MSG_AUDIO_AO_THREAD_EXIT         0x210030FF

#define MADP_MSG_AUDIO_AO_GETDEVATTR_VAL      0x21003023
#define MADP_MSG_AUDIO_AO_GETCHNATTR_VAL      0x21003025
#define MADP_MSG_AUDIO_AO_GETVOLUME_VAL       0x2100302B
#define MADP_MSG_AUDIO_AO_GETMUTE_VAL         0x2100302D


#define MADP_MSG_AUDIO_AI_INIT                0x21003101
#define MADP_MSG_AUDIO_AI_DEINIT              0x21003102
#define MADP_MSG_AUDIO_AI_SETDEVATTR          0x21003103
#define MADP_MSG_AUDIO_AI_GETDEVATTR          0x21003104
#define MADP_MSG_AUDIO_AI_SETCHNATTR          0x21003105
#define MADP_MSG_AUDIO_AI_GETCHNATTR          0x21003106
#define MADP_MSG_AUDIO_AI_ENABLECHN           0x21003107
#define MADP_MSG_AUDIO_AI_DISABLECHN          0x21003108
#define MADP_MSG_AUDIO_AI_ENABLE              0x21003109
#define MADP_MSG_AUDIO_AI_DISABLE             0x2100310A
#define MADP_MSG_AUDIO_AI_SETVOLUME           0x2100310B
#define MADP_MSG_AUDIO_AI_GETVOLUME           0x2100310C
#define MADP_MSG_AUDIO_AI_SETMUTE             0x2100310D
#define MADP_MSG_AUDIO_AI_GETMUTE             0x2100310E
#define MADP_MSG_AUDIO_AI_GETFD               0x2100310F
#define MADP_MSG_AUDIO_AI_ENABLERESMP         0x21003110
#define MADP_MSG_AUDIO_AI_DISABLERESMP        0x21003111
#define MADP_MSG_AUDIO_AI_STARTFRAME          0x21003112
#define MADP_MSG_AUDIO_AI_GETFRAME            0x21003113
#define MADP_MSG_AUDIO_AI_RELEASEFRAME        0x21003114
#define MADP_MSG_AUDIO_AI_ENABLEAEC           0x21003115
#define MADP_MSG_AUDIO_AI_DISABLEAEC          0x21003116
#define MADP_MSG_AUDIO_AI_ENABLEANR           0x21003117
#define MADP_MSG_AUDIO_AI_DISABLEANR          0x21003118
#define MADP_MSG_AUDIO_AI_ENABLEWNR           0x21003119
#define MADP_MSG_AUDIO_AI_DISABLEWNR          0x2100311A
#define MADP_MSG_AUDIO_AI_ENABLEAGC           0x2100311B
#define MADP_MSG_AUDIO_AI_DISABLEAGC          0x2100311C
#define MADP_MSG_AUDIO_AI_ENABLEEQ            0x2100311D
#define MADP_MSG_AUDIO_AI_DISABLEEQ           0x2100311E
#define MADP_MSG_AUDIO_AI_SETVQEATTR          0x2100311F
#define MADP_MSG_AUDIO_AI_GETVQEATTR          0x21003120
#define MADP_MSG_AUDIO_AI_GETRTVOLUME         0x21003121
#define MADP_MSG_AUDIO_AI_SETADC              0x21003122

#define MADP_MSG_AUDIO_AI_GETDEVATTR_VAL      0x21003184
#define MADP_MSG_AUDIO_AI_GETCHNATTR_VAL      0x21003186
#define MADP_MSG_AUDIO_AI_GETVOLUME_VAL       0x2100318C
#define MADP_MSG_AUDIO_AI_GETMUTE_VAL         0x2100318E
#define MADP_MSG_AUDIO_AI_GETFD_VAL           0x2100318F
#define MADP_MSG_AUDIO_AI_GETVQEATTR_VAL      0x21003190
#define MADP_MSG_AUDIO_AI_GETRTVOLUME_VAL     0x21003191

#define MADP_MSG_AUDIO_AI_THREAD_EXIT         0x210031FF

#define MADP_MSG_APPKEY_READ                  0x21003201
#define MADP_MSG_APPKEY_SAVE                  0x21003202

#define NAMED_SEMAPHORE_MADP                  "/named_semaphore_madp"
#define NAMED_SEMAPHORE_MADP_AI               "/named_semaphore_madp_ai"
#define NAMED_SEMAPHORE_MADP_AUDIO            "/named_semaphore_madp_audio"
#define NAMED_SEMAPHORE_MADP_OSD              "/named_semaphore_madp_osd"

#define SHM_KEY_MADP                          0x1230abcd
#define SHM_KEY_MADP_OSD                      0x1231abcd
#define SHM_KEY_MADP_AUDIO                    0x1232abcd


#define MADP_MSG_TEXT_SZ                      (128 - 4)
#define MADP_SHM_TEXT_SZ                      (320*180*3/2)    //image size * 1.5
#define APP_NAME_STRING_SIZE                  (32 * 10)        //10个字符，每个字符32*32bit

typedef struct _madp_msg_t
{
    U32 msgtype;
    U8  msgtext[MADP_MSG_TEXT_SZ];
}__attribute__((__packed__)) MADP_MSG_T;

typedef struct _madp_sha_t
{
    U32 u32MsgType;
    U8  u8ImgStr[MADP_MSG_TEXT_SZ];
    U8  u8Image[MADP_SHM_TEXT_SZ];
}__attribute__((__packed__)) MADP_SHM_T;


#if MADP_OSD_ENABLE
#define VI_MAX_CHANNEL_NUM           9
#define AI_MAX_CHANNEL_NUM           4
#define AO_MAX_CHANNEL_NUM           1
#define MAX_SUB_DEVICE               20

#define VENC_CHANNEL_NUM             10
#define VENC_MAX_CHANNEL_NUM         VENC_CHANNEL_NUM

#define OSD_TEXT_WIDGET_STRING_SIZE  32

typedef struct _madp_osd_t
{
    U8 u8Idx_w;
    U8 u8Idx_h;
    U8 u8Fill;
    U8 u8Hard;
    Color_t stColor;
    Rect_t stRect;
} MADP_OSD_T;

typedef struct MI_MADP_IE_ATTR_T
{
    U8 ViChn;
    U8 IEType;
    U8 W_Div; //U32 W_Div;
    U8 H_Div; //U32 H_Div;
    U8 Fill;
    U8 Hard;
    U8 Visible;
    Rect_t rect;
    Color_t color;
    U8 string[OSD_TEXT_WIDGET_STRING_SIZE];
} __attribute__((__packed__)) MADP_IE_ATTR_T;

typedef int(*CB_ReadKeyFile)(char *buff,size_t len);
typedef int(*CB_WriteKeyFile)(char *buff,size_t len);

MI_RET MI_MADP_SetReadKeyCb(CB_ReadKeyFile cb);
MI_RET MI_MADP_SetWriteKeyCb(CB_WriteKeyFile cb);
MI_RET MI_MADP_SetKeyBuffSize(size_t sz);

#endif //#if MADP_OSD_ENABLE
#endif //#define _MI_MADP_PUBLIC_
