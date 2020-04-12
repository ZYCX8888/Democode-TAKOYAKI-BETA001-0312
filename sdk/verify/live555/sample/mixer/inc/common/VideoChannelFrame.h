#ifndef _VIDEOCHANNELSTREAM_H_
#define _VIDEOCHANNELSTREAM_H_

#include "ChannelFrame.h"
#include "mid_common.h"
#include "mid_sys.h"

class VideoChannelFrame // : public CChannelFrame
{
public:
    /*
        * 构造VideoChannelFrame
        * streamtype: 视频通道号
        * length: FIFO长度参数，尽量使用默认设置
    */
    VideoChannelFrame(MI_U8 ch);
    ~VideoChannelFrame();

    /*
        * 开启视频帧获取
        * return: null
    */
    MI_S32 Open(MI_U8 streamtype, MI_U32 length = FIFO_LEN_DEF);

    /*
        * 关闭视频帧获取
        * return: null
    */
    MI_S32 Close();


    bool   State();

    /*
        * 获取一帧数据
        * pFrameInf: 帧数据存放结构体
        * return: 0=失败 1=成功
    */
    MI_S32 GetOneFrame(FrameInf_t &pFrameInf);

    MI_S32 SetbFlagNeedIFrame();
    MI_S32 CleanFrameBuf();
    /*
        * 释放一帧数据
        * pFrameInf: 帧数据存放结构体
        * return: 0=成功
    */
    MI_S32 ReleaseOneFrame(FrameInf_t &pFrameInf);
    MI_S32 OnData(const FrameInf_t &pFrameInf);
private:
    //pthread_mutex_t m_mutex;
    MI_U8 NeedIFrame;
    MI_U8 m_Channel;
    MI_S32 m_StreamType;
    CChannelFrame *m_CChannelFrame;
};

#endif

