#ifndef _MID_IPU_INTERFACE_H_
#define _MID_IPU_INTERFACE_H_

#include "mid_common.h"
#include "mi_ipu.h"

#define MAX_VARIABLE_BUF_SIZE       7372800//(4 * 1024 * 1024)
#define BASE_BUF_SIZE               7372800//4608000//(2 * 1024 * 1024)

#define ABGR8888ALPHA(pixelval)        (((pixelval) >> 24) & 0xff)
#define ABGR8888BLUE(pixelval)      (((pixelval) >> 16) & 0xff)
#define ABGR8888GREEN(pixelval)        (((pixelval) >> 8) & 0xff)
#define ABGR8888RED(pixelval)         ((pixelval) & 0xff)
// XB
#define BGRA8888BLUE(pixelval)        (((pixelval) >> 24) & 0xff)
#define BGRA8888GREEN(pixelval)      (((pixelval) >> 16) & 0xff)
#define BGRA8888RED(pixelval)        (((pixelval) >> 8) & 0xff)
#define BGRA8888ALPHA(pixelval)     ((pixelval) & 0xff)

#define RGBA8888RED(pixelval)        (((pixelval) >> 24) & 0xff)
#define RGBA8888GREEN(pixelval)      (((pixelval) >> 16) & 0xff)
#define RGBA8888BLUE(pixelval)        (((pixelval) >> 8) & 0xff)
#define RGBA8888ALPHA(pixelval)     ((pixelval) & 0xff)

class CMidIPUInterface
{
public:
    CMidIPUInterface(IPU_InitInfo_S &stInitInfo)
    {
        memcpy(&m_stIPUInfo, &stInitInfo, sizeof(IPU_InitInfo_S));
    }

    virtual ~CMidIPUInterface()
    {
    }
	virtual MI_S32 SetIeParam(IeParamInfo tmp,MI_U8 scop) = 0;

    virtual void DealDataProcess() = 0;

protected:
    IPU_InitInfo_S          m_stIPUInfo;
};

MI_S32 IPUCreateDevice(char *pFirmwarePath,MI_U32 u32VarBufSize);
MI_S32 IPUDestroyDevice(void);
MI_S32 IPUCreateChannel(MI_U32 *s32Channel, char *pModelImage, MI_U32 u32InBufDepth, MI_U32 u32OutBufDepth);
MI_S32 IPUDestroyChannel(MI_U32 s32Channel, MI_U32 u32InBufDepth, MI_U32 u32OutBufDepth);

#endif // _MID_IPU_INTERFACE_H_

