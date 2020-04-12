
#ifndef _CUVC_H_
#define _CUVC_H_

#include "mid_common.h"
#include "mid_utils.h"
#include "mid_VideoEncoder.h"
#include "VideoChannelFrame.h"
#include "mid_VideoEncoder.h"

class CUvc
{
public:
    CUvc(MI_U32 ch, MI_VideoEncoder *pVideoEncoder);
    ~CUvc();
    MI_U32 GetFrameData(void *buf);
    MI_U32 RegisterVideoEncoder();
    MI_U32 UnRegisterVideoEncoder();
    MI_U32 FlushVideoBuff();
    MI_U32 RequestIDR();

    MI_BOOL m_bNeedIFrame;
    MI_VideoEncoder *mpVideoEncoder;
    VideoChannelFrame mUvcChannelFrame;
};

#endif
