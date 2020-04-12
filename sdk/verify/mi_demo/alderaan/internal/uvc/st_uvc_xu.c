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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <assert.h>

#include "st_uvc.h"
#include "st_isp.h"
#include "st_uvc_xu.h"

extern UVC_DBG_LEVEL_e uvc_debug_level;

#if AIT_XU_DEF
FW_RELEASE_VERSION    gbFwVersion = { 0x01, 0x00, FW_VER_BCD }; // Add FW version.

uint8_t eu1_set_isp_val[EU1_SET_MMP_LEN];
uint8_t eu1_get_isp_result_val[EU1_GET_ISP_RESULT_LEN];
uint8_t eu1_set_mmp_val[EU1_SET_MMP_LEN];
uint8_t eu1_get_mmp_result_val[EU1_GET_MMP_RESULT_LEN];
uint8_t eu1_set_isp_ex_val[EU1_SET_ISP_EX_LEN];
uint8_t eu1_get_isp_ex_result_val[EU1_GET_ISP_EX_RESULT_LEN];
uint8_t eu1_get_chip_info_val[EU1_GET_CHIP_INFO_LEN];
uint8_t eu1_set_mmp_cmd16_val[EU1_SET_MMP_CMD16_LEN];
uint8_t eu1_get_mmp_cmd16_result_val[EU1_GET_MMP_CMD16_RESULT_LEN];
uint8_t eu1_get_data_32_val[EU1_GET_DATA_32_LEN];
uint8_t eu1_set_data_32_val[EU1_SET_DATA_32_LEN];

unsigned long WriteMemAddr = 0;
unsigned long WriteMemCount = 0;
unsigned long ReadMemAddr = 0;
unsigned long ReadMemCount = 0;

/*
 *  XU1 : AIT internal ISP / MMP control
 */
VC_CMD_CFG VC_XU_SET_ISP_CFG = {
    (CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_SET_SUPPORT),
    EU1_SET_ISP_LEN,1,
    0,0,0,0,0
};

VC_CMD_CFG VC_XU_GET_ISP_CFG = {
    (CAP_GET_INFO_CMD | CAP_GET_CUR_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_GET_SUPPORT),
    EU1_GET_ISP_RESULT_LEN,1,
    0,0,0,0,0
};


VC_CMD_CFG VC_XU_SET_FW_DATA_CFG = {
    (CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_SET_SUPPORT),
    EU1_SET_FW_DATA_LEN,1,
    0,0,0,0,0
};


VC_CMD_CFG VC_XU_SET_MMP_CFG = {
    (CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_SET_SUPPORT),
    EU1_SET_MMP_LEN,1,
    0,0,0,0,0
};


VC_CMD_CFG VC_XU_GET_MMP_CFG = {
    ( CAP_GET_INFO_CMD | CAP_GET_CUR_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_GET_SUPPORT),
    EU1_GET_MMP_RESULT_LEN,1,
    0,0,0,0,0
};

VC_CMD_CFG VC_XU_SET_ISP_EX_CFG = {
    (CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_SET_SUPPORT),
    EU1_SET_ISP_EX_LEN,1,
    0,0,0,0,0
};

VC_CMD_CFG VC_XU_GET_ISP_EX_CFG = {
    ( CAP_GET_INFO_CMD | CAP_GET_CUR_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_GET_SUPPORT),
    EU1_GET_ISP_EX_RESULT_LEN,1,
    0,0,0,0,0
};

VC_CMD_CFG VC_XU_READ_MMP_MEM_CFG = {
    ( CAP_GET_INFO_CMD | CAP_GET_CUR_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_GET_SUPPORT),
    EU1_READ_MMP_MEM_LEN,1,
    0,0,0,0,0
};

VC_CMD_CFG VC_XU_SET_MMP_MEM_CFG = {
    (CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_SET_SUPPORT),
    EU1_WRITE_MMP_MEM_LEN,1,
    0,0,0,0,0
};

VC_CMD_CFG VC_XU_ACCESS_CUSTOMER_DATA_CFG = {
    (CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_SET_SUPPORT),
    EU1_ACCESS_CUSTOMER_DATA_LEN,1,
    0,0,0,0,0
};



VC_CMD_CFG VC_XU_GET_CHIP_INFO_CFG = {
    ( CAP_GET_INFO_CMD | CAP_GET_CUR_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_GET_SUPPORT),
    EU1_GET_CHIP_INFO_LEN,1,
    0,0,0,0,0
};

VC_CMD_CFG VC_XU_GET_DATA_32_CFG = {
    (CAP_GET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_SET_SUPPORT),
    EU1_GET_DATA_32_LEN,1,
    0,0,0,0,0
};

VC_CMD_CFG VC_XU_SET_DATA_32_CFG = {
    (CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_SET_SUPPORT),
    EU1_SET_DATA_32_LEN,1,
    0,0,0,0,0
};

VC_CMD_CFG VC_XU_SET_MMP_CMD16_CFG = {
    (CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_SET_SUPPORT),
    EU1_SET_MMP_CMD16_LEN,1,
    0,0,0,0,0
};

DataExchangeParam gDEParam = { 0,0,0,0 };

VC_CMD_CFG VC_XU_GET_MMP_CMD16_RESULT_CFG = {
    (CAP_SET_CUR_CMD | CAP_GET_INFO_CMD | CAP_GET_CUR_CMD | CAP_GET_DEF_CMD | CAP_GET_MIN_CMD | CAP_GET_MAX_CMD | CAP_GET_RES_CMD | CAP_GET_LEN_CMD),
    (INFO_SET_SUPPORT | INFO_GET_SUPPORT),
    EU1_GET_MMP_CMD16_RESULT_LEN,1,
    0,0,0,0,0
};
#endif

// process xu isp command.
void usb_vc_eu1_isp_cmd(uint8_t *cmd)//cmd -> 8byte
{
    //cmd[2] = 0x01;
    //cmd[3] = 0x02;
    //cmd[4] = 0x03;
    //cmd[5] = 0x04;
    //cmd[6] = 0x05;
    //cmd[7] = 0x06;
    eu1_get_isp_result_val[0] = EU_ISP_CMD_OK;

    UVC_INFO("ISP cmd:%x %x %x %x %x %x %x %x\r\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);
    switch (cmd[0])
    {
        case 11:    //get FW ver
        {
            eu1_get_isp_result_val[2] = (gbFwVersion.major >> 8) & 0xFF;
            eu1_get_isp_result_val[3] = gbFwVersion.major & 0xFF;
            eu1_get_isp_result_val[4] = (gbFwVersion.minor >> 8) & 0xFF;
            eu1_get_isp_result_val[5] = gbFwVersion.minor & 0xFF;
            eu1_get_isp_result_val[6] = (gbFwVersion.build >> 8) & 0xFF;
            eu1_get_isp_result_val[7] = gbFwVersion.build & 0xFF;
        }
        break;

        default:
            break;
    }
}

#define RAW_STORE_ONE_IMG      0x33

#define I2C_MODE_1A1D 1
#define I2C_MODE_2A1D 0

static int usb_vc_eu1_mmp_cmd(unsigned char* cmd)//cmd -> 8byte
{
    int ret = 0;
#if (ISP_IQ_SERVER)
    PCAM_IQSVR_HANDLE *iqsvr;
#endif

    UVC_INFO("[mmp_cmd]: [%x] [%x] [%x] [%x] [%x] [%x] [%x] [%x]\r\n",
        cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);

    //eu1_get_mmp_result_val[0] = EU_ISP_CMD_OK;

    switch (cmd[0])
    {
    case DOWNLOAD_MMP_FW_CMD:
#if (ISP_IQ_SERVER)
        iqsvr = ISP_usb_get_iqsvr_handle();
#endif
        //FW burning,burn the firmware in task instead of ISR
        switch (cmd[1])
        {
        case 0:
#if (ISP_IQ_SERVER)
            /* Tool is going to send command data */
            if (iqsvr->ubCmdHandling == false)
            {
                iqsvr->usCmdLen = (unsigned short) (cmd[4] | (cmd[5] << 8));

                UVC_INFO("[Mess] ISP SVR Len: %d\r\n", iqsvr->usCmdLen);
                iqsvr->usCmdBufCurPos = 0;  // reset buffer write ptr
                if (cmd[3] == 6) {
                    iqsvr->usCmdType = IQ_XU_CMD_SETAPI;
                }
                else if (cmd[3] == 7) {
                    iqsvr->usCmdType = IQ_XU_CMD_GETAPI;
                    iqsvr->usDataBufCurPos = 0;
                }
                iqsvr->ubCmdHandling = true;
                iqsvr->ubCmdWaitAck = false;
            }
            else {
                // Pend acking this request until the previous command is handled.
                iqsvr->ubCmdWaitAck = true;
                return -1;
            }

#endif
            break;
        case 1:
#if (ISP_IQ_SERVER)
            //Tool sent command data completely

            UVC_INFO("[Mess]all command data received\n");

#endif
            break;
        case 0x10:
#if (ISP_IQ_SERVER)
            //Tool is going to get data back
            if (iqsvr->usCmdType == IQ_XU_CMD_GETAPI)
            {
                // Pend acking this request until command is handled by sensor task.
                if (iqsvr->ubCmdHandling) {
                    iqsvr->ubCmdHandling = false;
                    iqsvr->ubCmdWaitAck = true;
                    return -1;
                }
            }
#endif
            break;
        default:
            break;
        }
        break;
    default:
        eu1_get_mmp_result_val[0] = EU_ISP_CMD_NOT_SUPPORT;
        break;
    }
    return ret;
}

// To process XU data.
int8_t usb_vc_eu1_cs(uint8_t cs, uint8_t req, struct uvc_request_data *resp)
{
    switch (cs)
    {
        case EU1_SET_ISP:
        {
            VC_XU_SET_ISP_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_ISP_CFG.dwMinVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_ISP_CFG.dwResVal = 0;//(unsigned long)resVal ;
            UVC_INFO("EU1_SET_ISP\r\n");
            usb_vc_cmd_cfg(req, &VC_XU_SET_ISP_CFG, 0, resp);
            break;
        }
        case EU1_GET_ISP_RESULT:
        {
            VC_XU_SET_ISP_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_ISP_CFG.dwMinVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_ISP_CFG.dwResVal = 0;//(unsigned long)resVal ;
            UVC_INFO("EU1_GET_ISP\r\n");
            usb_vc_cmd_cfg(req, &VC_XU_GET_ISP_CFG, (unsigned long) eu1_get_isp_result_val, resp);
            break;
        }
        case EU1_SET_FW_DATA:
        {
            VC_XU_SET_ISP_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_ISP_CFG.dwMinVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_ISP_CFG.dwResVal = 0;//(unsigned long)resVal ;
            VC_XU_SET_ISP_CFG.dwCurVal = 0;//(unsigned long)resVal ;
            UVC_INFO("EU1_SET_FW_DATA\r\n");
            usb_vc_cmd_cfg(req, &VC_XU_SET_FW_DATA_CFG, 0, resp);
            break;
        }
        case EU1_SET_MMP:
        {
            VC_XU_SET_ISP_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_ISP_CFG.dwMinVal = 0;
            VC_XU_SET_ISP_CFG.dwResVal = 0;//(unsigned long)resVal ;

            //UVC_INFO("EU1_SET_MMP\r\n");
            usb_vc_cmd_cfg(req, &VC_XU_SET_MMP_CFG, 0, resp);
            break;
        }
        case EU1_GET_MMP_RESULT:
        {
            VC_XU_SET_ISP_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_ISP_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_ISP_CFG.dwResVal = 0;//(unsigned long)resVal ;
            //UVC_INFO("EU1_GET_MMP_RESULT\r\n");
            usb_vc_cmd_cfg(req, &VC_XU_GET_MMP_CFG, (unsigned long) eu1_get_mmp_result_val, resp);
            break;
        }
        case EU1_SET_ISP_EX:
        {
            VC_XU_SET_ISP_EX_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_ISP_EX_CFG.dwMinVal = 0;//(unsigned long)maxVal
            VC_XU_SET_ISP_EX_CFG.dwResVal = 0;//(unsigned long)resVal ;
            //UVC_INFO("EU1_SET_ISP_EX\r\n");
            usb_vc_cmd_cfg(req, &VC_XU_SET_ISP_EX_CFG, 0, resp);
            break;
        }
        case EU1_GET_ISP_EX_RESULT:
        {
            VC_XU_SET_ISP_EX_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_ISP_EX_CFG.dwMinVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_ISP_EX_CFG.dwResVal = 0;//(unsigned long)resVal ;
            //UVC_INFO("EU1_GET_ISP_EX\r\n");
            usb_vc_cmd_cfg(req, &VC_XU_GET_ISP_EX_CFG, (unsigned long) eu1_get_isp_ex_result_val, resp);
            break;
        }

        case EU1_READ_MMP_MEM:
        {
            unsigned char buf[EU1_READ_MMP_MEM_LEN];
            if (req == UVC_GET_CUR)
            {
                unsigned char n;
                unsigned char rlen;

                if (ReadMemCount>EU1_READ_MMP_MEM_LEN)
                    rlen = EU1_READ_MMP_MEM_LEN;
                else
                    rlen = ReadMemCount;

                ReadMemCount -= rlen;
                for (n = 0; n<16; ++n)
                {
                    if (n<rlen)
                        buf[n] = ((char*) ReadMemAddr)[n];
                    else
                        buf[n] = 0;
                }
                ReadMemAddr += rlen;
            }
            VC_XU_READ_MMP_MEM_CFG.dwMaxVal = 0;
            VC_XU_READ_MMP_MEM_CFG.dwMinVal = 0;
            VC_XU_READ_MMP_MEM_CFG.dwResVal = 0;
            usb_vc_cmd_cfg(req, &VC_XU_READ_MMP_MEM_CFG, (unsigned long) buf, resp);
            break;
        }
        case EU1_WRITE_MMP_MEM:
        {
            VC_XU_SET_MMP_MEM_CFG.dwMaxVal = 0;
            VC_XU_SET_MMP_MEM_CFG.dwMinVal = 0;
            VC_XU_SET_MMP_MEM_CFG.dwResVal = 0;
            usb_vc_cmd_cfg(req, &VC_XU_READ_MMP_MEM_CFG, 0, resp);
            break;
        }

        case EU1_GET_CHIP_INFO:
        {
            VC_XU_GET_CHIP_INFO_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
            VC_XU_GET_CHIP_INFO_CFG.dwMinVal = 0;//(unsigned long)maxVal ;
            VC_XU_GET_CHIP_INFO_CFG.dwResVal = 0;//(unsigned long)resVal ;

            UVC_INFO("EU1_GET_CHIP_INFO\r\n");
            eu1_get_chip_info_val[0] = '8';
            eu1_get_chip_info_val[1] = '4';
            eu1_get_chip_info_val[2] = '5';
            eu1_get_chip_info_val[3] = '5';
            eu1_get_chip_info_val[4] = 0;
            eu1_get_chip_info_val[5] = 0;;
            eu1_get_chip_info_val[6] = 0;
            eu1_get_chip_info_val[7] = 0;
            eu1_get_chip_info_val[8] = 0;
            eu1_get_chip_info_val[9] = 0;
            eu1_get_chip_info_val[10] = 0;
            eu1_get_chip_info_val[11] = 0;
            eu1_get_chip_info_val[12] = 0;
            eu1_get_chip_info_val[13] = 0;
            eu1_get_chip_info_val[14] = 0;
            eu1_get_chip_info_val[15] = 0;
            usb_vc_cmd_cfg(req, &VC_XU_GET_CHIP_INFO_CFG, (unsigned long) &eu1_get_chip_info_val[0], resp);
            break;
        }

        case EU1_GET_DATA_32:
        {
    //      unsigned char curVal[EU1_GET_DATA_32_LEN];
            unsigned char maxVal[EU1_GET_DATA_32_LEN];
            unsigned char resVal[EU1_GET_DATA_32_LEN];
            unsigned char i;
#if (ISP_IQ_SERVER)
            PCAM_IQSVR_HANDLE *iqsvr;

#endif

            for (i = 0; i<EU1_GET_DATA_32_LEN; i++) {
                maxVal[i] = 0xFF;
                resVal[i] = 0x00;
            }
            resVal[0] = 0x01;
            VC_XU_GET_DATA_32_CFG.dwMaxVal = (unsigned long) maxVal;
            VC_XU_GET_DATA_32_CFG.dwResVal = (unsigned long) resVal;
#if (ISP_IQ_SERVER)
            if (req == UVC_GET_CUR)
            {
                iqsvr = ISP_usb_get_iqsvr_handle();
                //here we send back data from IQ server
                if ((iqsvr->usDataBufCurPos + EU1_GET_DATA_32_LEN) <= IQ_XU_DATABUF_LEN)
                {
                    unsigned char *ptemp = (unsigned char*) (iqsvr->pubDataBuf + iqsvr->usDataBufCurPos);

                    UVC_INFO("EU1_GET_DATA_32: memcpy address:0x%x,0x%x\r\n", iqsvr->usDataBufCurPos, ptemp[0]);

                    memcpy(eu1_get_data_32_val,
                        iqsvr->pubDataBuf + iqsvr->usDataBufCurPos,
                        EU1_GET_DATA_32_LEN);
                }
                else {
                    memcpy(eu1_get_data_32_val, (void*)0xFF, EU1_GET_DATA_32_LEN);
                    UVC_INFO("[Err]: getting size is over than data buf size\n");
                }

                iqsvr->usDataBufCurPos += EU1_GET_DATA_32_LEN;
            }

#endif //(ISP_IQ_SERVER)
            //usb_vc_cmd_cfg(req, &VC_XU_GET_DATA_32_CFG, VC_XU_GET_DATA_32_CFG.dwCurVal, resp);
            usb_vc_cmd_cfg(req, &VC_XU_GET_DATA_32_CFG, (unsigned long) eu1_get_data_32_val, resp);
            break;
        }
        case EU1_SET_DATA_32:
        {
            VC_XU_SET_DATA_32_CFG.dwMaxVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_DATA_32_CFG.dwMinVal = 0;//(unsigned long)maxVal ;
            VC_XU_SET_DATA_32_CFG.dwResVal = 0;//(unsigned long)resVal ;
            VC_XU_SET_DATA_32_CFG.dwCurVal = 0;//(unsigned long)maxVal ;

            //UVC_INFO("EU1_SET_FW_DATA32\r\n");
            usb_vc_cmd_cfg(req, &VC_XU_SET_DATA_32_CFG, 0, resp);
            break;
        }

        case EU1_SET_MMP_CMD16:
        {
            VC_XU_SET_MMP_CMD16_CFG.dwMinVal = 0;
            VC_XU_SET_MMP_CMD16_CFG.dwMaxVal = 0;
            VC_XU_SET_MMP_CMD16_CFG.dwResVal = 0;
            usb_vc_cmd_cfg(req, &VC_XU_SET_MMP_CMD16_CFG, 0, resp);
            break;
        }
        case EU1_GET_MMP_CMD16_RESULT:
        {
            VC_XU_GET_MMP_CMD16_RESULT_CFG.dwMinVal = 0;
            VC_XU_GET_MMP_CMD16_RESULT_CFG.dwMaxVal = 0;
            VC_XU_GET_MMP_CMD16_RESULT_CFG.dwResVal = 0;
            usb_vc_cmd_cfg(req, &VC_XU_GET_MMP_CMD16_RESULT_CFG, (unsigned long) eu1_get_mmp_cmd16_result_val, resp);
            break;
        }

        case XU_CONTROL_UNDEFINED:
        default:
            // un-support
            goto EU1_CS_FAIL;
            break;
    }

    return ST_UVC_SUCCESS;

EU1_CS_FAIL:
    /*
    * We don't support this control, so STALL the
    * default control ep.
    */
    return -1;
}

int8_t usb_vc_eu1_cs_out(uint8_t entity_id, uint8_t cs, uint32_t len, struct uvc_request_data *data)
{
#if (ISP_IQ_SERVER)
    unsigned char len;
    PCAM_IQSVR_HANDLE *iqsvr;
#endif

    switch (cs)
    {
    case EU1_SET_ISP:
        usb_vc_eu1_isp_cmd(data->data);
        break;
    case EU1_GET_ISP_RESULT:
        break;
    case EU1_SET_FW_DATA:
        //set fw data
        //Move to USB task
#if (ISP_IQ_SERVER)
        iqsvr = ISP_usb_get_iqsvr_handle();
        if ((iqsvr->usCmdBufCurPos + len) <= IQ_XU_CMDBUF_LEN)
        {
            if (iqsvr->usCmdBufCurPos < iqsvr->usCmdLen)
            {
                char *cmd = (char*)data->data;
                UVC_INFO("EU1_SET_FW_DATA::Len:0x%x, cmd:%x %x %x %x %x %x %x %x\r\n", len,
                    cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);

                memcpy(iqsvr->pubCmdBuf + iqsvr->usCmdBufCurPos, data->data, len);
                iqsvr->usCmdBufCurPos += len;
                // Wake up IQ server to handle request from tool
                if (iqsvr->usCmdBufCurPos >= iqsvr->usCmdLen) {
                    //MMPF_OS_SetFlags(SENSOR_Flag, SENSOR_FLAG_PCAMOP, MMPF_OS_FLAG_SET);

                    ISP_Process(); // to process isp.
                    UVC_INFO("[Mes] ready to process ISP data\r\n");
                }
            }
            else {
                // show error
                UVC_INFO("[Err] Tool sends an over-size command (%d)\n", iqsvr->usCmdLen);
            }
        }
        else
        {
            UVC_INFO("[Err] Tool sends an over max size command (%d, %d)\n", iqsvr->usCmdLen, IQ_XU_CMDBUF_LEN);
        }

#endif //(ISP_IQ_SERVER)
        break;
    case EU1_SET_MMP:

        usb_vc_eu1_mmp_cmd(data->data);
        break;
    case EU1_GET_DATA_32:
    {
        uint8_t maxVal[EU1_GET_DATA_32_LEN];
        uint8_t resVal[EU1_GET_DATA_32_LEN];
        uint8_t i;

        UVC_INFO("EU1_GET_DATA_32\r\n");
        for (i = 0; i < EU1_GET_DATA_32_LEN; i++) {
            maxVal[i] = 0xFF;
            resVal[i] = 0x00;
        }
        VC_XU_GET_DATA_32_CFG.dwMaxVal = (uint32_t) maxVal;
        VC_XU_GET_DATA_32_CFG.dwResVal = (uint32_t) resVal;
    }
    break;
    default:
        UVC_WRN(":: not support.\r\n");
        break;
    }
    return ST_UVC_SUCCESS;
}

void usb_vc_cmd_cfg(uint8_t req, VC_CMD_CFG *cfg, unsigned long cur_val, struct uvc_request_data *resp)
{
    UVC_INFO("vc.req : 0x%x\n", req);
    UVC_INFO("vc.val : 0x%lx\n", cur_val);
    UVC_INFO("cmd cap : 0x%lx, info cap : 0x%x\n",cfg->bCmdCap, cfg->bInfoCap);

    switch (req)
    {
    case UVC_SET_CUR:
        if (cfg->bCmdCap & CAP_SET_CUR_CMD) {
            if (cfg->bInfoCap & INFO_AUTO_MODE_SUPPORT) {

            }
        }
        else {
            goto invalid_req;
        }
        break;
    case UVC_GET_CUR:
        if (cfg->bCmdCap & CAP_GET_CUR_CMD) {
            memcpy(resp->data, (void*)cur_val, cfg->bCmdLen);
            resp->length = cfg->bCmdLen;
        }
        else {
            goto invalid_req;
        }
        break;
    case UVC_GET_INFO:
        if (cfg->bCmdCap & CAP_GET_INFO_CMD) {
            UVC_INFO("vc.bInfoCap : %x\r\n", cfg->bInfoCap);
            resp->data[0] = (INFO_SET_SUPPORT | INFO_GET_SUPPORT);
            resp->length = sizeof(char);
        }
        else {
            goto invalid_req;
        }
        break;

    case UVC_GET_DEF:
        if (cfg->bCmdCap & CAP_GET_DEF_CMD) {
            UVC_INFO("vc.def : %lx\n",cfg->dwDefVal);
            resp->data[0] = 0x0;
            resp->data[1] = 0x0;
            resp->length = sizeof(unsigned short);
        }
        else {
            goto invalid_req;
        }
        break;
    case UVC_GET_MAX:
        if (cfg->bCmdCap & CAP_GET_MAX_CMD) {
            UVC_INFO("vc.max : %lx\n", cfg->dwMaxVal);
            resp->data[0] = 0x1;
            resp->data[1] = 0x0;
            resp->length = sizeof(unsigned short);
        }
        else {
            goto invalid_req;
        }
        break;
    case UVC_GET_MIN:
        if (cfg->bCmdCap & CAP_GET_MIN_CMD) {
            UVC_INFO("vc.mix : %lx\n", cfg->dwMinVal);
            resp->data[0] = 0x0;
            resp->length = sizeof(char);
        }
        else {
            goto invalid_req;
        }
        break;
    case UVC_GET_RES:
        if (cfg->bCmdCap & CAP_GET_RES_CMD) {
            UVC_INFO("vc.res : %lx\n", cfg->dwResVal);
            resp->data[0] = 0x1;
            resp->data[1] = 0x0;
            resp->length = sizeof(unsigned short);
        }
        else {
            goto invalid_req;
        }
        break;
    case UVC_GET_LEN:
        if (cfg->bCmdCap & CAP_GET_LEN_CMD) {
            unsigned short cmdLen = cfg->bCmdLen;
            memcpy(resp->data, (unsigned char *) &cmdLen, sizeof(unsigned short));
            resp->length = sizeof(unsigned short);
        }
        else {
            goto invalid_req;
        }
        break;
    default:
    invalid_req:
        UVC_INFO("Cfg.Err\r\n");
        //UsbWriteEp0CSR(SET_EP0_SENDSTALL);
        //gbVCERRCode = CONTROL_INVALID_REQUEST;
        break;
    }

    UVC_INFO("Cfg.End\r\n");
}
