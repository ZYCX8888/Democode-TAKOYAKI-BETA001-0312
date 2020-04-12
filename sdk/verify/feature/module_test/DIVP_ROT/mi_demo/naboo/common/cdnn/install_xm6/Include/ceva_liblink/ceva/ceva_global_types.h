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

/*
 * ceva_global_types.h
 *
 *  Created on: Oct 15, 2013
 *      Author: buntu
 */

#ifndef CEVA_GLOBAL_TYPES_H_
#define CEVA_GLOBAL_TYPES_H_

#define CEVALINK_WAITEVENT_EVENT_OR_LINK_NUM_EXCEEDS_MAX_VAL (-20)
#define CEVALINK_REGISTERCALLBACK_EVENT_OR_LINK_NUM_EXCEEDS_MAX_VAL (-21)
#define CEVALINK_REGISTERCALLBACK_CALLBACK_IS_NULL (-22)
#define CEVALINK_REGISTERCALLBACK_PAIR_OF_EVENT_AND_CTX_ALREADY_EXIST (-23)
#define CEVALINK_REGISTERCALLBACK_SUCCESS (0)
#define CEVALINK_UNREGISTERCALLBACK_EVENT_OR_LINK_NUM_EXCEEDS_MAX_VAL (-24)
#define CEVALINK_UNREGISTERCALLBACK_SUCCESS (0)

#define CEVALINK_API_CEVA_LINK_OBJECT_IS_NULL (-31)
#define CEVALINK_API_CEVA_LINK_SUCCESS 0

#define CEVAINTER_PROCESS_GETDUMMY_ENOMEM (-40)
#define CEVAINTER_PROCESS_GETDUMMY_READ (-41)
#define CEVAINTER_PROCESS_SETDUMMY_ENOMEM (-42)
#define CEVAINTER_PROCESS_SETDUMMY_WRITE (-43)
#define CEVAINTER_PROCESS_LOCK_IOCTL (-44)
#define CEVAINTER_PROCESS_LOCK_SUCCESS (0)
#define CEVAINTER_PROCESS_UNLOCK_IOCTL (-45)
#define CEVAINTER_PROCESS_UNLOCK_SUCCESS (0)

#define CEVADEVICE_GENERATEINTERRUPT_IOCTL_ERR (-50)
#define CEVADEVICE_GETDEBUGINFO_IOCTL_ERR (-51)
#define CEVADEVICE_PCI_READ_ENOMEM_ERR (-52)
#define CEVADEVICE_PCI_READ_READ_ERR (-53)
#define CEVADEVICE_PCI_WRITE_ENOMEM_ERR (-54)
#define CEVADEVICE_PCI_WRITE_WRITE_ERR (-55)
#define CEVADEVICE_PCI_GETDEBUG_INFO_SUCCESS (0)

#endif /* CEVA_GLOBAL_TYPES_H_ */
