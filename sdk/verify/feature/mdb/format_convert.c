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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int OpenFile(const char *pFile)
{
    int fd = 0;

    fd = open(pFile, O_RDONLY);
    if (fd == -1)
    {
        printf("Open file error [%s]", pFile);
        return -1;
    }
    
    return fd;
}
void CloseFile(int fd)
{
    close(fd);
}

int OpenFileToWrite(const char *pFile)
{
    int fd = 0;

    fd = open(pFile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd < 0)
    {

        perror("open");
        return -1;
    }
    return fd;
}

typedef struct
{
    unsigned int u32Alpha;
    unsigned int u32Red;
    unsigned int u32Green;
    unsigned int u32Blue;
}stPalette;

static stPalette gPalette[] = {
    {0, 0, 0, 0}, 
    {0, 33, 126, 0}, 
    {0, 22, 143, 0}, 
    {0, 32, 0, 188},
};

int main(int argc, char **argv)
{
    int fd = 0;
    int fd_write = 0;
    unsigned char u8Idx = 0;
    unsigned short u16Data = 0;
    unsigned int u32Data = 0;
    unsigned char au8Addr[32];
    unsigned int i = 0, read_size = 0;

    if (argc != 4)
        return 0;

    memset(au8Addr, 0, 32);
    fd = OpenFile(argv[1]);
    fd_write = OpenFileToWrite(argv[2]);
    if (!strcmp(argv[3], "1")) //I8
    {
        do
        {
            read_size =  read(fd, au8Addr, 32);
            for (i = 0; i < read_size; i++) 
            {
                u8Idx = au8Addr[i] & 0x3;
                write(fd_write, &u8Idx, 1);
                u8Idx = (au8Addr[i] & 0xC) >> 2;
                write(fd_write, &u8Idx, 1);
                u8Idx = (au8Addr[i] & 0x30) >> 4;
                write(fd_write, &u8Idx, 1);
                u8Idx = (au8Addr[i] & 0xC0) >> 6;
                write(fd_write, &u8Idx, 1);
            }
        }while(read_size == 32);
    }
    else if (!strcmp(argv[3], "2")) //rgb565
    {
        do
        {
            read_size =  read(fd, au8Addr, 32);
            for (i = 0; i < read_size; i++) 
            {
                u8Idx = au8Addr[i] & 0x3;
                u16Data = (gPalette[u8Idx].u32Red & 0xF8) << 8;
                u16Data |= (gPalette[u8Idx].u32Green & 0xFC) << 3;
                u16Data |= (gPalette[u8Idx].u32Blue & 0xF8) >> 3;
                write(fd_write, &u16Data, 2);
                u8Idx = (au8Addr[i] & 0xC) >> 2;
                u16Data = (gPalette[u8Idx].u32Red & 0xF8) << 8;
                u16Data |= (gPalette[u8Idx].u32Green & 0xFC) << 3;
                u16Data |= (gPalette[u8Idx].u32Blue & 0xF8) >> 3;
                write(fd_write, &u16Data, 2);
                u8Idx = (au8Addr[i] & 0x30) >> 4;
                u16Data = (gPalette[u8Idx].u32Red & 0xF8) << 8;
                u16Data |= (gPalette[u8Idx].u32Green & 0xFC) << 3;
                u16Data |= (gPalette[u8Idx].u32Blue & 0xF8) >> 3;
                write(fd_write, &u16Data, 2);
                u8Idx = (au8Addr[i] & 0xC0) >> 6;
                u16Data = (gPalette[u8Idx].u32Red & 0xF8) << 8;
                u16Data |= (gPalette[u8Idx].u32Green & 0xFC) << 3;
                u16Data |= (gPalette[u8Idx].u32Blue & 0xF8) >> 3;
                write(fd_write, &u16Data, 2);
            }
        }while(read_size == 32);
    }
    else if (!strcmp(argv[3], "3")) //argb8888
    {
        do
        {
            read_size =  read(fd, au8Addr, 32);
            for (i = 0; i < read_size; i++) 
            {
                u8Idx = au8Addr[i] & 0x3;
                u32Data = ((gPalette[u8Idx].u32Alpha & 0xFF) << 24) 
                    | ((gPalette[u8Idx].u32Red & 0xFF) << 16) 
                    | ((gPalette[u8Idx].u32Green & 0xFF) << 8) 
                    | (gPalette[u8Idx].u32Blue & 0xFF);
                write(fd_write, &u32Data, 4);
                u8Idx = (au8Addr[i] & 0xC) >> 2;
                u32Data = ((gPalette[u8Idx].u32Alpha & 0xFF) << 24) 
                    | ((gPalette[u8Idx].u32Red & 0xFF) << 16) 
                    | ((gPalette[u8Idx].u32Green & 0xFF) << 8) 
                    | (gPalette[u8Idx].u32Blue & 0xFF);
                write(fd_write, &u32Data, 4);
                u8Idx = (au8Addr[i] & 0x30) >> 4;
                u32Data = ((gPalette[u8Idx].u32Alpha & 0xFF) << 24) 
                    | ((gPalette[u8Idx].u32Red & 0xFF) << 16) 
                    | ((gPalette[u8Idx].u32Green & 0xFF) << 8) 
                    | (gPalette[u8Idx].u32Blue & 0xFF);
                write(fd_write, &u16Data, 4);
                u8Idx = (au8Addr[i] & 0xC0) >> 6;
                u32Data = ((gPalette[u8Idx].u32Alpha & 0xFF) << 24) 
                    | ((gPalette[u8Idx].u32Red & 0xFF) << 16) 
                    | ((gPalette[u8Idx].u32Green & 0xFF) << 8) 
                    | (gPalette[u8Idx].u32Blue & 0xFF);
                write(fd_write, &u32Data, 4);
            }
        }while(read_size == 32);
    }
    CloseFile(fd);
    CloseFile(fd_write);

    return 0;
}
