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
#include "video.h"
#include "st_isp.h"

#if (ISP_IQ_SERVER)
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

	printf("ISP_usb_iqsvr_open \r\n");
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
	//	m_stPcamIQSvrHandle.pubDataBuf[i] = (i & 0xFF);
	
}

//------------------------------------------------------------------------------
//  Function    : pcam_usb_iqsvr_close
//  Description : uninitial iq server handler and release buffers
//------------------------------------------------------------------------------
void ISP_usb_iqsvr_close(void)
{
	//CamOsRet_e eCamOsRet = CAM_OS_OK;

	printf("[Mess]ISP_usb_iqsvr_close \r\n");
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
#endif //(ISP_IQ_SERVER)