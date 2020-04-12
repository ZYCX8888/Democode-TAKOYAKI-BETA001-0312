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
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern void do_cmd(const char *pCmd);
int main(char argc, char **argv)
{
    int i = 0;
    char cTransBuffer[TRANS_BUFFER];
    char *recFile[50];
    char *env = NULL;
    
    if (argc < 2)
    {
        printf("Arg is error\n");

        return -1;
    }
    env = getenv("MDB_REC_PATH");
    if (env != NULL)
    {
        memset(recFile, 0, 50);
        sprintf(recFile, "%s/%s", env, "rec.sh");
    }
    else
    {
        strcpy(recFile, "rec.sh");
    }
    int dest_fd = open(recFile, O_WRONLY|O_CREAT|O_APPEND, 0777);
    if (dest_fd < 0)
    {
        perror("open");
        return -1;
    }
    memset(cTransBuffer, 0, TRANS_BUFFER);
    sprintf(cTransBuffer, "./mdbcmd");
    for (i = 1; i < argc; i++)
    {
        sprintf(cTransBuffer, "%s %s", cTransBuffer, argv[i]);
    }
    write(dest_fd, (void *)cTransBuffer, strlen(cTransBuffer));
    write(dest_fd, (void *)"\n", 1);
    close(dest_fd);
    memset(cTransBuffer, 0, TRANS_BUFFER);
    sprintf(cTransBuffer, "mdb");
    for (i = 1; i < argc; i++)
    {
        sprintf(cTransBuffer, "%s %s", cTransBuffer, argv[i]);
    }
    do_cmd(cTransBuffer);
    
    return 0;
}
