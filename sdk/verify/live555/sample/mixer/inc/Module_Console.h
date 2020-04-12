/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  Module_Console.h
* Author:     fisher.yang@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2019/9/10
* Description: mixer record source file
*
*
*
* History:
*
*    1. Date  :        2019/9/10
*       Author:        fisher.yang@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/


#ifndef _MODULE_CONSOLE_H_
#define  _MODULE_CONSOLE_H_

#include "mi_common_datatype.h"
#include "mid_common.h"
#include "module_config.h"

#define PARAMLIST 16
#define PARAMLENGTH 255

#define  ASCII_CR       0x0d
#define  ASCII_LF       0x0a
#define  ASCII_BS       0x08
#define  ASCII_BELL     0x07
#define  ASCII_TAB      0x09
#define  ASCII_XON      0x11
#define  ASCII_XOFF     0x13
#define  ASCII_ESC      0x1B
#define  ASCII_DEL      0x7F
#define  ASCII_BACK     0x08



class  CConsoleManager
{
public:
    static CConsoleManager * instance();

    CConsoleManager();
    ~CConsoleManager();

    void StrParse(void);
    MI_BOOL RThreadProc(void);

    MI_S8 *pmbuf[PARAMLIST];

    void Console_SnrFramerate(void);
    void Console_SnrFramerateHelp(void);

    void Console_AudioVqeMode(void);
    void Console_AudioVqeModeHelp(void);

    void Console_VideoBitrate(void);
    void Console_VideoBitrateHelp(void);

    void  Console_VideoSuperFrameSize();
    void  Console_VideoSuperFrameSizeHelp();

    void  Console_VideoIrcut();
    void  Console_VideoIrcutHelp();

    void  Console_VideoAfMoto();
    void  Console_VideoAfMotoHelp();
    void Console_StreamLogOnOff(void);
    void Console_StreamLogOnOffHelp(void);

    void  Console_RecordAudioInData(void);
    void  Console_RecordAudioInDataHelp(void);

    void Console_AudioAEDParam(void);
    void Console_AudioAEDParamHelp(void);
    void Console_MdParam(void);
    void Console_MdParamHelp(void);

    void Console_VideoFps(void);
    void Console_VideoFpsHelp(void);

    void Console_VideoGop(void);
    void Console_VideoGopHelp(void);
    void Console_VideoIframeInterval(void);
    void Console_VideoIframeIntervalHelp(void);

    void Console_IspParam();
    void Console_IspParamHelp();

    void Console_VideoRequestIDR(void);
    void Console_VideoRequestIDRHelp(void);
    void Console_AudioOnOff(void);
    void Console_AudioOnOffHelp(void);
    void Console_VideoCodec(void);
    void Console_VideoCodecHelp(void);

    void Console_VideoRotate(void);
    void Console_VideoRotateHelp(void);
#if MIXER_SUPPORT_DIVP_BIND_CHANGE
    void Console_DivpBindChange(void);
    void Console_DivpBindChangeHelp(void);
#endif
    void Console_VideoMaskOSD(void);
    void Console_VideoMaskOSDHelp(void);
    void Console_VideoSlices(void);
    void Console_VideoSlicesHelp(void);
    void Console_Video3DNR(void);
    void Console_Video3DNRHelp(void);
    void Console_VideoOsdOpen(void);
    void Console_VideoOsdOpenHelp(void);
    void Console_VideoOsdPrivateMaskParam(void);
    void Console_VideoOsdPrivateMaskParamHelp(void);
    void Console_VideoOnOff(void);
    void Console_VideoOnOffHelp(void);
#if MIXER_PWM_MOTO_ENABLE
    void Console_pwm_mode(void);
    void Console_pwm_modeHelp(void);
#endif
    void Console_VideoChnRoiConfig(void);
    void Console_VideoChnRoiConfigHelp(void);

    void Console_VideoResolution(void);
    void Console_VideoResolutionHelp(void);
    void Console_SnrResolution(void);
    void Console_SnrResolutionHelp(void);
    void Console_ShowFrameInterval(void);
    void Console_ShowFrameIntervalHelp(void);
    void Console_RunTimeCmd(void);
    void Console_RunTimeCmdHelp(void);
    void Console_SuperFrameMode(void);
    void Console_SuperFrameModeHelp(void);
    void Console_VideoMirrorFlip(void);
    void Console_VideoMirrorFlipHelp(void);
    void Console_VideoChnSaveTask(void);
    void Console_VideoChnSaveTaskHelp(void);
    void Console_Switch_HDR_Linear_Mode(void);
    void Console_Switch_HDR_Linear_ModeHelp(void);
    void Console_AudioInVolume(void);
    void Console_AudioInVolumeHelp(void);
    void Console_AudioOutVolume(void);
    void Console_AudioOutVolumeHelp(void);
    void Console_IeOnOff(void);
    void Console_IeOnOffHelp(void);

    void Console_CUS3AEnable(void);
    void Console_CUS3AHelp(void);
    void Console_Sed(void);
    void Console_SedHelp(void);
    void Console_MAF(void);
    void Console_MafHelp(void);
    void Console_IPUInitInfo(void);
    void Console_IPUInitInfoHelp(void);

    void Console_Exit(void);
    void Console_ExitHelp(void);

    void Console_Cmd(void);

private:
    MI_BOOL mThread;
    MI_S8 mData[256];
    MI_U8 mParamNum;
    MI_U16 m_iWordPosition;
    void OnData(const MI_S8 *pbuf, MI_S32 length=1);
    char * GetParam(MI_U8 index);
    MI_U8 GetParamNum(void);
};

typedef void (CConsoleManager::*ConsoleFunc)();
typedef void (CConsoleManager::*ConsoleFuncHelp)();
typedef struct _ConsoleCmdMap
{
    const char *CmdStr;
    ConsoleFunc proc;
    ConsoleFuncHelp prochelp;
} ConsoleCmdMap;


#define g_ConsoleManager (CConsoleManager::instance())

#endif

