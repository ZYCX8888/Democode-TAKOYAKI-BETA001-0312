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

#ifndef _ST_UVC_XU_DATATYPE_H_
#define _ST_UVC_XU_DATATYPE_H_

#define AIT_XU_DEF (1)

#define  CAP_SET_CUR_CMD            (1 << 0)
#define  CAP_GET_CUR_CMD            (1 << 1)
#define  CAP_GET_MIN_CMD            (1 << 2)
#define  CAP_GET_MAX_CMD            (1 << 3)
#define  CAP_GET_RES_CMD            (1 << 4)
#define  CAP_GET_LEN_CMD            (1 << 5)
#define  CAP_GET_MEM_CMD            (1 << 5)
#define  CAP_GET_INFO_CMD           (1 << 6)
#define  CAP_GET_DEF_CMD            (1 << 7)
#define  CAP_SET_CUR_ALL_CMD        (1 << 8)
#define  CAP_GET_MIN_ALL_CMD        (1 << 9)
#define  CAP_GET_MAX_ALL_CMD        (1 << 10)
#define  CAP_GET_RES_ALL_CMD        (1 << 11)
#define  CAP_GET_DEF_ALL_CMD        (1 << 12)

// capability info 
#define  INFO_GET_SUPPORT                0x01
#define  INFO_SET_SUPPORT                0x02
#define  INFO_AUTO_MODE_SUPPORT          0x04
#define  INFO_AUTOUPDATE_CONTROL_SUPPORT 0x08
#define  INFO_ASYNC_CONTROL_SUPPORT      0x10

#define UVC_XU_EU1_UNDEFINED	(0x0)
#define EU1_SET_ISP             (0x1)   //command number
#define EU1_SET_ISP_LEN         (0x08)
#define EU1_SET_ISP_MIN         (0x0)
#define EU1_SET_ISP_MAX         (0xFFFFFFFFFFFFFFFF)
#define EU1_SET_ISP_DEF         (0x0000000000000000)

#define EU1_GET_ISP_RESULT      (0x2)
#define EU1_GET_ISP_RESULT_LEN  (0x08)
#define EU1_GET_ISP_RESULT_MIN  (0x0)
#define EU1_GET_ISP_RESULT_MAX  (0xFFFFFFFFFFFFFFFF)
#define EU1_GET_ISP_RESULT_DEF  (0x0000000000000000)

#define EU1_SET_FW_DATA         (0x03)
#define EU1_SET_FW_DATA_LEN     (0x20)      //32bytes
//#define EU1_SET_FW_DATA_MIN   (0x00)
//#define EU1_SET_FW_DATA_MAX   (0xFF)
//#define EU1_SET_FW_DATA_DEF   (0x00)

#define EU1_SET_MMP             (0x04)  //command number
#define EU1_SET_MMP_LEN         (0x08)
#define EU1_SET_MMP_MIN         (0x0)
#define EU1_SET_MMP_MAX         (0xFFFFFFFFFFFFFFFF)
#define EU1_SET_MMP_DEF         (0x0000000000000000)

#define EU1_GET_MMP_RESULT      (0x5)
#define EU1_GET_MMP_RESULT_LEN  (0x08)
#define EU1_GET_MMP_RESULT_MIN  (0x0)
#define EU1_GET_MMP_RESULT_MAX  (0xFFFFFFFFFFFFFFFF)
#define EU1_GET_MMP_RESULT_DEF  (0x0000000000000000)

#define EU1_SET_ISP_EX			(0x6)
#define EU1_SET_ISP_EX_LEN      (0x10)
//#define EU1_SET_ISP_EX_MIN         (0x0)
//#define EU1_SET_ISP_EX_MAX         (0xFFFFFFFFFFFFFFFF)
//#define EU1_SET_ISP_EX_DEF         (0x0000000000000000)

#define EU1_GET_ISP_EX_RESULT      (0x7)
#define EU1_GET_ISP_EX_RESULT_LEN  (0x10)
//#define EU1_GET_ISP_EX_RESULT_MIN  (0x0)
//#define EU1_GET_ISP_EX_RESULT_MAX  (0xFFFFFFFFFFFFFFFF)
//#define EU1_GET_ISP_EX_RESULT_DEF  (0x0000000000000000)

#define EU1_READ_MMP_MEM			(0x08)
#define EU1_READ_MMP_MEM_LEN		(0x10)
//#define EU1_READ_MMP_MEM_MIN         (0x0)
//#define EU1_READ_MMP_MEM_MAX         (0xFFFFFFFFFFFFFFFF)
//#define EU1_READ_MMP_MEM_DEF         (0x0000000000000000)

#define EU1_WRITE_MMP_MEM			(0x09)
#define EU1_WRITE_MMP_MEM_LEN		(0x10)
//#define EU1_WRITE_MMP_MEM_MIN         (0x0)
//#define EU1_WRITE_MMP_MEM_MAX         (0xFFFFFFFFFFFFFFFF)
//#define EU1_WRITE_MMP_MEM_DEF         (0x0000000000000000)

#define EU1_GET_CHIP_INFO      (0xA)
#define EU1_GET_CHIP_INFO_LEN  (0x10)
#define EU1_GET_CHIP_INFO_MIN  (0x0)
#define EU1_GET_CHIP_INFO_MAX  (0xFFFFFFFFFFFFFFFF)
#define EU1_GET_CHIP_INFO_DEF  (0x0000000000000000)

#define EU1_GET_DATA_32			(0x0B)
#define EU1_GET_DATA_32_LEN		(0x20)

#define EU1_SET_DATA_32			(0x0C)
#define EU1_SET_DATA_32_LEN		(0x20)

#define EU1_SET_MMP_CMD16      (0xE)
#define EU1_SET_MMP_CMD16_LEN  (0x10)
#define EU1_SET_MMP_CMD16_MIN  (0x0)
#define EU1_SET_MMP_CMD16_MAX  (0xFFFFFFFFFFFFFFFF)
#define EU1_SET_MMP_CMD16_DEF  (0x0000000000000000)

#define EU1_GET_MMP_CMD16_RESULT      (0xF)
#define EU1_GET_MMP_CMD16_RESULT_LEN  (0x10)
#define EU1_GET_MMP_CMD16_RESULT_MIN  (0x0)
#define EU1_GET_MMP_CMD16_RESULT_MAX  (0xFFFFFFFFFFFFFFFF)
#define EU1_GET_MMP_CMD16_RESULT_DEF  (0x0000000000000000)

#define  XU_CONTROL_UNDEFINED                      0x00

#define EU_ISP_CMD_NOT_SUPPORT 0x80
#define EU1_ACCESS_CUSTOMER_DATA_LEN    (0x20)

#define MMP8_CMD_DISABLE                (254)

#define DOWNLOAD_MMP_FW_CMD             (1)
#define GET_DRAM_SIZE                   (2)
#define GET_FLASH_ID                    (3)
#define GET_FLASH_SIZE                  (4)
#define ERASE_FLASH                     (5)
#define MMP8_SET_MAX_QP                 (7)
#define MMP8_SET_MIN_QP                 (8)
#define MMP8_SET_GOP                    (9)
#define STREAM_VIEWER_CMD               (0x0C)
#define MMP8_SET_UVC_CT                 (0)//(0x10)	
#define ROMBOOT_RESET                   (0x16)	
#define MMP8_SET_SNAPSHOT_M1            (0x12)
#define MMP8_SET_OSD_DISPLAY_CONTROL    (0x21)
#define MMP8_SET_DATE_TIME              (0x22)
#define MMP8_SELECT_STREAM_ID           (0x23)
#define MMP8_TOGGLE_LAYER_CTL           (0x24)
#define MMP8_MULTICAST_COMMIT           (0x25)
#define MMP8_SET_DATE_TIME_FORMAT       (0x26)
#define MMP8_SET_OSD_DISPLAY_COLOR      (0x27)
#define MMP8_SET_EPTZ_CONTROL           (0)//(0x28)
#define MMP8_SET_MD_DIVWIN				(0x29)
#define MMP8_SET_AUDIO_WNR				(0x2C)	//ZK WNR 1031
#define MMP8_SET_AUDIO_NR				(0x2D)	//ZK NR 1031
#define MMP8_GET_RESERVE_GEAR_STATUS	(0x2B)//Gason@20150826, add reverse gear info.
#define MMP8_SET_MD_CFGWIN				(0x30)
#define MMP8_SET_MULTICAST_MIRROR		(0x31)

#endif // _ST_UVC_XU_DATATYPE_H_
