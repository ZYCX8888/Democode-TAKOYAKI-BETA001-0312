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
#ifndef MEMMAP_H
#define MAMMAP_H

#define BANK_TO_ADDR32(b) (b<<9)
#define REG_ADDR(riu_base,bank,reg_offset) ((riu_base)+BANK_TO_ADDR32(bank)+(reg_offset*4))

typedef struct
{
	unsigned char *virt_addr;
	unsigned char *mmap_base;
	unsigned int mmap_length;
}MmapHandle;
MmapHandle* devMemMMap(unsigned int phys_addr, unsigned int length);
int devMemUmap(MmapHandle* handle);

#endif