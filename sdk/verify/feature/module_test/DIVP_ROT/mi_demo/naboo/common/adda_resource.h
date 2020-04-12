/******************************************************************************

                  版权所有 (C), 2013-2023, 浙江大华技术股份有限公司

 ******************************************************************************
  文 件 名   : Adda_resource.h
  版 本 号   : 初稿
  作    者       : xu_longfei
  生成日期: 2013年11月19日
  最近修改:
  功能描述: Adda_resource.h 的头文件
  函数列表:
  修改历史:
  1.日    期    : 2013年11月19日
    作    者     : xu_longfei
    修改内容   : 创建文件

******************************************************************************/

#ifndef _AD_RESOURCE_H_
#define _AD_RESOURCE_H_

//#include "i2c_resource.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define MAX_AD_NUM (24)


/*AD 最大逻辑通道号，与输入逻辑顺序对应*/
#define MAX_AD_LOGIC_CHN_NUM    (32)
/*DA 最大逻辑通道号，与输出逻辑顺序对应*/
#define MAX_DA_LOGIC_CHN_NUM    (10)
/*语音对讲最大逻辑通道号*/
#define MAX_MIC_LOGIC_CHN_NUM   (2)

/*通信方式*/
typedef enum Ad_Commut_e
{
    AD_COMMUT_UNKNOW = 0,
    AD_COMMUT_I2C = 1,
    AD_COMMUT_PCI = 2,
    AD_COMMUT_I2CSWITCH = 3,
    AD_COMMUT_BUT,
} AD_COMMUT_E;

/*AD支持输出分辨率*/
typedef enum vd_out_resolution_e
{
    VD_OUT_RESOLUTION_UNKNOW  = 0,
    VD_OUT_RESOLUTION_D1        = 1,
    VD_OUT_RESOLUTION_960H  = 2,
    VD_OUT_RESOLUTION_480P    = 3,
    VD_OUT_RESOLUTION_576P    = 4,
    VD_OUT_RESOLUTION_720P25    = 5,
    VD_OUT_RESOLUTION_720P30    = 6,
    VD_OUT_RESOLUTION_720P50  = 7,
    VD_OUT_RESOLUTION_720P60  = 8,
    VD_OUT_RESOLUTION_720I50  = 9,
    VD_OUT_RESOLUTION_720I60  = 10,
    VD_OUT_RESOLUTION_1080P25 = 12,
    VD_OUT_RESOLUTION_1080P30 = 13,
    VD_OUT_RESOLUTION_1080P50 = 14,
    VD_OUT_RESOLUTION_1080P60 = 15,
    VD_OUT_RESOLUTION_1080I50 = 16,
    VD_OUT_RESOLUTION_1080I60 = 17,
    VD_OUT_RESOLUTION_BUT,
} VD_OUT_RESOLUTION_E;

/*AD支持输出格式*/
typedef enum video_out_format_e
{
    VIDEO_OUT_FORMAT_UNKNOW  = 0,
    VIDEO_OUT_FORMAT_HDMI_RGB    = 1,
    VIDEO_OUT_FORMAT_HDMI_YUV444 = 2,
    VIDEO_OUT_FORMAT_HDMI_YUV422 = 3,
    VIDEO_OUT_FORMAT_DVI_RGB  = 4,
    VIDEO_OUT_FORMAT_CVBS  = 5,
    VIDEO_OUT_FORMAT_BUT,
} VIDEO_OUT_FORMAT_E;


/*采集数据时钟触发方式*/
typedef enum LATCH_DATA_CLK_EDGE_e
{
    LATCH_CLK_UNKNOW = 0,
    LATCH_CLK_LEVEL = 1,/*电平触发*/
    LATCH_CLK_EDGE = 2, /*边沿触发*/
    LATCH_CLK_RISE_EDGE = 3,
    LATCH_CLK_FALL_EDGE = 4,
    LATCH_CLK_LOW = 5,
    LATCH_CLK_HIGH = 6,
    LATCH_CLK_BUT = 7,
} LATCH_DATA_CLK_EDGE_E;

/*AD或DA芯片功能*/
typedef enum ad_da_fuction_e
{
    AD_DA_FUCTION_UNKNOW = 0,
    AD_DA_FUCTION_AUDIO  = 1, /*只支持音频*/
    AD_DA_FUCTION_VIDEO = 2,/*支持视频*/
    AD_DA_FUCTION_VIDEO_BLIT = 3, /*2826做视频拼接*/
    AD_DA_FUCTION_VIDEO_TRANS = 4,/*用作输出视频转换*/
    AD_DA_FUCTION_PTZ_485 = 5, /*485 云台*/
    AD_DA_FUCTION_BUT,
} AD_DA_FUCTION_E;

/*VD PORT 模式复用方式*/
typedef enum vd_port_mux_mode_e
{
    VD_PORT_MODE_UNKNOW = 0,
    VD_PORT_MUX_MODE_1MUX = 1,      // 1X 模式
    VD_PORT_MUX_MODE_2MUX = 2,      // 2X 模式
    VD_PORT_MUX_MODE_4MUX = 4,      // 4X 模式
    VD_PORT_MUX_MODE_1MUXHALF = 5,  // 1X half模式
    VD_PORT_MUX_MODE_2MUXHALF = 6,  // 2X half模式
    VD_PORT_MUX_MODE_4MUXHALF = 7,  // 4X half模式
    VD_PORT_MUX_MODE_BUT,
} VD_PORT_MUX_MODE_E;

/*视频输出格式*/
typedef enum vd_port_out_format_e
{
    VD_PORT_OUT_FORMAT_UNKNOW = 0,
    VD_PORT_OUT_FORMAT_BT656  = 1,
    VD_PORT_OUT_FORMAT_BT1120 = 2,
    VD_PORT_OUT_FORMAT_BT601  = 3,
    VD_PORT_OUT_FORMAT_BUT,
} VD_PORT_OUT_FORMAT_E;


/*视频输出格式*/
typedef enum vd_port_CLK
{
    VD_PORT_CKL_AUTO, // 自动配置
    VD_PORT_CKL_27M,  //27MHZ
    VD_PORT_CKL_36M ,//36MHZ
    VD_PORT_CKL_37_125M,//37.125MHZ
    VD_PORT_CKL_74_25M,//74.25MHZ
    VD_PORT_CKL_148_5M,//148.5MHZ
    VD_PORT_CKL_297M,//297MHZ
    VD_PORT_CKL_144M,//155MHZ
    VD_PORT_CKL_288M,//288MHZ
} VD_PORT_CLK_E;

/*视频输出格式*/
typedef enum vd_port_EdgeMode
{
    VD_PORT_EDGE_UP,
    VD_PORT_EDGE_DUAL,
} VD_PORT_EDGE_E;

/*音频输出格式*/
typedef enum audio_port_out_format_e
{
    AUDIO_PORT_OUT_FORMAT_UNKNOW = 0,
    AUDIO_PORT_OUT_FORMAT_I2S  = 1,
    AUDIO_PORT_OUT_FORMAT_DSP  = 2,
    AUDIO_PORT_OUT_FORMAT_BUT,
} AUDIO_PORT_OUT_FORMAT_E;

typedef enum video_head_mode_e
{
    VIDEO_HEAD_SINGEL = 0,  /*单头采集 高清标清都可以使用bt656 建议使用此采集方式*/
    VIDEO_HEAD_DOUBLE = 1,  /*双采集 高清需要使用bt1120*/
    VIDEO_HEAD_BUT    = 2,
} VIDEO_HEAD_MODE_E;

typedef struct Vd_Port_Caps_s
{
    uint32  enSupportVdPortOutFormat;/*port 口输出时序VD_PORT_OUT_FORMAT_E*/
    uint32  enSupportVdPortMuxMode;/*port 口复用方式VD_PORT_MUX_MODE_E*/
} Vd_Port_Caps_S;

//最大的VD PORT数量
#define MAX_VD_PORT_COUNT (4)
#define MAX_VD_CHN_PER_PORT (4)

/*视频输出口配置*/
typedef struct vd_port_s
{
    U8 ucPortId;  /*prot口*/
    Vd_Port_Caps_S Vd_Port_Caps;
    VD_PORT_CLK_E enVdportClk;//BT656/bt1220 时钟选择
    VD_PORT_EDGE_E enVdportEdge;//BT656/bt1220边沿选择
    VD_PORT_OUT_FORMAT_E enVdPortOutFormat;       /*port 口输出时序*/
    VD_PORT_MUX_MODE_E enVdPortMuxMode;            /*port 口复用方式*/
    VIDEO_HEAD_MODE_E enOutHeadMode;               /*头模式 0为单头 1为双头 此模式对9901无效 且只能在1X采集的时候有效*/
    U8 ucOutSequence[MAX_VD_CHN_PER_PORT];         /*视频输出顺序*/
    U8 uClkDelay;                                  /*Clk Delay 0-0xf*/
} VD_PORT_S;

typedef struct vd_port_dev_s
{
    U8 ucCount;     /*配置的port口数*/
    VD_PORT_S stVdPort[MAX_VD_PORT_COUNT];
} VD_PORT_DEV_S;

/*音频级联主从模式*/
typedef enum audio_cascade_mode_e
{
    AUDIO_CASCADE_UNKOWN = 0,
    AUDIO_CASCADE_MASTER = 1,
    AUDIO_CASCADE_SLAVE1 = 2,
    AUDIO_CASCADE_SLAVE2 = 3,
    AUDIO_CASCADE_SLAVE3 = 4,
    AUDIO_CASCADE_BUT
} AUDIO_CASCADE_MODE_E;

/*音频时钟主从模式*/
typedef enum audio_clk_slave_mode_e
{
    AUDIO_CLK_UNKOWN = 0,
    AUDIO_CLK_SLAVE  = 1,
    AUDIO_CLK_MASTER = 2,
    AUDIO_CLK_BUT
} AUDIO_CLK_SLAVE_MODE_E;

/*视频采集同步方式*/
typedef enum video_sample_sync_mode_e
{
    SYNC_MODE_UNKOWN  = 0,
    SYNC_MODE_EMBEDED = 1, /*内同步*/
    SYNC_MODE_EXTEND  = 2, /*外同步*/
    SYNC_MODE_BUT
} VIDEO_SAMPLE_SYNC_MODE_E;

/*视频采集信号源格式*/
typedef enum video_source_format_e
{
    VIDEO_SOURCE_FORMAT_UNKOWN  = 0,
    VIDEO_SOURCE_FORMAT_BT1120  = 1,
    VIDEO_SOURCE_FORMAT_RGB888  = 2,
    VIDEO_SOURCE_FORMAT_BUT
} VIDEO_SOURCE_FORMAT_E;

typedef enum ad_audio_chn_mux_e
{
    AD_AUDIO_CHN_NONE  = 0,
    AD_AUDIO_CHN_1MUX  = 1,
    AD_AUDIO_CHN_2MUX  = 2,
    AD_AUDIO_CHN_4MUX  = 3,
    AD_AUDIO_CHN_8MUX  = 4,
    AD_AUDIO_CHN_16MUX = 5,
    AD_AUDIO_CHN_BUT
} AD_AUDIO_CHN_MUX_E;

typedef struct audio_port_s
{
    U8 ucPortId;  /*port口*/

    AD_AUDIO_CHN_MUX_E enChnMulti;  /*输出通道数*/
    AUDIO_CLK_SLAVE_MODE_E enSlaveMode; /*时钟主从模式*/
    AUDIO_PORT_OUT_FORMAT_E enAudioOutFormat; /*音频输出模式*/
} AUDIO_PORT_S;

typedef struct audio_connect_mode_s
{
    BOOL ucCascade; /*音频是否级联 ture / false*/
    AUDIO_CASCADE_MODE_E enCascadeMode;
    int iCascadeNum; /*音频级联数量*/
} AUDIO_CONNECT_MODE_S;

#define MAX_AUDIO_PORT_NUM (2)
typedef struct audio_port_dev_s
{
    U8 ucAuDACPowerDown;            /*是否使用音频DAC 默认0为打开 1为关闭*/
    U8 ucAudioPortCount;            /*使用PORT数目*/
    AUDIO_CONNECT_MODE_S stAudioConnect;  /*是否级联*/
    AUDIO_PORT_S stAuDioPorts[MAX_AUDIO_PORT_NUM];
} AUDIO_PORT_DEV_S, *AUDIO_PORT_DEV_P;

/*音频采样率*/
typedef enum ad_audio_samplerate_e
{
    AD_SAMPLE_RATE_8000,
    AD_SAMPLE_RATE_16000,
    AD_SAMPLE_RATE_32000,
    AD_SAMPLE_RATE_44100,
    AD_SAMPLE_RATE_48000,
    AD_SAMPLE_RATE_BUTT
} AD_AUDIO_SAMPLERATE_E;


/*音频芯片*/
typedef struct AUDIO_CHIP_PRI_RES_s
{
    u8 ucInPgaGain;        /* 音频输入模拟增益*/
    u8 ucOutPgaGain;       /* 音频输出模拟增益*/
    u8 ucInDigitGain;      /* 音频输入数字增益*/
    u8 ucOutDigitGain;     /* 音频输入数字增益*/
    u8 ucBitWidth;         /*采样位宽*/
    u8 ucAudMasterCtrlMode;/*音频主从控制模式 主要音频级联(例techwell 0xdb)*/
    u8 ucAudioChnStart;    /* AD 芯片管辖通道*/
    u8 ucAudioChnCount;    /* 可能 需要根据不同芯片调整*/
    u8 aucSeque[8];        /*音频输入顺序*/
    AD_AUDIO_SAMPLERATE_E enSamlerate;/*采样率*/
} AUDIO_CHIP_PRI_RES_S, *AUDIO_CHIP_PRI_RES_P;

typedef struct VIDEO_CHIP_PRI_RES_s
{
    u8 ucVdClockPloarity; /*视频输出口clock极性*/
    u8 ucDatePhaseDelay;  /*数据相位延时*/
    u8 ucClockPhaseDelay; /*时钟相位延时*/
    u8 ucDataPinDriver;   /*数据线驱动能力*/
    u8 ucClkPinDriver;    /*时钟线驱动能力*/
    u8 ucVideoChnStart;   /* AD 芯片管辖通道*/
    u8 ucVideoChnCount;   /* 可能 需要根据不同芯片调整*/
    u8 aucSeque[8];       /*视频输入顺序*/
} VIDEO_CHIP_PRI_RES_S, *VIDEO_CHIP_PRI_RES_P;


#ifndef Size_GUARD
#define Size_GUARD
/// 尺寸
typedef struct Size
{
    int w;
    int h;
} Size;
#endif

#define CHNS_PER_CHIP_1     (1)
#define CHNS_PER_CHIP_4     (4)
#define CHNS_PER_CHIP_8     (8)

/*逻辑通道号与物理通道号对应关系*/
typedef struct chn_map_s
{
    u8 ucLogicChn; /*逻辑通道号*/
    u8 ucPhyChn;   /*芯片物理通道号*/
} CHN_MAP_S;

/*图像均衡器均衡模式*/
typedef enum eq_mode_e
{
    EQ_MODE_IN = 0,  /*内部均衡*/
    EQ_MODE_EXTEND = 1,/*外部均衡*/
} EQ_MODE_E;


/* 芯片IO驱动能力 */
typedef enum driver_power_e
{
    DRIVER_POWER_18V = 0,   /* 1.8V */
    DRIVER_POWER_33V = 1,   /* 3.3V */
} DRIVER_POWER_E;


typedef struct tagAdRes_s
{
    int32 iChipAddr;        /*芯片I2C地址*/
    int32 iBusId;
    u8 ucChipIndex;         /*当前AD 序号，AHD设备必须保证此序号CVI全部在前，AHD全部在后!!! 使用Adda_Inner_GetChipIndex的新方案无此要求，但要保证通道和芯片顺序一致*/
    u8 u8Bt656Reverse;      /*标记各个port口BT656是否需要反序，每位表示一个port口，高四位保留，低位起依次为 0、1、2、3 */
    u8 ucChnCount;          /*单片AD使用的通道数*/

    CHN_MAP_S stChnMap[CHNS_PER_CHIP_8];    /*通道映射信息*/

    EQ_MODE_E enEqMode;                     /*EQ方式*/
    VD_PORT_DEV_S stVdPortDev;              /*VO配置*/
    AUDIO_PORT_DEV_S stAudioPortDev;        /*音频配置*/
    DRIVER_POWER_E enDriverPower;           /* 9931芯片驱动能力，1.8V/3.3V */

    int32 res[4];
} AD_RES_S;

/*板级所有AD/DA芯片资源集合*/
typedef struct tagAdsRes_s
{
    int32 iCount;   /*AD /DA 数量*/
    int32 iChipTypeNum; /* AD/DA 种类总数 */
    int32 iProductType;
    int32 iCustomType;
    int32 iAddaVideoStandard;
    Size  stViCapLimit[MAX_AD_LOGIC_CHN_NUM]; /*采集限制，超过采集大小，不支持*/
    AD_RES_S AdRes[MAX_AD_NUM];
} ADS_RES_S;


/// 获取AD 配置资源信息
///
/// \param [in] pAdsRes AD资源信息结构指针
/// \param [out] 无
/// \return 0 成功，其他失败
int32 Ads_GetRes(ADS_RES_S *pAdsRes);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
