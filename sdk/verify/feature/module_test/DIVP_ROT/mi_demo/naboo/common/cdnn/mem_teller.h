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
#ifndef _MEM_TELLER_H_
#define _MEM_TELLER_H_

#include "mi_common_datatype.h"

typedef  struct
{
    int sysfd;
    int memfd;
} mem_teller;

int mem_teller_create(mem_teller* teller);
int mem_teller_release(mem_teller* teller);
int mem_teller_alloc(mem_teller* teller, MI_PHY *phys_addr, unsigned int size, const char* buf_name);
int mem_teller_free(mem_teller* teller,  MI_PHY phys_addr, const char* buf_name);
int mem_teller_mmap(mem_teller* teller, MI_PHY phys_addr, MI_U8  **virt_addr, unsigned int size);
int mem_teller_unmmap(mem_teller* teller, MI_U8 *addr, unsigned int size);

#endif // _MEM_TELLER_H_