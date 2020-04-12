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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <sys/types.h>
#include <asm/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <poll.h>
#include <sys/time.h>
#include <errno.h>
#include <malloc.h>

#include <mdrv_verchk.h>
#include <mdrv_msys_io.h>
#include <mdrv_msys_io_st.h>

#include "mem_teller.h"


int mem_teller_create(mem_teller* teller)
{
    /* Open file /dev/msys */
   teller->sysfd = open("/dev/msys", O_RDWR|O_SYNC);
    if(teller->sysfd < 0)
    {
        printf("Out %s: can't open /dev/msys\n", __func__);
        return -1 ;
    }

    /* Open file /dev/mem */
    teller->memfd = open("/dev/mem", O_RDWR|O_SYNC);
    if(teller->memfd < 0)
    {
        printf("Out %s: can't open /dev/mem\n", __func__);
        close(teller->sysfd);
     return -1;

    }

    return 0;
}

int mem_teller_release(mem_teller* teller)
{
    /* close /dev/msys */
    /* close file /dev/mem */
    teller->memfd = open("/dev/mem", O_RDWR|O_SYNC);
    if(teller->memfd > 0)
    {
        close(teller->memfd);
    }
    if(teller->sysfd> 0)
    {
        close(teller->sysfd);
    }

    return 0;
}

int mem_teller_alloc(mem_teller* teller, MI_PHY *phys_addr, unsigned int size, const char* buf_name)
{
    MSYS_DMEM_INFO dmem;

    /* Must be non-zero size */
    if(size == 0)
    {
        printf("size is zero\n");
        return -1;
    }

    FILL_VERCHK_TYPE(dmem, dmem.VerChk_Version, dmem.VerChk_Size, IOCTL_MSYS_VERSION);
    strncpy(dmem.name, buf_name, sizeof(dmem.name));
    dmem.length = size;

    /* Request a physical buffer */
    if(ioctl(teller->sysfd, IOCTL_MSYS_REQUEST_DMEM, &dmem))
    {
        printf("Can't get memory from /dev/msys\n");
        return -1;
    }
    *phys_addr = (MI_PHY)dmem.phys;    //get physical memory address

    return 0;
}

int mem_teller_free(mem_teller* teller, MI_PHY phys_addr, const char* buf_name)
{
    MSYS_DMEM_INFO dmem;

    /* Release memory */
    memset(&dmem, 0, sizeof(dmem));
    strncpy(dmem.name, buf_name, sizeof(dmem.name));
    FILL_VERCHK_TYPE(dmem, dmem.VerChk_Version, dmem.VerChk_Size, IOCTL_MSYS_VERSION);
    dmem.phys = phys_addr;

    return ioctl(teller->sysfd, IOCTL_MSYS_RELEASE_DMEM, &dmem);
}

int mem_teller_mmap(mem_teller* teller, MI_PHY phys_addr, MI_U8 **virt_addr, unsigned int size)
{
    /* Map to logical address */
    *virt_addr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, teller->memfd, phys_addr);
    if(virt_addr == NULL)
    {
        printf("Can't map a virtual memory\n");
        return -1;
    }

    return 0;
}

int mem_teller_unmmap(mem_teller* teller, MI_U8 *virt_addr, unsigned int size)
{
    return munmap(virt_addr, size);
}


