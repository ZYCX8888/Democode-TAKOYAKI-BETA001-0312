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
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct shm_st
{
    char buf[TRANS_BUFFER];
    int bWrite;
};
static int gsocketFd = 0;

static int socket_init(const char *pAddr)
{
    struct sockaddr_in stAddress;
    int socketFd;
    int len;
    int intResult;

    if (pAddr == NULL)
    {
        return -1;
    }
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("socket");
        exit (EXIT_FAILURE);
    }

    stAddress.sin_family = AF_INET;
    stAddress.sin_port = htons(8000);
    if(inet_pton(AF_INET, pAddr, &stAddress.sin_addr) <= 0)
    {
        printf("inet_pton error for %s\n", pAddr);
        return 0;
    }  
    len = sizeof (stAddress);

    intResult = connect(socketFd, (struct sockaddr *)&stAddress, len);
    if (intResult == -1)
    {
        printf ("ensure the server is up\n");
        perror ("connect");
        exit (EXIT_FAILURE);
    }

    return socketFd;
}
static void socket_deinit(int socketFd)
{
    close (socketFd);
}
static void socket_send_data(int socketFd, char *pTransData, int intTransDataSize)
{
    if (pTransData == NULL || intTransDataSize == 0)
    {
        return;
    }
    if (send(socketFd, pTransData, strlen(pTransData) + 1, 0) == -1)
    {
        perror ("send");
        exit (EXIT_FAILURE);
    }
    memset(pTransData, 0, intTransDataSize);
    if (recv(socketFd, pTransData, intTransDataSize, 0) == -1)
    {
        perror ("read");
        exit (EXIT_FAILURE);
    }
}

static void * shm_send_data(key_t key, char *pTransData, int intTransDataSize)
{
    struct shm_st * p_shm_config = NULL;
    int shmid = 0;
    void *shm = NULL;

    if (intTransDataSize == 0 || pTransData == NULL)
    {
        return NULL;
    }
    shmid = shmget(key, sizeof(struct shm_st), 0666|IPC_CREAT);
    if(shmid == -1)
    {
        fprintf(stderr, "shmget failed\n");
        return NULL;
    }
    shm = shmat(shmid, (void*)0, 0);
    if(shm == (void*)-1 || shm == NULL)
    {
        fprintf(stderr, "shmat failed\n");
        return NULL;
    }
    p_shm_config = (struct shm_st *)shm;
    if (p_shm_config != NULL)
    {
        memcpy(p_shm_config->buf, pTransData, sizeof(p_shm_config->buf));
        p_shm_config->bWrite = 1;
        while (p_shm_config->bWrite == 1)
        {
            usleep(1000);
        }
        memset(pTransData, 0, intTransDataSize);
        memcpy(pTransData, p_shm_config->buf, sizeof(p_shm_config->buf));
        if(shmdt(shm) == -1)
        {
            fprintf(stderr, "shmdt failed\n");
            return NULL;
        }
    }

    return NULL;
}


static int send_cmd(char *pTransData, int intTransDataSize)
{
    if (pTransData == NULL || intTransDataSize == 0)
    {
        return 0;
    }

    if (-1 == gsocketFd)
        shm_send_data(88860611, pTransData, intTransDataSize);
    else
        socket_send_data(gsocketFd, pTransData, intTransDataSize);

    return strlen(pTransData);
}
void do_cmd(const char *pCmd)
{
    char cTransBuffer[TRANS_BUFFER];
    int intRetSize = 0;
    char *env = NULL;

    env = getenv("MDB_SERVER_IP");
    gsocketFd = socket_init(env);

    memset(cTransBuffer, 0, TRANS_BUFFER);
    strcpy(cTransBuffer, pCmd);
    printf("%s\n", cTransBuffer);
    intRetSize = send_cmd(cTransBuffer, sizeof(cTransBuffer));
    if (intRetSize != 0)
    {
        printf("%s", cTransBuffer);
    }
    while (intRetSize == TRANS_BUFFER - 1)
    {
        memset(cTransBuffer, 0, sizeof(cTransBuffer));
        strcpy(cTransBuffer, "mdb c");
        intRetSize = send_cmd(cTransBuffer, sizeof(cTransBuffer));
        if (intRetSize != 0)
        {
            printf("%s", cTransBuffer);
        }
    }

    socket_deinit(gsocketFd);

}
