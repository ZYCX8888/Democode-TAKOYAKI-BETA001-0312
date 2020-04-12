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
#ifndef _ST_UVC_H_
#define _ST_UVC_H_

#include "st_uvc_datatype.h"

extern int pthread_setname_np(pthread_t __target_thread, 
        const char *__name);

MI_S32 ST_UVC_Init(char *uvc_name);
MI_S32 ST_UVC_Uninit(void);
MI_S32 ST_UVC_CreateDev(const ST_UVC_ChnAttr_t* pstAttr);
MI_S32 ST_UVC_DestroyDev();
MI_S32 ST_UVC_SetChnAttr(const ST_UVC_ChnAttr_t* pstAttr);
MI_S32 ST_UVC_GetChnAttr(const ST_UVC_ChnAttr_t* pstAttr);
MI_S32 ST_UVC_StartDev(void);
MI_S32 ST_UVC_StopDev(void);
void save_file(void *buf,MI_U32 length,char type);
char* uvc_get_format(MI_U32 fcc);

/// process AIT XU control select.
MI_S8 usb_vc_eu1_cs(MI_U8 cs, MI_U8 req, struct uvc_request_data *resp);
MI_S8 usb_vc_eu1_cs_out(struct uvc_request_data * data);

void usb_vc_cmd_cfg(MI_U8 req, VC_CMD_CFG *cfg, unsigned long cur_val, struct uvc_request_data *resp);

// process PU, CT, XU job.
MI_S8 usb_vc_out_data(struct uvc_request_data * data);
void USB_Class_Get_TransferBuf(unsigned char *buf, unsigned long len);

int usb_vc_eu1_mmp_cmd(unsigned char *cmd);//cmd -> 8byte
void usb_vc_eu1_isp_cmd(MI_U8 *cmd);

///

typedef struct _DataExchangeParam
{
	unsigned short id;			//command ID
	unsigned long data_len;		//transfer lens
	unsigned long cur_offset;	//current read/write offset
	char *ptr;					//data buffer
}DataExchangeParam;

#endif //_ST_UVC_H_
