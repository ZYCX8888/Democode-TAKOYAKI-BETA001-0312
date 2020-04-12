/*
* iqserver_cali_func.h- Sigmastar
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
#ifndef _IQSERVER_CALI_FUNC_H_
#define _IQSERVER_CALI_FUNC_H_

#include "mi_common.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CALIBRATION_DB     "/data/cfg/calibration.db"
#define CALI_AWB_PATH      "/data/cfg/awb_cali.data"
#define CALI_AE_PATH       "/data/cfg/ae_cali.data"
#define CALI_OBC_PATH      "/data/cfg/obc_cali.data"
#define CALI_MINGAIN_PATH            "/data/cfg/mingain_cali.data"
#define CALI_SHUTTERLINEARITY_PATH   "/data/cfg/shutter_cali.data"
#define CALI_GAINLINEARITY_PATH      "/data/cfg/gain_cali.data"
#define CALI_DPC_PATH      "/data/cfg/dpc_cali.data"
#define CALI_ALSC_PATH               "/data/cfg/alsc_cali.data"


typedef enum
{
    CALI_ITEN_AWB     = 0,
    CALI_ITEN_AE      = 1,
    CALI_ITEN_OBC     = 2,
    CALI_ITEN_MINGAIN           = 3,
    CALI_ITEN_SHUTTERLINEARITY  = 4,
    CALI_ITEN_GAINLINEARITY     = 5,
    CALI_ITEN_DPC               = 6,
    CALI_ITEN_ALSC              = 7,
} CaliItem;

typedef struct
{
    BOOL bAWB ;
    BOOL bAE  ;
    BOOL bOBC ;
    BOOL bMINGAIN;
    BOOL bSHUTTER;
    BOOL bGAIN;
    BOOL bDPC ;
    BOOL bALSC;
} CalibrationINFO;

typedef struct
{
    CaliItem item;
    char filepath[128];
} CaliDataPath;
typedef struct
{
    char path[128];
} CaliDBPath;

typedef struct
{
    char awb_path[128];
    char ae_path[128];
    char obc_path[128];
    char mingain_path[128];
    char shutter_path[128];
    char gain_path[128];
    char dpc_path[128];
    char alsc_path[128];
} CaliDBInfo;
int  IQSERVER_set_calibration_db_path(char *path);
int  IQSERVER_get_calibration_db_path(char *path);
int IQSERVER_init_calibration_db(char *db_path, CaliDBInfo *calidb);
int IQSERVER_write_calibration_db(CaliDataPath cali_data);
int IQSERVER_read_calibration_db(CaliDBInfo *calidb);
int IQSERVER_read_user_calibration_db(CaliDBInfo *calidb, char *db_path);
int IQSERVER_save_calibration_data(char *pData, S32 data_size, CaliDataPath cali_data);
int IQSERVER_get_cali_data_path(CaliDataPath *cali_data);
int IQSERVER_load_calibration_data(char **pData, S32 *data_size, CaliItem cali_item);
int IQSERVER_load_all_calibration_data(char *db_path);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
