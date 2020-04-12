/*
**	********************************************************************************
**                                     adda_APIs
**
**   (c) Copyright 2010-2020, ZheJiang DaHua   Technology  CO.LTD.
**                            All Rights Reserved
**
**	File		: adda.h
**	Description	:
**	Modify		: 	xu_longfei1  Create the file
**	********************************************************************************
*/


//关键调用流程说明
#if 0

//****************************s********************************************
关键调用流程:
1)初始化流程

Adda_I2COpsRegister();//注册I2C读写方法，需要用户自己实现
			  |
Adda_HWResetOpsRegister();//注册I2C读写方法，需要用户自己实现
              |
Adda_Resource_Init(&AddaInitRes) ;//配置AD个数I2C地址、VO时钟模式、通道映射、音频配置、时钟源模式等,AddaInitRes详细配置实例见下文说明
              |
      Adda_Init()//初始化AD



AddaInitRes配置实例说明:


产品规格:8路模拟输入产品:2片DH9931+ 主控(3531A)


AddaInitRes.iCount = 2;/*产品一共使用了2片AD*/

/*第一片AD配置*/
AddaInitRes.AdRes[0].iI2CBusId = 0;
AddaInitRes.AdRes[0].iChipAddr = 0x60;
AddaInitRes.AdRes[0].ucChipIndex = 0;
AddaInitRes.AdRes[0].ucChnCount = 4;

/*通道映射配置，逻辑通道0 ~3 对应第一片物理通道3、2、1、0*/
AddaInitRes.AdRes[0].stChnMap[0].ucLogicChn = 0;
AddaInitRes.AdRes[0].stChnMap[0].ucPhyChn = 3;
AddaInitRes.AdRes[0].stChnMap[1].ucLogicChn = 1;
AddaInitRes.AdRes[0].stChnMap[1].ucPhyChn = 2;
AddaInitRes.AdRes[0].stChnMap[2].ucLogicChn = 2;
AddaInitRes.AdRes[0].stChnMap[2].ucPhyChn = 1;
AddaInitRes.AdRes[0].stChnMap[3].ucLogicChn = 3;
AddaInitRes.AdRes[0].stChnMap[3].ucPhyChn = 0;
AddaInitRes.AdRes[0].enEqMode = EQ_MODE_IN;/*使用内部EQ*/

/*配置每个VO口的时序模式*/
AddaInitRes.AdRes[0].stVdPortDev.ucCount=4;/*4个VO口均启用*/
AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enVdPortMuxMode = VD_PORT_MUX_MODE_1MUX;/*1路复用模式*/
AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enOutHeadMode = VIDEO_HEAD_SINGEL;/*单头模式*/
AddaInitRes.AdRes[0].stVdPortDev.stVdPort[0].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;/*8bit bt656模式*/
AddaInitRes.AdRes[0].stVdPortDev.stVdPort[1].enVdPortMuxMode = VD_PORT_MUX_MODE_1MUX;
AddaInitRes.AdRes[0].stVdPortDev.stVdPort[1].enOutHeadMode = VIDEO_HEAD_SINGEL;
AddaInitRes.AdRes[0].stVdPortDev.stVdPort[1].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;
AddaInitRes.AdRes[0].stVdPortDev.stVdPort[2].enVdPortMuxMode = VD_PORT_MUX_MODE_1MUX;
AddaInitRes.AdRes[0].stVdPortDev.stVdPort[2].enOutHeadMode = VIDEO_HEAD_SINGEL;
AddaInitRes.AdRes[0].stVdPortDev.stVdPort[2].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;
AddaInitRes.AdRes[0].stVdPortDev.stVdPort[3].enVdPortMuxMode = VD_PORT_MUX_MODE_1MUX;
AddaInitRes.AdRes[0].stVdPortDev.stVdPort[3].enOutHeadMode = VIDEO_HEAD_SINGEL;
AddaInitRes.AdRes[0].stVdPortDev.stVdPort[3].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;
AddaInitRes.AdRes[0].enDriverPower = DRIVER_POWER_18V;/*BT656的逻辑电平*/
AddaInitRes.AdRes[0].iClockSource = 0;/*AD时钟源使用外部晶振*/

/*第二片AD配置*/
AddaInitRes.AdRes[1].iI2CBusId = 0;
AddaInitRes.AdRes[1].iChipAddr = 0x62;
AddaInitRes.AdRes[1].ucChipIndex = 1;
AddaInitRes.AdRes[1].ucChnCount = 4;


/*通道映射配置，逻辑通道4 ~7 对应第二片物理通道3、2、1、0*/
AddaInitRes.AdRes[1].stChnMap[0].ucLogicChn = 4;
AddaInitRes.AdRes[1].stChnMap[0].ucPhyChn = 3;
AddaInitRes.AdRes[1].stChnMap[1].ucLogicChn = 5;
AddaInitRes.AdRes[1].stChnMap[1].ucPhyChn = 2;
AddaInitRes.AdRes[1].stChnMap[2].ucLogicChn = 6;
AddaInitRes.AdRes[1].stChnMap[2].ucPhyChn = 1;
AddaInitRes.AdRes[1].stChnMap[3].ucLogicChn = 7;
AddaInitRes.AdRes[1].stChnMap[3].ucPhyChn = 0;
AddaInitRes.AdRes[1].enEqMode = EQ_MODE_IN;

AddaInitRes.AdRes[1].stVdPortDev.ucCount=4;
AddaInitRes.AdRes[1].stVdPortDev.stVdPort[0].enVdPortMuxMode = VD_PORT_MUX_MODE_1MUX;
AddaInitRes.AdRes[1].stVdPortDev.stVdPort[0].enOutHeadMode = VIDEO_HEAD_SINGEL;
AddaInitRes.AdRes[1].stVdPortDev.stVdPort[0].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;
AddaInitRes.AdRes[1].stVdPortDev.stVdPort[1].enVdPortMuxMode = VD_PORT_MUX_MODE_1MUX;
AddaInitRes.AdRes[1].stVdPortDev.stVdPort[1].enOutHeadMode = VIDEO_HEAD_SINGEL;
AddaInitRes.AdRes[1].stVdPortDev.stVdPort[1].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;
AddaInitRes.AdRes[1].stVdPortDev.stVdPort[2].enVdPortMuxMode = VD_PORT_MUX_MODE_1MUX;
AddaInitRes.AdRes[1].stVdPortDev.stVdPort[2].enOutHeadMode = VIDEO_HEAD_SINGEL;
AddaInitRes.AdRes[1].stVdPortDev.stVdPort[2].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;
AddaInitRes.AdRes[1].stVdPortDev.stVdPort[3].enVdPortMuxMode = VD_PORT_MUX_MODE_1MUX;
AddaInitRes.AdRes[1].stVdPortDev.stVdPort[3].enOutHeadMode = VIDEO_HEAD_SINGEL;
AddaInitRes.AdRes[1].stVdPortDev.stVdPort[3].enVdPortOutFormat = VD_PORT_OUT_FORMAT_BT656;
AddaInitRes.AdRes[1].enDriverPower = DRIVER_POWER_18V;
AddaInitRes.AdRes[1].iClockSource = 0;



2)获取通道状态

Adda_GetVideoInStatus()//详细见接口说明


3)获取输入的信号类型
Adda_GetVideoInType()//详细见接口说明


4)手动强制指定输入信号类型
Adda_SetVideoInType()//详细见接口说明

5)其他辅助功能函数，见如下函数定义说明，不详细展开


**************************************************************************//

#endif



#ifndef __ADDA_LIB_H__
#define __ADDA_LIB_H__

typedef int				BOOL;

typedef signed char		s8;
typedef unsigned char	u8;

typedef u8				U8;
typedef	s8				S8;

typedef	s8				schar;
typedef	u8				uchar;

typedef	u8				BYTE;

typedef u8				uint8_t;
typedef s8				sint8_t;

typedef u8				uint8;
typedef s8				int8_t;

typedef	signed short	sint16_t;
typedef	unsigned short	uint16_t;

typedef uint16_t		uint16;
typedef uint16_t		u16;

typedef signed int		int32;
typedef unsigned int	uint32;

typedef signed int		Int32;

typedef	int32			int32_t;
typedef uint32			uint32_t;


#include "adda_resource.h"

typedef enum ad_video_in_format_e
{
	AD_VIDEO_IN_SD_NTSC = 0,
	AD_VIDEO_IN_SD_PAL,
	AD_VIDEO_IN_HD_720P_25HZ = 10,
	AD_VIDEO_IN_HD_720P_30HZ,
	AD_VIDEO_IN_HD_720P_50HZ,
	AD_VIDEO_IN_HD_720P_60HZ,
	AD_VIDEO_IN_HD_1080P_25HZ,
	AD_VIDEO_IN_HD_1080P_30HZ,
	AD_VIDEO_IN_HD_1080P_50HZ,
	AD_VIDEO_IN_HD_1080P_60HZ,
	AD_VIDEO_IN_HD_720I_50HZ,
	AD_VIDEO_IN_HD_720I_60HZ,
	AD_VIDEO_IN_HD_1080I_50HZ,
	AD_VIDEO_IN_HD_1080I_60HZ,
	/*笔记本常用分辨率*/
	AD_VIDEO_IN_HD_1600x900_60HZ = 22,
	AD_VIDEO_IN_HD_1440x900_60HZ,
	AD_VIDEO_IN_HD_1366x768_60HZ,
	AD_VIDEO_IN_HD_1280x1024_60HZ,
	AD_VIDEO_IN_HD_1280x800_60HZ,
	AD_VIDEO_IN_HD_1024x768_60HZ,
	AD_VIDEO_IN_HD_800x600_60HZ,
	AD_VIDEO_IN_HD_640x480_60HZ,
	AD_VIDEO_IN_HD_2560x1440_25HZ,	  /*2560x1440*/
	AD_VIDEO_IN_HD_2560x1440_30HZ,	  /*2560x1440*/
	AD_VIDEO_IN_HD_3840x2160_12HZ,	  /*3840x2160*/
	AD_VIDEO_IN_HD_3840x2160_15HZ,	  /*3840x2160*/
	AD_VIDEO_IN_HD_3840x2160_25HZ,	  /*3840x2160*/
	AD_VIDEO_IN_HD_3840x2160_30HZ,	  /*3840x2160*/
	AD_VIDEO_IN_HD_2560x1440_15HZ,	  /*2560x1440 15FPS*/
	AD_VIDEO_IN_HD_2048x1536_25HZ,	  /*2048x1536*/
	AD_VIDEO_IN_HD_2048x1536_30HZ,	  /*2048x1536*/
	AD_VIDEO_IN_HD_2048x1536_18HZ,	  /*2048x1536 18FPS*/
	AD_VIDEO_IN_HD_2592x1944_12HZ,	  /*5M 2592x1944 12FPS*/
	AD_VIDEO_IN_HD_2592x1944_20HZ,	  /*5M 2592x1944 20FPS*/
	AD_VIDEO_IN_HD_1920x1536_18HZ,    /*3M 1920x1536 18FPS*/
}AD_VIDEO_IN_FORMAT_E;

typedef struct AdDetectStatus_s
{
  int32 iVideoFormat;/*取值AD_VIDEO_IN_FORMAT_E*/
  int32 iLostStatus;
  int32 iReportFormat;/*上报使用制式*/
  int32 iVideoSignalType; /* AD上报的当前前端的信号类型, VideoType_e*/
  int32 iVideoChipType;/*当前AD芯片类型，取枚举类型 VIDEO_CHIP_E */
  int32 reserved[3];
}AD_DETECT_STATUS_S;

typedef struct AddaResInitParam_s
{
	uint32 u32ProductType; //产品类型
	uint32 u32VideoCapMask; //高标清混合设备采集类型掩码

	int reserved[8];
}AddaResInitParam_S;

/**分辨率相关信息，*不同ADDA可考虑用不同的参数*/
typedef union VideoResolution_u
{
    struct _ResInfo0_s
    {
        int width; 		/*宽度*/
        int height; 	/*高度*/
        int frmrate; 	/*帧率*/
        int Interlaed; 	/*隔行或逐行扫描*/
    }ResInfo0_s;

    struct _ResInfo1_s
    {
        int activeWidth; 	/*宽度*/
        int activeHeight; 	/*高度*/
        int toltalWidth; 	/*帧率*/
        int toltalHeight; 	/*隔行或逐行扫描*/
    }ResInfo1_s;
    uint8 reserved[64];
}VideoResolution_u;


/** 视频EQ 参数获取*/
typedef struct VideoEqParam_s
{
    int32 iCurrEqLevel; //当前EQ等级
    int32 iOptimumEqLevel; //当前接入EQ最佳等级
    int32 res[14]; //保留字节
}VideoEqParam_s;


/** 视频EQ 参数设置 */
typedef struct VideoEqSetParam_s
{
    int32 iEqSetMode; //EQ等级设置模式 0:偏差值模式  1:绝对值模式，取值video_in_eq_set_mode_e
    int32 iEqLevel; //EQ值 设置的时候范围为[-10, 10] 获取的时候范围为[0,10]
    int32 res[14]; //保留字节
}VideoEqSetParam_s;


/*视频信号类型*/
typedef enum VideoType_e
{
    VIDEO_TYPE_TV    = 0,
    VIDEO_TYPE_SDI   = 1,
    VIDEO_TYPE_VGA   = 2,
    VIDEO_TYPE_HDMI  = 3,
    VIDEO_TYPE_DVI   = 4,
    VIDEO_TYPE_HDCVI = 5,
    VIDEO_TYPE_AHD   = 6,
    VIDEO_TYPE_TVI   = 7,
    VIDEO_TYPE_AUTO  = 8,/*AUTO指信号类型不由应用指定（应用下发的也是AUTO），而是由AD自动检测并适应前端输入源，可以是CVI/AHD/TVI等的任意一种*/
    VIDEO_TYPE_CVBS  = 9,/*前端信号为CVBS*/
    VIDEO_TYPE_DETECTING  = 0x80,/*正在识别*/
    VIDEO_TYPE_UNKNOW  = 0x81,/*没有接入视频*/
    VIDEO_TYPE_BUT,
}VideoType_e;

typedef enum AdChipCategory_s
{
	CATE_VIDEO,   /* 视频类型 */
	CATE_AUDIO,	  /* 音频类型 */
	CATE_BUT
} AdChipCategory;

/*HDMI输出模式*/
typedef enum hdmiTransOutputMode_e
{
    HDMITRANS_MODE_DETECT_AUTO = 0,
    HDMITRANS_MODE_FORCE_HDMI  = 1,
    HDMITRANS_MODE_FORCE_DVI   = 2,
    HDMITRANS_MODE_NO_OUT      = 3,
    HDMITRANS_MODE_BUT
}hdmiTransOutputMode_e;


/**
 * 数据时钟触发方式
 * 资源里也定义了一套ClockEdge枚举,
 * 为了头文件映射方便，弄成独立的。
 * 即资源头文件中的ClockEdge只用于配置, 不能在任何API中使用
 */
typedef enum LatchDataClockEdge_e
{
	LATCH_DATA_CLK_UNKNOW = 0,
	LATCH_DATA_CLK_RISE_EDGE = 1, /* 上升沿或者正常相位 */
	LATCH_DATA_CLK_FALL_EDGE = 2, /* 下降沿或者180度相位 */
	LATCH_DATA_CLK_HIGH = 3,	  /* 高电平 */
	LATCH_DATA_CLK_LOW = 4,       /* 低电平 */
	LATCH_DATA_CLK_BUT,
}LatchDataClockEdge_e;

#ifndef CaptureUserLineType_e_GUARD
#define CaptureUserLineType_e_GUARD
typedef enum CaptureUserLineType_e
{
    USER_LINE_TYPE_COAXIAL = 0,//同轴
    USER_LINE_TYPE_UTP_10OHM,    //10欧姆阻抗双绞线
    USER_LINE_TYPE_UTP_17OHM,    //17欧姆阻抗双绞线
    USER_LINE_TYPE_UTP_25OHM,    //25欧姆阻抗双绞线
    USER_LINE_TYPE_UTP_35OHM,    //35欧姆阻抗双绞线

    USER_LINE_TYPE_BUT,
}CaptureUserLineType_e;
#endif

/*慢速升级为以前老的升级方式，快速升级为新的升级方式，新的升级方式根据速度不同
又可以分为两种升级方式，前端像机升级时会自动切换到720p制式，
快速升级可以在720p@25或720p@50两种制式下完成*/

typedef struct MetaParam
{
    int32_t iTransMode;    /*> 发送数据模式 0 普通模式，1 升级模式 2 正向数据关闭 3 反向数据关闭*/
    int32_t iSpeedMode;    /*升级速度模式，分快速升级和慢速升级*/
    int32_t iStdType;      /*升级制式，快速升级模式下又分两种升级方式，由制式区别*/
    int32_t aiRes[29];      /*> 保留字 */
}MetaParam_s;

/*AD 视频采集状态 0 正常 1寄存器异常*/
typedef enum ADDA_VIDEO_IN_STATE
{
    ADDA_VIDEO_IN_STATE_NORMAL,         /*正常状态*/
    ADDA_VIDEO_IN_STATE_REGISTER_ERR,   /*寄存器异常*/
}ADDA_VIDEO_IN_STATE;

typedef enum AudioInType_e
{
    AUDIO_IN_HDCVI = 1,
    AUDIO_IN_NORMAL = 2,
    AUDIO_IN_BUT,
}AudioInType_e;

/* AD 视频采集工作模式 */
typedef enum Adda_Video_In_Mode_e
{
    ADDA_VIDEO_IN_MODE_SD,   /* 标清模式 */
    ADDA_VIDEO_IN_MODE_HD,   /* 高清模式 */
}ADDA_VIDEO_IN_MODE_E;

typedef enum ADDA_META_UPDATE_SPEED
{
    ADDA_META_UPDATE_LOW_SPEED,  /*慢速升级*/
    ADDA_META_UPDATE_FSAT_SPEED, /*快速升级*/
}ADDA_META_UPDATE_SPEED;

#ifndef VIDEO_COLOR_GUARD
#define VIDEO_COLOR_GUARD
/// 视频颜色格式，所有字段非法值为255
typedef struct VIDEO_COLOR
{
	BYTE	Brightness;		///< 亮度，取值0-100。
	BYTE	Contrast;		///< 对比度，取值0-100。
	BYTE 	Saturation;		///< 饱和度，取值0-100。
	BYTE 	Hue;			///< 色调，取值0-100。
	BYTE 	Gain;			///< 增益，取值0-100。bit7置位表示自动增益，其他位被忽略。
	BYTE	WhiteBalance;	///< 自动白电平控制，bit7置位表示开启自动控制.0x0,0x1,0x2分别代表低,中,高等级
	BYTE	Sharpness;		///< 锐度，取值0-15。
	BYTE	Phase;			///<相位,取值0-100
}VIDEO_COLOR;
#endif

/// 视频位置调整
#ifndef VIDEO_POSITION_GUARD
#define VIDEO_POSITION_GUARD
typedef struct VIDEO_POSITION{
 int    TopOffset;         ///上下偏移0--1024
 int    LeftOffset;         ///左右偏移0--1024
 int    reserved[2];      //保留
}VIDEO_POSITION;
#endif


#ifndef Size_GUARD
#define Size_GUARD
/// 尺寸
typedef struct Size
{
	int w;
	int h;
} Size;
#endif

#ifdef __cplusplus
 extern "C" {
#endif


int32 Adda_DumpReg(int32 iChannel);


///初始化ADDA库, 调用其他接口前必须先调用该函数
/// \param none
/// \retval 0  成功
/// \retval <0  失败
int32_t Adda_Resource_Init(ADS_RES_S *pAdsRes);

///去初始化ADDA库, 调用其他接口前必须先调用该函数
/// \param none
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_Resource_DeInit(void);

///I2C 读写方法注册
/// \param in pWriteRegFunc I2C写实现函数
/// \param in pReadRegFunc I2C读实现函数
/// \retval 0  成功
/// \retval <0  失败

int32 Adda_I2COpsRegister(uint8_t (* pWriteRegFunc)(uint8_t i2c_addr,uint16_t reg_addr, uint8_t val, uint8_t bus_id),uint8_t (* pReadRegFunc)(uint8_t i2c_addr, uint16_t reg_addr, uint8_t bus_id));

///AD 硬件复位管脚的复位方法注册函数
/// \param in pRestFunc 硬件复位函数，需要用户实现，依赖于实际的硬件连接

int32 Adda_HWResetOpsRegister(void (* pResetFunc)(void));

///\初始化ADDA芯片
///
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_Init(void);

///\去初始化ADDA芯片
///
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_DeInit(void);


/*********************************视频采集输入相关接口**********************************************/

///\设置视频输入信号类型
///
///\param [in]  iChannel 通道号
///\param [in]  videoType，参考 videoType_e
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_SetVideoInType(int32 channel, VideoType_e videoType);

///\获取视频输入信号类型
///
///\param [in]  iChannel 通道号
///\param [out]  videoType，参考 videoType_e
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_GetVideoInType(int32 channel, VideoType_e *videoType);


///\调整输入图像的参数
///\param [in]  iChannel 通道号，
///\param [in]  pstColor 色彩参数指针
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_SetVideoInColor(int32 iChannel, const VIDEO_COLOR *pstColor);


/// 调整输入图像的参数
///
/// \param [in] iChannel 应用逻辑通道号。
/// \param [out] pColor 指向颜色结构VIDEO _ COLOR的指针。
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_GetVideoInColor(int32 iChannel, VIDEO_COLOR *pstColor);

/// \输出视频偏移调节
///
/// \在获取的参数基础上调节
/// \param [in] iChannel 应用逻辑通道号。
/// \param [in] pos VIDEO_POSITION偏移参数指针
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_SetVideoInPosition (int32 iChannel, const VIDEO_POSITION *pstPosition);


/// \获取输入视频偏移
///
/// \param [in] iChannel 应用逻辑通道号。
/// \param [out] pos VIDEO_POSITION偏移参数指针
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_GetVideoInPosition (int32 iChannel, VIDEO_POSITION *pstPosition);


/// \获取当前逻辑通道状态
/// \param [in] iChannel 通道号
/// \param [in] pStatus 通道丢失状态
/// \retval  0  获取成功
/// \retval <0  获取失败
int32 Adda_GetVideoInStatus(int32 iChannel, AD_DETECT_STATUS_S *pStatus);

///\发送辅助数据到前端设备堵塞接口
///
/// \param [in] channel 通道号。
/// \param [in] buf  数据buf
/// \param [in] buf 长度
/// \retval 0  成功
/// \retval <0  失败
Int32 Adda_PutVideoInMetaData(int32 iChannel, const void *buf, uint32 len);

 ///\发送辅助数据到前端设备非堵塞接口
///
/// \param [in] channel 通道号。
/// \param [in] buf  数据buf
/// \param [in] buf 长度
/// \retval 0  成功
/// \retval <0  失败
Int32 Adda_PutVideoInMetaDataNoBlock(int32 iChannel, const void *buf, uint32 len);

///\接收前端设备返回的辅助数据
///
///\适用芯片：DH9901
/// \param [in] channel 通道号。
/// \param [in out] buf  数据buf
/// \param [in out] buf 长度
/// \retval 0  成功
/// \retval <0  失败
Int32 Adda_GetVideoInMetaData(int32 iChannel, void *buf, uint32 *len);


/// \设置同轴升级的属性
///
/// \param[in] iChannel视频 通道号
/// \iMetaParam 升级属性
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_SetVideoInMetaParam(int32 iChannel, MetaParam_s iMetaParam);


/// \重置图像均衡
///
///\适用芯片：DH9901
/// \param[in] iChannel 通道号
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_SetVideoInEquation(int32 iChannel);


///设置传输线类型，需要根据线类型配置相应的AD参数
///\param [in]ichannel 通道号
///\param [in]elinetype 输入连接线的类型
//\param  [out]返回0表示成功
int32 Adda_SetVideoInLineType(int32 iChannel, CaptureUserLineType_e elinetype);


/// \AD 手动设置视频输入EQ参数
///
/// \param[in]  iChannel  视频输入通道号
/// \param[in]  EqParam   Eq配置
/// \retval 0   成功
/// \retval <0  失败
int32 Adda_SetVideoInEqParam(int32 iChannel, const VideoEqSetParam_s *pEqParam);

/// \AD 获取视频输入EQ参数
///
/// \param[in]  iChannel  视频输入通道号
/// \param[in]  EqParam   Eq配置
/// \retval 0   成功
/// \retval <0  失败
int32 Adda_GetVideoInEqParam(int32 iChannel, VideoEqParam_s *pEqParam);


/*********************************视频显示输出相关接口**********************************************/

///\设置输出视频参数
///\param [in]  iChannel 通道号，
///\param [in]  pstColor色彩参数指针
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_SetVideoOutColor(int32 iChannel, const VIDEO_COLOR *pstColor);


/// 获取输出视频的颜色参数。
///
/// \param [in] iChannel 应用逻辑通道号。
/// \param [in] pColor 指向颜色结构VIDEO _ COLOR的指针。
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_GetVideoOutColor(int32 iChannel, VIDEO_COLOR *pstColor);

/// \输出视频偏移调节
/// \在获取的参数基础上调节
/// \param [in] iChannel 应用逻辑通道号。
/// \param [in] pos VIDEO_POSITION偏移参数指针
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_SetVideoOutPosition(int32 iChannel, const VIDEO_POSITION * pstPosition);

/// \获取视频偏移
///
/// \param [in] iChannel 应用逻辑通道号。
/// \param [out] pos VIDEO_POSITION偏移参数指针
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_GetVideoOutPosition(int32 iChannel, VIDEO_POSITION * pstPosition);

/*********************************音频输入输出相关接口**********************************************/

/// 切换音频输出通道。
///
/// \param [in] iChannel 音频通道号。
/// \retval 0  切换成功。
/// \retval <0  切换失败。
int32 Adda_SetAudioOutCh(int32 iChannel);


/// 静音输出。
///
/// \param [in] iChannel 音频通道号。
/// \retval 0  切换成功。
/// \retval <0  切换失败。
int32 Adda_MuteAudioOut(int32 iChannel);


/// \输出音量调节
///
/// \param [in] iChannel 应用逻辑通道号。
/// \param [in] ucValue 音量值0 - 100
/// \retval 0  设置成功
/// \retval <0  设置失败
int32 Adda_SetAudioOutVolume(int32 iChannel, uint8 ucValue);

/// \输入音量调节
///
/// \param [in] iChannel 应用逻辑通道号。
/// \param [in] ucValue 音量值0 - 100
/// \retval 0  设置成功
/// \retval <0  设置失败
int32 Adda_SetAudioInVolume(int32 iChannel, uint8 ucValue);


/// \ 设置音频输入通道使能
///
///\param [in] i32Port 音频输入通道号。
///\param [in] enable  使能0:关闭;非0:开启
/// \retval 0  设置成功。
/// \retval <0  设置失败。
int32 Adda_SetAudioInEnable(int32_t iChannel, int32_t iEnable);

/// \AD 设置音频源
///
/// \param[in]  AudioInCh  音频输入通道号
/// \param[in]  pAudioInType   音频源类型
/// \retval 0   成功
/// \retval <0  失败
int32 Adda_SetAudioInSource(int32 iChannel, AudioInType_e AudioInType);


/// \获取ADDA工作状态 用于检测AD是否处于复位等异常状态
///
/// \param[in] iChannel 通道号
/// \无返回值
int32 Adda_GetDeviceState(int32 iChannel, ADDA_VIDEO_IN_STATE *pVideoInState);

/// \获取指定通道对应AD的工作状态
///
/// \param[in] iChannel 通道号
/// \param[in] eAdCate 芯片类型
/// \param[out] pVideoInState 芯片状态
/// \retval 0  成功
/// \retval <0  失败
int32 Adda_GetChipState(int32 iChannel, AdChipCategory eAdCate, ADDA_VIDEO_IN_STATE *pVideoInState);

/// \获取指定通道对应的芯片号
///
/// \param[in] iChannel 通道号
/// \param[in] eAdCate 芯片类型
/// \retval >=0: 通道对应的芯片号
/// \retval <0:  失败
int32 Adda_GetAdIndex(int iChannel, AdChipCategory eAdCate);


#ifdef __cplusplus
}
#endif


#endif
