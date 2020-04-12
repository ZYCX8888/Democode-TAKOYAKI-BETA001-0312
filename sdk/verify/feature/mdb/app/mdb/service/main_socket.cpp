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
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mdb_service.h"

int main(void)
{
    pid_t fPid;

    for (;;)
    {
        fPid = fork();
        if (fPid < 0)
        {
            printf("error in fork!");
            return -1;
        }
        else if (fPid == 0)
        {
            mdb_service(1);
            return 0;
        }
        waitpid(fPid, NULL, 0);
        printf("service exit!\n");
    }

    return 0;
}

