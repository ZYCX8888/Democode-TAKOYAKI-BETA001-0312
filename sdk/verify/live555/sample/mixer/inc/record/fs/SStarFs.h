/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  SStarFs.h
* Author:     fisher.yang@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2019/8/2
* Description: mixer record source file
*
*
*
* History:
*
*    1. Date  :        2019/8/2
*       Author:        fisher.yang@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/


#ifndef _SStarFs_H_
#define _SStarFs_H_

#define REMAINSPACE (512*1024)//(1024 * 100)    // 512M
#define CLEANOUTSPACE (500 *1024)
#define F_OK 0

typedef enum{
    FS_NO_ERROR = 0x0,
    FS_OK        = FS_NO_ERROR,
    //FS_READ_ERROR,
    //FS_WRITE_ERROR,
    FS_NO_SPACE,
    FS_NO_FORMAT,
    FS_NO_MOUNT,
    FS_NO_FIND
}SStarFsType;

typedef enum{
    DISK_ERROR    = -1,
    DISK_NO_ERROR = 0x0,
    DISK_NO_SPACE,
    DISK_NO_FORMAT,
    DISK_NO_FIND
}SStarDiskState;


typedef enum{
    REC_MODE_NONE = 0x0000,
    REC_MODE_TIM    = 0x0001,
    REC_MODE_MAN    = 0x0002,
    REC_MODE_CLS    = 0x0004,    //by stopVideoEncoder or sd cover
}REC_MODE;

typedef enum{
    REC_ERR_NO = 0,
    REC_ERR_CLOSE = 1,
    REC_ERR_OPEN,
    REC_ERR_DISKFULL,
    REC_ERR_WRITE,
    REC_ERR_CLSMODE,
    REC_ERR_GETFRAME,    //venc framerate is low, write SDcard faster than getstream
    REC_ERR_NEEDIFRAME
}RECORD_RET;
#endif

