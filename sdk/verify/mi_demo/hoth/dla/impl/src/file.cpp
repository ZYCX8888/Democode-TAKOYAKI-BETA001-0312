#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mi_common.h"
#include "mi_sys.h"
#include "mi_venc.h"

#include "file.h"

File::File()
{
}
File::~File()
{
}
void File::Init()
{
}
void File::Deinit()
{
    mapInputWrFile.clear();
}
void File::BindBlock(stModInputInfo_t & stIn)
{
    std::string strWriteFileName;

    strWriteFileName = GetIniString(stIn.curIoKeyString, "FILE_WRITE_PATH");
    int intWrFd = open(strWriteFileName.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0777);
    if (intWrFd < 0)
    {
        printf("dest_file: %s.\n", strWriteFileName.c_str());
        perror("open");
        return;
    }
    mapInputWrFile[stIn.curIoKeyString] = intWrFd;
    CreateReceiver(stIn.curPortId, DataReceiver, NULL, NULL, (void *)intWrFd);
    StartReceiver(stIn.curPortId);
}
void File::UnBindBlock(stModInputInfo_t & stIn)
{
    std::map<std::string, int>::iterator it;

    StopReceiver(stIn.curPortId);
    DestroyReceiver(stIn.curPortId);
    it = mapInputWrFile.find(stIn.curIoKeyString);
    if (it != mapInputWrFile.end())
    {
        close(it->second);
    }
}
void File::DataReceiver(void *pData, unsigned int dataSize, void *pUsrData)
{
    MI_SYS_BufInfo_t *pstBufInfo = NULL;
    MI_VENC_Stream_t *pstStream = NULL;
    int intFd = 0;

    intFd = (int)pUsrData;
    if (sizeof(MI_SYS_BufInfo_t) == dataSize)
    {
        pstBufInfo = (MI_SYS_BufInfo_t *)pData;
        switch (pstBufInfo->stFrameData.ePixelFormat)
        {
            case E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420:
            {
                //do yuv420 copy...
            }
            break;
            case E_MI_SYS_PIXEL_FRAME_YUV422_YUYV:
            case E_MI_SYS_PIXEL_FRAME_ARGB8888:
            {
            }
            break;
            default:
                printf("Not support!!\n");
                assert(0);
                break;
        }
    }
    else if (sizeof(MI_VENC_Stream_t) == dataSize)
    {
        pstStream = (MI_VENC_Stream_t *)pData;
        for (MI_U8 i = 0; i < pstStream->u32PackCount; i++)
        {
            write(intFd, (void *)pstStream->pstPack[i].pu8Addr, pstStream->pstPack[i].u32Len);
        }
    }
    else
    {
        assert(0);
    }
}

