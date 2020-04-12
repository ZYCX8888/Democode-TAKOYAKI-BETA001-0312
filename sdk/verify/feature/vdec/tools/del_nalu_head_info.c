#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<assert.h>

typedef unsigned char                            MI_U8;         // 1 byte
typedef unsigned short                           MI_U16;        // 2 bytes
typedef unsigned int                             MI_U32;        // 4 bytes
typedef unsigned long long                       MI_U64;        // 8 bytes
typedef signed char                              MI_S8;         // 1 byte
typedef signed short                             MI_S16;        // 2 bytes
typedef signed int                               MI_S32;        // 4 bytes
typedef signed long long                         MI_S64;        // 8 bytes
typedef float                                    MI_FLOAT;      // 4 bytes
typedef unsigned long long                       MI_PHY;        // 8 bytes
typedef unsigned long                            MI_VIRT;       // 4 bytes when 32bit toolchain, 8 bytes when 64bit toolchain.
typedef unsigned char                            MI_BOOL;


#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])
#define VESFILE_READER_BATCH (2 * 1024*1024)


int main(int argc, const char *argv[])
{
    MI_U8 _au8SrcFile[64] = "";
    MI_U8 _au8DstFile[64] = "";

    MI_S32 _hSrcFile;
    MI_S32 _hDstFile;

    MI_U8 *pu8Buf = NULL;
    MI_U32 u32ReadLen = 0;
    MI_U32 u32WriteLen = 0;
    MI_U32 u32FrameLen = 0;
    MI_U64 u64Pts = 0;
    MI_U8  au8Header[16] = {0};
    MI_U32 u32Pos = 0;

    if(3 != argc)
    {
        printf("Input err!!\n");
        printf("Usage: %s [SrcFile] [DstFile]\n", argv[0]);
        printf("Like: %s src/file1 dst/file2\n", argv[0]);
        return -1;
    }

    strcpy(_au8SrcFile, argv[1]);
    strcpy(_au8DstFile, argv[2]);

    printf("\033[1;32m""Open File:%s\n""\033[0m", _au8SrcFile);
    _hSrcFile = open(_au8SrcFile, O_RDONLY, 0);
    if(_hSrcFile <= 0)
    {
        printf("\033[1;31m""Open File:%s Error\n""\033[0m", _au8SrcFile);
    }

    printf("\033[1;32m""Open File:%s\n""\033[0m", _au8DstFile);
    _hDstFile = open(_au8DstFile, O_WRONLY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
    if(_hDstFile <= 0)
    {
        printf("\033[1;31m""Open File:%s Error\n""\033[0m", _au8DstFile);
    }

    pu8Buf = malloc(VESFILE_READER_BATCH);
    memset(pu8Buf, 0, VESFILE_READER_BATCH);

    while (1)
    {
        ///frame mode, read one frame lenght every time
        memset(au8Header, 0, 16);
        u32Pos = lseek(_hSrcFile, 0L, SEEK_CUR);
        u32ReadLen = read(_hSrcFile, au8Header, 16);
        if(u32ReadLen <= 0)
        {
            printf("File read over!\n");
            break;
        }

        u32FrameLen = MI_U32VALUE(au8Header, 4);
        if(u32FrameLen > VESFILE_READER_BATCH)
        {
            printf("err, file buffer cache too small!\n");
            break;
        }

        u32ReadLen = read(_hSrcFile, pu8Buf, u32FrameLen);
        if(u32ReadLen <= 0)
        {
            printf("File read err!\n");
            break;
        }

        u32WriteLen = write(_hDstFile, pu8Buf, u32ReadLen);
        if(u32WriteLen <= 0)
        {
            printf("File write err!\n");
            break;
        }
    }

    if (_hSrcFile >= 0)
    {
        close(_hSrcFile);
    }

    if (_hDstFile >= 0)
    {
        close(_hDstFile);
    }

    return 0;
}
