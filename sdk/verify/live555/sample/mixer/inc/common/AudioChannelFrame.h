#ifndef _AUDIOCHANNELSTREAM_H_
#define _AUDIOCHANNELSTREAM_H_


#include "mid_common.h"
#include "mid_sys.h"
#include "ChannelFrame.h"

#define AUDIO_FIFO_LEN_DEF 8

class AudioChannelFrame//: public CChannelFrame
{
public:
    /*
        * 构造AudioChannelFrame
        * length: FIFO长度参数，尽量使用默认设置
        * m_nChnId: 音频通道号
    */
    AudioChannelFrame(MI_U32 length = AUDIO_FIFO_LEN_DEF, MI_U8 m_nChnId = 0x0);
    ~AudioChannelFrame();

    /*
        * 获取一帧数据
        * pFrameInf: 帧数据存放结构体
        * return: 0=失败 1=成功
    */
    MI_S32 GetOneFrame(FrameInf_t &pFrameInf);

    /*
        * 释放一帧数据
        * pFrameInf: 帧数据存放结构体
        * return: 0=成功
    */
    MI_S32 ReleaseOneFrame(FrameInf_t &pFrameInf);

    /*
        * 开启音频帧获取
        * return: null
    */
    MI_S32 Open();
    /*
        * 关闭音频帧获取
        * return: null
    */
    MI_S32 Close();
    bool State();
    MI_S32 OnData(const FrameInf_t &objFrame);
private:



    MI_U8 m_nId;
    CChannelFrame *m_CChannelFrame;
};

#endif

