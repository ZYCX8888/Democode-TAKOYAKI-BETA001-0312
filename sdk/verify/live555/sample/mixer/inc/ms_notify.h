/*
* ms_notify.h- Sigmastar
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
#ifndef _NOTIFY_INFO_H
#define _NOTIFY_INFO_H

#define MS_MAX_PAYLOAD             256

typedef enum
{
    _type_video_,
    _type_audio_,
    _type_num_,
} mediaType_t;

typedef struct
{
    mediaType_t mediaType;
    int streamId;
    int streamEnd;
    int ipcId;
    long ipcSize;
    long frameNum;
    long frameOffset;
    long frameSize;
    int frameType;

    unsigned long long frameTimestamp;
} MsNotifyInfo_t;

#endif
