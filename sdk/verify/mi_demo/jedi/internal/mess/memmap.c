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
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "memmap.h"

#define DBG_ERR(fmt, args...)   printf(fmt, ##args)
#define DBG_INFO(fmt, args...)  printf(fmt, ##args)

static unsigned int const page_size_mask = 0xFFF;
MmapHandle* devMemMMap(unsigned int phys_addr, unsigned int length)
{
	int fd;
	unsigned int phys_offset;

	fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd == -1)
	{
		DBG_ERR("open /dev/mem fail\n");
		return NULL;
	}

	MmapHandle *handle = malloc(sizeof(MmapHandle));
	phys_offset =(phys_addr & (page_size_mask));
	phys_addr &= ~(page_size_mask);
	handle->mmap_length = length + phys_offset;
	handle->mmap_base = mmap(NULL, handle->mmap_length , PROT_READ|PROT_WRITE, MAP_SHARED, fd, phys_addr);
	handle->virt_addr = handle->mmap_base + phys_offset;
	DBG_INFO("phys_addr: %#x\n", phys_addr);
	DBG_INFO("virt_addr: %p\n", handle->virt_addr);
	DBG_INFO("phys_offset: %#x\n", phys_offset);

	if (handle->mmap_base == MAP_FAILED)
	{
		DBG_ERR("mmap fail\n");
		close(fd);
		free(handle);
		return NULL;
	}

	close(fd);
	return handle;
}

int devMemUmap(MmapHandle* handle)
{
	int ret = 0;

	ret = munmap(handle->mmap_base, handle->mmap_length);
	if(ret != 0)
	{
		printf("munmap fail\n");
		return ret;
	}
	free(handle);
	return ret;
}
