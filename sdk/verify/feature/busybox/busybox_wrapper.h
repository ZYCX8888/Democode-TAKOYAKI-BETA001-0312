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
#ifndef _BUSYBOX_WRAPPER_H_
#define _BUSYBOX_WRAPPER_H_

typedef unsigned char shell_rcode;

typedef struct {
	const char *name;
	shell_rcode (*func)(const char *argv[], int count);
} busybox_cmd_t;

void busybox_loop(busybox_cmd_t *cmdlist, int count);

#endif /* _BUSYBOX_WRAPPER_H_ */
