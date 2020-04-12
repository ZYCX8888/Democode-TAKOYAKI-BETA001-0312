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
#include "st_isp.h"
#include "st_uvc_datatype.h"

#if (ISP_IQ_SERVER)
#include "mi_iqserver_datatype.h"
#include "mi_iqserver.h"
static PCAM_IQSVR_HANDLE m_stPcamIQSvrHandle = { 0 };
#endif

#if (ISP_IQ_SERVER)
//------------------------------------------------------------------------------
//  Function    : pcam_usb_iqsvr_open
//  Description : initial iq server handler and allocate buffers
//------------------------------------------------------------------------------
void ISP_usb_iqsvr_open(void)
{
    void*       pvBufVirt = NULL;
    unsigned long i;
    //CamOsRet_e  eCamOsRet = CAM_OS_OK;

    printf("[%s] - Start.\n", __FUNCTION__);
    memset(&m_stPcamIQSvrHandle, 0, sizeof(m_stPcamIQSvrHandle));

    // Allocate IQ command buffer
#if 1
    pvBufVirt = (void*)calloc(IQ_XU_CMDBUF_LEN, sizeof(char));
#else
    eCamOsRet = CamOsDirectMemAlloc("PCAM_IQCMD", IQ_XU_CMDBUF_LEN, &pvBufVirt, NULL, NULL);
    CAM_OS_RET_CHK(eCamOsRet);
#endif

    m_stPcamIQSvrHandle.pubCmdBuf = pvBufVirt;

    // Allocate IQ data buffer
#if 1
    pvBufVirt = (void*) calloc(IQ_XU_DATABUF_LEN, sizeof(char));
#else
    eCamOsRet = CamOsDirectMemAlloc("PCAM_IQDATA", IQ_XU_DATABUF_LEN, &pvBufVirt, NULL, NULL);
    CAM_OS_RET_CHK(eCamOsRet);
#endif

    m_stPcamIQSvrHandle.pubDataBuf = pvBufVirt;

    // for test.
    //for (i = 0; i < IQ_XU_DATABUF_LEN; i++)
    //    m_stPcamIQSvrHandle.pubDataBuf[i] = (i & 0xFF);

}

//------------------------------------------------------------------------------
//  Function    : pcam_usb_iqsvr_close
//  Description : uninitial iq server handler and release buffers
//------------------------------------------------------------------------------
void ISP_usb_iqsvr_close(void)
{
    //CamOsRet_e eCamOsRet = CAM_OS_OK;

    printf("[%s] - Start.\n", __FUNCTION__);
    if (m_stPcamIQSvrHandle.pubCmdBuf) {
#if 1
        free(m_stPcamIQSvrHandle.pubCmdBuf);
#else
        eCamOsRet = CamOsDirectMemRelease((void*) m_stPcamIQSvrHandle.pubCmdBuf, IQ_XU_CMDBUF_LEN);
        CAM_OS_RET_CHK(eCamOsRet);
#endif
        m_stPcamIQSvrHandle.pubCmdBuf = 0;
    }

    if (m_stPcamIQSvrHandle.pubDataBuf) {
#if 1
        free(m_stPcamIQSvrHandle.pubDataBuf);
#else
        eCamOsRet = CamOsDirectMemRelease((void*) m_stPcamIQSvrHandle.pubDataBuf, IQ_XU_DATABUF_LEN);
        CAM_OS_RET_CHK(eCamOsRet);
#endif
        m_stPcamIQSvrHandle.pubDataBuf = 0;
    }

    m_stPcamIQSvrHandle.usCmdLen = 0;
    m_stPcamIQSvrHandle.usCmdBufCurPos = 0;
    m_stPcamIQSvrHandle.usDataLen = 0;
    m_stPcamIQSvrHandle.usDataBufCurPos = 0;
}

//------------------------------------------------------------------------------
//  Function    : ISP_usb_get_iqsvr_handle
//  Description :
//------------------------------------------------------------------------------
PCAM_IQSVR_HANDLE *ISP_usb_get_iqsvr_handle(void)
{
    return &m_stPcamIQSvrHandle;
}

void ISP_Process()
{
#if 1
    char *CmdData = NULL, *GetBuf = NULL;
    int CmdLength = 0, GetLength = 0;
    unsigned short apiId;
    PCAM_IQSVR_HANDLE *iqsvr;
    MI_IQServer_CmdHeader_t stCmdHdr;

    iqsvr = ISP_usb_get_iqsvr_handle();
    if (iqsvr->pubCmdBuf) {
        memcpy(&stCmdHdr, iqsvr->pubCmdBuf, sizeof(MI_IQServer_CmdHeader_t));     // command header
        CmdData = (char *) (iqsvr->pubCmdBuf + sizeof(MI_IQServer_CmdHeader_t));   // payload
        CmdLength = (int) (iqsvr->usCmdLen - sizeof(MI_IQServer_CmdHeader_t));     // payload size
        if (iqsvr->usCmdType == IQ_XU_CMD_GETAPI) {
            GetBuf = (char *) iqsvr->pubDataBuf;
            GetLength = (int) iqsvr->usDataLen;
//            printf("=> GETAPI 0x%X, cmd len %d, get len %d\n", apiId, CmdLength, GetLength);
        }

        // call IQ server routine here
        MI_IQServer_ProcessCmd(stCmdHdr, CmdData, CmdLength, GetBuf, &GetLength);

        iqsvr->usDataLen = GetLength;
        iqsvr->ubCmdHandling = NULL;
        /* Ack EP0 data phase after command being handled */
        /*if (iqsvr->ubCmdWaitAck) {
            USB_EP0_ServicedRxPktRdy_DataEnd();
        }//*/

        /* For debug only */
#if 0
        // dump command data
        {
            int i;
            for (i = 0; i < CmdLength; i++) {
                if ((i % 0xf) == 0)
                    UartSendTrace("\n");
                UartSendTrace(" %02x", CmdData[i]);
            }
        }
        /* fill get data as sequential number, for testing only */
        if (iqsvr->usCmdType == IQ_XU_CMD_GETAPI) {
            int i = 0;
            do {
                iqsvr->pubDataBuf[i] = i & 0xFF;
            } while (++i < IQ_XU_DATABUF_LEN);
        }
#endif
#if 0
        apiId = *(unsigned short *) (CmdData);
        if (iqsvr->usCmdType == IQ_XU_CMD_SETAPI) {
            printf("[%s] SETAPI 0x%X, cmd len %d\n", __FUNCTION__, apiId, CmdLength);
        }
        else if (iqsvr->usCmdType == IQ_XU_CMD_GETAPI) {
            printf("[%s] GETAPI 0x%X, cmd len %d, get len %d\n", __FUNCTION__, apiId, CmdLength, GetLength);
        }
#endif
    }
    else {
        printf("[%s] USB Camera preview not start, can't connect to IQ server\n", __FUNCTION__);
    }//*/
#endif
}
#endif //(ISP_IQ_SERVER)
