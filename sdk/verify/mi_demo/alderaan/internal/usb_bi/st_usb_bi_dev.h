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
#ifndef _ST_USB_BI_DEV_H_
#define _ST_USB_BI_DEV_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <linux/types.h>
#include <linux/ioctl.h>

#define MAX_NAME_LENGTH 52

typedef enum {
    TRANSFER_IN  = 0xABBA,
    TRANSFER_OUT = 0xBAAB
} buf_type_e;

typedef struct
{
    __u32 count;
    __u32 type;
    __u32 length;
} requeset_buf_t;

typedef struct
{
    __u32 type;
    __u32 index;
    __u32 length;
    __u32 bytesused;
    __u32 mem_offset;
    __u8 *vaddr;
} buffer_t;

typedef struct
{
    buffer_t buf;
    void *start;
} Usb_Bi_Buf_t;

typedef struct
{
    int size;
    Usb_Bi_Buf_t b[];
} mem_t;

typedef struct {
    __u8 cmd;
    __u8 hostready;
    __u16 reserved;
    __u32 dataLength;
    __u32 offset;
    char name[MAX_NAME_LENGTH];
} nerve_control_header_t;

#define USB_NERVE_DEQUEUE_EVENT   _IOR('N', 1, buffer_t)
#define USB_NERVE_FINISH_EVENT    _IOW('N', 2, buffer_t)
#define USB_NERVE_DEQUEUE_BUF     _IOWR('N', 3, buffer_t)
#define USB_NERVE_QUEUE_BUF       _IOWR('N', 4, buffer_t)
#define USB_NERVE_REQBUFS         _IOWR('N', 5, requeset_buf_t)
#define USB_NERVE_QUERYBUF        _IOWR('N', 6, buffer_t)

int ST_Usb_Bi_Init();
int ST_Usb_Bi_Deinit();
int ST_Usb_Bi_SendData(char *data, unsigned int size);
int ST_Usb_Bi_RecvData(char *data, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif
