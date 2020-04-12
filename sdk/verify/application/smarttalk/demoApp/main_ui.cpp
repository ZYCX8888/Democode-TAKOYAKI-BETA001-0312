#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/time.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include <mgeff/mgeff.h>
#include <mgncs/mgncs.h>

#include "mgncs4touch/mgncs4touch.h"
#include "mgncs/mdblist.h"
#include "app_config.h"
#include "main_ui.h"

#include "cmdqueue.h"

#include "st_voice.h"
#include "char_conversion.h"
#include "st_xmlprase.h"
#include "st_usb.h"

#define HIDE_CURSOR     1
#define ROUNDRECT_ICON  0
#define AR_SUPPORT      1

// trackbar margin
#define GAP_TIP_SLIDER          4
#define TB_BORDER               2

// setting page test
// btn on sub page
typedef enum
{
    E_SELECT_BUTTON = 0,
    E_SWITCH_BUTTON,
    E_NORMAL_BUTTON,
    E_STYLE_BUTT
}BtnStyle_e;

typedef struct
{
    int nPageIdx;
    int nSubPageIdx;
    int nItemIdx;
    HWND hBtn;
    BtnStyle_e eStyle;
    BOOL bLastOff;
    BOOL bCurOff;
    char szTtile[16];
}ItemBtnInfo_t;

typedef struct
{
    int nPageIdx;
    int nSubPageIdx;
    HWND hBtn;
    BtnStyle_e eStyle;
    char szTtile[16];
}SubPageBtnInfo_t;

typedef struct
{
    HWND hBtn;
    HWND hSelectSubBtn;
    int nSubCnt;
    BtnStyle_e eStyle;
    char szTtile[16];
}PageBtnInfo_t;

typedef  struct tagTRACKBARINFO
{
    int nMin;
    int nMax;
    int nPos;
    int nTickFreq;
    int nLineSize;
    int nPageSize;
    char sStartTip[32];
    char sEndTip[32];
    BITMAP *pRulerBmp;
    BITMAP *pHThumbBmp;
    BITMAP *pVThumbBmp;
    BITMAP *pProcBmp;
}TRACKBARINFO;

typedef struct tagSKIN_BMPINFO
{
    const BITMAP *bmp;
    unsigned int nr_line;
    unsigned int nr_col;
    unsigned int idx_line;
    unsigned int idx_col;
    unsigned int margin1;
    unsigned int margin2;
    BOOL direct;
    BOOL flip;
    unsigned int style;
}SKIN_BMPINFO;

typedef enum
{
    E_SKIN_FIll_TILE,
    E_SKIN_FILL_STRETCH
}TrackBarFillStyle_e;

typedef struct
{
    int nPageIdx;
    int nSubPageIdx;
    int nItemIdx;
    HWND hTrackBar;
    int nMinVal;
    int nMaxVal;
    int nTickFreq;
    int nCurPos;
    int nLastPos;
}TrackBarInfo_t;

typedef void (*COMBOXPROC)(HWND, int);

typedef struct
{
    int nPageIdx;
    int nSubPageIdx;
    int nItemIdx;
    HWND hComBox;
    int nMaxItemCnt;
    int nCurSelect;
    char **ppText;
    COMBOXPROC comboxProc;
}ComboxInfo_t;

typedef struct
{
    list_t btnProcList;
    HWND hBtn;
    WNDPROC btnOldProc;
}BtnProcListData_t;

typedef struct
{
    list_t trackBarProcList;
    HWND hTrackBar;
    TRACKBARINFO trackInfo;
    WNDPROC trackBarOldProc;
}TrackBarProcListData_t;

typedef struct
{
    list_head wordList;
    int index;
    char szWord[62];
}TrainingWordData_t;

typedef enum
{
    E_PLAY_FORWARD,
    E_PLAY_BACKWARD
}PlayDirection_e;

typedef enum
{
    E_PLAY_NORMAL_MODE,
    E_PLAY_FAST_MODE,
    E_PLAY_SLOW_MODE
}PlayMode_e;

typedef enum
{
    E_NORMAL_SPEED = 0,
    E_2X_SPEED,
    E_4X_SPEED,
    E_8X_SPEED,
    E_16X_SPEED,
    E_32X_SPEED
}PlaySpeedMode_e;


static list_t g_btnProcListHead;
static list_t g_trackBarProcListHead;
static list_t g_trainingWordListHead;
static TrainingWordData_t g_oldTrainWord = {{NULL, NULL}, -1, ""};

// btn list opt
#define SAVE_BUTTON_OLD_PROC(btnHandle, btnProc, listHead)    \
{   \
    BtnProcListData_t *pBtnProcListData = (BtnProcListData_t*)malloc(sizeof(BtnProcListData_t));    \
    memset(pBtnProcListData, 0, sizeof(BtnProcListData_t)); \
    INIT_LIST_HEAD(&pBtnProcListData->btnProcList); \
    pBtnProcListData->hBtn = btnHandle;    \
    pBtnProcListData->btnOldProc = btnProc; \
    list_add_tail(&pBtnProcListData->btnProcList, &listHead);   \
}

#define DEL_BUTTON_OLD_PROC(btnHandle, listHead)    \
{   \
    BtnProcListData_t *pos = NULL;  \
    BtnProcListData_t *posN = NULL;  \
    list_for_each_entry_safe(pos, posN, &listHead, btnProcList)   \
    {   \
        if (pos->hBtn == btnHandle)   \
        {   \
            list_del(&pos->btnProcList); \
            free(pos);  \
            break;  \
        }   \
    }   \
}

#define CLEAR_BUTTON_OLD_PROC_LIST(listHead) \
{   \
    BtnProcListData_t *pos = NULL;  \
    BtnProcListData_t *posN = NULL;  \
    list_for_each_entry_safe(pos, posN, &listHead, btnProcList)   \
    {   \
        list_del(&pos->btnProcList); \
        free(pos);  \
    }   \
}

// trackbar list opt
#define SAVE_TRACKBAR_OLD_PROC(hwnd, trackBarInfo, trackBarProc, listHead)    \
{   \
    TrackBarProcListData_t *pTrackBarProcListData = (TrackBarProcListData_t*)malloc(sizeof(TrackBarProcListData_t));    \
    memset(pTrackBarProcListData, 0, sizeof(TrackBarProcListData_t)); \
    INIT_LIST_HEAD(&pTrackBarProcListData->trackBarProcList); \
    pTrackBarProcListData->hTrackBar = hwnd;    \
    pTrackBarProcListData->trackInfo = trackBarInfo;    \
    pTrackBarProcListData->trackBarOldProc = trackBarProc; \
    list_add_tail(&pTrackBarProcListData->trackBarProcList, &listHead);   \
}

#define DEL_TRACKBAR_OLD_PROC(hwnd, listHead)    \
{   \
    TrackBarProcListData_t *pos = NULL;  \
    TrackBarProcListData_t *posN = NULL;  \
    list_for_each_entry_safe(pos, posN, &listHead, trackBarProcList)   \
    {   \
        if (pos->hTrackBar == hwnd)   \
        {   \
            list_del(&pos->trackBarProcList); \
            free(pos);  \
            break;  \
        }   \
    }   \
}

#define CLEAR_TRACKBAR_OLD_PROC_LIST(listHead) \
{   \
    TrackBarProcListData_t *pos = NULL;  \
    TrackBarProcListData_t *posN = NULL;  \
    list_for_each_entry_safe(pos, posN, &listHead, trackBarProcList)   \
    {   \
        list_del(&pos->trackBarProcList); \
        free(pos);  \
    }   \
}

// trainning word list opt
#define SAVE_TRAINING_WORD(idx, word, listHead)    \
{   \
    TrainingWordData_t *pTrainingWordData = (TrainingWordData_t*)malloc(sizeof(TrainingWordData_t));    \
    memset(pTrainingWordData, 0, sizeof(TrainingWordData_t)); \
    INIT_LIST_HEAD(&pTrainingWordData->wordList); \
    pTrainingWordData->index = idx;    \
    strcpy(pTrainingWordData->szWord, word);    \
    list_add_tail(&pTrainingWordData->wordList, &listHead);   \
}

#define DEL_TRAINING_WORD(word, listHead)    \
{   \
    TrainingWordData_t *pos = NULL;  \
    TrainingWordData_t *posN = NULL;  \
    list_for_each_entry_safe(pos, posN, &listHead, wordList)   \
    {   \
        if (!strcmp(pos->szWord, word))   \
        {   \
            list_del(&pos->wordList); \
            free(pos);  \
            break;  \
        }   \
    }   \
}

#define FIND_TRAINING_WORD(word, listHead, pIndex)      \
{   \
    TrainingWordData_t *pos = NULL;  \
    list_for_each_entry(pos, &listHead, wordList)   \
    {   \
        if (!strcmp(pos->szWord, word))   \
        {   \
            *pIndex = pos->index; \
            break;  \
        }   \
    }   \
}

#define CLEAR_TRAINING_WORD_LIST(listHead) \
{   \
    TrainingWordData_t *pos = NULL;  \
    TrainingWordData_t *posN = NULL;  \
    list_for_each_entry_safe(pos, posN, &listHead, wordList)   \
    {   \
        list_del(&pos->wordList); \
        free(pos);  \
    }   \
}

#define WINDOW_BGCOLOR(hdc)     RGBA2Pixel(hdc, 0x0, 0xC8, 0xC8, 0xFF)

#ifdef UI_1024_600
static MI_S32 g_s32UiMainWnd_W = 1024;
static MI_S32 g_s32UiMainWnd_H = 600;
#else
static MI_S32 g_s32UiMainWnd_W = 800;
static MI_S32 g_s32UiMainWnd_H = 480;
#endif

//=========================================START====static ui bmp=============================
static BITMAP bmp_bkgnd;
static BITMAP bmp_subbk1gnd;

static BITMAP btn_setting;
static BITMAP btn_video;
static BITMAP btn_smartmic;
static BITMAP btn_call;
static BITMAP btn_time;
static BITMAP bmp_label_weather;
static BITMAP bmp_label_sunshine;
static BITMAP bmp_label_face;

static BITMAP btn_confirm;
static BITMAP btn_return;
static BITMAP btn_select;
static BITMAP btn_notselect;
static BITMAP bmp_mic;
static BITMAP bmp_speaker;
static BITMAP bmp_btnon;
static BITMAP bmp_btnoff;

static BITMAP btn_0;
static BITMAP btn_1;
static BITMAP btn_2;
static BITMAP btn_3;
static BITMAP btn_4;
static BITMAP btn_5;
static BITMAP btn_6;
static BITMAP btn_7;
static BITMAP btn_8;
static BITMAP btn_9;
static BITMAP btn_X;
static BITMAP btn_abc;

static BITMAP btn_calling;
static BITMAP btn_monitor;

static BITMAP trackbar_ruler;
static BITMAP trackbar_hthumb;
static BITMAP trackbar_vthumb;
static BITMAP trackbar_procBar;
static BITMAP g_item_upfolder;
static BITMAP g_item_folder;
static BITMAP g_item_file;

static BITMAP g_playfile_updir;
static BITMAP g_play_btn;
static BITMAP g_pause_btn;
static BITMAP g_stop_btn;
static BITMAP g_slow_btn;
static BITMAP g_fast_btn;
static BITMAP g_playtoolbar;






//==========================================END===static ui bmp=============================

//==========================================START===static font=============================
static LOGFONT  *logfont, *logfontgb12, *logfontbig24;
//==========================================END===static font==========================

static HWND hMainWnd = HWND_INVALID;
static HWND hMainSettingWnd = HWND_INVALID;
static HWND hMainVideoWnd = HWND_INVALID;
static HWND hMainSmartMicWnd = HWND_INVALID;
static HWND hMainPlayFileWnd = HWND_INVALID;
static HWND hMainCallWnd = HWND_INVALID;
static HWND hMainTimeWnd = HWND_INVALID;
static HWND hFaceDispWnd = HWND_INVALID;
static HWND hIconvWnd = HWND_INVALID;

// smartmic variables
static HWND g_hSmartmicBtn[3];
static char g_szSmartmicBtnText[3][16] = {"训练词汇", "开始侦测", "停止侦测"};
static WNDPROC g_oldSmartmicBtnProc;

// file explorer variables
static HWND g_hUpperDirBtn = HWND_INVALID;
static HWND g_hDirName = HWND_INVALID;
static HWND g_hFileView = HWND_INVALID;

// video variables
static HWND g_hCamaraSnapBtn = HWND_INVALID;
static char g_CamaraSnapBtnText[16] = {"抓拍"};
static WNDPROC g_oldCamaraSnapBtnProc;


// setting variables
static WNDPROC g_oldNormalBtnProc;
static WNDPROC g_oldSwitchBtnProc;
static WNDPROC g_oldPageBtnProc;
static WNDPROC g_oldSubPageBtnProc;
static HWND g_hCurSelectHwnd = HWND_INVALID;
static ST_Rect_T stSettingItem[4] = {{704, 432, 96, 48}, {776, 540, 124, 60}, {10, 40, 120, 60},
      {180, 40, 120, 60}};

// page0
// page0_0
static HWND g_hSpeaker = HWND_INVALID;
static HWND g_hMic = HWND_INVALID;
static HWND g_hSpkTrackBar = HWND_INVALID;
static HWND g_hMicTrackBar = HWND_INVALID;
static HWND g_hMicMuteTitle = HWND_INVALID;
static HWND g_hMicMute = HWND_INVALID;
static HWND g_hSpkMuteTitle = HWND_INVALID;
static HWND g_hSpkMute = HWND_INVALID;

// page0_1
static HWND g_hVthRing = HWND_INVALID;
static HWND g_hVthRingCombox = HWND_INVALID;
static HWND g_hVthTrackBar = HWND_INVALID;
static HWND g_hVtoRing = HWND_INVALID;
static HWND g_hVtoRingCombox = HWND_INVALID;
static HWND g_hVtoTrackBar = HWND_INVALID;

// page0_2
static HWND g_hAlarm = HWND_INVALID;
static HWND g_hAlarmCombox = HWND_INVALID;
static HWND g_hAlarmTrackBar = HWND_INVALID;

// page0_3
static HWND g_hVthRingTime = HWND_INVALID;
static HWND g_hVthRingTimeTrackBar = HWND_INVALID;
static HWND g_hVtoRingTime = HWND_INVALID;
static HWND g_hVtoRingTimeTrackBar = HWND_INVALID;
static HWND g_hRingMuteTitle = HWND_INVALID;
static HWND g_hRingMute = HWND_INVALID;

// page1
static HWND g_hTestLbl = HWND_INVALID;
static HWND g_hTestLv = HWND_INVALID;

// ...

// page6
static HWND g_hMachineInfoGroup = HWND_INVALID;
static HWND g_hSysVerTitle = HWND_INVALID;
static HWND g_hSysVer = HWND_INVALID;
static HWND g_hMachineVerTitle = HWND_INVALID;
static HWND g_hMachineVer = HWND_INVALID;
static HWND g_hRestart = HWND_INVALID;
static char g_szItemCaption6[3][16] = {"本机信息", "系统版本：", "单片机版本："};
static char g_szItemContent6[2][64] = {"Sigmastar_V1.0.20190301", "I2_V0.01"};

static HWND hMainMonitorWnd = HWND_INVALID;
static HWND hMainCalledWnd = HWND_INVALID;

// setting page
// page content btn, normal & switcher style
static ItemBtnInfo_t g_astItemBtn[] = { {0, 0, 0, HWND_INVALID, E_SWITCH_BUTTON, true, true, ""},         // mic mute
                                        {0, 0, 1, HWND_INVALID, E_SWITCH_BUTTON, true, true, ""},         // speaker mute
                                        {0, 3, 0, HWND_INVALID, E_SWITCH_BUTTON, true, true, ""},         // ring mute(shake mode)
                                        {1, 0, 0, HWND_INVALID, E_NORMAL_BUTTON, false, false, "确定"},      // confirm
                                        {2, 0, 0, HWND_INVALID, E_NORMAL_BUTTON, false, false, "确定"},      // confirm
                                        {3, 0, 0, HWND_INVALID, E_SWITCH_BUTTON, false, false, ""},        // on/off
                                        {3, 0, 1, HWND_INVALID, E_SWITCH_BUTTON, false, false, ""},        // on/off
                                        {3, 0, 2, HWND_INVALID, E_SWITCH_BUTTON, true, true, ""},          // on/off
                                        {3, 0, 3, HWND_INVALID, E_SWITCH_BUTTON, false, false, ""},        // on/off
                                        {3, 1, 0, HWND_INVALID, E_SWITCH_BUTTON, false, false, ""},        // on/off
                                        {3, 1, 1, HWND_INVALID, E_SWITCH_BUTTON, false, false, ""},        // on/off
                                        {3, 1, 2, HWND_INVALID, E_SWITCH_BUTTON, false, false, ""},        // on/off
                                        {3, 1, 3, HWND_INVALID, E_SWITCH_BUTTON, false, false, ""},        // on/off
                                        {3, 2, 0, HWND_INVALID, E_SWITCH_BUTTON, false, false, ""},        // on/off
                                        {3, 2, 1, HWND_INVALID, E_SWITCH_BUTTON, false, false, ""},        // on/off
                                        {3, 2, 2, HWND_INVALID, E_SWITCH_BUTTON, false, false, ""},        // on/off
                                        {3, 2, 3, HWND_INVALID, E_SWITCH_BUTTON, false, false, ""},        // on/off
                                        {3, 3, 0, HWND_INVALID, E_SWITCH_BUTTON, true, true, ""},         // on/off
                                        {3, 3, 1, HWND_INVALID, E_SWITCH_BUTTON, true, true, ""},         // on/off
                                        {3, 3, 2, HWND_INVALID, E_SWITCH_BUTTON, true, true, ""},         // on/off
                                        {3, 3, 3, HWND_INVALID, E_SWITCH_BUTTON, true, true, ""},         // on/off
                                        {4, 0, 0, HWND_INVALID, E_SWITCH_BUTTON, true, true, ""},         // on/off
                                        {4, 0, 1, HWND_INVALID, E_SWITCH_BUTTON, true, true, ""},         // on/off
                                        {4, 0, 2, HWND_INVALID, E_SWITCH_BUTTON, true, true, ""},         // on/off
                                        {4, 0, 3, HWND_INVALID, E_NORMAL_BUTTON, false, false, "确定"},      // confirm
                                        {5, 1, 0, HWND_INVALID, E_NORMAL_BUTTON, false, false, "触屏清洁"},// clear screen
                                        {5, 2, 0, HWND_INVALID, E_NORMAL_BUTTON, false, false, "确定"},      // confirm
                                        {5, 3, 0, HWND_INVALID, E_SWITCH_BUTTON, false, false, ""},        // enable talk
                                        {5, 3, 1, HWND_INVALID, E_SWITCH_BUTTON, true, true, ""},         // enable key sound
                                        {6, 0, 0, HWND_INVALID, E_NORMAL_BUTTON, false, false, "重启系统"},// restart system
                                      };

static SubPageBtnInfo_t g_astSubPageBtn[] = { {0, 0, HWND_INVALID, E_SELECT_BUTTON, "对讲设置"},
                                              {0, 1, HWND_INVALID, E_SELECT_BUTTON, "铃声设置"},
                                              {0, 2, HWND_INVALID, E_SELECT_BUTTON, "报警铃声设置"},
                                              {0, 3, HWND_INVALID, E_SELECT_BUTTON, "其它"},
                                              {2, 0, HWND_INVALID, E_SELECT_BUTTON, "有线防区"},
                                              {3, 0, HWND_INVALID, E_SELECT_BUTTON, "在家模式"},
                                              {3, 1, HWND_INVALID, E_SELECT_BUTTON, "外出模式"},
                                              {3, 2, HWND_INVALID, E_SELECT_BUTTON, "就寝模式"},
                                              {3, 3, HWND_INVALID, E_SELECT_BUTTON, "自定义模式"},
                                              {5, 0, HWND_INVALID, E_SELECT_BUTTON, "时间设置"},
                                              {5, 1, HWND_INVALID, E_SELECT_BUTTON, "显示设置"},
                                              {5, 2, HWND_INVALID, E_SELECT_BUTTON, "密码设置"},
                                              {5, 3, HWND_INVALID, E_SELECT_BUTTON, "其它"},
                                            };

static PageBtnInfo_t g_astPageBtn[] = { {HWND_INVALID, HWND_INVALID, 4, E_SELECT_BUTTON, "声音设置"},
                                        {HWND_INVALID, HWND_INVALID, 0, E_SELECT_BUTTON, "免扰设置"},
                                        {HWND_INVALID, HWND_INVALID, 1, E_SELECT_BUTTON, "报警设置"},
                                        {HWND_INVALID, HWND_INVALID, 4, E_SELECT_BUTTON, "模式设置"},
                                        {HWND_INVALID, HWND_INVALID, 0, E_SELECT_BUTTON, "转移设置"},
                                        {HWND_INVALID, HWND_INVALID, 4, E_SELECT_BUTTON, "通用设置"},
                                        {HWND_INVALID, HWND_INVALID, 0, E_SELECT_BUTTON, "本机信息"},
                                      };

static TrackBarInfo_t g_astTrackBar[] = { {0, 0, 0, HWND_INVALID, 0, 100, 10, 0, 0},        // mic vol
                                          {0, 0, 1, HWND_INVALID, 0, 100, 10, 0, 0},        // speaker vol
                                          {0, 1, 0, HWND_INVALID, 0, 100, 1, 0, 0},         // vth vol
                                          {0, 1, 1, HWND_INVALID, 0, 100, 1, 0, 0},         // vto vol
                                          {0, 2, 0, HWND_INVALID, 0, 100, 1, 0, 0},         // alarm vol
                                          {0, 3, 0, HWND_INVALID, 0, 120, 8, 90, 90},       // vth time
                                          {0, 3, 1, HWND_INVALID, 0, 120, 8, 120, 120},     // vto time
                                        };


static char *g_paVthRingName[] = {"vth_ring1.pcm", "vth_ring2.pcm", "vth_ring3.pcm"};
static char *g_paVtoRingName[] = {"vto_ring1.pcm", "vto_ring2.pcm", "vto_ring3.pcm", "vto_ring4.pcm"};
static char *g_paAlarmRingName[] = {"alarm_ring1.pcm", "alarm_ring2.pcm", "alarm_ring3.pcm", "alarm_ring4.pcm", "alarm_ring5.pcm", "alarm_ring6.pcm"};

static void VthRingComboxProc(HWND hwnd, int nCurSelect);
static void VtoRingComboxProc(HWND hwnd, int nCurSelect);
static void AlarmComboxProc(HWND hwnd, int nCurSelect);

static ComboxInfo_t g_astCombox[] = { {0, 1, 0, HWND_INVALID, 3, 0, g_paVthRingName, VthRingComboxProc},
                                      {0, 1, 1, HWND_INVALID, 4, 0, g_paVtoRingName, VtoRingComboxProc},
                                      {0, 2, 0, HWND_INVALID, 6, 0, g_paAlarmRingName, AlarmComboxProc},
                                    };

// play file page
static MI_S32 g_s32SelectIdx = 0;
static MI_U8 g_pu8RootPath[256] = "/customer";
static MI_U8 g_pu8FullPath[256] = {0};
static MI_U8 g_pu8SelectPath[256] = {0};            // save select file
static MI_BOOL g_bShowPlayToolBar = FALSE;          // select file page or play file page

static MI_BOOL g_bPlaying = FALSE;
static MI_BOOL g_bPause = FALSE;
static MI_BOOL g_ePlayDirection = E_PLAY_FORWARD;
static PlayMode_e g_ePlayMode = E_PLAY_NORMAL_MODE;
static PlaySpeedMode_e g_eSpeedMode = E_NORMAL_SPEED;
static MI_U32 g_u32SpeedNumerator = 1;
static MI_U32 g_u32SpeedDenomonator = 1;



static pthread_t g_tid_msg;
MsgQueue g_toUI_msg_queue;
sem_t g_toUI_sem;

extern MsgQueue g_toAPP_msg_queue;
extern sem_t g_toAPP_sem;

static bool g_run = FALSE;

//===============extern var===============start
extern int g_faceSnap;
extern int g_faceDetect;
//===============extern var===============end

//===============trackbar drawing=====================start
/*
 *  rcClient: controller area
 *  rcRuler: ruler area
 *  rcBar: thumb area
 *  rcProcBar: process area
 *  rcBorder: border area
 *
*/
static void calc_trackbar_rect(HWND hWnd, TRACKBARINFO *info, DWORD dwStyle, const RECT* rcClient, RECT* rcRuler, RECT* rcBar, RECT* rcProcBar, RECT* rcBorder)
{
    int x, y, w, h;
    int pos, min, max;
    int sliderx, slidery, sliderw, sliderh;
    const BITMAP *rulerBmp = NULL;
    const BITMAP *thumbBmp = NULL;
    const BITMAP *procBarBmp = NULL;

    x = rcClient->left;
    y = rcClient->top;
    w = RECTWP (rcClient);
    h = RECTHP (rcClient);

    pos = info->nPos;
    max = info->nMax;
    min = info->nMin;

    /* Calculate border rect. */
    if (dwStyle & TBS_BORDER)
    {
        x += TB_BORDER;
        y += TB_BORDER;
        w -= TB_BORDER << 1;
        h -= TB_BORDER << 1;
    }

    if (rcBorder)
    {
        SetRect (rcBorder, x, y, x+w, y+h);
    }

    if (!rcRuler && !rcBar)
        return;

    /* Calculate ruler rect. */
    if (rcRuler)
    {
        rulerBmp = info->pRulerBmp;

        if (dwStyle & TBS_VERTICAL)
        {
            rcRuler->left   = x + ((w - rulerBmp->bmWidth)>>1);     // ruler width为图片实际的ruler width
            rcRuler->top    = y;
            rcRuler->right  = x + ((w + rulerBmp->bmWidth)>>1);
            rcRuler->bottom = y + h;
        }
        else
        {
            rcRuler->left   = x;
            rcRuler->top    = y + ((h - rulerBmp->bmHeight)>>1);
            rcRuler->right  = x + w;
            rcRuler->bottom = y + ((h + rulerBmp->bmHeight)>>1);
        }
    }

    if (rcBar)
    {
        /* Calculate slider rect. */
        if (dwStyle & TBS_VERTICAL)
        {
            thumbBmp = info->pVThumbBmp;
            //sliderw = thumbBmp->bmWidth;
            //sliderh = thumbBmp->bmHeight>>2;
            sliderw = thumbBmp->bmWidth<<1;
            sliderh = thumbBmp->bmHeight<<1;
            sliderx = x + ((w - sliderw) >> 1);
            slidery = y + (int)(max - pos) * (h - sliderh) / (max - min);
        }
        else
        {
            thumbBmp = info->pHThumbBmp;
            //sliderw = thumbBmp->bmWidth;
            //sliderh = thumbBmp->bmHeight>>2;
            sliderw = thumbBmp->bmWidth<<1;
            sliderh = thumbBmp->bmHeight<<1;
            slidery = y + ((h - sliderh) >> 1);
            sliderx = x + (int)(pos - min) * (w - sliderw) / (max - min);
        }
        SetRect (rcBar, sliderx, slidery, sliderx + sliderw, slidery + sliderh);
    }

    if (rcProcBar)
    {
        procBarBmp = info->pProcBmp;
        if (dwStyle & TBS_VERTICAL)
        {
            rcProcBar->left = rcRuler->left;
            rcProcBar->top = slidery - sliderh;
            rcProcBar->right = rcRuler->right;
            rcProcBar->bottom = rcRuler->bottom;
        }
        else
        {
            rcProcBar->top = rcRuler->top;
            rcProcBar->left = rcRuler->left;
            rcProcBar->right = sliderx;
            rcProcBar->bottom = rcRuler->bottom;
        }
    }
}

static void unload_sub_bitmap (PBITMAP sub_bmp)
{
    if (NULL != sub_bmp)
    {
        UnloadBitmap (sub_bmp);
        free (sub_bmp);
        sub_bmp = NULL;
    }
}

static PBITMAP get_sub_bitmap (HDC hdc, const SKIN_BMPINFO *bmp_info)
{
    Uint8   *start_bits;
    int     bpp, pitch, i, w, h;
    PBITMAP sub_bmp = NULL;
    const BITMAP *full_bmp = NULL;

    if (NULL == (sub_bmp = (PBITMAP)calloc (1, sizeof(BITMAP))))
    {
        fprintf (stderr, "no memory for sub bmp in skin LF.");
        return NULL;
    }

    full_bmp = bmp_info->bmp;
    memcpy (sub_bmp, full_bmp, sizeof(BITMAP));

    w     = full_bmp->bmWidth / bmp_info->nr_col;
    h     = full_bmp->bmHeight / bmp_info->nr_line;
    bpp   = full_bmp->bmBytesPerPixel;
    pitch = full_bmp->bmPitch / bmp_info->nr_col;

    /*initialize private info*/
    sub_bmp->bmWidth = w;
    sub_bmp->bmHeight = h;
    sub_bmp->bmPitch = pitch;

    if (NULL == (sub_bmp->bmBits = (Uint8*)calloc (1, pitch*h)))
    {
        fprintf (stderr, "no memory for sub bmp in skin LF.");
        return NULL;
    }

    start_bits = full_bmp->bmBits
    + h * bmp_info->idx_line * full_bmp->bmPitch
    + w * bmp_info->idx_col * bpp;

    for (i = 0; i < h; i++)
    {
        memcpy (sub_bmp->bmBits + pitch*i, start_bits, w*bpp);
        start_bits += full_bmp->bmPitch;
    }
    if (sub_bmp->bmType & BMP_TYPE_ALPHA_MASK)
    {
        pitch = ((w+3) & ~3);
        sub_bmp->bmAlphaMask = (Uint8*)calloc(1, h * pitch);
        sub_bmp->bmAlphaPitch = pitch;
        start_bits = full_bmp->bmAlphaMask
        + h * bmp_info->idx_line * full_bmp->bmAlphaPitch
        + w * bmp_info->idx_col;

        for (i = 0; i < h; i++)
        {
            memcpy (sub_bmp->bmAlphaMask + pitch*i, start_bits, w);
            start_bits += full_bmp->bmAlphaPitch;
        }
    }

    if (bmp_info->flip)
    {
        VFlipBitmap (sub_bmp, sub_bmp->bmBits);
    }

    return sub_bmp;
}

static void draw_area_from_bitmap(HDC hdc, const RECT* rc, const SKIN_BMPINFO *bmp_info, BOOL do_clip)
{
    int margin1 = bmp_info->margin1;
    int margin2 = bmp_info->margin2;
    int new_left = 0, new_top=  0;

    // check valid
    if (rc->left == rc->right || rc->top == rc->bottom)
        return;

    if (do_clip)
        SelectClipRect (hdc, rc);

    if (!(bmp_info->direct))
    {
        FillBoxWithBitmap(hdc, rc->left, rc->top, (rc->right-rc->left), (rc->bottom-rc->top), bmp_info->bmp);
    }
    else
    {
        FillBoxWithBitmap(hdc, rc->left, rc->top, (rc->right-rc->left), (rc->bottom-rc->top), bmp_info->bmp);
    }
}

static void draw_trackbar_subctrl(HWND hWnd, HDC hdc, const RECT* pRect, const BITMAP *bmp, DWORD dwStyle)
{
    /** trackbar status, pressed or hilite */
    RECT rc_draw;
    BOOL vertical;
    DWORD file;
    SKIN_BMPINFO bmp_info;

    /** leave little margin */
    rc_draw.left   = pRect->left;
    rc_draw.top    = pRect->top ;
    rc_draw.right  = pRect->right;
    rc_draw.bottom = pRect->bottom;

    if (dwStyle & TBS_VERTICAL)
        vertical = TRUE;
    else
        vertical = FALSE;

    bmp_info.bmp      = bmp;
    bmp_info.nr_line  = 4;
    bmp_info.nr_col   = 1;
    bmp_info.idx_col  = 0;
    bmp_info.idx_line = 0;
    bmp_info.margin1  = 0;
    bmp_info.margin2  = 0;
    bmp_info.direct   = FALSE;
    bmp_info.flip     = FALSE;
    bmp_info.style    = E_SKIN_FILL_STRETCH;

    if (dwStyle & LFRDR_TBS_PRESSED)
        bmp_info.idx_line = 2;
    else if (dwStyle & LFRDR_TBS_HILITE)
        bmp_info.idx_line = 1;
    else if (dwStyle & WS_DISABLED)
        bmp_info.idx_line = 3;

   draw_area_from_bitmap (hdc, &rc_draw, &bmp_info, FALSE);
}

static void draw_trackbar(HWND hWnd, HDC hdc, TRACKBARINFO *info, DWORD dwStyle)
{
    DWORD file;
    SKIN_BMPINFO bmp_info;
    const BITMAP *bmp;
    RECT rc_client, rc_border, rc_ruler, rc_bar, rc_procBar;

    GetClientRect (hWnd, &rc_client);
    calc_trackbar_rect(hWnd, info, dwStyle, &rc_client, &rc_ruler, &rc_bar, &rc_procBar, &rc_border);
//    printf("client(x:%d y:%d w:%d h:%d) ruler(x:%d y:%d w:%d h:%d)\nbar(x:%d y:%d w:%d h:%d) border(x:%d y:%d w:%d h:%d)\n", rc_client.left, rc_client.top,
//            rc_client.right-rc_client.left, rc_client.bottom-rc_client.top, rc_ruler.left, rc_ruler.top, rc_ruler.right-rc_ruler.left, rc_ruler.bottom-rc_ruler.top,
//            rc_bar.left, rc_bar.top, rc_bar.right-rc_bar.left, rc_bar.bottom-rc_bar.top,
//            rc_border.left, rc_border.top, rc_border.right-rc_border.left, rc_border.bottom-rc_border.top);

    if (dwStyle & TBS_VERTICAL)
    {
        bmp_info.direct = TRUE;
        bmp = info->pVThumbBmp;
    }
    else
    {
        bmp_info.direct = FALSE;
        bmp = info->pHThumbBmp;
    }

    bmp_info.bmp = info->pRulerBmp;
    bmp_info.nr_line = 1;
    bmp_info.nr_col = 1;
    bmp_info.idx_line = 0;
    bmp_info.idx_col = 0;
    bmp_info.margin1 = 2;
    bmp_info.margin2 = 2;
    bmp_info.flip = FALSE;
    bmp_info.style = E_SKIN_FILL_STRETCH;

    draw_area_from_bitmap (hdc, &rc_ruler, &bmp_info, FALSE);
    draw_trackbar_subctrl (hWnd, hdc, &rc_bar, bmp, dwStyle);
    draw_trackbar_subctrl(hWnd, hdc, &rc_procBar, (const BITMAP *)info->pProcBmp, dwStyle);
}

static void TrackBarOnDraw (HWND hwnd, HDC hdc, TRACKBARINFO* pData, DWORD dwStyle)
{
    RECT    rc_client;
    GetClientRect (hwnd, &rc_client);
    draw_trackbar (hwnd, hdc, pData, dwStyle);

    /* draw the tip of trackbar. */
    if ((dwStyle & TBS_TIP) && !(dwStyle & TBS_VERTICAL))
    {
        SIZE    text_ext;
        char    sPos[10];
        RECT    rc_bar, rc_border;
        int sliderh, EndTipLen, x, y, w, h;

        calc_trackbar_rect(hwnd, pData, dwStyle, &rc_client, NULL, &rc_bar, NULL, &rc_border);
//        printf("client(x:%d y:%d w:%d h:%d) ruler(x:%d y:%d w:%d h:%d)\nbar(x:%d y:%d w:%d h:%d) border(x:%d y:%d w:%d h:%d)\n", rc_client.left, rc_client.top,
//            rc_client.right-rc_client.left, rc_client.bottom-rc_client.top,
//            rc_bar.left, rc_bar.top, rc_bar.right-rc_bar.left, rc_bar.bottom-rc_bar.top,
//            rc_border.left, rc_border.top, rc_border.right-rc_border.left, rc_border.bottom-rc_border.top);

        sliderh = RECTH (rc_bar);
        x = rc_border.left;
        y = rc_border.top;
        w = RECTW (rc_border);
        h = RECTH (rc_border);

        SelectFont(hdc, (PLOGFONT)GetWindowElementAttr(hwnd, WE_FONT_TOOLTIP));
        SetBkMode(hdc, BM_TRANSPARENT);
        SetBkColor(hdc, GetWindowBkColor(hwnd));
        SetTextColor(hdc, GetWindowElementPixel(hwnd, WE_FGC_THREED_BODY));
        TextOut (hdc, x + 1, y + (h>>1) - (sliderh>>1) - GAP_TIP_SLIDER, pData->sStartTip);

        GetTextExtent (hdc, pData->sEndTip, -1, &text_ext);
        EndTipLen = text_ext.cx + 4;
        TextOut (hdc, (EndTipLen > (w>>1) - 20 ? x + (w>>1) + 20 : x + w -EndTipLen),
                        y + (h>>1) - (sliderh>>1) - GAP_TIP_SLIDER, pData->sEndTip);
        sprintf (sPos, "%d", pData->nPos);
        GetTextExtent (hdc, sPos, -1, &text_ext);
        TextOut (hdc, x + ((w - text_ext.cx) >> 1), y+(h>>1)-(sliderh>>1)- GAP_TIP_SLIDER, sPos);
    }
}

//===============trackbar drawing=====================end

// paint bitmap button
/*GetBtnRects:
 *      get the client rect(draw body of button), the content rect
 *      (draw content), and bitmap rect(draw a little icon)
 */
static void GetBtnRects (HWND hwnd, RECT* prcClient, RECT* prcContent)
{
    GetClientRect (hwnd, prcClient);
    SetRect (prcContent, prcClient->left, prcClient->top, prcClient->right, prcClient->bottom);
}

/**
 * draw bitmap content in button with BS_BITMAP style
 */
static void DrawBitmapBtn (HWND hwnd, bool bSetDefault, PBITMAP pDefaultBmp, PBITMAP pUsrBmp, HDC hdc, DWORD dwStyle, RECT *prcText)
{
    RECT clientRect;
    PBITMAP pBmp;

    if (!bSetDefault && pUsrBmp)
        pBmp = pUsrBmp;
    else
        pBmp = pDefaultBmp;

    int x = prcText->left;
    int y = prcText->top;
    int w = RECTWP (prcText);
    int h = RECTHP (prcText);

    GetClientRect (hwnd, &clientRect);

    if (dwStyle & BS_REALSIZEIMAGE)
    {
        x += (w - pBmp->bmWidth) >> 1;
        y += (h - pBmp->bmHeight) >> 1;
        w = h = 0;

        if (pBmp->bmWidth > RECTW(clientRect)){
            x = clientRect.left;
            w = RECTW(clientRect)-1;
        }

        if (pBmp->bmHeight > RECTH(clientRect)){
            y = clientRect.top;
            h = RECTH(clientRect)-1;
        }
    }
    else
    {
        x = clientRect.top;
        y = clientRect.left;
        w = RECTW (clientRect)-1;
        h = RECTH (clientRect)-1;
    }

    HDC hdcMem = CreateCompatibleDC(hdc);
    FillBoxWithBitmap(hdcMem, x, y, w, h, pBmp);
    BitBlt(hdcMem, x, y, w, h, hdc, 0, 0, 0);
    DeleteCompatibleDC(hdcMem);
}

static void paint_content_focus(HDC hdc, HWND hwnd, bool bSetDefault, PBITMAP pDefaultBmp, PBITMAP pUsrBmp, RECT* prc_cont)
{
    DWORD dt_fmt = 0;
    DWORD fg_color = 0xFFFF;
    DWORD text_pixel;
    BOOL is_get_fg = FALSE;
    BOOL is_get_fmt = FALSE;
    gal_pixel old_pixel;
    RECT focus_rc;
    const WINDOW_ELEMENT_RENDERER* win_rdr;
    DWORD dwStyle = GetWindowStyle(hwnd);
    win_rdr = GetWindowInfo(hwnd)->we_rdr;

    //draw bmp & text
    DrawBitmapBtn(hwnd, bSetDefault, pDefaultBmp, pUsrBmp, hdc, dwStyle, prc_cont);
    fg_color = GetWindowElementAttr(hwnd, WE_FGC_THREED_BODY);

    is_get_fg = TRUE;
    text_pixel = RGBA2Pixel(hdc, GetRValue(fg_color),
                            GetGValue(fg_color), GetBValue(fg_color),
                            GetAValue(fg_color));

    old_pixel = SetTextColor(hdc, text_pixel);
    dt_fmt = DT_SINGLELINE | DT_CENTER | DT_VCENTER;
    is_get_fmt = TRUE;
    SetBkMode (hdc, BM_TRANSPARENT);
    DrawText (hdc, GetWindowCaption(hwnd), -1, prc_cont, dt_fmt);
    SetTextColor(hdc, old_pixel);

    /*disable draw text*/
    if (!IsWindowEnabled(hwnd) | (dwStyle & WS_DISABLED))
        win_rdr->disabled_text_out (hwnd, hdc, GetWindowCaption(hwnd), prc_cont, dt_fmt);
}

static void OnBtnPaint(HDC hdc, HWND hwnd, bool bSetDefault, PBITMAP pDefaultBmp, PBITMAP pUsrBmp)
{
    const WINDOW_ELEMENT_RENDERER* win_rdr;
    DWORD main_color;
    RECT rcClient;
    RECT rcContent;

    win_rdr = GetWindowInfo(hwnd)->we_rdr;
    main_color = GetWindowElementAttr(hwnd, WE_MAINC_THREED_BODY);
    GetBtnRects(hwnd, &rcClient, &rcContent);
    win_rdr->draw_push_button(hwnd, hdc, &rcClient, main_color, 0xFFFFFFFF, BST_NORMAL);
    paint_content_focus(hdc, hwnd, bSetDefault, pDefaultBmp, pUsrBmp, &rcContent);
}

static void SetBtnBmp(HWND hwnd, DWORD bmpData)
{
    if (IsWindow(hwnd))
    {
        SetWindowAdditionalData(hwnd, bmpData);
        InvalidateRect(hwnd, NULL, TRUE);
//        PostMessage(hwnd, MSG_ERASEBKGND, 0, 0);
    }
}

bool IsPageBtn(HWND hwnd)
{
    int i = 0;
    bool bRet = false;

    if (hwnd == HWND_INVALID)
        return false;

    for (i = 0; i < sizeof(g_astPageBtn)/sizeof(PageBtnInfo_t); i++)
    {
        if (hwnd == g_astPageBtn[i].hBtn)
        {
            bRet = true;
            break;
        }
    }

    return bRet;
}

bool IsSubPageBtn(HWND hwnd)
{
    int i = 0;
    bool bRet = false;

    if (hwnd == HWND_INVALID)
        return false;

    for (i = 0; i < sizeof(g_astSubPageBtn)/sizeof(SubPageBtnInfo_t); i++)
    {
        if (hwnd == g_astSubPageBtn[i].hBtn)
        {
            bRet = true;
            break;
        }
    }

    return bRet;
}

bool IsItemBtn(HWND hwnd)
{
    int i = 0;
    bool bRet = false;

    for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
    {
        if (hwnd == g_astItemBtn[i].hBtn)
        {
            bRet = true;
            break;
        }
    }

    return bRet;
}

int GetItemBtnPageIndex(HWND hwnd)
{
    int i = 0;
    int nIndex = -1;

    for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
    {
        if (hwnd == g_astItemBtn[i].hBtn)
        {
            nIndex = g_astItemBtn[i].nPageIdx;
            break;
        }
    }

    return nIndex;
}

int GetItemBtnSubPageIndex(HWND hwnd)
{
    int i = 0;
    int nIndex = -1;

    for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
    {
        if (hwnd == g_astItemBtn[i].hBtn)
        {
            nIndex = g_astItemBtn[i].nSubPageIdx;
            break;
        }
    }

    return nIndex;
}

char *GetItemBtnText(int nPageIdx, int nSubPageIdx, int nItemIdx)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
    {
        if (g_astItemBtn[i].nPageIdx == nPageIdx && g_astItemBtn[i].nSubPageIdx == nSubPageIdx
            && g_astItemBtn[i].nItemIdx == nItemIdx)
        {
            return g_astItemBtn[i].szTtile;
        }
    }

    return NULL;
}

BtnStyle_e GetItemBtnStyle(int nPageIdx, int nSubPageIdx, int nItemIdx)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
    {
        if (g_astItemBtn[i].nPageIdx == nPageIdx && g_astItemBtn[i].nSubPageIdx == nSubPageIdx
            && g_astItemBtn[i].nItemIdx == nItemIdx)
        {
            return g_astItemBtn[i].eStyle;
        }
    }

    return E_NORMAL_BUTTON;
}

void TriggleSwitchBtn(HWND hwnd)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
    {
        if (hwnd == g_astItemBtn[i].hBtn)
        {
            g_astItemBtn[i].bCurOff = !g_astItemBtn[i].bCurOff;
            break;
        }
    }
}

bool GetSwitchBtnStatus(HWND hwnd)
{
    bool bRet = false;
    int i = 0;

    for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
    {
        if (hwnd == g_astItemBtn[i].hBtn)
        {
            bRet = g_astItemBtn[i].bCurOff;
            break;
        }
    }

    return bRet;
}

static LRESULT SwitcherBtnProc(HWND hwnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    DWORD dwStyle = GetWindowStyle(hwnd);
    WNDPROC btnProc = NULL;
    BtnProcListData_t *pos =  NULL;

    list_for_each_entry(pos, &g_btnProcListHead, btnProcList)
    {
        if (pos->hBtn == hwnd)
        {
            btnProc = pos->btnOldProc;
            break;
        }
    }

    if (!btnProc)
    {
//        printf("switch btn destroyed or the old proc of switch btn %d has not been saved\n", hwnd);
        return 0;
    }

    switch (message)
    {
        case MSG_LBUTTONDOWN:
            TriggleSwitchBtn(hwnd);
            break;
        case MSG_MOUSEMOVEIN:
            return 0;
        case MSG_PAINT:
            if (dwStyle & BS_BITMAP)
            {
                hdc = BeginPaint (hwnd);
                SelectFont (hdc, GetWindowFont(hwnd));
                OnBtnPaint(hdc, hwnd, GetSwitchBtnStatus(hwnd), &bmp_btnoff, &bmp_btnon);
                EndPaint(hwnd, hdc);
                return 0;
            }
            break;
    }

    return (*btnProc)(hwnd, message, wParam, lParam);
}

static LRESULT NormalBtnProc(HWND hwnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    DWORD dwStyle = GetWindowStyle(hwnd);
    DWORD dwAddData = GetWindowAdditionalData(hwnd);
    WNDPROC btnProc = NULL;
    BtnProcListData_t *pos =  NULL;

    list_for_each_entry(pos, &g_btnProcListHead, btnProcList)
    {
        if (pos->hBtn == hwnd)
        {
            btnProc = pos->btnOldProc;
            break;
        }
    }

    if (!btnProc)
    {
//        printf("normal btn destroyed or the old proc of normal btn %d has not been saved\n", hwnd);
        return 0;
    }

    switch (message)
    {
        case MSG_MOUSEMOVEIN:
            return 0;
        case MSG_PAINT:
            if (dwStyle & BS_BITMAP)
            {
                hdc = BeginPaint (hwnd);
                SelectFont (hdc, GetWindowFont(hwnd));
                //OnBtnPaint(hdc, hwnd, true, &btn_notselect, (PBITMAP)dwAddData);
                OnBtnPaint(hdc, hwnd, false, &btn_notselect, (PBITMAP)dwAddData);
                EndPaint(hwnd, hdc);
                return 0;
            }
            break;
    }

    return (*btnProc)(hwnd, message, wParam, lParam);
}

WNDPROC GetItemBtnProc(HWND hwnd)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
    {
        if (hwnd == g_astItemBtn[i].hBtn)
        {
            if (g_astItemBtn[i].eStyle == E_NORMAL_BUTTON)
            {
                return NormalBtnProc;
            }
            else if (g_astItemBtn[i].eStyle == E_SWITCH_BUTTON)
            {
                return SwitcherBtnProc;
            }
        }
    }

    return NULL;
}

int GetSubPageBtnPageIndex(HWND hwnd)
{
    int i = 0;
    int nIndex = -1;

    for (i = 0; i < sizeof(g_astSubPageBtn)/sizeof(SubPageBtnInfo_t); i++)
    {
        if (hwnd == g_astSubPageBtn[i].hBtn)
        {
            nIndex = g_astSubPageBtn[i].nPageIdx;
            break;
        }
    }

    return nIndex;
}

char *GetSubPageBtnText(int nPageIdx, int nSubPageIdx)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astSubPageBtn)/sizeof(SubPageBtnInfo_t); i++)
    {
        if (g_astSubPageBtn[i].nPageIdx == nPageIdx && g_astSubPageBtn[i].nSubPageIdx == nSubPageIdx)
        {
            return g_astSubPageBtn[i].szTtile;
        }
    }

    return NULL;
}

BtnStyle_e GetSubPageBtnStyle(int nPageIdx, int nSubPageIdx)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astSubPageBtn)/sizeof(SubPageBtnInfo_t); i++)
    {
        if (g_astSubPageBtn[i].nPageIdx == nPageIdx && g_astSubPageBtn[i].nSubPageIdx == nSubPageIdx)
        {
            return g_astSubPageBtn[i].eStyle;
        }
    }

    return E_NORMAL_BUTTON;
}

HWND GetSubPageBtnHwnd(int nPageIdx, int nSubPageIdx)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astSubPageBtn)/sizeof(SubPageBtnInfo_t); i++)
    {
        if (g_astSubPageBtn[i].nPageIdx == nPageIdx && g_astSubPageBtn[i].nSubPageIdx == nSubPageIdx)
        {
            return g_astSubPageBtn[i].hBtn;
        }
    }

    return HWND_INVALID;
}


void SetSubPageBtnHwnd(int nPageIdx, int nSubPageIdx, HWND hwnd)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astSubPageBtn)/sizeof(SubPageBtnInfo_t); i++)
    {
        if (g_astSubPageBtn[i].nPageIdx == nPageIdx && g_astSubPageBtn[i].nSubPageIdx == nSubPageIdx)
        {
            g_astSubPageBtn[i].hBtn = hwnd;
        }
    }
}

void SetItemBtnHwnd(int nPageIdx, int nSubPageIdx, int nItemIdx, HWND hwnd)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
    {
        if (g_astItemBtn[i].nPageIdx == nPageIdx && g_astItemBtn[i].nSubPageIdx == nSubPageIdx && g_astItemBtn[i].nItemIdx == nItemIdx)
        {
            g_astItemBtn[i].hBtn = hwnd;
        }
    }
}

void SetItemBtnStatus(HWND hwnd, MI_BOOL bMute)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
    {
        if (g_astItemBtn[i].hBtn == hwnd)
        {
            g_astItemBtn[i].bLastOff = bMute;
            g_astItemBtn[i].bCurOff = bMute;
        }
    }
}

void SetTrackBarHwnd(int nPageIdx, int nSubPageIdx, int nItemIdx, HWND hwnd)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astTrackBar)/sizeof(TrackBarInfo_t); i++)
    {
        if (g_astTrackBar[i].nPageIdx == nPageIdx && g_astTrackBar[i].nSubPageIdx == nSubPageIdx && g_astTrackBar[i].nItemIdx == nItemIdx)
        {
            g_astTrackBar[i].hTrackBar = hwnd;
        }
    }
}

int GetTrackBarMinVal(HWND hwnd)
{
    int nMinVal = 0;
    int i = 0;

    for (i = 0; i < sizeof(g_astTrackBar)/sizeof(TrackBarInfo_t); i++)
    {
        if (hwnd == g_astTrackBar[i].hTrackBar)
        {
            nMinVal = g_astTrackBar[i].nMinVal;
            break;
        }
    }

    return nMinVal;
}

int GetTrackBarMaxVal(HWND hwnd)
{
    int nMaxVal = 0;
    int i = 0;

    for (i = 0; i < sizeof(g_astTrackBar)/sizeof(TrackBarInfo_t); i++)
    {
        if (hwnd == g_astTrackBar[i].hTrackBar)
        {
            nMaxVal = g_astTrackBar[i].nMaxVal;
            break;
        }
    }

    return nMaxVal;
}

int GetTrackBarTickFreq(HWND hwnd)
{
    int nTickFreq = 0;
    int i = 0;

    for (i = 0; i < sizeof(g_astTrackBar)/sizeof(TrackBarInfo_t); i++)
    {
        if (hwnd == g_astTrackBar[i].hTrackBar)
        {
            nTickFreq = g_astTrackBar[i].nTickFreq;
            break;
        }
    }

    return nTickFreq;
}

int GetTrackBarCurPos(HWND hwnd)
{
    int nPos = 0;
    int i = 0;

    for (i = 0; i < sizeof(g_astTrackBar)/sizeof(TrackBarInfo_t); i++)
    {
        if (hwnd == g_astTrackBar[i].hTrackBar)
        {
            nPos = g_astTrackBar[i].nCurPos;
            break;
        }
    }

    return nPos;
}

void InitTrackBarPos(HWND hwnd, MI_S32 s32Pos)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astTrackBar)/sizeof(TrackBarInfo_t); i++)
    {
        if (hwnd == g_astTrackBar[i].hTrackBar)
        {
            g_astTrackBar[i].nCurPos = s32Pos;
            g_astTrackBar[i].nLastPos = s32Pos;
            break;
        }
    }
}

void SetTrackBarPos(HWND hwnd)
{
    int i = 0;
    int nCurPos = SendMessage(hwnd, TBM_GETPOS, 0, 0);

    for (i = 0; i < sizeof(g_astTrackBar)/sizeof(TrackBarInfo_t); i++)
    {
        if (hwnd == g_astTrackBar[i].hTrackBar)
        {
            g_astTrackBar[i].nCurPos = nCurPos;
            break;
        }
    }
}

static void TrackBarNotifyProc(HWND hwnd, LINT id, int nc, DWORD add_data)
{
    if (TBN_CHANGE == nc)
    {
        SetTrackBarPos(hwnd);
    }
}

static LRESULT TrackBarProc(HWND hwnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    DWORD dwStyle = GetWindowStyle(hwnd);
    WNDPROC trackBarProc = NULL;
    TrackBarProcListData_t *pos =  NULL;

    list_for_each_entry(pos, &g_trackBarProcListHead, trackBarProcList)
    {
        if (pos->hTrackBar == hwnd)
        {
            trackBarProc = pos->trackBarOldProc;
            break;
        }
    }

    if (!trackBarProc)
        return 0;

    switch (message)
    {
        case MSG_PAINT:
            hdc = BeginPaint (hwnd);
            pos->trackInfo.nPos = GetTrackBarCurPos(hwnd);
            TrackBarOnDraw(hwnd, hdc, &(pos->trackInfo), dwStyle);
            EndPaint(hwnd, hdc);
            return 0;
    }

    return (*trackBarProc)(hwnd, message, wParam, lParam);
}


void SetTrackBarStatus(HWND hwnd, BITMAP *pRulerBmp, BITMAP *pThumbBmp, BITMAP *pProcBmp)
{
    TRACKBARINFO trackBarInfo;
    WNDPROC trackBarProc;
    DWORD   dwStyle;

    memset(&trackBarInfo, 0, sizeof(TRACKBARINFO));
    dwStyle = GetWindowStyle (hwnd);

    if (dwStyle & TBS_VERTICAL)
    {
        trackBarInfo.pVThumbBmp = pThumbBmp;
    }
    else
    {
        trackBarInfo.pHThumbBmp = pThumbBmp;
    }

    trackBarInfo.pRulerBmp = pRulerBmp;
    trackBarInfo.pProcBmp = pProcBmp;
    trackBarInfo.nLineSize = 5;
    trackBarInfo.nPageSize = 10;
    trackBarInfo.nMax = GetTrackBarMaxVal(hwnd);
    trackBarInfo.nMin = GetTrackBarMinVal(hwnd);
    trackBarInfo.nTickFreq = GetTrackBarTickFreq(hwnd);
    trackBarInfo.nPos = GetTrackBarCurPos(hwnd);
    sprintf(trackBarInfo.sStartTip, "%d", trackBarInfo.nMin);
    sprintf(trackBarInfo.sEndTip, "%d", trackBarInfo.nMax);

    // save hwnd-trackbarInfo map
    trackBarProc = SetWindowCallbackProc(hwnd, TrackBarProc);
    SAVE_TRACKBAR_OLD_PROC(hwnd, trackBarInfo, trackBarProc, g_trackBarProcListHead);

    SendMessage(hwnd, TBM_SETRANGE, trackBarInfo.nMin, trackBarInfo.nMax);
    SendMessage(hwnd, TBM_SETLINESIZE, trackBarInfo.nLineSize, 0);
    SendMessage(hwnd, TBM_SETPAGESIZE, trackBarInfo.nPageSize, 0);
    SendMessage(hwnd, TBM_SETTICKFREQ, trackBarInfo.nTickFreq, 0);
    SendMessage(hwnd, TBM_SETTIP, (WPARAM)trackBarInfo.sStartTip, (LPARAM)trackBarInfo.sEndTip);
    SendMessage(hwnd, TBM_SETPOS, trackBarInfo.nPos, 0);
    SetNotificationCallback (hwnd, TrackBarNotifyProc);
}

static void VthRingComboxProc(HWND hwnd, int nCurSelect)
{
    switch (nCurSelect)
    {
        case 0:
            // set ring file1
            break;
        case 1:
            // set ring file2
            break;
        case 2:
            // set ring file3
        default:
            break;
    }
}

static void VtoRingComboxProc(HWND hwnd, int nCurSelect)
{
    switch (nCurSelect)
    {
        case 0:
            // set ring file1
            break;
        case 1:
            // set ring file2
            break;
        case 2:
            // set ring file3
        case 3:
            // set ring file4
        default:
            break;
    }
}

static void AlarmComboxProc(HWND hwnd, int nCurSelect)
{
    switch (nCurSelect)
    {
        case 0:
            // set ring file1
            break;
        case 1:
            // set ring file2
            break;
        case 2:
            // set ring file3
        case 3:
            // set ring file4
            break;
        case 4:
            // set ring file5
            break;
        case 5:
            // set ring file6
        default:
            break;
    }
}

void SetComboxHwnd(int nPageIdx, int nSubPageIdx, int nItemIdx, HWND hwnd)
{
    int i = 0;
    int j = 0;

    for (i = 0; i < sizeof(g_astCombox)/sizeof(ComboxInfo_t); i++)
    {
        if (g_astCombox[i].nPageIdx == nPageIdx && g_astCombox[i].nSubPageIdx == nSubPageIdx && g_astCombox[i].nItemIdx == nItemIdx)
        {
            g_astCombox[i].hComBox = hwnd;

            if (g_astCombox[i].ppText)
            {
                for (j = 0; j < g_astCombox[i].nMaxItemCnt; j++)
                {
                    SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)g_astCombox[i].ppText[j]);
                    //SendMessage(hwnd, CB_INSERTSTRING, 0, (LPARAM)g_astCombox[i].ppText[j]);
                }
                SendMessage(hwnd, CB_SETCURSEL, 0, 0);
            }

            break;
        }
    }
}

int GetComboxCurSelect(HWND hwnd)
{
    int i = 0;
    int nCurSelect = 0;

    for (i = 0; i < sizeof(g_astCombox)/sizeof(ComboxInfo_t); i++)
    {
        if (g_astCombox[i].hComBox == hwnd)
        {
            nCurSelect = g_astCombox[i].nCurSelect;
            break;
        }
    }

    return nCurSelect;
}

void SetComboxCurSelect(HWND hwnd, int nCurSelect)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astCombox)/sizeof(ComboxInfo_t); i++)
    {
        if (g_astCombox[i].hComBox == hwnd)
        {
            g_astCombox[i].nCurSelect = nCurSelect;
            break;
        }
    }
}

char *GetComboxCurText(HWND hwnd)
{
    int i = 0;

    for (i = 0; i < sizeof(g_astCombox)/sizeof(ComboxInfo_t); i++)
    {
        if (g_astCombox[i].hComBox == hwnd)
        {
            if (g_astCombox[i].ppText)
            {
                return g_astCombox[i].ppText[g_astCombox[i].nCurSelect];
            }
        }
    }

    return NULL;
}

static void ComboxNotifyProc(HWND hwnd, LINT id, int nc, DWORD add_data)
{
    int nCurSelect = -1;
    int i = 0;

    if (CBN_SELCHANGE == nc)
    {
        nCurSelect = SendMessage(hwnd, CB_GETCURSEL, 0, 0);

        if (nCurSelect >= 0)
        {
            SetComboxCurSelect(hwnd, nCurSelect);

            for (i = 0; i < sizeof(g_astCombox)/sizeof(ComboxInfo_t); i++)
            {
                if (g_astCombox[i].hComBox == hwnd)
                {
                    g_astCombox[i].comboxProc(hwnd, nCurSelect);
                    break;
                }
            }
        }
    }
}

//=============== FileList functions =================
// create file tree
typedef struct _FileTree_t
{
    char name[256];
    char time[32];
    int dirFlag;
    int depth;
    int childCnt;
    long size;
    list_t headNodeList;
    list_t childNodeList;
} FileTree_t;

static int bigMonth[] = {1, 3, 5, 7, 8, 10, 12};
static FileTree_t g_fileRoot;

char *getDayOfWeek(int day)
{
    switch (day)
    {
        case 0:
            return "周日";
        case 1:
            return "周一";
        case 2:
            return "周二";
        case 3:
            return "周三";
        case 4:
            return "周四";
        case 5:
            return "周五";
        case 6:
            return "周六";
    }

    printf("invalid day %d\n", day);
    return NULL;
}

// month: [1, 12]
int getBigMonthCount(int month)
{
    int i = 0;
    int count = 0;

    for (i = 0; i < sizeof(bigMonth)/sizeof(int); i++)
    {
        if (month > bigMonth[i])
            count++;
        else
            break;
    }

    //printf("count is %d, curMonth is %d\n", count, month);
    return count;
}

// month: [1, 12]
int getDateOfMonth(int year, int month, int date)
{
    int febDays = 28;
    int monthDays = 30;
    int curMonthDate = 0;

    if ((year%400 == 0) || (year%100 != 0 && year%4 == 0))
    {
        febDays = 29;
    }

    if (month > 2)
    {
        curMonthDate = date - ((month-2)*monthDays + febDays + getBigMonthCount(month));
    }
    else if (month > 1)
    {
        curMonthDate = date - ((month-1)*monthDays + getBigMonthCount(month));
    }
    else
    {
        curMonthDate = date;
    }

    //printf("date is %d, curMonthDate is %d, febDays is %d\n", date, curMonthDate, febDays);
    return curMonthDate;
}

void InitFileTreeRoot(FileTree_t *root, char *pRootName)
{
    memset(root, 0, sizeof(FileTree_t));
    root->dirFlag = 1;
    strcpy(root->name, pRootName);
    INIT_LIST_HEAD(&root->headNodeList);
    INIT_LIST_HEAD(&root->childNodeList);
}

int CreateFileTree(FileTree_t *root)
{
    DIR *pDir =  NULL;
    struct dirent *ent;
    struct stat statbuf;
    struct tm *modifytm;
    FileTree_t *child = NULL;
    int dirFlag = 0;

    pDir = opendir(root->name);
    if (!pDir)
    {
        //printf("%s directory is not exist or open failed\n", root->name);
        return -1;
    }

    lstat(root->name, &statbuf);
    modifytm = localtime(&(statbuf.st_mtime));
    root->size = statbuf.st_size;
    sprintf(root->time, "%d/%d/%d %d:%d:%d %s", modifytm->tm_year+1900, modifytm->tm_mon+1,
            getDateOfMonth(modifytm->tm_year+1900, modifytm->tm_mon+1, modifytm->tm_yday+1),
            modifytm->tm_hour, modifytm->tm_min, modifytm->tm_sec,
            getDayOfWeek(modifytm->tm_wday));

    while ((ent=readdir(pDir)) != NULL)
    {
        if (ent->d_type & DT_DIR)
        {
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
                continue;

            dirFlag = 1;
        }
        else
        {
            dirFlag = 0;
        }

        child = (FileTree_t*)malloc(sizeof(FileTree_t));
        memset(child, 0, sizeof(FileTree_t));
        INIT_LIST_HEAD(&child->headNodeList);;
        INIT_LIST_HEAD(&child->childNodeList);
        if (!strcmp(root->name, "/"))
            sprintf(child->name, "%s%s", root->name, ent->d_name);
        else
            sprintf(child->name, "%s/%s", root->name, ent->d_name);
        child->depth = root->depth + 1;
        root->childCnt++;
        list_add_tail(&child->childNodeList, &root->headNodeList);

        if (dirFlag)
        {
            child->dirFlag = 1;
            CreateFileTree(child);
        }
        else
        {
            child->dirFlag = 0;
            lstat(child->name, &statbuf);
            modifytm = localtime(&(statbuf.st_mtime));
            child->size = statbuf.st_size;
            sprintf(child->time, "%d/%d/%d %d:%d:%d %s", modifytm->tm_year+1900, modifytm->tm_mon+1,
                    getDateOfMonth(modifytm->tm_year+1900, modifytm->tm_mon+1, modifytm->tm_yday+1),
                    modifytm->tm_hour, modifytm->tm_min, modifytm->tm_sec,
                    getDayOfWeek(modifytm->tm_wday));
        }
    }

    closedir(pDir);
    pDir = NULL;

    return 0;
}

void BrowseFileTree(FileTree_t *root)
{
    FileTree_t *pos = NULL;

    if (!root)
    {
        printf("FileTree is not exist\n");
        return;
    }

    printf("name:%-50sdirFlag:%-5dsize:%-8ld KB time:%-32sdepth:%-5dchildCnt:%-5d\n", root->name, root->dirFlag, (root->size+1023)/1024,
            root->time, root->depth, root->childCnt);

    if (list_empty(&root->headNodeList))
    {
        return;
    }

    list_for_each_entry(pos, &root->headNodeList, childNodeList)
    {
        BrowseFileTree(pos);
    }
}

FileTree_t *FindFileTreeNode(FileTree_t *root, char *name)
{
    FileTree_t *pos = NULL;
    FileTree_t *result = NULL;

    if (!root)
        return NULL;

    if (!strcmp(root->name, name))
        return root;

    if (list_empty(&root->headNodeList))
    {
        //printf("%s is not exist in %s\n", name, root->name);
        return NULL;
    }

    list_for_each_entry(pos, &(root->headNodeList), childNodeList)
    {
        if ((result = FindFileTreeNode(pos, name)) != NULL)
            return result;
    }

    return NULL;
}

int InsertFileTreeNode(FileTree_t *father, FileTree_t *child)
{
    child->depth = father->depth + 1;
    father->childCnt++;
    INIT_LIST_HEAD(&child->childNodeList);
    list_add_tail(&child->childNodeList, &father->headNodeList);

    return 0;
}

int DestroyFileTree(FileTree_t *root)
{
    FileTree_t *pos = NULL;
    FileTree_t *posN = NULL;

    if (!root)
    {
        printf("root is NULL\n");
        return 0;
    }

    if (list_empty(&root->headNodeList))
    {
    //    printf("FileTree is empty\n");
        free(root);
        root = NULL;
        return 0;
    }

    list_for_each_entry_safe(pos, posN, &root->headNodeList, childNodeList)
    {
        DestroyFileTree(pos);
    }

    return 0;
}

int DeleteFileTreeNode(FileTree_t *father, FileTree_t *child)
{
    list_del(&child->childNodeList);
    child->depth = 0;
    father->childCnt--;

    DestroyFileTree(child);
    return 0;
}


// sort files
// sort by name asc

// sort by name desc

// sort by last modify time desc

// sort by last modify time asc

// sort by size asc

// sort by size desc


// detect usb dir
char *detectUsbDir()
{
    char *pDir = NULL;


    return pDir;
}


#define DIR_TYPE        "文件夹"
#define FILE_TYPE       "文件"
//#define DIR_TYPE        "Dir"
//#define FILE_TYPE       "File"

#define COL_DIV         16
#define COL_GAP         2
#define ROW_HEIGHT      40

#define COL_NUM         TABLESIZE(g_lv_caption)


typedef struct _FILEINFO
{
    int fileFlag;
    char *pFileName;
    char *pSize;
    char *pDate;
}FILEINFO;

typedef struct _DIRINFO
{
    char *pDirName;         // 绝对路径
    FILEINFO *pChildList;
    int nChildNum;
}DIRINFO;

static char * g_lv_caption[] =
{
    "名称", "类型", "大小", "修改日期"
};

static int g_colDiv[] =
{
    5, 2, 3, 6
};

static NCS_RDR_INFO g_rdr_info[] = {
    { "fashion", "fashion", NULL },
};

static ST_Rect_T g_stPlayToolBarItem[11] = {{900, 540, 124, 60}, {800, 100, 124, 60}, {800, 100, 124, 60}, {800, 200, 124, 60}, {800, 200, 124, 60}, {800, 200, 124, 60},
                                            {800, 300, 124, 60}, {800, 300, 124, 60}, {800, 300, 124, 60}, {800, 300, 124, 60}, {800, 300, 124, 60}};

static PLOGFONT g_pPlayFileFont = NULL;
static MI_BOOL g_bMoveInUpBtn = FALSE;

static PLOGFONT createExplorerFont(unsigned size);
static void lv_notify(mWidget *self, int id, int nc, DWORD add_data);
static void updir_btn_notify(mWidget *button, int id, int nc, DWORD add_data);
static void play_btn_notify(mWidget *button, int id, int nc, DWORD add_data);
static void stop_btn_notify(mWidget *button, int id, int nc, DWORD add_data);
static void playslow_btn_notify(mWidget *button, int id, int nc, DWORD add_data);
static void playfast_btn_notify(mWidget *button, int id, int nc, DWORD add_data);
static void add_fileinfo_item(mListView *self, NCS_LISTV_ITEMINFO *info, FileTree_t *pFileNode);
static void playFileWnd_notify(mWidget *self, int message, int code, DWORD usrData);
static void play_trk_notify(mTrackBar* self, int id, int code, DWORD add_data);
static void SetBtnImg(mButton *pBtnObj, PBITMAP pBmp);
static void show_lv_page(mListView *lvObj, MI_BOOL bShow);
static void show_playfile_page(mListView *lvObj, MI_BOOL bShow);


static NCS_EVENT_HANDLER lv_handlers [] = {
    NCS_MAP_NOTIFY(NCSN_LISTV_SELCHANGED, lv_notify),
    NCS_MAP_NOTIFY(NCSN_WIDGET_CLICKED, lv_notify),
    {0, NULL}
};

static NCS_EVENT_HANDLER updir_btn_handlers [] = {
    NCS_MAP_NOTIFY(NCSN_WIDGET_CLICKED, updir_btn_notify),
    {0, NULL}
};

static NCS_EVENT_HANDLER play_btn_handlers [] = {
    NCS_MAP_NOTIFY(NCSN_WIDGET_CLICKED, play_btn_notify),
    {0, NULL}
};

static NCS_EVENT_HANDLER stop_btn_handlers [] = {
    NCS_MAP_NOTIFY(NCSN_WIDGET_CLICKED, stop_btn_notify),
    {0, NULL}
};

static NCS_EVENT_HANDLER playslow_btn_handlers [] = {
    NCS_MAP_NOTIFY(NCSN_WIDGET_CLICKED, playslow_btn_notify),
    {0, NULL}
};

static NCS_EVENT_HANDLER playfast_btn_handlers [] = {
    NCS_MAP_NOTIFY(NCSN_WIDGET_CLICKED, playfast_btn_notify),
    {0, NULL}
};

static NCS_EVENT_HANDLER play_trk_handlers [] = {
    NCS_MAP_NOTIFY(NCSN_TRKBAR_CHANGED, play_trk_notify),
    {0, NULL}
};

static NCS_PROP_ENTRY play_trk_props [] = {
    {NCSP_TRKBAR_MINPOS, 0},
    {NCSP_TRKBAR_MAXPOS, 760},
    {NCSP_TRKBAR_CURPOS, 0},
    {0, 0}
};

static NCS_WND_TEMPLATE _playFileWnd_ctrl_tmpl[] = {
    // updirBtn/returnBtn
    {
        NCSCTRL_BUTTON,
        IDC_PLAYFILE_BUTTON_UPPER_DIR,
        240, 255, 80, 30,
        WS_VISIBLE | NCSS_NOTIFY | NCSS_BUTTON_IMAGE,
        WS_EX_NONE,
        "updir",
        NULL,
        NULL,
        updir_btn_handlers,
        NULL,
        0,
        0
    },

    // current dirPath label
    {
        NCSCTRL_STATIC,
        IDC_PLAYFILE_STATIC_DIR_NAME,
        240, 255, 80, 30,
        WS_VISIBLE,
        WS_EX_TRANSPARENT,
        "",
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        0
    },

    // play files listview
    {
        NCSCTRL_LISTVIEW,
        IDC_PLAYFILE_LISTVIEW_FILELIST,
        10, 10, 320, 220,
        WS_BORDER | WS_VISIBLE | NCSS_LISTV_SORT | NCSS_LISTV_NOTIFY,
        WS_EX_NONE,
        "play file list",
        NULL,
        g_rdr_info,
        lv_handlers,
        NULL,
        0,
        0
    },

    // playtoolbar container static
    {
        NCSCTRL_STATIC,
        IDC_PLAYFILE_STATIC_PLAYTOOLBAR,
        240, 255, 80, 30,
        WS_NONE,        //WS_VISIBLE,
        WS_EX_NONE,
        "",
        NULL,
        g_rdr_info,
        NULL,
        NULL,
        0,
        0
    },

    // play progress trackbar
    {
        NCSCTRL_TRACKBAR,
        IDC_PLAYFILE_TRACKBAR_PLAY_PROGRESS,
        10, 260, 240, 40,
        NCSS_TRKBAR_HORIZONTAL | NCSS_TRKBAR_NOTICK | NCSS_NOTIFY, // WS_VISIBLE
        WS_EX_TRANSPARENT,
        "",
        play_trk_props,
        //play_trk_rdr_info,
        g_rdr_info,
        play_trk_handlers,
        NULL,
        0,
        0,
    },

    // play/pause btn
    {
        NCSCTRL_BUTTON,
        IDC_PLAYFILE_BUTTON_PLAY_PAUSE,
        240, 255, 80, 30,
        NCSS_NOTIFY | NCSS_BUTTON_IMAGE,        //WS_VISIBLE
        WS_EX_NONE,
        "confirm",
        NULL,
        g_rdr_info,
        play_btn_handlers,
        NULL,
        0,
        0
    },

    // stop btn
    {
        NCSCTRL_BUTTON,
        IDC_PLAYFILE_BUTTON_STOP,
        240, 255, 80, 30,
        NCSS_NOTIFY | NCSS_BUTTON_IMAGE,        //WS_VISIBLE
        WS_EX_NONE,
        "confirm",
        NULL,
        g_rdr_info,
        stop_btn_handlers,
        NULL,
        0,
        0
    },

    // play slow btn
    {
        NCSCTRL_BUTTON,
        IDC_PLAYFILE_BUTTON_PLAY_SLOW,
        240, 255, 80, 30,
        NCSS_NOTIFY | NCSS_BUTTON_IMAGE,        //WS_VISIBLE
        WS_EX_NONE,
        "confirm",
        NULL,
        g_rdr_info,
        playslow_btn_handlers,
        NULL,
        0,
        0
    },

    // play fast btn
    {
        NCSCTRL_BUTTON,
        IDC_PLAYFILE_BUTTON_PLAY_FAST,
        240, 255, 80, 30,
        NCSS_NOTIFY | NCSS_BUTTON_IMAGE,        //WS_VISIBLE
        WS_EX_NONE,
        "confirm",
        NULL,
        g_rdr_info,
        playfast_btn_handlers,
        NULL,
        0,
        0
    },

    // play speed label
    {
        NCSCTRL_STATIC,
        IDC_PLAYFILE_STATIC_SPEED_MODE,
        240, 255, 80, 30,
        WS_NONE,        //WS_VISIBLE,
        WS_EX_TRANSPARENT,
        "",
        NULL,
        g_rdr_info,
        NULL,
        NULL,
        0,
        0
    },

    // play time label
    {
        NCSCTRL_STATIC,
        IDC_PLAYFILE_STATIC_PLAY_TIME,
        240, 255, 80, 30,
        WS_NONE,        //WS_VISIBLE,
        WS_EX_TRANSPARENT,
        "",
        NULL,
        g_rdr_info,
        NULL,
        NULL,
        0,
        0
    },
};

static NCS_EVENT_HANDLER playFileWnd_handlers[] = {
    {MSG_CLOSE, (void *)playFileWnd_notify},
    {0, NULL}
};


static NCS_MNWND_TEMPLATE playFileWnd_tmpl = {
    NCSCTRL_DIALOGBOX,
    IDC_PLAYFILEPAGE_WINNOW,
    0, 0, MAINWND_W, MAINWND_H,
    WS_VISIBLE,
    WS_EX_NONE,
    "",
    NULL,
    g_rdr_info,
    playFileWnd_handlers,
    _playFileWnd_ctrl_tmpl,
    sizeof(_playFileWnd_ctrl_tmpl)/sizeof(NCS_WND_TEMPLATE),
    0,
    0, 0,
};

static void playFileWnd_notify(mWidget *self, int message, int code, DWORD usrData)
{
    if (message == MSG_CLOSE)
    {
        printf("playFileWnd close...\n");
        g_bMoveInUpBtn = FALSE;
        printf("close playFileWnd, bMoveIn:%d\n", g_bMoveInUpBtn);
        DestroyLogFont(g_pPlayFileFont);
        ncsDestroyWindow(self, 0);
        hMainPlayFileWnd = HWND_INVALID;
    }
}

static void show_lv_page(mListView *lvObj, MI_BOOL bShow)
{
    mStatic *pathObj = (mStatic*)ncsGetChildObj(GetParent(lvObj->hwnd), IDC_PLAYFILE_STATIC_DIR_NAME);
    mButton *upDirObj = (mButton*)ncsGetChildObj(GetParent(lvObj->hwnd), IDC_PLAYFILE_BUTTON_UPPER_DIR);

    printf("show lv page\n");

    if (bShow)
    {
        IncludeWindowStyle(upDirObj->hwnd, WS_VISIBLE);
        IncludeWindowStyle(lvObj->hwnd, WS_VISIBLE);
        IncludeWindowStyle(pathObj->hwnd, WS_VISIBLE);
    }
    else
    {
        ExcludeWindowStyle(upDirObj->hwnd, WS_VISIBLE);
        ExcludeWindowStyle(lvObj->hwnd, WS_VISIBLE);
        ExcludeWindowStyle(pathObj->hwnd, WS_VISIBLE);
    }
}

static void show_playfile_page(mListView *lvObj, MI_BOOL bShow)
{
    mStatic *pToolBarObj = (mStatic*)ncsGetChildObj(GetParent(lvObj->hwnd), IDC_PLAYFILE_STATIC_PLAYTOOLBAR);
    mStatic *pSpeedModeObj = (mStatic*)ncsGetChildObj(GetParent(lvObj->hwnd), IDC_PLAYFILE_STATIC_SPEED_MODE);
    mStatic *pPlayTimeObj = (mStatic*)ncsGetChildObj(GetParent(lvObj->hwnd), IDC_PLAYFILE_STATIC_PLAY_TIME);
    mTrackBar *pProgressObj = (mTrackBar*)ncsGetChildObj(GetParent(lvObj->hwnd), IDC_PLAYFILE_TRACKBAR_PLAY_PROGRESS);
    mButton *pPlayBtnObj = (mButton*)ncsGetChildObj(GetParent(lvObj->hwnd), IDC_PLAYFILE_BUTTON_PLAY_PAUSE);
    mButton *pStopBtnObj = (mButton*)ncsGetChildObj(GetParent(lvObj->hwnd), IDC_PLAYFILE_BUTTON_STOP);
    mButton *pSlowBtnObj = (mButton*)ncsGetChildObj(GetParent(lvObj->hwnd), IDC_PLAYFILE_BUTTON_PLAY_SLOW);
    mButton *pFastBtnObj = (mButton*)ncsGetChildObj(GetParent(lvObj->hwnd), IDC_PLAYFILE_BUTTON_PLAY_FAST);

    printf("show playfile page\n");

    if (bShow)
    {
        IncludeWindowStyle(pToolBarObj->hwnd, WS_VISIBLE);
        IncludeWindowStyle(pSpeedModeObj->hwnd, WS_VISIBLE);
        IncludeWindowStyle(pPlayTimeObj->hwnd, WS_VISIBLE);
        IncludeWindowStyle(pProgressObj->hwnd, WS_VISIBLE);
        IncludeWindowStyle(pPlayBtnObj->hwnd, WS_VISIBLE);
        IncludeWindowStyle(pStopBtnObj->hwnd, WS_VISIBLE);
        IncludeWindowStyle(pSlowBtnObj->hwnd, WS_VISIBLE);
        IncludeWindowStyle(pFastBtnObj->hwnd, WS_VISIBLE);
    }
    else
    {
        ExcludeWindowStyle(pToolBarObj->hwnd, WS_VISIBLE);
        ExcludeWindowStyle(pSpeedModeObj->hwnd, WS_VISIBLE);
        ExcludeWindowStyle(pPlayTimeObj->hwnd, WS_VISIBLE);
        ExcludeWindowStyle(pProgressObj->hwnd, WS_VISIBLE);
        ExcludeWindowStyle(pPlayBtnObj->hwnd, WS_VISIBLE);
        ExcludeWindowStyle(pStopBtnObj->hwnd, WS_VISIBLE);
        ExcludeWindowStyle(pSlowBtnObj->hwnd, WS_VISIBLE);
        ExcludeWindowStyle(pFastBtnObj->hwnd, WS_VISIBLE);
    }
}

static void lv_notify(mWidget *self, int id, int nc, DWORD add_data)
{
    mListView *lstvObj = (mListView*)self;

    if (!lstvObj)
        return;

    // event occur order: selchanged clicked itemdbcliked
    if (nc == NCSN_LISTV_SELCHANGED)
    {
        g_s32SelectIdx = _c(lstvObj)->indexOf(lstvObj, (HITEM)add_data);
        printf("select item index is %d\n", g_s32SelectIdx);
    }

    if (nc == NCSN_WIDGET_CLICKED)
    {
        const char *fileName = _c(lstvObj)->getItemText(lstvObj, g_s32SelectIdx, 0);
        const char *type = _c(lstvObj)->getItemText(lstvObj, g_s32SelectIdx, 1);
        mStatic *pathObj = (mStatic*)ncsGetChildObj(GetParent(lstvObj->hwnd), IDC_PLAYFILE_STATIC_DIR_NAME);
        mStatic *pSpeedModeObj = (mStatic*)ncsGetChildObj(GetParent(lstvObj->hwnd), IDC_PLAYFILE_STATIC_SPEED_MODE);
        NCS_LISTV_ITEMINFO info;
        FileTree_t *curFileNode = NULL;
        int i = 0;
        int color = 0x00FFFF;

        if (!pathObj)
            return;

        printf("select item: name is %s, type is %s\n", fileName, type);

        // confirm btn is disable utill selected file
        if (!strcmp(type, FILE_TYPE))
        {
            // save selectpath & play file
            memset(g_pu8SelectPath, 0, sizeof(g_pu8SelectPath));
            sprintf((char*)g_pu8SelectPath, "%s/%s", (char*)g_pu8FullPath, fileName);

            // hide
            _c(pSpeedModeObj)->setProperty(pSpeedModeObj, NCSP_WIDGET_TEXT, (DWORD)"");
            g_bShowPlayToolBar = TRUE;
            show_lv_page(lstvObj, FALSE);
            show_playfile_page(lstvObj, TRUE);
            g_ePlayDirection = E_PLAY_FORWARD;
            g_ePlayMode = E_PLAY_NORMAL_MODE;
            g_eSpeedMode = E_NORMAL_SPEED;
            g_u32SpeedNumerator = 1;
            g_u32SpeedDenomonator = 1;
            InvalidateRect(GetParent(lstvObj->hwnd), NULL, TRUE);

            // sendmessage to play file
        }
        else
        {
            for (i = 0; i < COL_NUM; i++)
            {
                _c(lstvObj)->setBackground(lstvObj, g_s32SelectIdx, i, &color);
            }

            info.height     = ROW_HEIGHT;
            info.flags      = 0;
            info.foldIcon   = 0;
            info.unfoldIcon = 0;
            info.parent = 0;

            if (!strcmp(type, DIR_TYPE))
            {
                if (!strcmp((char*)g_pu8FullPath, "/"))
                    sprintf((char*)g_pu8FullPath, "%s%s", (char*)g_pu8FullPath, fileName);
                else
                    sprintf((char*)g_pu8FullPath, "%s/%s", (char*)g_pu8FullPath, fileName);

                _c(pathObj)->setProperty(pathObj, NCSP_WIDGET_TEXT, (DWORD)g_pu8FullPath);
                curFileNode = FindFileTreeNode(&g_fileRoot, (char*)g_pu8FullPath);
                printf("notify, fullPath:%s pFileNode:%s\n", (char*)g_pu8FullPath, curFileNode?curFileNode->name:"null");

                if (curFileNode)
                {
                    _c(lstvObj)->freeze(lstvObj, TRUE);
                    _c(lstvObj)->removeAll(lstvObj);
                    add_fileinfo_item(lstvObj, &info, curFileNode);
                    _c(lstvObj)->freeze(lstvObj, FALSE);
                }
            }
        }
    }
}

static void updir_btn_notify(mWidget *button, int id, int nc, DWORD add_data)
{
    mListView *lvObj = NULL;
    mStatic *pathObj = NULL;
    NCS_LISTV_ITEMINFO info;
    int i  = 0;
    int color = 0x00FFFF;

    printf("Clicked updir btn\n");
    // do recollection stuff
    if (!button)
        return;

    lvObj = (mListView*)ncsGetChildObj(GetParent(button->hwnd), IDC_PLAYFILE_LISTVIEW_FILELIST);
    pathObj = (mStatic*)ncsGetChildObj(GetParent(button->hwnd), IDC_PLAYFILE_STATIC_DIR_NAME);

    if(nc == NCSN_WIDGET_CLICKED)
    {
        g_ePlayDirection = E_PLAY_FORWARD;
        g_ePlayMode = E_PLAY_NORMAL_MODE;
        g_eSpeedMode = E_NORMAL_SPEED;
        g_bMoveInUpBtn = FALSE;

        if (g_bShowPlayToolBar)
        {
            // sendmessage to stop playing file

            g_bShowPlayToolBar = FALSE;
            show_playfile_page(lvObj, FALSE);
            show_lv_page(lvObj, TRUE);
            InvalidateRect(GetParent(lvObj->hwnd), NULL, TRUE);
        }
        else
        {
            if (!strcmp((char*)g_pu8FullPath, (char*)g_pu8RootPath))
            {
                //PostMessage(GetParent(button->hwnd), MSG_CLOSE, 0, 0);
                PostMessage(hMainPlayFileWnd, MSG_CLOSE, 0, 0);
            }
            else
            {
                FileTree_t *curFileNode = NULL;
                char *p = strrchr((char*)g_pu8FullPath, '/');
                *p = 0;

                _c(pathObj)->setProperty(pathObj, NCSP_WIDGET_TEXT, (DWORD)g_pu8FullPath);
                curFileNode = FindFileTreeNode(&g_fileRoot, (char*)g_pu8FullPath);
                printf("notify, fullPath:%s pFileNode:%s\n", (char*)g_pu8FullPath, curFileNode?curFileNode->name:"null");

                if (curFileNode)
                {

                    for (i = 0; i < COL_NUM; i++)
                    {
                        _c(lvObj)->setBackground(lvObj, g_s32SelectIdx, i, &color);
                    }

                    info.height     = ROW_HEIGHT;
                    info.flags      = 0;
                    info.foldIcon   = 0;
                    info.unfoldIcon = 0;
                    info.parent = 0;

                    _c(lvObj)->freeze(lvObj, TRUE);
                    _c(lvObj)->removeAll(lvObj);
                    add_fileinfo_item(lvObj, &info, curFileNode);
                    _c(lvObj)->freeze(lvObj, FALSE);
                }
            }
        }
    }
}

static void play_btn_notify(mWidget *button, int id, int nc, DWORD add_data)
{
    mButton *pBtnObj = (mButton*)button;
    printf("Clicked play/pause btn\n");

    if (!button)
        return;

    if(nc == NCSN_WIDGET_CLICKED)
    {
        if (g_bPlaying)
        {
            g_bPause = !g_bPause;
            // sendmessage to pause/resume playing
        }
        else
        {
            g_bPlaying = TRUE;
            g_bPause = FALSE;

            // sendmessage to start playing
        }

        if (g_bPause)
            SetBtnImg(pBtnObj, &g_play_btn);
        else
            SetBtnImg(pBtnObj, &g_pause_btn);
    }
}

static void stop_btn_notify(mWidget *button, int id, int nc, DWORD add_data)
{
    mButton *pBtnObj = (mButton*)ncsGetChildObj(GetParent(button->hwnd), IDC_PLAYFILE_BUTTON_PLAY_PAUSE);
    mStatic *pSpeedModeObj = (mStatic*)ncsGetChildObj(GetParent(button->hwnd), IDC_PLAYFILE_STATIC_SPEED_MODE);
    mListView *lvObj = (mListView*)ncsGetChildObj(GetParent(button->hwnd), IDC_PLAYFILE_LISTVIEW_FILELIST);

    printf("Clicked stop btn\n");

    if (!button)
        return;

    if(nc == NCSN_WIDGET_CLICKED)
    {
        g_bPlaying = FALSE;
        g_bPause = FALSE;

        // sendmessage to stop playing

        SetBtnImg(pBtnObj, &g_play_btn);
        g_ePlayDirection = E_PLAY_FORWARD;
        g_ePlayMode = E_PLAY_NORMAL_MODE;
        g_eSpeedMode = E_NORMAL_SPEED;
        g_u32SpeedNumerator = 1;
        g_u32SpeedDenomonator = 1;
        g_bMoveInUpBtn = FALSE;

        // sendmessage to stop playing file

        _c(pSpeedModeObj)->setProperty(pSpeedModeObj, NCSP_WIDGET_TEXT, (DWORD)"");
        g_bShowPlayToolBar = FALSE;
        show_playfile_page(lvObj, FALSE);
        show_lv_page(lvObj, TRUE);
        InvalidateRect(GetParent(lvObj->hwnd), NULL, TRUE);

        //PostMessage(GetParent(button->hwnd), MSG_CLOSE, 0, 0);
    }
}

static void playslow_btn_notify(mWidget *button, int id, int nc, DWORD add_data)
{
    mStatic *pSpeedModeObj = (mStatic*)ncsGetChildObj(GetParent(button->hwnd), IDC_PLAYFILE_STATIC_SPEED_MODE);
    char speedMode[16] = {0};

    printf("Clicked slow btn\n");

    if (!button || !pSpeedModeObj)
        return;

    if (!g_bPlaying)
        return;

    if(nc == NCSN_WIDGET_CLICKED)
    {
        if (g_ePlayDirection == E_PLAY_FORWARD)
        {
            // slow down
            if (g_ePlayMode == E_PLAY_FAST_MODE)
            {
                g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode - 1);
                g_u32SpeedNumerator = 1 << (int)g_eSpeedMode;
                g_u32SpeedDenomonator = 1;

                if (g_eSpeedMode == E_NORMAL_SPEED)
                    g_ePlayMode = E_PLAY_NORMAL_MODE;
            }
            else
            {
                if (g_eSpeedMode < E_32X_SPEED)
                {
                    g_ePlayMode = E_PLAY_SLOW_MODE;
                    g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode + 1);
                }
                else    // turn to play backward
                {
                    g_ePlayDirection = E_PLAY_BACKWARD;
                    g_ePlayMode = E_PLAY_NORMAL_MODE;
                    g_eSpeedMode = E_NORMAL_SPEED;
                }

                g_u32SpeedNumerator = 1;
                g_u32SpeedDenomonator = 1 << (int)g_eSpeedMode;
            }
        }
        else
        {
            // speed up
            if (g_ePlayMode == E_PLAY_SLOW_MODE)
            {
                g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode - 1);
                g_u32SpeedNumerator = 1;
                g_u32SpeedDenomonator = 1 << (int)g_eSpeedMode;

                if (g_eSpeedMode == E_NORMAL_SPEED)
                {
                    g_ePlayMode = E_PLAY_NORMAL_MODE;
                }
            }
            else
            {
                if (g_eSpeedMode < E_32X_SPEED)
                {
                    g_ePlayMode = E_PLAY_FAST_MODE;
                    g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode + 1);
                    g_u32SpeedNumerator = 1 << (int)g_eSpeedMode;
                    g_u32SpeedDenomonator = 1;
                }
            }
        }

        memset(speedMode, 0, sizeof(speedMode));
        if (g_u32SpeedNumerator == g_u32SpeedDenomonator)
            sprintf(speedMode, "", g_u32SpeedNumerator);
        else if (g_u32SpeedNumerator > g_u32SpeedDenomonator)
            sprintf(speedMode, "%dX %s", g_u32SpeedNumerator, ((g_ePlayDirection == E_PLAY_FORWARD) ? ">>" : "<<"));
        else
            sprintf(speedMode, "1/%dX %s", g_u32SpeedDenomonator, ((g_ePlayDirection == E_PLAY_FORWARD) ? ">>" : "<<"));
        _c(pSpeedModeObj)->setProperty(pSpeedModeObj, NCSP_WIDGET_TEXT, (DWORD)speedMode);

        // sendmessage to adjust speed
    }
}

static void playfast_btn_notify(mWidget *button, int id, int nc, DWORD add_data)
{
    mStatic *pSpeedModeObj = (mStatic*)ncsGetChildObj(GetParent(button->hwnd), IDC_PLAYFILE_STATIC_SPEED_MODE);
    char speedMode[16] = {0};

    printf("Clicked fast btn\n");

    if (!button || !pSpeedModeObj)
        return;

    if (!g_bPlaying)
        return;

    if(nc == NCSN_WIDGET_CLICKED)
    {
        if (g_ePlayDirection == E_PLAY_FORWARD)
        {
            // speed up
            if (g_ePlayMode == E_PLAY_SLOW_MODE)
            {
                g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode - 1);
                g_u32SpeedNumerator = 1;
                g_u32SpeedDenomonator = 1 << (int)g_eSpeedMode;

                if (g_eSpeedMode == E_NORMAL_SPEED)
                    g_ePlayMode = E_PLAY_NORMAL_MODE;
            }
            else
            {
                if (g_eSpeedMode < E_32X_SPEED)
                {
                    g_ePlayMode = E_PLAY_FAST_MODE;
                    g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode + 1);
                    g_u32SpeedNumerator = 1 << (int)g_eSpeedMode;
                    g_u32SpeedDenomonator = 1;
                }
            }
        }
        else
        {
            // slow down
            if (g_ePlayMode == E_PLAY_FAST_MODE)
            {
                g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode - 1);
                g_u32SpeedNumerator = 1 << (int)g_eSpeedMode;
                g_u32SpeedDenomonator = 1;

                if (g_eSpeedMode == E_NORMAL_SPEED)
                    g_ePlayMode = E_PLAY_NORMAL_MODE;
            }
            else
            {
                // 1/32X speed backward to normal speed forward
                if (g_eSpeedMode == E_32X_SPEED)
                {
                    g_eSpeedMode = E_NORMAL_SPEED;
                    g_ePlayMode = E_PLAY_NORMAL_MODE;
                    g_ePlayDirection = E_PLAY_FORWARD;
                }
                else
                {
                    g_ePlayMode = E_PLAY_SLOW_MODE;
                    g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode + 1);
                }

                g_u32SpeedNumerator = 1;
                g_u32SpeedDenomonator = 1 << (int)g_eSpeedMode;
            }
        }

        memset(speedMode, 0, sizeof(speedMode));
        if (g_u32SpeedNumerator == g_u32SpeedDenomonator)
            sprintf(speedMode, "", g_u32SpeedNumerator);
        else if (g_u32SpeedNumerator > g_u32SpeedDenomonator)
            sprintf(speedMode, "%dX %s", g_u32SpeedNumerator, ((g_ePlayDirection == E_PLAY_FORWARD) ? ">>" : "<<"));
        else
            sprintf(speedMode, "1/%dX %s", g_u32SpeedDenomonator, ((g_ePlayDirection == E_PLAY_FORWARD) ? ">>" : "<<"));
        _c(pSpeedModeObj)->setProperty(pSpeedModeObj, NCSP_WIDGET_TEXT, (DWORD)speedMode);

        // sendmessage to adjust speed
    }
}

static void play_trk_notify(mTrackBar* self, int id, int code, DWORD add_data)
{
    printf("play trackbar notify\n");
}

static void add_fileinfo_item(mListView *self, NCS_LISTV_ITEMINFO *info, FileTree_t *pFileNode)
{
    int i = 0, j = 0, insertNum = 0;
    NCS_LISTV_ITEMDATA subdata[COL_NUM];
    int color = 0xFFFFFF;
    int nRowIdx = 0;
    FileTree_t *pos = NULL;

    // font
    HDC clientDc = NULL;
    PLOGFONT logfont;
    FONTMETRICS fm;

    if (!pFileNode)
        return;

    clientDc = GetClientDC(self->hwnd);
    logfont = GetWindowFont(self->hwnd);
    GetFontMetrics(logfont, &fm);
    printf("old font size is max_w:%d avg_w:%d h:%d\n", fm.max_width, fm.ave_width, fm.font_height);

    SelectFont(clientDc, g_pPlayFileFont);
    logfont = GetWindowFont(self->hwnd);
    GetFontMetrics(logfont, &fm);
    printf("current font size is max_w:%d avg_w:%d h:%d\n", fm.max_width, fm.ave_width, fm.font_height);

    info->dataSize = COL_NUM;
    info->data = subdata;

    printf("filenode: depth:%d, childCnt:%d, name:%s\n", pFileNode->depth, pFileNode->childCnt, pFileNode->name);

    list_for_each_entry(pos, &pFileNode->headNodeList, childNodeList)
    {
        info->index = nRowIdx;
        for (j = 0; j < COL_NUM; j++)
        {
            subdata[j].row = info->index;
            subdata[j].col = j;
            subdata[j].textColor = 0;
            subdata[j].flags = 0;
            subdata[j].text = "";
            subdata[j].image = 0;

            switch (j)
            {
                case 0:
                    {
                        char dirName[256];
                        char *p = NULL;
                        memset(dirName, 0, sizeof(dirName));
                        memcpy(dirName, pos->name, strlen(pos->name));
                        p = strrchr(dirName, '/');
                        *p = 0;
                        subdata[j].flags = NCSF_ITEM_USEBITMAP;
                        subdata[j].text = pos->name+strlen(dirName)+1;
                        if (pos->dirFlag)
                            subdata[j].image = (DWORD)&g_item_folder;
                        else
                            subdata[j].image = (DWORD)&g_item_file;
                    }
                    break;
                case 1:
                    subdata[j].text = (char*)(pos->dirFlag ? DIR_TYPE:FILE_TYPE);
                    break;
                case 2:
                    {
                        char szSize[32];
                        memset(szSize, 0, sizeof(szSize));
                        sprintf(szSize, "%d KB", (pos->size+1023)/1024);
                        subdata[j].text = szSize;
                    }
                    break;
                case 3:
                    subdata[j].text = pos->time;
                    break;
                default:
                    break;
            }
        }

        _c(self)->addItem(self, info);
        nRowIdx++;
    }

    for (i = 0; i < nRowIdx; i++)
    {
        for (j = 0; j < COL_NUM; j++)
        {
            _c(self)->setBackground(self, i, j, &color);
        }
    }

    ReleaseDC(clientDc);
}

static void SetBtnImg(mButton *pBtnObj, PBITMAP pBmp)
{
    if (pBtnObj)
    {
        mPushButtonPiece *body;
        mImagePiece *content;

        _c(pBtnObj)->setProperty(pBtnObj, NCSP_BUTTON_IMAGE, (DWORD)pBmp);
        body = (mPushButtonPiece*)((mWidget*)pBtnObj->body);
        content = (mImagePiece*)body->content;
        _c(content)->setProperty(content, NCSP_IMAGEPIECE_DRAWMODE, NCS_DM_SCALED);
    }
}

static BOOL lstv_init(mDialogBox* self)
{
    int i;
    FileTree_t *pCurFile = NULL;
    NCS_LISTV_ITEMINFO info;
    NCS_LISTV_CLMINFO lstv_clminfo;
    HWND lstvWnd = GetDlgItem (self->hwnd, IDC_PLAYFILE_LISTVIEW_FILELIST);
    HWND hPathLbl = GetDlgItem(self->hwnd, IDC_PLAYFILE_STATIC_DIR_NAME);
    HWND hUpDirBtn = GetDlgItem(self->hwnd, IDC_PLAYFILE_BUTTON_UPPER_DIR);
    HWND hToolBar = GetDlgItem(self->hwnd, IDC_PLAYFILE_STATIC_PLAYTOOLBAR);
    HWND hPlay = GetDlgItem(self->hwnd, IDC_PLAYFILE_BUTTON_PLAY_PAUSE);
    HWND hStop = GetDlgItem(self->hwnd, IDC_PLAYFILE_BUTTON_STOP);
    HWND hSlow = GetDlgItem(self->hwnd, IDC_PLAYFILE_BUTTON_PLAY_SLOW);
    HWND hFast = GetDlgItem(self->hwnd, IDC_PLAYFILE_BUTTON_PLAY_FAST);
    HWND hTime = GetDlgItem(self->hwnd, IDC_PLAYFILE_STATIC_PLAY_TIME);
    HWND hSpeedMode = GetDlgItem(self->hwnd, IDC_PLAYFILE_STATIC_SPEED_MODE); // not display when speed's factor is 1/1

    mListView *lstvObj = (mListView*)ncsObjFromHandle(lstvWnd);
    mStatic *pPathObj = (mStatic *)ncsObjFromHandle(hPathLbl);
    mButton *pUpDirObj = (mButton*)ncsObjFromHandle(hUpDirBtn);

    mStatic *pToolBarObj = (mStatic*)ncsObjFromHandle(hToolBar);
    mButton *pPlayObj = (mButton*)ncsObjFromHandle(hPlay);
    mButton *pStopObj = (mButton*)ncsObjFromHandle(hStop);
    mButton *pSlowObj = (mButton*)ncsObjFromHandle(hSlow);
    mButton *pFastObj = (mButton*)ncsObjFromHandle(hFast);
    mStatic *pTimeObj = (mStatic*)ncsObjFromHandle(hTime);
    mStatic *pSpeedModeObj = (mStatic*)ncsObjFromHandle(hSpeedMode);

    if (!lstvObj || !pPathObj || !pUpDirObj)
        return FALSE;

    _c(pPathObj)->setProperty(pPathObj, NCSP_STATIC_ALIGN, NCS_ALIGN_LEFT);
    _c(pPathObj)->setProperty(pPathObj, NCSP_WIDGET_TEXT, (DWORD)g_pu8FullPath);
    _c(pToolBarObj)->setProperty(pToolBarObj, NCSP_WIDGET_BKIMAGE, (DWORD)&g_playtoolbar);
    _c(pToolBarObj)->setProperty(pToolBarObj, NCSP_WIDGET_BKIMAGE_MODE, NCS_DM_SCALED);
    SetBtnImg(pUpDirObj, &g_playfile_updir);
    SetBtnImg(pPlayObj, &g_play_btn);           // if be paused, use g_pause_btn image
    SetBtnImg(pStopObj, &g_stop_btn);
    SetBtnImg(pSlowObj, &g_slow_btn);
    SetBtnImg(pFastObj, &g_fast_btn);

    // get file time info
    char timeInfo[] = "00:00:00 / 00:30:22";
    _c(pTimeObj)->setProperty(pTimeObj, NCSP_STATIC_ALIGN, NCS_ALIGN_CENTER);
    _c(pTimeObj)->setProperty(pTimeObj, NCSP_WIDGET_TEXT, (DWORD)timeInfo);
    _c(pSpeedModeObj)->setProperty(pSpeedModeObj, NCSP_STATIC_ALIGN, NCS_ALIGN_CENTER);

    _c(lstvObj)->freeze(lstvObj, TRUE);
    for (i = 0; i < COL_NUM; i++)
    {
        lstv_clminfo.index  = i;
        lstv_clminfo.text   = g_lv_caption[i];
        lstv_clminfo.width  = g_stPlayToolBarItem[2].s16PicW * g_colDiv[i] / COL_DIV - COL_GAP;
        lstv_clminfo.pfnCmp = NULL;
        lstv_clminfo.sort = NCSID_LSTCLM_LOSORTED;
        lstv_clminfo.flags  = NCSF_LSTCLM_LEFTALIGN | NCSF_LSTCLM_VCENTERALIGN | NCSF_LSTHDR_LEFTALIGN;
        _c(lstvObj)->addColumn(lstvObj, &lstv_clminfo);
    }

    info.height     = ROW_HEIGHT;
    info.flags      = 0;
    info.foldIcon   = 0;
    info.unfoldIcon = 0;
    info.parent = 0;

    pCurFile = FindFileTreeNode(&g_fileRoot, (char*)g_pu8FullPath);
    add_fileinfo_item(lstvObj, &info, pCurFile);
    _c(lstvObj)->freeze(lstvObj, FALSE);

    return TRUE;
}

void PredacteModify()
{
    MI_S32 i = 0;

    for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
    {
        g_astItemBtn[i].bCurOff = g_astItemBtn[i].bLastOff;
    }

    for (i = 0; i < sizeof(g_astTrackBar)/sizeof(TrackBarInfo_t); i++)
    {
        g_astTrackBar[i].nCurPos = g_astTrackBar[i].nLastPos;
    }
}

void SaveModify()
{
    MI_S32 i = 0;

    for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
    {
        g_astItemBtn[i].bLastOff = g_astItemBtn[i].bCurOff;
    }

    for (i = 0; i < sizeof(g_astTrackBar)/sizeof(TrackBarInfo_t); i++)
    {
        g_astTrackBar[i].nLastPos = g_astTrackBar[i].nCurPos;
    }

    ST_XmlUpdateDevVolumeCfg((const MI_U8 *)"SpeakerVolume", (MI_S32)GetTrackBarCurPos(g_hSpkTrackBar), (MI_BOOL)GetSwitchBtnStatus(g_hSpkMute));
}

// create setting sub page
static void CreateSettingSubPage(HWND hWnd, int nPageIdx, int nSubPageIdx)
{
    switch (nPageIdx)
    {
        case 0:
            {
                switch (nSubPageIdx)
                    {
                        case 0:
                            {
                                static MI_S32 s32ParseVolumeCfg = 0;
                                MI_S32 s32SpkVolValue = 0;
                                MI_BOOL bSpkMute = FALSE;
                                MI_S32 s32MicVolValue = 0;
                                MI_BOOL bMicMute = FALSE;

                                g_hMic = CreateWindowEx(CTRL_STATIC, "麦克风音量",      WS_CHILD | SS_LEFT | WS_VISIBLE, WS_EX_TRANSPARENT,
                                                        IDC_STATIC_VOLMIC, stSettingItem[3].s32X, stSettingItem[3].s32Y+80, 100, 30, hWnd, 0);
                                g_hMicTrackBar = CreateWindowEx(CTRL_TRACKBAR, "", TBS_NOTIFY | TBS_NOTICK | TBS_TIP | WS_VISIBLE, WS_EX_TRANSPARENT,
                                                                IDC_TRACKBAR_VOLMIC, stSettingItem[3].s32X, stSettingItem[3].s32Y+110, stSettingItem[2].s16PicW*4, 40, hWnd, 0);
                                g_hSpeaker = CreateWindowEx(CTRL_STATIC, "扬声器音量", WS_CHILD | SS_LEFT | WS_VISIBLE, WS_EX_TRANSPARENT,
                                                            IDC_STATIC_VOLSPK, stSettingItem[3].s32X, stSettingItem[3].s32Y+155, 100, 30, hWnd, 0);
                                g_hSpkTrackBar = CreateWindowEx(CTRL_TRACKBAR, "", TBS_NOTIFY | TBS_NOTICK | TBS_TIP | WS_VISIBLE, WS_EX_TRANSPARENT,
                                                                IDC_TRACKBAR_VOLSPK, stSettingItem[3].s32X, stSettingItem[3].s32Y+185, stSettingItem[2].s16PicW*4, 40, hWnd, 0);
                                g_hMicMuteTitle = CreateWindowEx(CTRL_STATIC, "麦克风静音", WS_CHILD | SS_LEFT | WS_VISIBLE, WS_EX_TRANSPARENT,
                                                                 IDC_STATIC_MIC_MUTE, stSettingItem[3].s32X, stSettingItem[3].s32Y+230, 100, 30, hWnd, 0);
                                g_hSpkMuteTitle = CreateWindowEx(CTRL_STATIC, "扬声器静音", WS_CHILD | SS_LEFT | WS_VISIBLE, WS_EX_TRANSPARENT,
                                                                 IDC_STATIC_SPK_MUTE, stSettingItem[3].s32X+stSettingItem[2].s16PicW*2, stSettingItem[3].s32Y+230, 100, 30, hWnd, 0);
                                g_hMicMute = CreateWindow (CTRL_BUTTON, GetItemBtnText(nPageIdx, nSubPageIdx, 0), WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | BS_BITMAP | WS_VISIBLE,
                                                           IDC_BUTTON_MIC_MUTE, stSettingItem[3].s32X, stSettingItem[3].s32Y+260, stSettingItem[2].s16PicW*5/4, 40, hWnd, (DWORD)&bmp_btnoff);
                                g_hSpkMute = CreateWindow (CTRL_BUTTON, GetItemBtnText(nPageIdx, nSubPageIdx, 1), WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | BS_BITMAP | WS_VISIBLE,
                                                           IDC_BUTTON_SPK_MUTE, stSettingItem[3].s32X+stSettingItem[2].s16PicW*2, stSettingItem[3].s32Y+260, stSettingItem[2].s16PicW*5/4, 40, hWnd, (DWORD)&bmp_btnoff);
                                SetItemBtnHwnd(nPageIdx, nSubPageIdx, 0, g_hMicMute);
                                SetItemBtnHwnd(nPageIdx, nSubPageIdx, 1, g_hSpkMute);
                                SetTrackBarHwnd(nPageIdx, nSubPageIdx, 0, g_hMicTrackBar);
                                SetTrackBarHwnd(nPageIdx, nSubPageIdx, 1, g_hSpkTrackBar);

                                if (!s32ParseVolumeCfg)
                                {
                                    ST_XmlParseDevVolumeCfg((const MI_U8*)"SpeakerVolume", &s32SpkVolValue, &bSpkMute);
                                    ST_XmlParseDevVolumeCfg((const MI_U8*)"MicVolume", &s32MicVolValue, &bMicMute);
                                    printf("spk volume: %d, %s\n", s32SpkVolValue, bSpkMute?"Mute On":"Mute Off");
                                    printf("mic volume: %d, %s\n", s32MicVolValue, bMicMute?"Mute On":"Mute Off");
                                    SetItemBtnStatus(g_hSpkMute, bSpkMute);
                                    InitTrackBarPos(g_hSpkTrackBar, s32SpkVolValue);
                                    SetItemBtnStatus(g_hMicMute, bMicMute);
                                    InitTrackBarPos(g_hMicTrackBar, s32MicVolValue);
                                    s32ParseVolumeCfg = 1;
                                }

                                g_oldSwitchBtnProc = SetWindowCallbackProc(g_hMicMute, GetItemBtnProc(g_hMicMute));
                                SetWindowCallbackProc(g_hSpkMute, GetItemBtnProc(g_hSpkMute));
                                SAVE_BUTTON_OLD_PROC(g_hMicMute, g_oldSwitchBtnProc, g_btnProcListHead);
                                SAVE_BUTTON_OLD_PROC(g_hSpkMute, g_oldSwitchBtnProc, g_btnProcListHead);

                                SetTrackBarStatus(g_hMicTrackBar, &trackbar_ruler, &trackbar_hthumb, &trackbar_procBar);
                                SetTrackBarStatus(g_hSpkTrackBar, &trackbar_ruler, &trackbar_hthumb, &trackbar_procBar);
                            }
                            break;
                        case 1:
                            {
                                g_hVthRing = CreateWindowEx(CTRL_STATIC, "室内机铃声", WS_CHILD | SS_LEFT, WS_EX_TRANSPARENT,
                                                            IDC_STATIC_VTH, stSettingItem[3].s32X, stSettingItem[3].s32Y+80, 100, 30, hWnd, 0);
                                g_hVthRingCombox = CreateWindowEx(CTRL_COMBOBOX, "", CBS_DROPDOWNLIST | CBS_READONLY | CBS_SORT | WS_TABSTOP, WS_EX_TRANSPARENT,
                                                                  IDC_VTH_RING_COMBOX, stSettingItem[3].s32X, stSettingItem[3].s32Y+110, stSettingItem[2].s16PicW*3/2, 40, hWnd, 0);
                                g_hVthTrackBar = CreateWindowEx(CTRL_TRACKBAR, "", TBS_NOTIFY | TBS_NOTICK | TBS_TIP, WS_EX_TRANSPARENT,
                                                                IDC_TRACKBAR_VOLVTH, stSettingItem[3].s32X+stSettingItem[2].s16PicW*3/2+5, stSettingItem[3].s32Y+110, stSettingItem[2].s16PicW*5/2, 40, hWnd, 0);

                                g_hVtoRing = CreateWindowEx(CTRL_STATIC, "室外机铃声", WS_CHILD | SS_LEFT, WS_EX_TRANSPARENT,
                                                            IDC_STATIC_VTO, stSettingItem[3].s32X, stSettingItem[3].s32Y+155, 100, 30, hWnd, 0);
                                g_hVtoRingCombox = CreateWindowEx(CTRL_COMBOBOX, "", CBS_DROPDOWNLIST | CBS_READONLY | CBS_SORT | WS_TABSTOP, WS_EX_TRANSPARENT,
                                                                  IDC_VTO_RING_COMBOX, stSettingItem[3].s32X, stSettingItem[3].s32Y+185, stSettingItem[2].s16PicW*3/2, 40, hWnd, 0);
                                g_hVtoTrackBar = CreateWindowEx(CTRL_TRACKBAR, "", TBS_NOTIFY | TBS_NOTICK | TBS_TIP, WS_EX_TRANSPARENT,
                                                                IDC_TRACKBAR_VOLVTO, stSettingItem[3].s32X+stSettingItem[2].s16PicW*3/2+5, stSettingItem[3].s32Y+185, stSettingItem[2].s16PicW*5/2, 40, hWnd, 0);
                                SetTrackBarHwnd(nPageIdx, nSubPageIdx, 0, g_hVthTrackBar);
                                SetTrackBarHwnd(nPageIdx, nSubPageIdx, 1, g_hVtoTrackBar);
                                SetComboxHwnd(nPageIdx, nSubPageIdx, 0, g_hVthRingCombox);
                                SetComboxHwnd(nPageIdx, nSubPageIdx, 1, g_hVtoRingCombox);

                                SetTrackBarStatus(g_hVthTrackBar, &trackbar_ruler, &trackbar_hthumb, &trackbar_procBar);
                                SetTrackBarStatus(g_hVtoTrackBar, &trackbar_ruler, &trackbar_hthumb, &trackbar_procBar);
                                SetNotificationCallback(g_hVthRingCombox, ComboxNotifyProc);
                                SetNotificationCallback(g_hVtoRingCombox, ComboxNotifyProc);
                            }
                            break;
                        case 2:
                            {
                                g_hAlarm = CreateWindowEx(CTRL_STATIC, "报警铃声", WS_CHILD | SS_LEFT, WS_EX_TRANSPARENT,
                                                          IDC_STATIC_ALARM, stSettingItem[3].s32X, stSettingItem[3].s32Y+80, 100, 30, hWnd, 0);
                                g_hAlarmCombox = CreateWindowEx(CTRL_COMBOBOX, "", CBS_DROPDOWNLIST | CBS_READONLY | CBS_SORT | WS_TABSTOP, WS_EX_TRANSPARENT,
                                                                IDC_ALARM_COMBOX, stSettingItem[3].s32X, stSettingItem[3].s32Y+110, stSettingItem[2].s16PicW*3/2, 40, hWnd, 0);
                                g_hAlarmTrackBar = CreateWindowEx(CTRL_TRACKBAR, "", TBS_NOTIFY | TBS_NOTICK | TBS_TIP, WS_EX_TRANSPARENT,
                                                                  IDC_TRACKBAR_VOLALARM, stSettingItem[3].s32X+stSettingItem[2].s16PicW*3/2+5, stSettingItem[3].s32Y+110, stSettingItem[2].s16PicW*5/2, 40, hWnd, 0);
                                SetTrackBarHwnd(nPageIdx, nSubPageIdx, 0, g_hAlarmTrackBar);
                                SetComboxHwnd(nPageIdx, nSubPageIdx, 0, g_hAlarmCombox);

                                SetTrackBarStatus(g_hAlarmTrackBar, &trackbar_ruler, &trackbar_hthumb, &trackbar_procBar);
                                SetNotificationCallback(g_hAlarmCombox, ComboxNotifyProc);
                            }
                            break;
                        case 3:
                            {
                                g_hVthRingTime = CreateWindowEx(CTRL_STATIC, "室内机振铃时间", WS_CHILD | SS_LEFT, WS_EX_TRANSPARENT,
                                                                IDC_STATIC_VTH_TIME, stSettingItem[3].s32X, stSettingItem[3].s32Y+80, 150, 30, hWnd, 0);
                                g_hVthRingTimeTrackBar = CreateWindowEx(CTRL_TRACKBAR, "", TBS_NOTIFY | TBS_NOTICK | TBS_TIP, WS_EX_TRANSPARENT,
                                                                        IDC_TRACKBAR_VTH_TIME, stSettingItem[3].s32X, stSettingItem[3].s32Y+110, stSettingItem[2].s16PicW*4, 40, hWnd, 0);
                                g_hVtoRingTime = CreateWindowEx(CTRL_STATIC, "室外机振铃时间", WS_CHILD | SS_LEFT, WS_EX_TRANSPARENT,
                                                                IDC_STATIC_VTO_TIME, stSettingItem[3].s32X, stSettingItem[3].s32Y+155, 150, 30, hWnd, 0);
                                g_hVtoRingTimeTrackBar = CreateWindowEx(CTRL_TRACKBAR, "", TBS_NOTIFY | TBS_NOTICK | TBS_TIP, WS_EX_TRANSPARENT,
                                                                        IDC_TRACKBAR_VTO_TIME, stSettingItem[3].s32X, stSettingItem[3].s32Y+185, stSettingItem[2].s16PicW*4, 40, hWnd, 0);
                                g_hRingMuteTitle = CreateWindowEx(CTRL_STATIC, "振铃静音", WS_CHILD | SS_LEFT, WS_EX_TRANSPARENT,
                                                                 IDC_STATIC_RING_MUTE, stSettingItem[3].s32X, stSettingItem[3].s32Y+230, 100, 30, hWnd, 0);
                                g_hRingMute = CreateWindow (CTRL_BUTTON, GetItemBtnText(nPageIdx, nSubPageIdx, 0), WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | BS_BITMAP,
                                                           IDC_BUTTON_MIC_MUTE, stSettingItem[3].s32X, stSettingItem[3].s32Y+260, stSettingItem[2].s16PicW*5/4, 40, hWnd, (DWORD)&bmp_btnoff);
                                SetItemBtnHwnd(nPageIdx, nSubPageIdx, 0, g_hRingMute);
                                SetTrackBarHwnd(nPageIdx, nSubPageIdx, 0, g_hVthRingTimeTrackBar);
                                SetTrackBarHwnd(nPageIdx, nSubPageIdx, 1, g_hVtoRingTimeTrackBar);

                                SetWindowCallbackProc(g_hRingMute, GetItemBtnProc(g_hRingMute));
                                SAVE_BUTTON_OLD_PROC(g_hRingMute, g_oldSwitchBtnProc, g_btnProcListHead);

                                SetTrackBarStatus(g_hVthRingTimeTrackBar, &trackbar_ruler, &trackbar_hthumb, &trackbar_procBar);
                                SetTrackBarStatus(g_hVtoRingTimeTrackBar, &trackbar_ruler, &trackbar_hthumb, &trackbar_procBar);
                            }
                            break;
                        default:
                            printf("error page (%d, %d)\n", nPageIdx, nSubPageIdx);
                    }
            }
            break;
        case 1:
            {
                int i = 0, j = 0;
                LVSUBITEM subData;
                LVITEM item;
                LVCOLUMN col;
                char *pCaption[] = {"English", "中文"};
                char *pEngItem[4] = {"make a call", "hung up", "open the door", "close the door"};
                char *pCnItem[4] = {"打电话", "挂断", "开门", "关门"};

                g_hTestLbl = CreateWindowEx(CTRL_STATIC, "列表控件测试",      WS_CHILD | SS_LEFT, WS_EX_TRANSPARENT,
                                                        IDC_STATIC_TESTLV, stSettingItem[3].s32X, stSettingItem[3].s32Y+30, 150, 30, hWnd, 0);
                g_hTestLv = CreateWindowEx(CTRL_LISTVIEW, "", WS_CHILD | WS_VSCROLL, WS_EX_TRANSPARENT,
                                                        IDC_LISTVIEW_TEST, stSettingItem[3].s32X, stSettingItem[3].s32Y+60, col.width = stSettingItem[2].s16PicW*3, 140, hWnd, 0);

                // add column
                for (i = 0; i < 2; i++)
                {
                    col.nCols = i;
                    col.pszHeadText = pCaption[i];
                    col.width = stSettingItem[2].s16PicW*3/2;
                    col.pfnCompare = NULL;
                    col.colFlags = 0;
                    SendMessage(g_hTestLv, LVM_ADDCOLUMN, 0, (LPARAM)&col);
                }

                // add row
                for (i = 0; i < 4; i++)
                {
                    item.nItem = i;
                    SendMessage(g_hTestLv, LVM_ADDITEM, 0, (LPARAM)&item);

                    subData.nItem = i;
                    subData.subItem = 0;
                    subData.pszText = pEngItem[i];
                    subData.nTextColor = PIXEL_darkyellow;
                    subData.flags = 0;
                    subData.image = 0;
                    SendMessage(g_hTestLv, LVM_SETSUBITEM, 0, (LPARAM)&subData);

                    subData.nItem = i;
                    subData.subItem = 1;
                    subData.pszText = pCnItem[i];
                    subData.nTextColor = PIXEL_darkyellow;
                    subData.flags = 0;
                    subData.image = 0;
                    SendMessage(g_hTestLv, LVM_SETSUBITEM, 0, (LPARAM)&subData);
                }
            }
            break;
        case 2:
            {

            }
            break;
        case 3:
            {

            }
            break;
        case 4:
            {

            }
            break;
        case 5:
            {

            }
            break;
        case 6:
            {
                g_hMachineInfoGroup = CreateWindowEx(CTRL_STATIC, g_szItemCaption6[0],
                                                     WS_CHILD | SS_GROUPBOX, WS_EX_BROUNDCNS | WS_EX_TROUNDCNS | WS_EX_TRANSPARENT,
                                                     IDC_STATIC_GROUP_MACHINE_INFO,
                                                     stSettingItem[3].s32X, stSettingItem[3].s32Y+40, stSettingItem[2].s16PicW*4, 200, hWnd, 0);
                g_hSysVerTitle = CreateWindowEx(CTRL_STATIC, g_szItemCaption6[1],
                                                WS_CHILD | SS_LEFT, WS_EX_TRANSPARENT,
                                                IDC_STATIC_SYS_VER_TITLE,
                                                stSettingItem[3].s32X+10, stSettingItem[3].s32Y+70, 100, 30, hWnd, 0);
                g_hSysVer = CreateWindowEx(CTRL_STATIC, g_szItemContent6[0],
                                           WS_CHILD | SS_LEFT, WS_EX_TRANSPARENT,
                                           IDC_STATIC_SYS_VER,
                                           stSettingItem[3].s32X+120, stSettingItem[3].s32Y+70, 200, 30, hWnd, 0);
                g_hMachineVerTitle = CreateWindowEx(CTRL_STATIC, g_szItemCaption6[2],
                                                    WS_CHILD | SS_LEFT, WS_EX_TRANSPARENT,
                                                    IDC_STATIC_MACHINE_VER_TITLE,
                                                    stSettingItem[3].s32X+10, stSettingItem[3].s32Y+100, 100, 30, hWnd, 0);
                g_hMachineVer = CreateWindowEx(CTRL_STATIC, g_szItemContent6[1],
                                               WS_CHILD | SS_LEFT, WS_EX_TRANSPARENT,
                                               IDC_STATIC_MACHINE_VER,
                                               stSettingItem[3].s32X+120, stSettingItem[3].s32Y+100, 200, 30, hWnd, 0);
                g_hRestart = CreateWindow (CTRL_BUTTON, GetItemBtnText(nPageIdx, 0, 0),
                                           WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | BS_BITMAP,
                                           IDC_BUTTON_RESTART,
                                           stSettingItem[3].s32X+10, stSettingItem[3].s32Y+130, stSettingItem[2].s16PicW, stSettingItem[2].s16PicH, hWnd, (DWORD)&btn_notselect);

                SetItemBtnHwnd(nPageIdx, 0, 0, g_hRestart);
                g_oldNormalBtnProc = SetWindowCallbackProc(g_hRestart, GetItemBtnProc(g_hRestart));

                SAVE_BUTTON_OLD_PROC(g_hRestart, g_oldNormalBtnProc, g_btnProcListHead);
            }
            break;
        default:
            break;
    }

}

static void EnableShowSubPage(int nPageIdx, int nSubPageIdx, bool bEn)
{
    switch (nPageIdx)
    {
        case 0: // 4 subPage, 4 subPageBtn
            {
                switch (nSubPageIdx)
                {
                    case 0:
                        if (!bEn)
                        {
                            ShowWindow(g_hMic, SW_HIDE);
                            ShowWindow(g_hSpeaker, SW_HIDE);
                            ShowWindow(g_hMicTrackBar, SW_HIDE);
                            ShowWindow(g_hSpkTrackBar, SW_HIDE);
                            ShowWindow(g_hMicMuteTitle, SW_HIDE);
                            ShowWindow(g_hMicMute, SW_HIDE);
                            ShowWindow(g_hSpkMuteTitle, SW_HIDE);
                            ShowWindow(g_hSpkMute, SW_HIDE);
                        }
                        else
                        {
                            ShowWindow(g_hMic, SW_SHOW);
                            ShowWindow(g_hSpeaker, SW_SHOW);
                            ShowWindow(g_hMicTrackBar, SW_SHOW);
                            ShowWindow(g_hSpkTrackBar, SW_SHOW);
                            ShowWindow(g_hMicMuteTitle, SW_SHOW);
                            ShowWindow(g_hMicMute, SW_SHOW);
                            ShowWindow(g_hSpkMuteTitle, SW_SHOW);
                            ShowWindow(g_hSpkMute, SW_SHOW);
                        }
                        break;
                    case 1:
                        if (!bEn)
                        {
                            ShowWindow(g_hVthRing, SW_HIDE);
                            ShowWindow(g_hVthRingCombox, SW_HIDE);
                            ShowWindow(g_hVthTrackBar, SW_HIDE);
                            ShowWindow(g_hVtoRing, SW_HIDE);
                            ShowWindow(g_hVtoRingCombox, SW_HIDE);
                            ShowWindow(g_hVtoTrackBar, SW_HIDE);
                        }
                        else
                        {
                            ShowWindow(g_hVthRing, SW_SHOW);
                            ShowWindow(g_hVthRingCombox, SW_SHOW);
                            ShowWindow(g_hVthTrackBar, SW_SHOW);
                            ShowWindow(g_hVtoRing, SW_SHOW);
                            ShowWindow(g_hVtoRingCombox, SW_SHOW);
                            ShowWindow(g_hVtoTrackBar, SW_SHOW);
                        }
                        break;
                    case 2:
                        if (!bEn)
                        {
                            ShowWindow(g_hAlarm, SW_HIDE);
                            ShowWindow(g_hAlarmCombox, SW_HIDE);
                            ShowWindow(g_hAlarmTrackBar, SW_HIDE);
                        }
                        else
                        {
                            ShowWindow(g_hAlarm, SW_SHOW);
                            ShowWindow(g_hAlarmCombox, SW_SHOW);
                            ShowWindow(g_hAlarmTrackBar, SW_SHOW);
                        }
                        break;
                    case 3:
                        if (!bEn)
                        {
                            ShowWindow(g_hVthRingTime, SW_HIDE);
                            ShowWindow(g_hVthRingTimeTrackBar, SW_HIDE);
                            ShowWindow(g_hVtoRingTime, SW_HIDE);
                            ShowWindow(g_hVtoRingTimeTrackBar, SW_HIDE);
                            ShowWindow(g_hRingMuteTitle, SW_HIDE);
                            ShowWindow(g_hRingMute, SW_HIDE);
                        }
                        else
                        {
                            ShowWindow(g_hVthRingTime, SW_SHOW);
                            ShowWindow(g_hVthRingTimeTrackBar, SW_SHOW);
                            ShowWindow(g_hVtoRingTime, SW_SHOW);
                            ShowWindow(g_hVtoRingTimeTrackBar, SW_SHOW);
                            ShowWindow(g_hRingMuteTitle, SW_SHOW);
                            ShowWindow(g_hRingMute, SW_SHOW);
                        }
                        break;
                    default:
                        printf("error sub page (%d, %d)\n", nPageIdx, nSubPageIdx);
                        break;
                }
            }
            break;
        case 1: // 1 subPage, no subPageBtn
            {
                if (!bEn)
                {
                    ShowWindow(g_hTestLbl, SW_HIDE);
                    ShowWindow(g_hTestLv, SW_HIDE);
                }
                else
                {
                    ShowWindow(g_hTestLbl, SW_SHOW);
                    ShowWindow(g_hTestLv, SW_SHOW);
                }
            }
            break;
        case 2: // 1 subPage, 1 subPageBtn
            {

            }
            break;
        case 3: // 4 subPage, 4 subPageBtn
            {

            }
            break;
        case 4: // 1 subPage, no subPageBtn
            {

            }
            break;
        case 5: // 4 subPage, 4 subPageBtn
            {

            }
            break;
        case 6: // 1 subPage, 0 subPageBtn
            {
                if (!bEn)
                {
                    ShowWindow(g_hMachineInfoGroup, SW_HIDE);
                    ShowWindow(g_hSysVerTitle, SW_HIDE);
                    ShowWindow(g_hSysVer, SW_HIDE);
                    ShowWindow(g_hMachineVerTitle, SW_HIDE);
                    ShowWindow(g_hMachineVer, SW_HIDE);
                    ShowWindow(g_hRestart, SW_HIDE);
                }
                else
                {
                    ShowWindow(g_hMachineInfoGroup, SW_SHOW);
                    ShowWindow(g_hSysVerTitle, SW_SHOW);
                    ShowWindow(g_hSysVer, SW_SHOW);
                    ShowWindow(g_hMachineVerTitle, SW_SHOW);
                    ShowWindow(g_hMachineVer, SW_SHOW);
                    ShowWindow(g_hRestart, SW_SHOW);
                }
            }
            break;
        default:
            printf("sub page is not exsit\n");
            break;
    }
}

static void EnableShowPage(int nPageIdx, bool bEn)
{
    int i = 0;
    int nSubPageIdx = -1;

    for (i = 0; i< sizeof(g_astSubPageBtn)/sizeof(SubPageBtnInfo_t); i++)
    {
        if (g_astSubPageBtn[i].nPageIdx == nPageIdx)
        {
            nSubPageIdx = g_astSubPageBtn[i].nSubPageIdx;
            g_astPageBtn[nPageIdx].hSelectSubBtn = GetSubPageBtnHwnd(nPageIdx, 0);

            if (!bEn)
            {
                ShowWindow(g_astSubPageBtn[i].hBtn, SW_HIDE);
                EnableShowSubPage(nPageIdx, nSubPageIdx, false);
            }
            else
            {
                ShowWindow(g_astSubPageBtn[i].hBtn, SW_SHOW);
            }
        }
    }

    EnableShowSubPage(nPageIdx, 0, bEn);
}

static void ShowSettingPage()
{
    int i = 0;
    int nPageIdx = -1;

    for (i = 0; i < sizeof(g_astPageBtn)/sizeof(PageBtnInfo_t); i++)
    {
        if (g_astPageBtn[i].hBtn == g_hCurSelectHwnd)
        {
            printf("select page %d\n", i);
            nPageIdx = i;
        }

        EnableShowPage(i, false);
    }

    EnableShowPage(nPageIdx, true);
}

void ShowSubPage(int nPageIdx)
{
    int i = 0;
    int nSubPageIdx = -1;

    for (i = 0; i < sizeof(g_astSubPageBtn)/sizeof(SubPageBtnInfo_t); i++)
    {
        if (g_astSubPageBtn[i].nPageIdx == nPageIdx && g_astSubPageBtn[i].hBtn == g_astPageBtn[nPageIdx].hSelectSubBtn)
        {
            printf("select sub page %d\n", g_astSubPageBtn[i].nSubPageIdx);
            nSubPageIdx = g_astSubPageBtn[i].nSubPageIdx;
            break;
        }
    }

    for (i = 0; i < g_astPageBtn[nPageIdx].nSubCnt; i++)
    {
        EnableShowSubPage(nPageIdx, i, false);
    }

    EnableShowSubPage(nPageIdx, nSubPageIdx, true);

}


// select page proc
static LRESULT PageBtnProc(HWND hwnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    DWORD dwStyle = GetWindowStyle(hwnd);
    WNDPROC btnProc = NULL;
    BtnProcListData_t *pos =  NULL;

    list_for_each_entry(pos, &g_btnProcListHead, btnProcList)
    {
        if (pos->hBtn == hwnd)
        {
            btnProc = pos->btnOldProc;
            break;
        }
    }

    if (!btnProc)
    {
//        printf("page btn destroyed or the old proc of select btn %d has not been saved\n", hwnd);
        return 0;
    }

    switch (message)
    {
        case MSG_LBUTTONDOWN:
            // chg self bg, clear others bg,then call default proc
            if (IsPageBtn(hwnd))
            {
                if (g_hCurSelectHwnd != hwnd)
                {
                    HWND oldHwnd = g_hCurSelectHwnd;
                    g_hCurSelectHwnd = hwnd;
                    if (oldHwnd != HWND_INVALID)
                        UpdateWindow(oldHwnd, true);
                    ShowSettingPage();
                }
            }
            else if (IsSubPageBtn(hwnd))
            {
                int nIndex = GetSubPageBtnPageIndex(hwnd);
                if (nIndex >= 0)
                {
                    if (g_astPageBtn[nIndex].hSelectSubBtn != hwnd)
                    {
                        HWND oldHwnd = g_astPageBtn[nIndex].hSelectSubBtn;
                        g_astPageBtn[nIndex].hSelectSubBtn = hwnd;
                        if (oldHwnd != HWND_INVALID)
                            UpdateWindow(oldHwnd, true);
                        ShowSubPage(nIndex);
                    }
                }
            }
            break;
        case MSG_LBUTTONDBLCLK:
        case MSG_MOUSEMOVEIN:
            return 0;
        case MSG_PAINT:
            if (dwStyle & BS_BITMAP)
            {
                hdc = BeginPaint (hwnd);
                SelectFont (hdc, GetWindowFont(hwnd));
                if (IsPageBtn(hwnd))
                    OnBtnPaint(hdc, hwnd, (g_hCurSelectHwnd == hwnd), &btn_select, &btn_notselect);
                else
                {
                    int nIndex = GetSubPageBtnPageIndex(hwnd);
                    if (nIndex >= 0)
                        OnBtnPaint(hdc, hwnd, (g_astPageBtn[nIndex].hSelectSubBtn == hwnd), &btn_select, &btn_notselect);
                }
                EndPaint(hwnd, hdc);
                return 0;
            }
            break;
    }

    return (*btnProc)(hwnd, message, wParam, lParam);
}


// create setting page
static void CreateSettingPage(HWND hWnd, int nPageIdx)
{
    int i = 0, j = 0;
    DWORD wsVisible = 0L;

    if (nPageIdx == 0)
        wsVisible = WS_VISIBLE;
    else
        wsVisible = 0L;

    if (g_astPageBtn[nPageIdx].nSubCnt == 0)
        CreateSettingSubPage(hWnd, nPageIdx, 0);
    else
    {
        for (i = 0; i < g_astPageBtn[nPageIdx].nSubCnt; i++)
        {
            HWND hSubHwnd = CreateWindow (CTRL_BUTTON, GetSubPageBtnText(nPageIdx, i),
            WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | wsVisible | BS_BITMAP | BS_FLAT,
            IDC_BUTTON_SOUND_TALK+i,
            stSettingItem[3].s32X +i*stSettingItem[3].s16PicW, stSettingItem[3].s32Y, stSettingItem[3].s16PicW, stSettingItem[3].s16PicH, hWnd, (DWORD)&btn_notselect);

            SetSubPageBtnHwnd(nPageIdx, i, hSubHwnd);

            if (i == 0)
            {
                g_astPageBtn[nPageIdx].hSelectSubBtn = hSubHwnd;
                g_oldSubPageBtnProc = SetWindowCallbackProc(hSubHwnd, PageBtnProc);
            }
            else
                SetWindowCallbackProc(hSubHwnd, PageBtnProc);

            SAVE_BUTTON_OLD_PROC(hSubHwnd, g_oldSubPageBtnProc, g_btnProcListHead);

            CreateSettingSubPage(hWnd, nPageIdx, i);
        }
    }
}

static int LoadProjectPicture()
{
    if (LoadBitmap (HDC_SCREEN, &btn_setting, "appres/setting.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_video, "appres/video.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_smartmic, "appres/mic.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_call, "appres/call.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_time, "appres/time.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_confirm, "appres/confirm.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_return, "appres/return.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_select, "appres/select_btn.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_notselect, "appres/notselectbtn.png"))
    {
        return -1;
    }

    if (LoadBitmap (HDC_SCREEN, &bmp_mic, "appres/mic.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &bmp_speaker, "appres/speaker.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &bmp_btnon, "appres/on_btn.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &bmp_btnoff, "appres/off_btn.png"))
    {
        return -1;
    }

    if (LoadBitmap (HDC_SCREEN, &btn_0, "appres/0.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_1, "appres/1.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_2, "appres/2.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_3, "appres/3.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_4, "appres/4.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_5, "appres/5.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_6, "appres/6.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_7, "appres/7.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_8, "appres/8.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_9, "appres/9.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_X, "appres/X.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_abc, "appres/abc.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_calling, "appres/calling.jpg"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &btn_monitor, "appres/monitor.jpg"))
    {
        return -1;
    }

    // trackbar picture
    if (LoadBitmap (HDC_SCREEN, &trackbar_ruler, "appres/ruler.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &trackbar_hthumb, "appres/hthumb.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &trackbar_vthumb, "appres/vthumb.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &trackbar_procBar, "appres/processbar.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &g_item_upfolder, "appres/upfolder.bmp"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &g_item_folder, "appres/folder.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &g_item_file, "appres/file.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &g_playfile_updir, "appres/upDir.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &g_play_btn, "appres/play.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &g_pause_btn, "appres/pause.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &g_stop_btn, "appres/stop.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &g_slow_btn, "appres/slow.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &g_fast_btn, "appres/fast.png"))
    {
        return -1;
    }
    if (LoadBitmap (HDC_SCREEN, &g_playtoolbar, "appres/playtoolbar2.png"))
    {
        return -1;
    }

    return 0;
}

static LRESULT FaceDispMainWinProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    unsigned long msg[4];
    int id, code;
    HDC hdc;
    static MI_S32 s32PraseUiItemFlag = 0;
    static ST_Rect_T stFaceDisp[4] = {{352, 70, 320, 320}, {380, 440, 80, 40}, {560, 440, 80, 40},
        {500, 50, 100, 50}};

    if (((access(UI_SURFACE_CFG_FILE, F_OK))!=-1) && !s32PraseUiItemFlag)
    {
        if (0 == ST_XmlPraseUiCfg((MI_U8*)UI_SURFACE_CFG_FILE, (MI_U8*)"FaceDispWindow_LAYOUT", stFaceDisp))
        {
            for (int i = 0; i < 4; i++)
            {
                printf("index=%d (%d-%d-%d-%d)...\n", i,
                    stFaceDisp[i].s32X, stFaceDisp[i].s32Y, stFaceDisp[i].s16PicW, stFaceDisp[i].s16PicH);
            }
            s32PraseUiItemFlag = 1;
        }
    }
    if (MSG_NCMOUSEMOVE == message || MSG_MOUSEMOVE == message)
    {
        return 0;
    }
    switch (message)
    {
        case MSG_CREATE:
            CreateWindow (CTRL_BUTTON,
                "FaceDisp",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP,
                IDC_VIDEO_BUTTON_FACE_DISP,
                stFaceDisp[0].s32X, stFaceDisp[0].s32Y, stFaceDisp[0].s16PicW, stFaceDisp[0].s16PicH, hWnd, (DWORD)&bmp_label_face);
            CreateWindow (CTRL_BUTTON,
                "OK",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE,
                IDC_VIDEO_BUTTON_FACE_OK,
                stFaceDisp[1].s32X, stFaceDisp[1].s32Y, stFaceDisp[1].s16PicW, stFaceDisp[1].s16PicH, hWnd, 0);
            CreateWindow (CTRL_BUTTON,
                "CANCEL",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE,
                IDC_VIDEO_BUTTON_FACE_CANCEL,
                stFaceDisp[2].s32X, stFaceDisp[2].s32Y, stFaceDisp[2].s16PicW, stFaceDisp[2].s16PicH, hWnd, 0);
            break;
        case MSG_COMMAND:
            id = LOWORD(wParam);
            code = HIWORD(wParam);
            switch (code)
            {
                case BN_CLICKED:
                {
                    switch (id)
                    {
                        case IDC_VIDEO_BUTTON_FACE_OK:
                            memset(msg, 0, 16);
                            msg[0] = MSG_TYPE_CONFIRM_REGSITER_FACE;
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)msg,sizeof(msg),&g_toAPP_sem);
                            PostMessage(hWnd, MSG_CLOSE, 0, 0);
                            g_faceDetect = 1;
                            break;
                        case IDC_VIDEO_BUTTON_FACE_CANCEL:
                            PostMessage(hWnd, MSG_CLOSE, 0, 0);
                            g_faceDetect = 1;
                            break;
                        case IDC_VIDEO_BUTTON_FACE_DISP:
                        {
                            //PostMessage(hWnd, MSG_CLOSE, 0, 0);
                            return 0;
                        }
                    }
                }
                case BN_DOUBLECLICKED:
                {
                    switch (id)
                    {
                        case IDC_VIDEO_BUTTON_FACE_OK:
                        case IDC_VIDEO_BUTTON_FACE_CANCEL:
                        case IDC_VIDEO_BUTTON_FACE_DISP:
                            return 0;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        case MSG_PAINT:
            hdc = BeginPaint (hWnd);
            TextOut (hdc, stFaceDisp[3].s32X, stFaceDisp[3].s32Y, "Register?");
            EndPaint (hWnd, hdc);
            return 0;
        case MSG_ERASEBKGND:
            {
                HDC hdcMem = CreateCompatibleDC((HDC)wParam);
                SetBrushColor (hdcMem, WINDOW_BGCOLOR((HDC)wParam));
                FillBox (hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H);
                BitBlt(hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H, (HDC)wParam, 0, 0, 0);
                DeleteCompatibleDC(hdcMem);
            }
            return 1;
        case MSG_DESTROY:
            DestroyAllControls (hWnd);
            hFaceDispWnd = HWND_INVALID;
            return 0;
        case MSG_CLOSE:
            DestroyMainWindow (hWnd);
            MainWindowCleanup (hWnd);
            return 0;
    }

    return DefaultMainWinProc (hWnd, message, wParam, lParam);
}

static void InitCreateInfoFaceDispWnd(PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle =  WS_VISIBLE;
    pCreateInfo->dwExStyle = WS_EX_NONE;
    pCreateInfo->spCaption = "FaceRegsiter";
    pCreateInfo->hMenu = 0;
#if HIDE_CURSOR
    pCreateInfo->hCursor = GetSystemCursor(IDC_NONE);
#else
    pCreateInfo->hCursor = GetSystemCursor(1);
#endif
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = FaceDispMainWinProc;
    pCreateInfo->lx = 0;
    pCreateInfo->ty = 0;
    pCreateInfo->rx = g_s32UiMainWnd_W;
    pCreateInfo->by = g_s32UiMainWnd_H;
    pCreateInfo->iBkColor = GetWindowElementColor (WE_MAINC_THREED_BODY);
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

void FaceDispMainWindow (HWND hwnd)
{
    MAINWINCREATE CreateInfo;
    if (hFaceDispWnd != HWND_INVALID) {
        ShowWindow(hFaceDispWnd, SW_SHOWNORMAL);
        return;
    }
    InitCreateInfoFaceDispWnd(&CreateInfo);
    CreateInfo.hHosting = hwnd;
    hFaceDispWnd = CreateMainWindow(&CreateInfo);
}

static LRESULT TimeMainWinProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    unsigned long msg[4];
    int id, code;

    static MI_S32 s32PraseUiItemFlag = 0;
    static ST_Rect_T stTimeWndItem[5] = {{900, 540, 124, 60}, {800, 100, 124, 60}, {800, 200, 124, 60},
        {800, 300, 124, 60}, {150, 50, 0, 0}};
    if (((access(UI_SURFACE_CFG_FILE, F_OK))!=-1) && !s32PraseUiItemFlag)
    {
        if (0 == ST_XmlPraseUiCfg((MI_U8*)UI_SURFACE_CFG_FILE, (MI_U8*)"TimeWindow_LAYOUT", stTimeWndItem))
        {
            for (int i = 0; i < sizeof(stTimeWndItem)/sizeof(ST_Rect_T); i++)
            {
                printf("index=%d (%d-%d-%d-%d)...\n", i,
                    stTimeWndItem[i].s32X, stTimeWndItem[i].s32Y, stTimeWndItem[i].s16PicW, stTimeWndItem[i].s16PicH);
            }
            s32PraseUiItemFlag = 1;
        }
    }

    switch (message)
    {
        case MSG_CREATE:
            CreateWindow (CTRL_BUTTON,
                "return",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP, IDC_BUTTON_SUBCLOSE,
                stTimeWndItem[0].s32X, stTimeWndItem[0].s32Y, stTimeWndItem[0].s16PicW, stTimeWndItem[0].s16PicH,
                hWnd, (DWORD)&btn_return);
            CreateWindow (CTRL_BUTTON,
                "start",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE, IDC_BUTTON_TEST_START_GETES,
                stTimeWndItem[1].s32X, stTimeWndItem[1].s32Y, stTimeWndItem[1].s16PicW, stTimeWndItem[1].s16PicH,
                hWnd, 0);
            CreateWindow (CTRL_BUTTON,
                "stop",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE, IDC_BUTTON_TEST_STOP_GETES,
                stTimeWndItem[2].s32X, stTimeWndItem[2].s32Y, stTimeWndItem[2].s16PicW, stTimeWndItem[2].s16PicH,
                hWnd, 0);
            CreateWindow (CTRL_BUTTON,
                "CreateXML",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE, IDC_BUTTON_TEST_CREATE_XMLCFG,
                stTimeWndItem[3].s32X, stTimeWndItem[3].s32Y, stTimeWndItem[3].s16PicW, stTimeWndItem[3].s16PicH,
                hWnd, 0);
            CreateWindow (CTRL_BUTTON,
                "PraseXML",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE, IDC_BUTTON_TEST_PRASE_XMLCFG,
                stTimeWndItem[4].s32X, stTimeWndItem[4].s32Y, stTimeWndItem[4].s16PicW, stTimeWndItem[4].s16PicH,
                hWnd, 0);
            break;
        case MSG_COMMAND:
            id = LOWORD(wParam);
            code = HIWORD(wParam);
            switch (code)
            {
                case BN_CLICKED:
                {
                    switch (id)
                    {
                        case IDC_BUTTON_SUBCLOSE:
                            PostMessage(hWnd, MSG_CLOSE, 0, 0);
                            break;
                        case IDC_BUTTON_TEST_START_GETES:
                            memset(msg, 0, 16);
                            msg[0] = MSG_TYPE_START_SEND_VIDEO;
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)msg,sizeof(msg),&g_toAPP_sem);
                            return 0;
                        case IDC_BUTTON_TEST_STOP_GETES:
                            memset(msg, 0, 16);
                            msg[0] = MSG_TYPE_STOP_SEND_VIDEO;
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)msg,sizeof(msg),&g_toAPP_sem);
                            return 0;
                        case IDC_BUTTON_TEST_CREATE_XMLCFG:
                            memset(msg, 0, 16);
                            msg[0] = MSG_TYPE_CREATE_XMLCFG;
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)msg,sizeof(msg),&g_toAPP_sem);
                            return 0;
                        case IDC_BUTTON_TEST_PRASE_XMLCFG:
                            memset(msg, 0, 16);
                            msg[0] = MSG_TYPE_PRASE_XMLCFG;
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)msg,sizeof(msg),&g_toAPP_sem);
                            return 0;
                        default:
                            return 0;
                    }
                }
                case BN_DOUBLECLICKED:
                {
                    switch (id)
                    {
                        case IDC_BUTTON_SUBCLOSE:
                        case IDC_BUTTON_TEST_START_GETES:
                        case IDC_BUTTON_TEST_STOP_GETES:
                            return 0;
                        default:
                            return 0;
                    }
                }
                default:
                    break;
            }
            break;
        case MSG_ERASEBKGND:
            {
                HDC hdcMem = CreateCompatibleDC((HDC)wParam);
                SetBrushColor (hdcMem, WINDOW_BGCOLOR((HDC)wParam));
                FillBox (hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H);
                BitBlt(hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H, (HDC)wParam, 0, 0, 0);
                DeleteCompatibleDC(hdcMem);
            }
            return 1;
        case MSG_DESTROY:
            DestroyAllControls (hWnd);
            hMainTimeWnd = HWND_INVALID;
            return 0;
        case MSG_CLOSE:
            printf("close my window....0x%x\n", hWnd);
            DestroyMainWindow (hWnd);
            MainWindowCleanup (hWnd);
            return 0;
    }

    return DefaultMainWinProc (hWnd, message, wParam, lParam);
}

static void InitCreateInfoTimeWnd(PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle =  WS_VISIBLE;
    pCreateInfo->dwExStyle = WS_EX_NONE;
    pCreateInfo->spCaption = "Setting";
    pCreateInfo->hMenu = 0;
#if HIDE_CURSOR
    pCreateInfo->hCursor = GetSystemCursor(IDC_NONE);
#else
    pCreateInfo->hCursor = GetSystemCursor(1);
#endif
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = TimeMainWinProc;
    pCreateInfo->lx = 0;
    pCreateInfo->ty = 0;
    pCreateInfo->rx = g_s32UiMainWnd_W;
    pCreateInfo->by = g_s32UiMainWnd_H;
    pCreateInfo->iBkColor = GetWindowElementColor (WE_MAINC_THREED_BODY);
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

void TimeMainWindow (HWND hwnd)
{
    MAINWINCREATE CreateInfo;
    if (hMainTimeWnd != HWND_INVALID) {
        ShowWindow(hMainTimeWnd, SW_SHOWNORMAL);
        return;
    }
    InitCreateInfoTimeWnd(&CreateInfo);
    CreateInfo.hHosting = hwnd;
    hMainTimeWnd = CreateMainWindow(&CreateInfo);
}

//static HWND hSmartMicDetectEdit = HWND_INVALID;
static ST_Rect_T stSmartMicItem[7] = {{900, 540, 124, 60}, {800, 100, 124, 60}, {800, 200, 124, 60},
      {800, 300, 124, 60}, {150, 50, 0, 0}, {410, 170, 0, 0}, {410, 200, 150, 40}};

//static ST_Rect_T stSmartMicItem[] = {{900, 540, 124, 60}, {800, 100, 124, 60}, {800, 200, 124, 60},
//        {800, 300, 124, 60}, {150, 50, 0, 0} };

static LRESULT SmartMicMainWinProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    int id, code, i;
    static int start_analyze = 0;
    static int analyze_init = 0;
    static int start_audioIn = 0;
    static MI_S32 s32PraseUiItemFlag = 0;
#if AR_SUPPORT
    static int do_analyze = 0;
#endif

    if (((access(UI_SURFACE_CFG_FILE, F_OK))!=-1) && !s32PraseUiItemFlag)
    {
        if (0 == ST_XmlPraseUiCfg((MI_U8*)UI_SURFACE_CFG_FILE, (MI_U8*)"SmartMicWindow_LAYOUT", stSmartMicItem))
        {
            for (int i = 0; i < sizeof(stSmartMicItem)/sizeof(ST_Rect_T); i++)
            {
                printf("index=%d (%d-%d-%d-%d)...\n", i,
                    stSmartMicItem[i].s32X, stSmartMicItem[i].s32Y, stSmartMicItem[i].s16PicW, stSmartMicItem[i].s16PicH);
            }
            s32PraseUiItemFlag = 1;
        }
    }

    switch (message)
    {
        case MSG_CREATE:
            CreateWindow (CTRL_BUTTON,
                "return",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP,
                IDC_BUTTON_SUBCLOSE,
                stSmartMicItem[0].s32X, stSmartMicItem[0].s32Y, stSmartMicItem[0].s16PicW, stSmartMicItem[0].s16PicH, hWnd, (DWORD)&btn_return);

            // create btn
            g_hSmartmicBtn[0] = CreateWindow (CTRL_BUTTON, g_szSmartmicBtnText[0],
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_BUTTON_SMARTMIC_SUPPORTWORD,
                stSmartMicItem[1].s32X, stSmartMicItem[1].s32Y, stSmartMicItem[1].s16PicW, stSmartMicItem[1].s16PicH, hWnd, (DWORD)&btn_notselect);

            g_hSmartmicBtn[1] = CreateWindow (CTRL_BUTTON, g_szSmartmicBtnText[1],
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_BUTTON_START_SMARTMIC,
                stSmartMicItem[2].s32X, stSmartMicItem[2].s32Y, stSmartMicItem[2].s16PicW, stSmartMicItem[2].s16PicH, hWnd, (DWORD)&btn_notselect);

            g_hSmartmicBtn[2] = CreateWindow (CTRL_BUTTON, g_szSmartmicBtnText[2],
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_BUTTON_STOP_SMARTMIC,
                stSmartMicItem[3].s32X, stSmartMicItem[3].s32Y, stSmartMicItem[3].s16PicW, stSmartMicItem[3].s16PicH, hWnd, (DWORD)&btn_select);

            printf("set wincallback\n");
            g_oldSmartmicBtnProc = SetWindowCallbackProc(g_hSmartmicBtn[0], NormalBtnProc);
            for (i = 1; i < sizeof(g_hSmartmicBtn)/sizeof(HWND); i++)
                SetWindowCallbackProc(g_hSmartmicBtn[i], NormalBtnProc);

            // save hanld & proc to btnProcList
            printf("save win pro & hwnd\n");
            for (i = 0; i < sizeof(g_hSmartmicBtn)/sizeof(HWND); i++)
            {
                SAVE_BUTTON_OLD_PROC(g_hSmartmicBtn[i], g_oldSmartmicBtnProc, g_btnProcListHead);
            }
            INIT_LIST_HEAD(&g_trainingWordListHead);
            return 0;
        case MSG_COMMAND:
            id = LOWORD(wParam);
            code = HIWORD(wParam);
            switch (code)
            {
                case BN_CLICKED:
                {
                    switch (id)
                    {
                        case IDC_BUTTON_SUBCLOSE:
                            printf("smartmic close\n");
                            if (start_analyze == 1)
                            {
#if AR_SUPPORT
                                if (do_analyze)
                                {
                                    ST_VoiceAnalyzeStop();
                                    do_analyze = 0;
                                }
#endif

                                if (start_audioIn)
                                {
                                    ST_AudioInStop();
                                    start_audioIn = 0;
                                    start_analyze = 0;
                                    SetBtnBmp(g_hSmartmicBtn[0], DWORD(&btn_notselect));
                                    SetBtnBmp(g_hSmartmicBtn[1], DWORD(&btn_notselect));
                                    SetBtnBmp(g_hSmartmicBtn[2], DWORD(&btn_notselect));
                                }
                            }

                            for (i = 0; i < sizeof(g_hSmartmicBtn)/sizeof(HWND); i++)
                                DEL_BUTTON_OLD_PROC(g_hSmartmicBtn[i], g_btnProcListHead);

                            CLEAR_TRAINING_WORD_LIST(g_trainingWordListHead);
                            PostMessage(hWnd, MSG_CLOSE, 0, 0);
                            break;
                        case IDC_BUTTON_SMARTMIC_SUPPORTWORD:
                        {
                            HDC mhdc;
                            printf("smartmic search dict\n");
                            mhdc = GetDC (hWnd);
                            SelectFont(mhdc, logfont);
                            SetBkMode (mhdc, BM_TRANSPARENT);
                            TextOut (mhdc, stSmartMicItem[4].s32X, stSmartMicItem[4].s32Y, "已训练词汇");//text1
                            ReleaseDC(mhdc);
                            g_oldTrainWord.index = -1;
                            strcpy(g_oldTrainWord.szWord, "");

                            if (!analyze_init)
                            {
#if AR_SUPPORT
                                if (MI_SUCCESS == ST_VoiceAnalyzeInit())
                                {
                                    analyze_init = 1;
                                    SetBtnBmp(g_hSmartmicBtn[0], DWORD(&btn_select));
                                }
#endif
                            }
                            break;
                        }
                        case IDC_BUTTON_START_SMARTMIC:
                            printf("smartmic start train\n");
                            if (!list_empty(&g_trainingWordListHead))
                            {
                                if (!start_analyze)
                                {
                                    if (!start_audioIn)
                                    {
                                        if (MI_SUCCESS == ST_AudioInStart())
                                        {
                                            start_audioIn = 1;
                                        }
                                    }

                                    if (start_audioIn)
                                    {
#if AR_SUPPORT
                                        if (MI_SUCCESS == ST_VoiceAnalyzeStart())
                                        {
                                            do_analyze = 1;
#endif
                                            start_analyze = 1;
                                            SetBtnBmp(g_hSmartmicBtn[1], DWORD(&btn_select));
                                            SetBtnBmp(g_hSmartmicBtn[2], DWORD(&btn_notselect));
#if AR_SUPPORT
                                        }
#endif
                                    }
                                }
                            }
                            else
                                printf("please train word first\n");
                            break;
                        case IDC_BUTTON_STOP_SMARTMIC:
                            printf("smartmic stop training\n");
                            if (start_analyze == 1)
                            {
#if AR_SUPPORT
                                if (do_analyze)
                                {

                                    if (MI_SUCCESS == ST_VoiceAnalyzeStop())
                                    {
                                        do_analyze = 0;
                                    }
                                }

                                if (!do_analyze)
                                {
#endif
                                    if (MI_SUCCESS == ST_AudioInStop())
                                    {
                                        start_analyze = 0;
                                        start_audioIn = 0;
                                        SetBtnBmp(g_hSmartmicBtn[1], DWORD(&btn_notselect));
                                        SetBtnBmp(g_hSmartmicBtn[2], DWORD(&btn_select));
                                    }
#if AR_SUPPORT
                                }
#endif
                            }
                            break;
                    }
                    return 0;
                }
                case BN_DOUBLECLICKED:
                {
                    return 0;
                }
                default:
                    break;
            }
            break;
        case MSG_ERASEBKGND:
            {
                HDC hdcMem = CreateCompatibleDC((HDC)wParam);
                SetBrushColor (hdcMem, WINDOW_BGCOLOR((HDC)wParam));
                FillBox (hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H);
                BitBlt(hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H, (HDC)wParam, 0, 0, 0);
                DeleteCompatibleDC(hdcMem);
            }
            return 1;
        case MSG_DESTROY:
            DestroyAllControls (hWnd);
            hMainSmartMicWnd = HWND_INVALID;

            if (analyze_init == 1)
            {
                ST_VoiceAnalyzeDeInit();
                analyze_init = 0;
            }
            return 0;
        case MSG_CLOSE:
            printf("close my window....0x%x\n", hWnd);
            DestroyMainWindow (hWnd);
            MainWindowCleanup (hWnd);
            return 0;
    }

    return DefaultMainWinProc (hWnd, message, wParam, lParam);
}

static void InitCreateInfoSmartMicWnd(PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle =  WS_VISIBLE;
    pCreateInfo->dwExStyle = WS_EX_NONE;
    pCreateInfo->spCaption = "Setting";
    pCreateInfo->hMenu = 0;
#if HIDE_CURSOR
    pCreateInfo->hCursor = GetSystemCursor(IDC_NONE);
#else
    pCreateInfo->hCursor = GetSystemCursor(1);
#endif
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = SmartMicMainWinProc;
    pCreateInfo->lx = 0;
    pCreateInfo->ty = 0;
    pCreateInfo->rx = g_s32UiMainWnd_W;
    pCreateInfo->by = g_s32UiMainWnd_H;
    pCreateInfo->iBkColor = GetWindowElementColor (WE_MAINC_THREED_BODY);
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

void SmartMicMainWindow (HWND hwnd)
{
    MAINWINCREATE CreateInfo;
    if (hMainSmartMicWnd != HWND_INVALID) {
        ShowWindow(hMainSmartMicWnd, SW_SHOWNORMAL);
        return;
    }
    InitCreateInfoSmartMicWnd(&CreateInfo);
    CreateInfo.hHosting = hwnd;
    hMainSmartMicWnd = CreateMainWindow(&CreateInfo);
}

static LRESULT SettingMainWinProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    int id, code;
    HWND hVolSpkTrackbar = HWND_INVALID;
    HWND hVolMicTrackbar = HWND_INVALID;
    int i = 0;
    RECT rect;
    HWND hItem;
    HDC mhdc;
    UINT dtfmt;
    static MI_S32 s32PraseUiItemFlag = 0;

    if (((access(UI_SURFACE_CFG_FILE, F_OK))!=-1) && !s32PraseUiItemFlag)
    {
        if (0 == ST_XmlPraseUiCfg((MI_U8*)UI_SURFACE_CFG_FILE, (MI_U8*)"SettingWindow_LAYOUT", stSettingItem))
        {
            for (int i = 0; i < sizeof(stSettingItem)/sizeof(ST_Rect_T); i++)
            {
                printf("index=%d (%d-%d-%d-%d)...\n", i,
                    stSettingItem[i].s32X, stSettingItem[i].s32Y, stSettingItem[i].s16PicW, stSettingItem[i].s16PicH);
            }
            s32PraseUiItemFlag = 1;
        }
    }

    switch (message)
    {
        case MSG_CREATE:
            UpdateWindow(hWnd, TRUE);
            CreateWindow (CTRL_BUTTON,
                "return",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP,
                IDC_BUTTON_SUBCLOSE,
                stSettingItem[0].s32X, stSettingItem[0].s32Y, stSettingItem[0].s16PicW, stSettingItem[0].s16PicH, hWnd, (DWORD)&btn_return);
            CreateWindow (CTRL_BUTTON,
                "confirm",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP,
                IDC_BUTTON_CONFIRM,
                stSettingItem[1].s32X, stSettingItem[1].s32Y, stSettingItem[1].s16PicW, stSettingItem[1].s16PicH, hWnd, (DWORD)&btn_confirm);

            // setting page
            // create page btn
            for (i = 0; i < sizeof(g_astPageBtn)/sizeof(PageBtnInfo_t); i++)
            {
                g_astPageBtn[i].hBtn = CreateWindow (CTRL_BUTTON, g_astPageBtn[i].szTtile,
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_BUTTON_SOUND_SETTING+i,
                stSettingItem[2].s32X, stSettingItem[2].s32Y+i*stSettingItem[2].s16PicH, stSettingItem[2].s16PicW, stSettingItem[2].s16PicH, hWnd, (DWORD)&btn_notselect);
            }

            g_oldPageBtnProc = SetWindowCallbackProc(g_astPageBtn[0].hBtn, PageBtnProc);
            for (i = 1; i < sizeof(g_astPageBtn)/sizeof(PageBtnInfo_t); i++)
                SetWindowCallbackProc(g_astPageBtn[i].hBtn, PageBtnProc);

            for (i = 0; i < sizeof(g_astPageBtn)/sizeof(PageBtnInfo_t); i++)
            {
                SAVE_BUTTON_OLD_PROC(g_astPageBtn[i].hBtn, g_oldPageBtnProc, g_btnProcListHead);
                CreateSettingPage(hWnd, i);
            }

            g_hCurSelectHwnd = g_astPageBtn[0].hBtn;
            EnableShowPage(0, TRUE);
            return 0;
        case MSG_COMMAND:
            id = LOWORD(wParam);
            code = HIWORD(wParam);
            switch (code)
            {
                case BN_CLICKED:
                {
                    switch (id)
                    {
                        case IDC_BUTTON_SUBCLOSE:
                            printf("setting return\n");
                            PredacteModify();

                            for (i = 0; i < sizeof(g_astItemBtn)/sizeof(ItemBtnInfo_t); i++)
                                DEL_BUTTON_OLD_PROC(g_astItemBtn[i].hBtn, g_btnProcListHead);

                            for (i = 0; i < sizeof(g_astSubPageBtn)/sizeof(SubPageBtnInfo_t); i++)
                                DEL_BUTTON_OLD_PROC(g_astSubPageBtn[i].hBtn, g_btnProcListHead);

                            for (i = 0; i < sizeof(g_astPageBtn)/sizeof(PageBtnInfo_t); i++)
                                DEL_BUTTON_OLD_PROC(g_astPageBtn[i].hBtn, g_btnProcListHead);

                            for (i = 0; i < sizeof(g_astTrackBar)/sizeof(TrackBarInfo_t); i++)
                                DEL_TRACKBAR_OLD_PROC(g_astTrackBar[i].hTrackBar, g_trackBarProcListHead);

                            PostMessage(hWnd, MSG_CLOSE, 0, 0);
                            break;
                        case IDC_BUTTON_CONFIRM:
                            SaveModify();
                            break;
                        case IDC_BUTTON_RESTART:
                            {
                                if (IDOK == MessageBox(hWnd, "点击确定后，系统将重启！", "信息提示", MB_OKCANCEL))
                                {
                                    // restart device
                                }
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case BN_DOUBLECLICKED:
                {
                    break;
                }
                default:
                    break;
            }
            return 0;
        case MSG_ERASEBKGND:
            {
                HDC hdcMem = CreateCompatibleDC((HDC)wParam);
                SetBrushColor (hdcMem, WINDOW_BGCOLOR((HDC)wParam));
                FillBox (hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H);
                BitBlt(hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H, (HDC)wParam, 0, 0, 0);
                DeleteCompatibleDC(hdcMem);
            }
            return 1;
        case MSG_DESTROY:
            DestroyAllControls (hWnd);
            hMainSettingWnd = HWND_INVALID;
            return 0;
        case MSG_CLOSE:
            printf("close my window....0x%x\n", hWnd);
            DestroyMainWindow (hWnd);
            MainWindowCleanup (hWnd);
            return 0;
    }

    return DefaultMainWinProc (hWnd, message, wParam, lParam);
}

static void InitCreateInfoSettingWnd(PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle =  WS_VISIBLE;
    pCreateInfo->dwExStyle = WS_EX_NONE;
    pCreateInfo->spCaption = "Setting";
    pCreateInfo->hMenu = 0;
#if HIDE_CURSOR
    pCreateInfo->hCursor = GetSystemCursor(IDC_NONE);
#else
    pCreateInfo->hCursor = GetSystemCursor(IDC_ARROW);
#endif
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = SettingMainWinProc;
    pCreateInfo->lx = 0;
    pCreateInfo->ty = 0;
    pCreateInfo->rx = g_s32UiMainWnd_W;
    pCreateInfo->by = g_s32UiMainWnd_H;
    pCreateInfo->iBkColor = GetWindowElementColor (WE_MAINC_THREED_BODY);
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

void SettingMainWindow (HWND hwnd)
{
    MAINWINCREATE CreateInfo;
    if (hMainSettingWnd != HWND_INVALID) {
        ShowWindow(hMainSettingWnd, SW_SHOWNORMAL);
        return;
    }
    InitCreateInfoSettingWnd(&CreateInfo);
    CreateInfo.hHosting = hwnd;
    hMainSettingWnd = CreateMainWindow(&CreateInfo);
}

static MI_S32 s32g_CallType = 0; //0--caller  1--called
static LRESULT CalledMainWinProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    int id, code;
    unsigned long msg[4];
    static MI_S32 s32PraseUiItemFlag = 0;
    static ST_Rect_T stCalledItem[4] = {{900, 540, 124, 60}, {900, 100, 100, 60}, {900, 200, 100, 60},
        {200, 60, 640, 480}};

    if (((access(UI_SURFACE_CFG_FILE, F_OK))!=-1) && !s32PraseUiItemFlag)
    {
        if (0 == ST_XmlPraseUiCfg((MI_U8*)UI_SURFACE_CFG_FILE, (MI_U8*)"CalledWindow_LAYOUT", stCalledItem))
        {
            for (int i = 0; i < 4; i++)
            {
                printf("index=%d (%d-%d-%d-%d)...\n", i,
                    stCalledItem[i].s32X, stCalledItem[i].s32Y, stCalledItem[i].s16PicW, stCalledItem[i].s16PicH);
            }
            s32PraseUiItemFlag = 1;
        }
    }
    switch (message)
    {
        case MSG_CREATE:
            CreateWindow (CTRL_BUTTON,
                "Return",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP,
                IDC_BUTTON_SUBCLOSE,
                stCalledItem[0].s32X, stCalledItem[0].s32Y, stCalledItem[0].s16PicW, stCalledItem[0].s16PicH, hWnd, (DWORD)&btn_return);
            if (s32g_CallType == 1) //called
            {
            CreateWindow (CTRL_BUTTON,
                "HOLD ON",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE,
                IDC_VIDEO_BUTTON_HOLDON,
                stCalledItem[1].s32X, stCalledItem[1].s32Y, stCalledItem[1].s16PicW, stCalledItem[1].s16PicH, hWnd, 0);
            }
            CreateWindow (CTRL_BUTTON,
                "HANG UP",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE,
                IDC_VIDEO_BUTTON_HANGUP,
                stCalledItem[2].s32X, stCalledItem[2].s32Y, stCalledItem[2].s16PicW, stCalledItem[2].s16PicH, hWnd, 0);
            break;
        case MSG_COMMAND:
            id = LOWORD(wParam);
            code = HIWORD(wParam);
            switch (code)
            {
                case BN_CLICKED:
                {
                    switch (id)
                    {
                        case IDC_VIDEO_BUTTON_HOLDON:
                        {
                            if (s32g_CallType == 0) //caller
                            {
                                return 0;
                            }
                            printf("holdon talk %d %d...\n", msg[0], s32g_CallType);
                            memset(msg, 0, 16);
                            msg[0] = MSG_TYPE_TALK_CALLED_HOLDON;
                            msg[1] = 0;
                            msg[2] = 0;
                            msg[3] = 0;
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)msg,sizeof(msg),&g_toAPP_sem);
                            return 0;
                        }
                        case IDC_VIDEO_BUTTON_HANGUP:
                        case IDC_BUTTON_SUBCLOSE:
                        {
                            printf("close talk %d %d...\n", msg[0], s32g_CallType);
                            memset(msg, 0, 16);
                            if (s32g_CallType == 0) //caller
                            {
                                msg[0] = MSG_TYPE_TALK_CALLER_HANGUP;
                            }
                            else if (s32g_CallType == 1) //called
                            {
                                msg[0] = MSG_TYPE_TALK_CALLED_HANGUP;
                            }
                            msg[1] = 0;
                            msg[2] = 0;
                            msg[3] = 0;
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)msg,sizeof(msg),&g_toAPP_sem);
                            PostMessage(hWnd, MSG_CLOSE, 0, 0);
                            return 0;
                        }
                    }
                }
                case BN_DOUBLECLICKED:
                    break;
                default:
                    break;
            }
            break;
        case MSG_ERASEBKGND:
            {
                HDC hdcMem = CreateCompatibleDC((HDC)wParam);
                SetBrushColor (hdcMem, WINDOW_BGCOLOR((HDC)wParam));
                FillBox (hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H);
                SetBrushColor (hdcMem, RGBA2Pixel(hdcMem, 0x80, 0x80, 0x80, 0x80));
                FillBox (hdcMem, stCalledItem[3].s32X, stCalledItem[3].s32Y, stCalledItem[3].s16PicW, stCalledItem[3].s16PicH);
                BitBlt(hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H, (HDC)wParam, 0, 0, 0);
                DeleteCompatibleDC(hdcMem);
            }
            return 1;
        case MSG_DESTROY:
            DestroyAllControls (hWnd);
            hMainCalledWnd = HWND_INVALID;
            return 0;
        case MSG_CLOSE:
            printf("close my window....0x%x\n", hWnd);
            DestroyMainWindow (hWnd);
            MainWindowCleanup (hWnd);
            return 0;
    }

    return DefaultMainWinProc (hWnd, message, wParam, lParam);
}

static void InitCreateInfoCalledrWnd(PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle =  WS_VISIBLE;
    pCreateInfo->dwExStyle = WS_EX_NONE;
    pCreateInfo->spCaption = "called";
    pCreateInfo->hMenu = 0;
#if HIDE_CURSOR
    pCreateInfo->hCursor = GetSystemCursor(IDC_NONE);
#else
    pCreateInfo->hCursor = GetSystemCursor(IDC_ARROW);
#endif
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = CalledMainWinProc;
    pCreateInfo->lx = 0;
    pCreateInfo->ty = 0;
    pCreateInfo->rx = g_s32UiMainWnd_W;
    pCreateInfo->by = g_s32UiMainWnd_H;
    pCreateInfo->iBkColor = GetWindowElementColor (WE_MAINC_THREED_BODY);
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

void CalledMainWindow (HWND hwnd)
{
    MAINWINCREATE CreateInfo;
    if (hMainCalledWnd != HWND_INVALID) {
        ShowWindow(hMainCalledWnd, SW_SHOWNORMAL);
        return;
    }
    InitCreateInfoCalledrWnd(&CreateInfo);
    CreateInfo.hHosting = hwnd;
    hMainCalledWnd = CreateMainWindow(&CreateInfo);
}

void MonitorMainWindow (HWND hwnd);
static char g_CallStr[64];
static LRESULT CallMainWinProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    MI_S32 id, code;
    unsigned long sendmsg[4];
    HWND hCallEdit, mhdc;
    HWND hButton_1 = HWND_INVALID;
    static char CallId[32];
    static MI_S32 callnumcount = 0;
    static MI_S32 s32ReadUiPostionInfo = 0;
    static MI_S32 s32PraseUiItemFlag = 0;
    static ST_Rect_T stCallItem[16] = {{200, 40, 500, 50}, {200, 130, 164, 80}, {380, 130, 164, 80},
        {560, 130, 164, 80}, {200, 220, 164, 80}, {380, 220, 164, 80}, {560, 220, 164, 80},
        {200, 310, 164, 80}, {380, 310, 164, 80}, {560, 310, 164, 80}, {200, 400, 164, 80},
        {380, 400, 164, 80}, {560, 400, 164, 80}, {820, 200, 164, 80},{820, 300, 164, 80},
        {900, 540, 124, 60}};

    if (((access(UI_SURFACE_CFG_FILE, F_OK))!=-1) && !s32PraseUiItemFlag)
    {
        if (0 == ST_XmlPraseUiCfg((MI_U8*)UI_SURFACE_CFG_FILE, (MI_U8*)"CallWindow_LAYOUT", stCallItem))
        {
            for (int i = 0; i < 16; i++)
            {
                printf("index=%d (%d-%d-%d-%d)...\n", i,
                    stCallItem[i].s32X, stCallItem[i].s32Y, stCallItem[i].s16PicW, stCallItem[i].s16PicH);
            }
            s32PraseUiItemFlag = 1;
        }
    }

    switch (message)
    {
        case MSG_CREATE:
            hCallEdit  = CreateWindow (CTRL_EDIT,
                "",
                WS_CHILD | WS_VISIBLE | WS_BORDER,
                IDC_CALL_EDIT_CALLNUM,
                stCallItem[0].s32X, stCallItem[0].s32Y, stCallItem[0].s16PicW, stCallItem[0].s16PicH, hWnd, 0);
            hButton_1 = CreateWindow (CTRL_BUTTON,
                "1",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_1,
                stCallItem[1].s32X, stCallItem[1].s32Y, stCallItem[1].s16PicW, stCallItem[1].s16PicH, hWnd, (DWORD)&btn_1);
            //g_oldPageBtnProc = SetWindowCallbackProc(hButton_1, PageBtnProc);
            CreateWindow (CTRL_BUTTON,
                "2",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_2,
                stCallItem[2].s32X, stCallItem[2].s32Y, stCallItem[2].s16PicW, stCallItem[2].s16PicH, hWnd, (DWORD)&btn_2);
            CreateWindow (CTRL_BUTTON,
                "3",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_3,
                stCallItem[3].s32X, stCallItem[3].s32Y, stCallItem[3].s16PicW, stCallItem[3].s16PicH, hWnd, (DWORD)&btn_3);
            CreateWindow (CTRL_BUTTON,
                "4",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_4,
                stCallItem[4].s32X, stCallItem[4].s32Y, stCallItem[4].s16PicW, stCallItem[4].s16PicH, hWnd, (DWORD)&btn_4);
            CreateWindow (CTRL_BUTTON,
                "5",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_5,
                stCallItem[5].s32X, stCallItem[5].s32Y, stCallItem[5].s16PicW, stCallItem[5].s16PicH, hWnd, (DWORD)&btn_5);
            CreateWindow (CTRL_BUTTON,
                "6",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_6,
                stCallItem[6].s32X, stCallItem[6].s32Y, stCallItem[6].s16PicW, stCallItem[6].s16PicH, hWnd, (DWORD)&btn_6);
            CreateWindow (CTRL_BUTTON,
                "7",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_7,
                stCallItem[7].s32X, stCallItem[7].s32Y, stCallItem[7].s16PicW, stCallItem[7].s16PicH, hWnd, (DWORD)&btn_7);
            CreateWindow (CTRL_BUTTON,
                "8",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_8,
                stCallItem[8].s32X, stCallItem[8].s32Y, stCallItem[8].s16PicW, stCallItem[8].s16PicH, hWnd, (DWORD)&btn_8);
            CreateWindow (CTRL_BUTTON,
                "9",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_9,
                stCallItem[9].s32X, stCallItem[9].s32Y, stCallItem[9].s16PicW, stCallItem[9].s16PicH, hWnd, (DWORD)&btn_9);
            CreateWindow (CTRL_BUTTON,
                "abc",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_ABC,
                stCallItem[10].s32X, stCallItem[10].s32Y, stCallItem[10].s16PicW, stCallItem[10].s16PicH, hWnd, (DWORD)&btn_abc);
            CreateWindow (CTRL_BUTTON,
                "0",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_0,
                stCallItem[11].s32X, stCallItem[11].s32Y, stCallItem[11].s16PicW, stCallItem[11].s16PicH, hWnd, (DWORD)&btn_0);
            CreateWindow (CTRL_BUTTON,
                "X",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_X,
                stCallItem[12].s32X, stCallItem[12].s32Y, stCallItem[12].s16PicW, stCallItem[12].s16PicH, hWnd, (DWORD)&btn_X);
            CreateWindow (CTRL_BUTTON,
                "Monitor",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_VIDEO_BUTTON_START_MONITOR,
                stCallItem[13].s32X, stCallItem[13].s32Y, stCallItem[13].s16PicW, stCallItem[13].s16PicH, hWnd, (DWORD)&btn_monitor);
            CreateWindow (CTRL_BUTTON,
                "calling",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_CALL_BUTTON_CALLING,
                stCallItem[14].s32X, stCallItem[14].s32Y, stCallItem[14].s16PicW, stCallItem[14].s16PicH, hWnd, (DWORD)&btn_calling);
            CreateWindow (CTRL_BUTTON,
                "return",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
                IDC_BUTTON_SUBCLOSE,
                stCallItem[15].s32X, stCallItem[15].s32Y, stCallItem[15].s16PicW, stCallItem[15].s16PicH, hWnd, (DWORD)&btn_return);
            break;
        case MSG_COMMAND:
            id = LOWORD(wParam);
            code = HIWORD(wParam);
            switch (code)
            {
                case BN_CLICKED:
                {
                    switch (id)
                    {
                        case IDC_BUTTON_SUBCLOSE:
                            PostMessage(hWnd, MSG_CLOSE, 0, 0);
                            break;
                        case IDC_CALL_BUTTON_1:
                            CallId[callnumcount++] = '1';
                            break;
                        case IDC_CALL_BUTTON_2:
                            CallId[callnumcount++] = '2';
                            break;
                        case IDC_CALL_BUTTON_3:
                            CallId[callnumcount++] = '3';
                            break;
                        case IDC_CALL_BUTTON_4:
                            CallId[callnumcount++] = '4';
                            break;
                        case IDC_CALL_BUTTON_5:
                            CallId[callnumcount++] = '5';
                            break;
                        case IDC_CALL_BUTTON_6:
                            CallId[callnumcount++] = '6';
                            break;
                        case IDC_CALL_BUTTON_7:
                            CallId[callnumcount++] = '7';
                            break;
                        case IDC_CALL_BUTTON_8:
                            CallId[callnumcount++] = '8';
                            break;
                        case IDC_CALL_BUTTON_9:
                            CallId[callnumcount++] = '9';
                            break;
                        case IDC_CALL_BUTTON_ABC:
                            break;
                        case IDC_CALL_BUTTON_0:
                            CallId[callnumcount++] = '0';
                            break;
                        case IDC_CALL_BUTTON_X:
                            if (callnumcount >= 1)
                            {
                                CallId[callnumcount-1] = '\0';
                                callnumcount--;
                                break;
                            }
                            else
                            {
                                return 0;
                            }
                        case IDC_CALL_BUTTON_CALLING:
                        {
                            SetDlgItemText (hWnd, IDC_CALL_EDIT_CALLNUM, CallId);
                            GetDlgItemText (hWnd, IDC_CALL_EDIT_CALLNUM, g_CallStr, sizeof(g_CallStr));
                            printf("calling %s\n", g_CallStr);
                            s32g_CallType = 0;//caller
                            CalledMainWindow(hWnd);
                            memset(sendmsg, 0, 16);
                            sendmsg[0] = MSG_TYPE_LOCAL_CALL_ROOM;
                            sendmsg[1] = (unsigned long)g_CallStr;
                            sendmsg[2] = 0;
                            sendmsg[3] = 0;
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                            break;
                        }
                        case IDC_VIDEO_BUTTON_START_MONITOR:
                        {
                            SetDlgItemText (hWnd, IDC_CALL_EDIT_CALLNUM, CallId);
                            GetDlgItemText (hWnd, IDC_CALL_EDIT_CALLNUM, g_CallStr, sizeof(g_CallStr));
                            printf("monitor %s\n", g_CallStr);
                            //MonitorMainWindow(hWnd);
                            memset(sendmsg, 0, 16);
                            sendmsg[0] = MSG_TYPE_LOCAL_START_MONITOR;
                            sendmsg[1] = (unsigned long)g_CallStr;
                            sendmsg[2] = 0;
                            sendmsg[3] = 0;
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                            break;
                        }
                    }
                    printf("callid = %s...len = %d\n", CallId, callnumcount);
                    mhdc = GetDC (hWnd);
                    SelectFont(mhdc, logfont);
                    SetDlgItemText (hWnd, IDC_CALL_EDIT_CALLNUM, CallId);
                    ReleaseDC(mhdc);
                    return 0; //button click event always return.
                }
                case BN_DOUBLECLICKED:
                {
                    break;
                }
                default:
                    break;
            }
            break;
        case MSG_ERASEBKGND:
            {
                HDC hdcMem = CreateCompatibleDC((HDC)wParam);
                SetBrushColor (hdcMem, WINDOW_BGCOLOR((HDC)wParam));
                FillBox (hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H);
                BitBlt(hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H, (HDC)wParam, 0, 0, 0);
                DeleteCompatibleDC(hdcMem);
            }
            return 1;
        case MSG_DESTROY:
            DestroyAllControls (hWnd);
            memset(CallId, 0, 64);
            callnumcount = 0;
            hMainCallWnd = HWND_INVALID;
            return 0;
        case MSG_CLOSE:
            printf("close my window....0x%x\n", hWnd);
            DestroyMainWindow (hWnd);
            MainWindowCleanup (hWnd);
            return 0;
        case MSG_USER_MONITORDOOR:
            {
                MonitorMainWindow(hWnd);
                return 0;
            }
    }

    return DefaultMainWinProc (hWnd, message, wParam, lParam);
}

static void InitCreateInfoCallWnd(PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle =  WS_VISIBLE;
    pCreateInfo->dwExStyle = WS_EX_NONE;
    pCreateInfo->spCaption = "call";
    pCreateInfo->hMenu = 0;
#if HIDE_CURSOR
    pCreateInfo->hCursor = GetSystemCursor(IDC_NONE);
#else
    pCreateInfo->hCursor = GetSystemCursor(1);
#endif
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = CallMainWinProc;
    pCreateInfo->lx = 0;
    pCreateInfo->ty = 0;
    pCreateInfo->rx = g_s32UiMainWnd_W;
    pCreateInfo->by = g_s32UiMainWnd_H;
    pCreateInfo->iBkColor = GetWindowElementColor (WE_MAINC_THREED_BODY);
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

void CallMainWindow (HWND hwnd)
{
    MAINWINCREATE CreateInfo;
    if (hMainCallWnd != HWND_INVALID) {
        ShowWindow(hMainCallWnd, SW_SHOWNORMAL);
        return;
    }
    InitCreateInfoCallWnd(&CreateInfo);
    CreateInfo.hHosting = hwnd;
    hMainCallWnd = CreateMainWindow(&CreateInfo);
}

static LRESULT LocalVideoMainWinProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    int id, code;
    unsigned long msg[4];
    static ST_Rect_T stCallItem[3] = {{900, 540, 124, 60}, {900, 100, 100, 60}, {200, 60, 640, 480}};
    static MI_S32 s32PraseUiItemFlag = 0;

    if (((access(UI_SURFACE_CFG_FILE, F_OK))!=-1) && !s32PraseUiItemFlag)
    {
        if (0 == ST_XmlPraseUiCfg((MI_U8*)UI_SURFACE_CFG_FILE, (MI_U8*)"LocalVideoWindow_LAYOUT", stCallItem))
        {
            for (int i = 0; i < 16; i++)
            {
                printf("index=%d (%d-%d-%d-%d)...\n", i,
                    stCallItem[i].s32X, stCallItem[i].s32Y, stCallItem[i].s16PicW, stCallItem[i].s16PicH);
            }
            s32PraseUiItemFlag = 1;
        }
    }
    switch (message)
    {
        case MSG_CREATE:
            CreateWindow (CTRL_BUTTON,
                "Return",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP,
                IDC_BUTTON_SUBCLOSE,
                stCallItem[0].s32X, stCallItem[0].s32Y, stCallItem[0].s16PicW, stCallItem[0].s16PicH, hWnd, (DWORD)&btn_return);

            //g_hCamaraSnapBtn = CreateWindow (CTRL_BUTTON, g_CamaraSnapBtnText,
            //    WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP | BS_FLAT,
            //    IDC_VIDEO_BUTTON_START_REGISTER,
            //    stCallItem[1].s32X, stCallItem[1].s32Y, stCallItem[1].s16PicW, stCallItem[1].s16PicH, hWnd, (DWORD)&btn_notselect);

            //g_oldCamaraSnapBtnProc = SetWindowCallbackProc(g_hCamaraSnapBtn, NormalBtnProc);
            //SAVE_BUTTON_OLD_PROC(g_hCamaraSnapBtn, g_oldCamaraSnapBtnProc, g_btnProcListHead);

            memset(msg, 0, 16);
            msg[0] = MSG_TYPE_LOCAL_VIDEO_DISP;
            msg[1] = 1;
            g_toAPP_msg_queue.send_message(MODULE_MSG, (void*)msg, sizeof(msg), &g_toAPP_sem);
            g_faceDetect = 1;
            break;
        case MSG_COMMAND:
            id = LOWORD(wParam);
            code = HIWORD(wParam);
            switch (code)
            {
                case BN_CLICKED:
                {
                    switch (id)
                    {
                        case IDC_BUTTON_SUBCLOSE:
                            //DEL_BUTTON_OLD_PROC(g_hCamaraSnapBtn, g_btnProcListHead);
                            PostMessage(hWnd, MSG_CLOSE, 0, 0);
                            memset(msg, 0, 16);
                            msg[0] = MSG_TYPE_LOCAL_VIDEO_DISP;
                            msg[1] = 0;
                            printf("return --------------\n");
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)msg,sizeof(msg),&g_toAPP_sem);
                            break;
                        //case IDC_VIDEO_BUTTON_START_REGISTER:
                        //{
                        //    unsigned long msg[4];
                        //    memset(msg, 0, 16);
                        //    msg[0] = MSG_TYPE_FACEREGISTER_CTRL;
                        //    msg[1] = 1;
                        //    msg[2] = 0;
                        //    msg[3] = 0;
                        //    g_toUI_msg_queue.send_message(MODULE_MSG,(void*)msg,sizeof(msg),&g_toUI_sem);
                        //    break;
                        //}
                    }
                }
                case BN_DOUBLECLICKED:
                    break;
                default:
                    break;
            }
            break;
        case MSG_ERASEBKGND:
            {
                HDC hdcMem = CreateCompatibleDC((HDC)wParam);
                SetBrushColor (hdcMem, WINDOW_BGCOLOR((HDC)wParam));
                FillBox (hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H);
                //SetBrushColor (hdcMem, UI_ARGB888_BLUE);
                SetBrushColor (hdcMem, RGBA2Pixel(hdcMem, 0x80, 0x80, 0x80, 0x80));
                //SetBrushColor (hdcMem, MakeRGBA(0x0, 0x0, 0xFF, 0x0));
                FillBox (hdcMem, stCallItem[2].s32X, stCallItem[2].s32Y, stCallItem[2].s16PicW, stCallItem[2].s16PicH);
                BitBlt(hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H, (HDC)wParam, 0, 0, 0);
                DeleteCompatibleDC(hdcMem);
            }
            return 1;
        case MSG_DESTROY:
            DestroyAllControls (hWnd);
            hMainVideoWnd = HWND_INVALID;
            return 0;
        case MSG_CLOSE:
            printf("close my window....0x%x\n", hWnd);
            DestroyMainWindow (hWnd);
            MainWindowCleanup (hWnd);
            return 0;
    }

    return DefaultMainWinProc (hWnd, message, wParam, lParam);
}

static void InitCreateInfoLocalVideoWnd(PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle =  WS_VISIBLE;
    pCreateInfo->dwExStyle = WS_EX_NONE;
    pCreateInfo->spCaption = "video";
    pCreateInfo->hMenu = 0;
#if HIDE_CURSOR
    pCreateInfo->hCursor = GetSystemCursor(IDC_NONE);
#else
    pCreateInfo->hCursor = GetSystemCursor(1);
#endif
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = LocalVideoMainWinProc;
    pCreateInfo->lx = 0;
    pCreateInfo->ty = 0;
    pCreateInfo->rx = g_s32UiMainWnd_W;
    pCreateInfo->by = g_s32UiMainWnd_H;
    pCreateInfo->iBkColor = GetWindowElementColor (WE_MAINC_THREED_BODY);
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

void LocalVideoMainWindow (HWND hwnd)
{
    MAINWINCREATE CreateInfo;
    if (hMainVideoWnd != HWND_INVALID) {
        ShowWindow(hMainVideoWnd, SW_SHOWNORMAL);
        return;
    }
    InitCreateInfoLocalVideoWnd(&CreateInfo);
    CreateInfo.hHosting = hwnd;
    hMainVideoWnd = CreateMainWindow(&CreateInfo);
}

static LRESULT MonitorMainWinProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
    int id, code;
    unsigned long msg[4];
    static ST_Rect_T stCallItem[2] = {{900, 540, 124, 60}, {200, 60, 640, 480}};
    static MI_S32 s32PraseUiItemFlag = 0;

    if (((access(UI_SURFACE_CFG_FILE, F_OK))!=-1) && !s32PraseUiItemFlag)
    {
        if (0 == ST_XmlPraseUiCfg((MI_U8*)UI_SURFACE_CFG_FILE, (MI_U8*)"MonitorWindow_LAYOUT", stCallItem))
        {
            for (int i = 0; i < 16; i++)
            {
                printf("index=%d (%d-%d-%d-%d)...\n", i,
                    stCallItem[i].s32X, stCallItem[i].s32Y, stCallItem[i].s16PicW, stCallItem[i].s16PicH);
            }
            s32PraseUiItemFlag = 1;
        }
    }
    switch (message)
    {
        case MSG_CREATE:
            CreateWindow (CTRL_BUTTON,
                "Return",
                WS_CHILD | BS_PUSHBUTTON | BS_CHECKED | WS_VISIBLE | BS_BITMAP,
                IDC_BUTTON_SUBCLOSE,
                stCallItem[0].s32X, stCallItem[0].s32Y, stCallItem[0].s16PicW, stCallItem[0].s16PicH, hWnd, (DWORD)&btn_return);
            break;
        case MSG_COMMAND:
            id = LOWORD(wParam);
            code = HIWORD(wParam);
            switch (code)
            {
                case BN_CLICKED:
                {
                    switch (id)
                    {
                        case IDC_BUTTON_SUBCLOSE:
                        {
                            memset(msg, 0, 16);
                            msg[0] = MSG_TYPE_LOCAL_STOP_MONITOR;
                            msg[1] = 0;
                            msg[2] = 0;
                            msg[3] = 0;
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)msg,sizeof(msg),&g_toAPP_sem);
                            PostMessage(hWnd, MSG_CLOSE, 0, 0);
                            return 0;
                        }
                    }
                }
                case BN_DOUBLECLICKED:
                    break;
                default:
                    break;
            }
            break;
        case MSG_ERASEBKGND:
            {
                HDC hdcMem = CreateCompatibleDC((HDC)wParam);
                SetBrushColor (hdcMem, WINDOW_BGCOLOR((HDC)wParam));
                FillBox (hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H);
                SetBrushColor (hdcMem, RGBA2Pixel(hdcMem, 0x80, 0x80, 0x80, 0x80));
                FillBox (hdcMem, stCallItem[1].s32X, stCallItem[1].s32Y, stCallItem[1].s16PicW, stCallItem[1].s16PicH);
                BitBlt(hdcMem, 0, 0, MAINWND_W, MAINWND_H, (HDC)wParam, 0, 0, 0);
                DeleteCompatibleDC(hdcMem);
            }
            return 1;
        case MSG_DESTROY:
            DestroyAllControls (hWnd);
            hMainMonitorWnd = HWND_INVALID;
            return 0;
        case MSG_CLOSE:
            printf("close my window....0x%x\n", hWnd);
            DestroyMainWindow (hWnd);
            MainWindowCleanup (hWnd);
            return 0;
    }

    return DefaultMainWinProc (hWnd, message, wParam, lParam);
}

static void InitCreateInfoMonitorWnd(PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle =  WS_VISIBLE;
    pCreateInfo->dwExStyle = WS_EX_NONE;
    pCreateInfo->spCaption = "video";
    pCreateInfo->hMenu = 0;
#if HIDE_CURSOR
    pCreateInfo->hCursor = GetSystemCursor(IDC_NONE);
#else
    pCreateInfo->hCursor = GetSystemCursor(1);
#endif
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = MonitorMainWinProc;
    pCreateInfo->lx = 0;
    pCreateInfo->ty = 0;
    pCreateInfo->rx = g_s32UiMainWnd_W;
    pCreateInfo->by = g_s32UiMainWnd_H;
    pCreateInfo->iBkColor = GetWindowElementColor (WE_MAINC_THREED_BODY);
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

void MonitorMainWindow (HWND hwnd)
{
    MAINWINCREATE CreateInfo;
    if (hMainMonitorWnd != HWND_INVALID) {
        ShowWindow(hMainMonitorWnd, SW_SHOWNORMAL);
        return;
    }
    InitCreateInfoMonitorWnd(&CreateInfo);
    CreateInfo.hHosting = hwnd;
    hMainMonitorWnd = CreateMainWindow(&CreateInfo);
}

static char *mk_time(char *buff)
{
    time_t timer;
    struct tm *tblock;
    timer = time(NULL);
    tblock = localtime(&timer);
    sprintf(buff, "%s\n", asctime(tblock));
    return buff;
}

void *msg_toUIcmd_process(void *args)
{
    int err = 0;
    Msg* pMsg = NULL;
    unsigned long RMsgBuff[4];

    while (g_run)
    {
        memset(RMsgBuff,0,sizeof(RMsgBuff));
        sem_wait(&g_toUI_sem);
        pMsg = g_toUI_msg_queue.get_message();
        if(pMsg){
            err = cmd_parse_msg(pMsg,RMsgBuff);
            if(err < 0)
                break;
            if(err == 0)
                continue;
            switch (RMsgBuff[0])
            {
                case MSG_TYPE_SMARTMIC_START:
                {
                    printf("index=%d name=%s len=%d\n", RMsgBuff[1], RMsgBuff[2], RMsgBuff[3]);
                    char cmdGb2312[100] = "";
                    utf8ToGb2312(cmdGb2312, 100, (char *)RMsgBuff[2], RMsgBuff[3]);
                    printf("Result: %s \n", cmdGb2312);
                    if (HWND_INVALID != hMainSmartMicWnd)
                    {

                        if (RMsgBuff[1] >= 0 && RMsgBuff[1] < 32)
                        {
                            HDC mhdc;
//                            HDC hdcMem;

                            SAVE_TRAINING_WORD(RMsgBuff[1], cmdGb2312, g_trainingWordListHead);

                            mhdc = GetDC (hMainSmartMicWnd);
//                            hdcMem = CreateCompatibleDC(mhdc);
                            SelectFont(mhdc, logfontgb12);
                            SetPenColor(mhdc, PIXEL_darkyellow);

                            if (RMsgBuff[1] < 16)
                            {
                                Rectangle (mhdc, stSmartMicItem[5].s32X, stSmartMicItem[5].s32Y+25*RMsgBuff[1], stSmartMicItem[5].s32X+180, stSmartMicItem[5].s32Y+25*RMsgBuff[1]+25);
                                SetBkMode (mhdc, BM_TRANSPARENT);
                                SetTextColor(mhdc, PIXEL_blue);
                                TextOut (mhdc, stSmartMicItem[5].s32X+5, stSmartMicItem[5].s32Y+25*RMsgBuff[1]+5, (const char*)cmdGb2312);
                            }
                            else
                            {
                                Rectangle (mhdc, stSmartMicItem[5].s32X+200, stSmartMicItem[5].s32Y+25*(RMsgBuff[1] - 16), stSmartMicItem[5].s32X+380, stSmartMicItem[5].s32Y+25*(RMsgBuff[1] - 16)+25);
                                SetBkMode (mhdc, BM_TRANSPARENT);
                                SetTextColor(mhdc, PIXEL_blue);
                                TextOut (mhdc, stSmartMicItem[5].s32X+205, stSmartMicItem[5].s32Y+25*(RMsgBuff[1] - 16)+5, (const char*)cmdGb2312);
                            }

//                            BitBlt(hdcMem, stSmartMicItem[5].s32X, stSmartMicItem[5].s32Y, 380, 25*16, mhdc, stSmartMicItem[5].s32X, stSmartMicItem[5].s32Y, 0);
//                            DeleteCompatibleDC(hdcMem);
                            ReleaseDC(mhdc);
                        }
                    }
                    break;
                }
                case MSG_TYPE_SMARTMIC_MACTCH:
                {
                    if (HWND_INVALID != hMainSmartMicWnd)
                    {
                        HDC mhdc;
//                        HDC hdcMem;
                        int index = -1;
                        char cmdGb2312[100] = "";

                        utf8ToGb2312(cmdGb2312, 100, (char *)RMsgBuff[2], RMsgBuff[3]);
                        printf("Result: utf-8(%s) gb2313(%s) \n", (char *)RMsgBuff[2], cmdGb2312);
                        FIND_TRAINING_WORD(cmdGb2312, g_trainingWordListHead, &index);
                        printf("training word index %d\n", index);

                        mhdc = GetDC (hMainSmartMicWnd);
//                        hdcMem = CreateCompatibleDC(mhdc);
                        SetPenColor(mhdc, PIXEL_darkyellow);
                        SelectFont(mhdc, logfontgb12);

                        // recover old training word
                        printf("old trainword index %d\n", g_oldTrainWord.index);
                        if (g_oldTrainWord.index >= 0 && g_oldTrainWord.index < 32)
                        {
                            SetBrushColor(mhdc, RGBA2Pixel(mhdc, 0x0, 0xC8, 0xC8, 0xFF));
                            if (g_oldTrainWord.index < 16)
                            {
                                FillBox(mhdc, stSmartMicItem[5].s32X, stSmartMicItem[5].s32Y+25*g_oldTrainWord.index, 180, 25);
                                Rectangle (mhdc, stSmartMicItem[5].s32X, stSmartMicItem[5].s32Y+25*g_oldTrainWord.index, stSmartMicItem[5].s32X+180, stSmartMicItem[5].s32Y+25*g_oldTrainWord.index+25);
                                SetBkMode (mhdc, BM_TRANSPARENT);
                                SetTextColor(mhdc, PIXEL_blue);
                                TextOut (mhdc, stSmartMicItem[5].s32X+5, stSmartMicItem[5].s32Y+25*g_oldTrainWord.index+5, g_oldTrainWord.szWord);

                                printf("clear %s index:%d\n", g_oldTrainWord.szWord, g_oldTrainWord.index);
                            }
                            else
                            {
                                FillBox(mhdc, stSmartMicItem[5].s32X+200, stSmartMicItem[5].s32Y+25*(g_oldTrainWord.index-16), 180, 25);
                                Rectangle (mhdc, stSmartMicItem[5].s32X+200, stSmartMicItem[5].s32Y+25*(g_oldTrainWord.index - 16), stSmartMicItem[5].s32X+380, stSmartMicItem[5].s32Y+25*(g_oldTrainWord.index - 16)+25);
                                SetBkMode (mhdc, BM_TRANSPARENT);
                                SetTextColor(mhdc, PIXEL_blue);
                                TextOut (mhdc, stSmartMicItem[5].s32X+205, stSmartMicItem[5].s32Y+25*(g_oldTrainWord.index - 16)+5, g_oldTrainWord.szWord);

                                printf("clear %s index:%d\n", g_oldTrainWord.szWord, g_oldTrainWord.index);
                            }
                        }

                        if (index >= 0 && index < 32)
                        {
                            SetBrushColor (mhdc, PIXEL_green);
                            if (index < 16)
                            {
                                FillBox(mhdc, stSmartMicItem[5].s32X, stSmartMicItem[5].s32Y+25*index, 180, 25);
                                Rectangle (mhdc, stSmartMicItem[5].s32X, stSmartMicItem[5].s32Y+25*index, stSmartMicItem[5].s32X+180, stSmartMicItem[5].s32Y+25*index+25);
                                SetBkMode (mhdc, BM_TRANSPARENT);
                                SetTextColor(mhdc, PIXEL_blue);
                                TextOut (mhdc, stSmartMicItem[5].s32X+5, stSmartMicItem[5].s32Y+25*index+5, (const char*)cmdGb2312);
                            }
                            else
                            {
                                FillBox(mhdc, stSmartMicItem[5].s32X+200, stSmartMicItem[5].s32Y+25*(index-16), 180, 25);
                                Rectangle (mhdc, stSmartMicItem[5].s32X+200, stSmartMicItem[5].s32Y+25*(index - 16), stSmartMicItem[5].s32X+380, stSmartMicItem[5].s32Y+25*(index - 16)+25);
                                SetBkMode (mhdc, BM_TRANSPARENT);
                                SetTextColor(mhdc, PIXEL_blue);
                                TextOut (mhdc, stSmartMicItem[5].s32X+205, stSmartMicItem[5].s32Y+25*(index - 16)+5, (const char*)cmdGb2312);
                            }

                            // save training word
                            g_oldTrainWord.index = index;
                            strcpy(g_oldTrainWord.szWord, cmdGb2312);
                        }

//                        BitBlt(hdcMem, 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H, mhdc, 0, 0, 0);
//                        DeleteCompatibleDC(hdcMem);
                        ReleaseDC(mhdc);
                    }
                    break;
                }
                case MSG_TYPE_FACEREGISTER_CTRL:
                {
                    g_faceSnap = RMsgBuff[1];
                    break;
                }
                case MSG_TYPE_DISP_DETECT_FACE:
                {
                    if (LoadBitmap (HDC_SCREEN, &bmp_label_face, "appres/faceR.jpg"))
                    {
                        return NULL;
                    }
                    if (HWND_INVALID != hMainVideoWnd)
                    {
                        FaceDispMainWindow(hMainVideoWnd);
                    }
                    g_faceSnap = 0;
                    break;
                }
                case MSG_TYPE_REMOTE_CALL_ME:
                {
                    printf("ui cmd process MSG_TYPE_REMOTE_CALL_ME\n");
                    if (HWND_INVALID != hMainWnd)
                    {
                        printf("CalledMainWindow\n");
                        s32g_CallType = 1;
                        PostMessage(hMainWnd, MSG_USER_DOORCALLME, 0, 0);
                    }
                    break;
                }
                case MSG_UI_CALLER_HANGUP:
                {
                    if (HWND_INVALID != hMainCalledWnd)
                    {
                        PostMessage(hMainCalledWnd, MSG_CLOSE, 0, 0);
                    }
                    break;
                }
                case MSG_UI_CALLED_HANGUP:
                {
                    if (HWND_INVALID != hMainCalledWnd)
                    {
                        PostMessage(hMainCalledWnd, MSG_CLOSE, 0, 0);
                    }
                    break;
                }
                case MSG_UI_CREATE_MONITOR_WIN:
                {
                    if (HWND_INVALID != hMainCallWnd)
                    {
                         PostMessage(hMainCallWnd, MSG_USER_MONITORDOOR, 0, 0);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    return NULL;
}

//==================================================Picture View================
static BITMAP icon_demos [8];

static const char *iconfiles[8] =
{
    "appres/media/1.jpg",
    "appres/media/2.jpg",
    "appres/media/3.jpg",
    "appres/media/4.jpg",
    "appres/media/5.jpg",
    "appres/media/6.jpg",
    "appres/media/7.jpg",
    "appres/media/8.jpg",
};

static const char *iconlabels[8] =
{
    "music",
    "ebook",
    "game",
    "picture",
    "video",
    "calendar",
    "setting",
    "watch",
};

static BOOL iconv_init(mDialogBox *self)
{
    NCS_ICONFLOW_ITEMINFO info;
    static int i = 0, j = 0, pos = 0;
    mIconFlow *iconvObj;
    HWND iconvWnd;
    RECT rc;
#if ROUNDRECT_ICON
    mShapeTransRoundPiece *bk = NEWPIECE(mShapeTransRoundPiece);
#else
    mRectPiece *bk = NEWPIECE(mRectPiece);
#endif
    ARGB COLORS_ICONFLOW_BK[] = {0xff646873, 0xff2e2f32};

    for(i = 0; i < TABLESIZE(icon_demos); i++)
    {
        LoadBitmap(HDC_SCREEN, &icon_demos[i], iconfiles[i]);
    }

    iconvWnd = GetDlgItem(self->hwnd, IDC_TEST_ICONFLOW);
    iconvObj = (mIconFlow *)ncsObjFromHandle(iconvWnd);
    GetWindowRect(iconvWnd, &rc);

    if(!iconvObj)
        return FALSE;

// START_OF_ADDITEMS

    for(i = 0; i < TABLESIZE(icon_demos); i++)
    {
        pos = 0;
        memset(&info, 0, sizeof(NCS_ICONFLOW_ITEMINFO));
        info.bmp = &icon_demos[i];
        info.index = TABLESIZE(icon_demos) * j + i;
        info.label = iconlabels[i];
        info.addData = (DWORD)iconlabels[i];
        _c(iconvObj)->addItem(iconvObj, &info, &pos);
    }

    _c(iconvObj)->setCurSel(iconvObj, 0);
// END_OF_ADDITEMS

    _c(iconvObj)->setIconSize(iconvObj, RECTW(rc)/2,RECTH(rc)/2);

    _c(iconvObj)->setProperty(iconvObj, NCSP_ICONFLOW_SPAN, (DWORD)(400));

#if 0
    LoadBitmap(HDC_SCREEN, &bkicon_demo, bkiconfile);
    _c(iconvObj)->setProperty(iconvObj, NCSP_ICONFLOW_ICONFRAME, (DWORD)&bkicon_demo);
#else
    //_c(iconvObj)->setProperty(iconvObj, NCSP_ICONFLOW_ICONBORDER, 1);
#endif

    _c(bk)->setRect(bk, &rc);
#if ROUNDRECT_ICON
    _c(bk)->setProperty(bk, NCSP_TRANROUND_RADIUS, 0);
    _c(bk)->setProperty(bk, NCSP_TRANROUND_FILLMODE, (DWORD)NCSP_TRANROUND_GRADIENT_FILL);
    _c(bk)->setGradientFillColors(bk, COLORS_ICONFLOW_BK, TABLESIZE(COLORS_ICONFLOW_BK));
#else
    _c(bk)->setProperty(bk, NCSP_RECTPIECE_FILLCOLOR, MakeRGBA(0x64, 0xf68, 0x73, 0xff));
#endif
    _c(iconvObj)->setProperty(iconvObj, NCSP_ICONFLOW_BKGNDPIECE, (DWORD)bk);

    return TRUE;
}

static BOOL iconvexit_init(mDialogBox *self)
{
    NCS_ICONFLOW_ITEMINFO info;
    static int i = 0, j = 0, pos = 0;
    mIconFlow *iconvObj;
    HWND iconvWnd;
    HDC hdc;
    RECT rc;
    mRectPiece *bk = NEWPIECE(mRectPiece);
    ARGB COLORS_ICONFLOW_BK[] = {0xff646873, 0xff2e2f32};

    iconvWnd = GetDlgItem(self->hwnd, IDC_TEST_ICONFLOW_EXIT);
    iconvObj = (mIconFlow *)ncsObjFromHandle(iconvWnd);
    GetWindowRect(iconvWnd, &rc);

    hdc = GetDC (iconvWnd);
    FillBoxWithBitmap(hdc, 0, 0, 0, 0, &btn_return);
    ReleaseDC(hdc);

    if(!iconvObj)
        return FALSE;

    return TRUE;
}


// START_OF_WNDHANDLERS
static BOOL mainwnd_onKeyDown(mWidget *self,
                              int message, int code, DWORD key_status)
{
    if(message == MSG_KEYDOWN)
    {
        if(code == SCANCODE_REMOVE)
        {
            mIconFlow *iconView;
            int curSel, count;
            HITEM delItem;

            iconView =
                (mIconFlow *)ncsObjFromHandle(GetDlgItem(self->hwnd, IDC_TEST_ICONFLOW));
            count = _c(iconView)->getItemCount(iconView);

            if(iconView)
            {
                curSel = _c(iconView)->getCurSel(iconView);

                if(curSel >= 0)
                {
                    delItem = _c(iconView)->getItem(iconView, curSel);
                    _c(iconView)->removeItem(iconView, delItem);

                    if(curSel == count - 1)
                        curSel--;

                    _c(iconView)->setCurSel(iconView, curSel);
                }
            }
        }

    }

    if ((message == MSG_RBUTTONDOWN) || (message == MSG_CLOSE))
    {
        printf("MSG_RBUTTONDOWNMSG_RBUTTONDOWNMSG_RBUTTONDOWN\n");
        DestroyMainWindow ((mWidget*)self->hwnd);
        MainWindowCleanup ((mWidget*)self->hwnd);
        hIconvWnd = HWND_INVALID;
    }

    return FALSE;
}

static NCS_EVENT_HANDLER mainwnd_handlers[] =
{
    {MSG_KEYDOWN, (void *)mainwnd_onKeyDown},
    {MSG_RBUTTONDOWN, (void *)mainwnd_onKeyDown},
    {MSG_CLOSE, (void *)mainwnd_onKeyDown},
    {0, NULL}
};
// END_OF_WNDHANDLERS

// START_OF_ICONVHANDLERS
static void iconv_notify(mWidget *self, int id, int nc, DWORD add_data)
{
    if(nc == NCSN_ICONFLOW_CLICKED)
    {
        if(self)
        {
            int idx;
            const char  *text;
            mIconFlow   *iconvObj = (mIconFlow *)self;

            idx = _c(iconvObj)->indexOf(iconvObj, (HITEM)add_data);
            text = _c(iconvObj)->getText(iconvObj, (HITEM)add_data);
            fprintf(stderr, "click icon[%d], text is %s \n", idx, text);
        }
    }
}

static NCS_EVENT_HANDLER iconv_handlers[] =
{
    NCS_MAP_NOTIFY(NCSN_ICONFLOW_CLICKED, iconv_notify),
    NCS_MAP_NOTIFY(NCSN_ICONFLOW_SELCHANGED, iconv_notify),
    {0, NULL }
};
// END_OF_ICONVHANDLERS

static NCS_RDR_INFO iconv_rdr_info =
{
    "classic", "classic", NULL
};

static void iconvexit_notify(mWidget *self, int id, int nc, DWORD add_data)
{
    if(nc == NCSN_ICONFLOW_CLICKED)
    {
        if(self)
        {
            if (hIconvWnd != HWND_INVALID)
            {
                for(int i = 0; i < TABLESIZE(icon_demos); i++)
                {
                    UnloadBitmap(&icon_demos[i]);
                }
                PostMessage(hIconvWnd, MSG_CLOSE, 0, 0);
            }
        }
    }
}

static NCS_EVENT_HANDLER iconvexit_handlers[] =
{
    NCS_MAP_NOTIFY(NCSN_ICONFLOW_CLICKED, iconvexit_notify),
    {0, NULL }
};
// END_OF_ICONVHANDLERS

static NCS_RDR_INFO iconvexit_rdr_info =
{
    "exit", "exit", NULL
};

static NCS_WND_TEMPLATE _ctrl_tmpl[] =
{
    {
        NCSCTRL_ICONFLOW,
        IDC_TEST_ICONFLOW,
        0, 0, MAINWND_W, MAINWND_H,
        WS_BORDER | WS_CHILD | WS_VISIBLE | NCSS_NOTIFY | NCSS_ICONFLOW_LOOP,
        WS_EX_NONE,
        "",
        NULL,
        &iconv_rdr_info,
        iconv_handlers,
        NULL,
        0,
        0
    },
    {
        NCSCTRL_ICONFLOW,
        IDC_TEST_ICONFLOW_EXIT,
        MAINWND_W - 96, MAINWND_H - 48, 96, 48,
        WS_BORDER | WS_CHILD | WS_VISIBLE | NCSS_NOTIFY | NCSS_ICONFLOW_LOOP,
        WS_EX_NONE,
        "",
        NULL,
        &iconvexit_rdr_info,
        iconvexit_handlers,
        NULL,
        0,
        0
    }

};

static NCS_MNWND_TEMPLATE mainwnd_tmpl =
{
    NCSCTRL_DIALOGBOX,
    IDC_PICTURE_WINDOW,
    0, 0, MAINWND_W, MAINWND_H,
    WS_VISIBLE,
    WS_EX_NONE,
    "IconView Demo",
    NULL,
    NULL,
    mainwnd_handlers,
    _ctrl_tmpl,
    sizeof(_ctrl_tmpl) / sizeof(NCS_WND_TEMPLATE),
    0,
    0, 0,
};
//=====================================================picture==============

BITMAP *bmp = NULL;

static char  _VARG_FENCE;
static char *_PVARG_FENCE = &_VARG_FENCE;
#define VARG_FENCE _PVARG_FENCE
typedef struct tagDspItem
{
    BOOL    cdpath;
    char    path [PATH_MAX + 1];
    char    name [NAME_MAX + 1];
#ifdef _MGRM_PROCESSES
        char layer[LEN_LAYER_NAME + 1];
#endif
    char    bmp_path [PATH_MAX + NAME_MAX + 1];
    mImagePiece *imagepiece;
    BITMAP  bmp;
} DSPITEM;

typedef struct _ICON_INFO
{
    int      nr_apps;
    int      focus;
    DSPITEM *app_items;
} ICON_INFO;

static ICON_INFO icon_info;

static BOOL onkeydown(mWidget *widget, int message, int code, DWORD keyStatus)
{
    mHScrollViewPiece *view = (mHScrollViewPiece *)((mContainerCtrl *)widget)->body;
    RECT rc;
    int offset_x = 0;
    int offset_y = 0;

    _c(view)->getViewport(view, &rc);

    switch(code)
    {
        case SCANCODE_J:
            offset_y = 10;
            break;

        case SCANCODE_K:
            offset_y = -10;
            break;

        case SCANCODE_L:
            offset_x = 10;
            break;

        case SCANCODE_H:
            offset_x = -10;
            break;

        case SCANCODE_S:
        {
            static BOOL show = TRUE;
            _c(view)->showScrollBar(view, show);
            show = !show;
        }
        break;

        default:
            return FALSE;
    }

    _c(view)->moveViewport(view, rc.left + offset_x, rc.top + offset_y);
    return TRUE;
}

static NCS_EVENT_HANDLER g_handles[] =
{
    {MSG_KEYDOWN, (void *)onkeydown},
    {0, NULL}
};

static BOOL onTimer(mWidget *widget, int message, int id, DWORD tick)
{
    mTextPiece *textPiece;
    mPanelPiece *panelPiece = (mPanelPiece *)((mContainerCtrl *)widget)->body;
    char *b = NULL;

    //SetDlgItemText (widget->hwnd, 0, mk_time(buff));
    if (panelPiece)
        textPiece = (mTextPiece *)_c(panelPiece)->childAt(panelPiece, 0, 0);

    if (textPiece)
    {
        time_t timer;
        struct tm *tblock;
        timer = time(NULL);
        tblock = localtime(&timer);
        _c(textPiece)->setProperty(textPiece, NCSP_LABELPIECE_LABEL, (DWORD)asctime(tblock));
        b = (char *)_c(textPiece)->getProperty(textPiece, NCSP_LABELPIECE_LABEL);
        InvalidateRect(widget->hwnd, NULL, TRUE);
    }
    return TRUE;
}

static NCS_EVENT_HANDLER g_timerhandles[] =
{
    {MSG_TIMER, (void *)onTimer},
    {0, NULL}
};

static PLOGFONT createLogFont(unsigned size)
{
    return CreateLogFont("ttf", "helvetica", "UTF-8",
                         FONT_WEIGHT_BOOK, FONT_SLANT_ROMAN,
                         FONT_SETWIDTH_NORMAL, FONT_OTHER_AUTOSCALE,
                         FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE,
                         size, 0);
}

static PLOGFONT CreateArialFont(unsigned size)
{
    return CreateLogFont(FONT_TYPE_NAME_SCALE_TTF, "fzcircle", "ISO8859-1",
                         FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN,
                         FONT_FLIP_NIL, FONT_OTHER_NIL,
                         FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE,
                         size, 0);
}

static PLOGFONT CreateFzcircleFont(unsigned size)
{
    return CreateLogFont(FONT_TYPE_NAME_SCALE_TTF, "fzcircle", "GB2312",
                         FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN,
                         FONT_FLIP_NIL, FONT_OTHER_NIL,
                         FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE,
                         size, 0);
}

static PLOGFONT createExplorerFont(unsigned size)
{
    return CreateLogFont("ttf", "helvetica", "GB2312",
                         FONT_WEIGHT_BOOK, FONT_SLANT_ROMAN,
                         FONT_SETWIDTH_NORMAL, FONT_OTHER_AUTOSCALE,
                         FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE,
                         size, 0);
}

static int s_onMouseEvent(mHotPiece *_self, int message, WPARAM wParam, LPARAM lParam, mObject *owner)
{
    printf("%s(message=%d)\n", __FUNCTION__, message);

    switch(message)
    {
        case MSG_LBUTTONDOWN:
        {
            RECT rc;
            WORD x, y;
            int i;
            DSPITEM *item;
            char buff [PATH_MAX + NAME_MAX + 1];
            char resolution[4][8];
            x = LOWORD(lParam);
            y = HIWORD(lParam);
            mPanelPiece *panelPiece = (mPanelPiece *)_self;
            mImagePiece *imagePiece = (mImagePiece *)_c(panelPiece)->childAt(panelPiece, x, y);


            for(i = 0; i < icon_info.nr_apps; i++)
            {
                if(icon_info.app_items[i].imagepiece == imagePiece)
                {
                    break;
                }
            }

            if(i == icon_info.nr_apps)
            {
                printf("cann't find imagepiece\n");
                return -1;
            }

            item = icon_info.app_items + i;

            if(item->cdpath)
            {
                chdir(item->path);
            }

            strcpy(buff, item->path);
            strcat(buff, item->name);
            printf("path=%s  name=%s item = %d...\n", item->path, item->name, i);
            memset(resolution, 0, 2 * 8 * sizeof(char));
            switch (i)
            {
                case 0:
                    if (hMainWnd != HWND_INVALID)
                    {
                        CallMainWindow(hMainWnd);
                    }
                    break;
                case 1:
                    if (hMainWnd != HWND_INVALID)
                    {
                        LocalVideoMainWindow(hMainWnd);
                    }
                    break;
                case 3:
                    {
                        if (hMainWnd != HWND_INVALID)
                        {
                            mainwnd_tmpl.x = 0;
                            mainwnd_tmpl.y = 0;
                            mainwnd_tmpl.w = MAINWND_W;
                            mainwnd_tmpl.h = MAINWND_H;
                            mainwnd_tmpl.ctrls[0].x = 0;
                            mainwnd_tmpl.ctrls[0].y = 0;
                            mainwnd_tmpl.ctrls[0].w = MAINWND_W;
                            mainwnd_tmpl.ctrls[0].h = MAINWND_H;
                            mDialogBox *mydlg = (mDialogBox *)ncsCreateMainWindowIndirect(&mainwnd_tmpl, hMainWnd);
                            iconv_init(mydlg);
                            iconvexit_init(mydlg);
                            hIconvWnd = mydlg->hwnd;
                        }
                        break;
                    }
                case 9:
                    if (hMainWnd != HWND_INVALID)
                    {
                        SettingMainWindow(hMainWnd);
                    }
                    break;
                case 2:
                    if (hMainWnd != HWND_INVALID)
                    {
                        SmartMicMainWindow(hMainWnd);
                    }
                    break;
                case 5:
                    if (hMainWnd != HWND_INVALID)
                    {

                    }
                    break;
                case 7:
                    if (hMainWnd != HWND_INVALID)
                    {
                        // create playtoolbar test

                        int id, code, i;
                        static MI_S32 s32PraseUiItemFlag = 0;

                        // detect usb device
                        memset(g_pu8RootPath, 0, sizeof(g_pu8RootPath));
                        if (MI_SUCCESS != ST_DetectUsbDev((char*)g_pu8RootPath, sizeof(g_pu8RootPath)))
                        {
                            printf("Please insert u disk first\n");
                            break;
                        }

                        // create filetree according to rootpath
                        InitFileTreeRoot(&g_fileRoot, (char*)g_pu8RootPath);
                        CreateFileTree(&g_fileRoot);
                        //BrowseFileTree(&g_fileRoot);

                        // get ui layout from xml
                        if (((access(UI_SURFACE_CFG_FILE, F_OK))!=-1) && !s32PraseUiItemFlag)
                        {
                            if (0 == ST_XmlPraseUiCfg((MI_U8*)UI_SURFACE_CFG_FILE, (MI_U8*)"PlayToolBar_LAYOUT", g_stPlayToolBarItem))
                            {
                                for (int i = 0; i < sizeof(g_stPlayToolBarItem)/sizeof(ST_Rect_T); i++)
                                {
                                    printf("index=%d (%d-%d-%d-%d)...\n", i,
                                        g_stPlayToolBarItem[i].s32X, g_stPlayToolBarItem[i].s32Y, g_stPlayToolBarItem[i].s16PicW, g_stPlayToolBarItem[i].s16PicH);
                                }
                                s32PraseUiItemFlag = 1;
                            }
                        }

                        memset(g_pu8FullPath, 0, sizeof(g_pu8FullPath));
                        printf("root path is %s\n", (char*)g_pu8RootPath);
                        if( (access((char*)g_pu8SelectPath, F_OK)) != -1 && strstr((char*)g_pu8SelectPath, (char*)g_pu8RootPath))
                        {
                            char *p = NULL;
                            memcpy(g_pu8FullPath, g_pu8SelectPath, strlen((char*)g_pu8SelectPath));
                            p = strrchr((char*)g_pu8FullPath, '/');
                            *p = 0;
                        }
                        else
                        {
                            strcpy((char*)g_pu8FullPath, (char*)g_pu8RootPath);
                        }

                        printf("init full path:%s\n", g_pu8FullPath);

                        // init controller pos & size
                        for (i = 0; i < sizeof(_playFileWnd_ctrl_tmpl)/sizeof(NCS_WND_TEMPLATE); i++)
                        {
                            _playFileWnd_ctrl_tmpl[i].x = g_stPlayToolBarItem[i].s32X;
                            _playFileWnd_ctrl_tmpl[i].y = g_stPlayToolBarItem[i].s32Y;
                            _playFileWnd_ctrl_tmpl[i].w = g_stPlayToolBarItem[i].s16PicW;
                            _playFileWnd_ctrl_tmpl[i].h = g_stPlayToolBarItem[i].s16PicH;
                        }

                        playFileWnd_tmpl.ctrls = _playFileWnd_ctrl_tmpl;
                        g_pPlayFileFont = createExplorerFont(36);
                        g_bShowPlayToolBar = FALSE;

                        mDialogBox* dialog = (mDialogBox *)ncsCreateMainWindowIndirect (&playFileWnd_tmpl, hMainWnd);
                        lstv_init(dialog);
                        hMainPlayFileWnd = dialog->hwnd;
                    }
                    break;
                case 10:
                    if (hMainWnd != HWND_INVALID)
                    {
                        //TimeMainWindow(hMainWnd);     //for test
                    }
                    break;
                case 11:
                    if (hMainWnd != HWND_INVALID)
                    {

                    }
                default:
                    break;
            }

            //sprintf(resolution[0],"%d",(1920 / 2 - MAINWND_W / 2));
            //sprintf(resolution[1],"%d",(1080 / 2 - MAINWND_H / 2) + HEADER_LOGO_HEIGHT);
            //sprintf(resolution[2],"%d",MAINWND_W);
            //sprintf(resolution[3],"%d",MAINWND_H-HEADER_LOGO_HEIGHT);
            //exec_app(buff, item->name,resolution[0],resolution[1],resolution[2],resolution[3],"-layer", VARG_FENCE);
            return 0;
        }
        break;
        default:
        {

        }
    }

    return -1;
}



static void naviBar_freeItems(void)
{
    int i;
    DSPITEM *item;

    item = icon_info.app_items;

    for(i = 0; i < icon_info.nr_apps; i++, item++)
    {
        if(item->bmp.bmBits)
        {
            UnloadBitmap(&item->bmp);
            item->bmp.bmBits = NULL;
        }
    }

    free(icon_info.app_items);
    icon_info.app_items = NULL;
}



static BOOL naviBar_getItems(void)
{
#define APP_INFO_FILE "hscrollview.rc"
    int i;
    DSPITEM *item;
    char section [10];
    SIZE size;

    if(GetIntValueFromEtcFile(APP_INFO_FILE, "navibar", "app_nr", &icon_info.nr_apps) != ETC_OK)
        return FALSE;

    if(icon_info.nr_apps <= 0)
        return FALSE;


    if((icon_info.app_items = (DSPITEM *)calloc(icon_info.nr_apps, sizeof(DSPITEM))) == NULL)
    {
        return FALSE;
    }

    item = icon_info.app_items;

    for(i = 0; i < icon_info.nr_apps; i++, item++)
    {
        sprintf(section, "navi-app%d", i);

        if(GetValueFromEtcFile(APP_INFO_FILE, section, "path", item->path, PATH_MAX) != ETC_OK)
            goto error;

        if(GetValueFromEtcFile(APP_INFO_FILE, section, "name", item->name, NAME_MAX) != ETC_OK)
            goto error;
#ifdef _MGRM_PROCESSES
        if(GetValueFromEtcFile(APP_INFO_FILE, section, "layer", item->layer, LEN_LAYER_NAME) != ETC_OK)
            goto error;
#endif
        if(GetValueFromEtcFile(APP_INFO_FILE, section, "icon", item->bmp_path, PATH_MAX + NAME_MAX) != ETC_OK)
            goto error;

        if(LoadBitmap(HDC_SCREEN, &item->bmp, item->bmp_path) != ERR_BMP_OK)
        {
            fprintf(stderr, "desktop load resource:%s error. \n", item->bmp_path);
            goto error;
        }

        item->cdpath = TRUE;
        //GetTextExtent(HDC_SCREEN, item->name, -1, &size);

    }

    return TRUE;
error:
    naviBar_freeItems();
    return FALSE;
}


static BOOL _mymain_onCreate_naviBar(mMainWnd *self, DWORD dwAddData)
{

#define ITEM_INTERVAL 25
    mContainerCtrl *ctnr;
    mPanelPiece *content;
    mHScrollViewPiece *view;
    mHotPiece *backPiece;
    int i, shiftx;
    RECT rc;
    int barW = 0;
    int barH = 0;
    int itemH = 0;
    int naviBarItemNum ;
    naviBar_getItems();
    naviBarItemNum = icon_info.nr_apps;


    GetClientRect(self->hwnd, &rc);

    barW = RECTW(rc);
    barH = RECTH(rc) >> 2;
    ctnr = (mContainerCtrl *)ncsCreateWindow(NCSCTRL_CONTAINERCTRL,
            "navibar",
            WS_VISIBLE, 0, 0,
            0, (RECTH(rc) - barH), RECTW(rc), RECTH(rc) >> 2,
            self->hwnd,
            NULL, NULL, NULL, 0);
#if HIDE_CURSOR
    ShowCursor(FALSE);
#endif
    content = (mPanelPiece *)NEWPIECE(mPanelPiece);
    _c(content)->appendEventHandler(content, MSG_LBUTTONDOWN, s_onMouseEvent);
    _c(content)->appendEventHandler(content, MSG_LBUTTONUP, s_onMouseEvent);
    _c(content)->appendEventHandler(content, MSG_MOUSEMOVE, s_onMouseEvent);
    _c(content)->appendEventHandler(content, MSG_MOUSEMOVEIN, s_onMouseEvent);

    view = NEWPIECE(mHScrollViewPiece);
    GetClientRect(ctnr->hwnd, &rc);
    _c(view)->setRect(view, &rc);
    _c(view)->addContent(view, (mHotPiece *)content, 0, 0);

    _c(ctnr)->setBody(ctnr, (mHotPiece *)view);

    /* add a rect piece as background.*/
    backPiece = (mHotPiece *)NEWPIECE(mRectPiece);
    GetClientRect(ctnr->hwnd, &rc);
    itemH = (barH >> 1);
    shiftx = rc.left + (itemH + ITEM_INTERVAL) * naviBarItemNum + ITEM_INTERVAL;

    if(shiftx < rc.right)
    {
        shiftx = (rc.right - shiftx) / 2;
    }
    else
    {
        rc.right = shiftx;

        shiftx = 0;
    }

    _c(content)->setRect(content, &rc);
    _c(backPiece)->setRect(backPiece, &rc);
    _c(backPiece)->setProperty(backPiece, NCSP_RECTPIECE_FILLCOLOR, MakeRGBA(0xf0, 0xf0, 0xf0, 0xff));

    _c(content)->addContent(content, backPiece, 0, 0);

    rc.left = rc.top = 0;
#if 0
    for(i = 0; i < naviBarItemNum; i++)
    {
        mHotPiece *imagePiece = (mHotPiece *)NEWPIECE(mImagePiece);
        mHotPiece *labelPiece = (mHotPiece *)NEWPIECE(mLabelPiece);

        rc.right = itemH;
        rc.bottom = itemH;

        _c(imagePiece)->setRect(imagePiece, &rc);
        _c(imagePiece)->setProperty(imagePiece, NCSP_IMAGEPIECE_IMAGE, (DWORD)&icon_info.app_items[i].bmp);
        _c(imagePiece)->setProperty(imagePiece, NCSP_IMAGEPIECE_DRAWMODE, NCS_DM_SCALED);
        _c(content)->addContent(content, imagePiece, ITEM_INTERVAL + (itemH + ITEM_INTERVAL) * i + shiftx, 10);
        icon_info.app_items[i].imagepiece = (mImagePiece *) imagePiece;

        rc.bottom = 10;
        rc.right = itemH;
        _c(labelPiece)->setRect(labelPiece, &rc);
        _c(labelPiece)->setProperty(labelPiece, NCSP_LABELPIECE_LABEL, (DWORD)icon_info.app_items[i].name);
        _c(content)->addContent(content, labelPiece, ITEM_INTERVAL + (itemH + ITEM_INTERVAL) * i + shiftx, itemH + ITEM_INTERVAL);
    }
#endif
    for(i = 0; i < naviBarItemNum; i++)
    {
        mHotPiece *imagePiece = (mHotPiece *)NEWPIECE(mImagePiece);
        mHotPiece *textPiece = (mHotPiece *)NEWPIECE(mTextPiece);

        rc.right = itemH;
        rc.bottom = itemH;

        _c(imagePiece)->setRect(imagePiece, &rc);
        _c(imagePiece)->setProperty(imagePiece, NCSP_IMAGEPIECE_IMAGE, (DWORD)&icon_info.app_items[i].bmp);
        _c(imagePiece)->setProperty(imagePiece, NCSP_IMAGEPIECE_DRAWMODE, NCS_DM_SCALED);
        _c(content)->addContent(content, imagePiece, ITEM_INTERVAL + (itemH + ITEM_INTERVAL) * i + shiftx, 10);
        icon_info.app_items[i].imagepiece = (mImagePiece *) imagePiece;

        rc.bottom = 30;
        rc.right = itemH;
        _c(textPiece)->setRect(textPiece, &rc);
        //char cmdGb2312[100] = "";
        //utf8ToGb2312(cmdGb2312, 100, (char *)icon_info.app_items[i].name, strlen(icon_info.app_items[i].name));
        //printf("Result: %s \n", cmdGb2312);
        //_c(textPiece)->setProperty(textPiece, NCSP_LABELPIECE_LABEL, (DWORD)cmdGb2312);
        _c(textPiece)->setProperty(textPiece, NCSP_LABELPIECE_LABEL, (DWORD)icon_info.app_items[i].name);
        _c(textPiece)->setProperty(textPiece, NCSP_TEXTPIECE_TEXTCOLOR, (DWORD)0xFF0000FF);
        _c(textPiece)->setProperty(textPiece, NCSP_TEXTPIECE_LOGFONT, (DWORD)CreateFzcircleFont(15));
        _c(textPiece)->setProperty(textPiece, NCSP_LABELPIECE_AUTOWRAP, (DWORD)TRUE);
        _c(content)->addContent(content, textPiece, ITEM_INTERVAL + (itemH + ITEM_INTERVAL) * i + shiftx, itemH + ITEM_INTERVAL);
    }

    ncsSetComponentHandlers((mComponent *)ctnr, g_handles, -1);
    //free(bmp);
    return TRUE;
}

static BOOL _mymain_onCreate_Header(mMainWnd *self, DWORD dwAddData)
{
#define HEADER_LOGO_MARGIN 2
    mContainerCtrl *ctnr;
    mPanelPiece *view;
    mHotPiece *backPiece;
    RECT rc;
    static BITMAP logo;
    static int loadmap = 0;
    if (0 == loadmap)
    {
        LoadBitmap(HDC_SCREEN, &logo, "appres/sigmalogo.jpg");
        loadmap = 1;
    }
    GetClientRect(self->hwnd, &rc);

    ctnr = (mContainerCtrl *)ncsCreateWindow(NCSCTRL_CONTAINERCTRL,
            "logo",
            WS_VISIBLE, 0, 0,
            0, 0, RECTW(rc), HEADER_LOGO_HEIGHT,
            self->hwnd,
            NULL, NULL, NULL, 0);
#if HIDE_CURSOR
    ShowCursor(FALSE);
#endif
    view = (mPanelPiece *)NEWPIECE(mPanelPiece);
    _c(ctnr)->setBody(ctnr, (mHotPiece *)view);

    /* add a rect piece as background.*/
    backPiece = (mHotPiece *)NEWPIECE(mRectPiece);
    GetClientRect(ctnr->hwnd, &rc);
    _c(view)->setRect(view, &rc);
    _c(backPiece)->setRect(backPiece, &rc);
    _c(backPiece)->setProperty(backPiece, NCSP_RECTPIECE_FILLCOLOR, MakeRGBA(0xff, 0xff, 0xff, 0xff));

    _c(view)->addContent(view, backPiece, 0, 0);

    /* add 18 imagePiece */
    rc.left = rc.top = 0;
    rc.right = HEADER_LOGO_WIDTH;
    rc.bottom = HEADER_LOGO_HEIGHT - 2 * HEADER_LOGO_MARGIN;

    mHotPiece *imagePiece = (mHotPiece *)NEWPIECE(mImagePiece);

    _c(imagePiece)->setRect(imagePiece, &rc);
    _c(imagePiece)->setProperty(imagePiece, NCSP_IMAGEPIECE_IMAGE, (DWORD)&logo);
    _c(imagePiece)->setProperty(imagePiece, NCSP_IMAGEPIECE_DRAWMODE, NCS_DM_SCALED);
    _c(view)->addContent(view, imagePiece, HEADER_LOGO_MARGIN, HEADER_LOGO_MARGIN);
    return TRUE;
}

static BOOL _mymain_onCreateBriefInfo(mMainWnd *self, DWORD dwAddData)
{
#define BRIEF_TMP_HEIGHT 60
    mContainerCtrl *ctnr;
    mPanelPiece *view;
    mHotPiece *backPiece;
    RECT rc;
    static BITMAP weather;
    static const char *tmp = "sunny \n 15 ~ 25 *C";
    static int loadmap = 0;
    if (0 == loadmap)
    {
        LoadBitmap(HDC_SCREEN, &weather, "appres/cloud_1.png");
        loadmap = 1;
    }

    GetClientRect(self->hwnd, &rc);

    ctnr = (mContainerCtrl *)ncsCreateWindow(NCSCTRL_CONTAINERCTRL,
            "logo",
            WS_VISIBLE, 0, 0,
            0, HEADER_LOGO_HEIGHT, RECTW(rc) / 2, RECTH(rc) - HEADER_LOGO_HEIGHT - (RECTH(rc) >> 2),
            self->hwnd,
            NULL, NULL, NULL, 0);
#if HIDE_CURSOR
    ShowCursor(FALSE);
#endif
    view = (mPanelPiece *)NEWPIECE(mPanelPiece);
    _c(ctnr)->setBody(ctnr, (mHotPiece *)view);

    /* add a rect piece as background.*/
    backPiece = (mHotPiece *)NEWPIECE(mRectPiece);
    GetClientRect(ctnr->hwnd, &rc);
    printf("%s %d %d %d %d %d\n", __FUNCTION__, __LINE__, rc.left, rc.top, rc.right, rc.bottom);
    _c(view)->setRect(view, &rc);
    _c(backPiece)->setRect(backPiece, &rc);
    _c(backPiece)->setProperty(backPiece, NCSP_RECTPIECE_FILLCOLOR, MakeRGBA(0xff, 0xff, 0xff, 0xff));

    _c(view)->addContent(view, backPiece, 0, 0);

    /* add 18 imagePiece */
    rc.left = rc.top = 2;
    rc.right = RECTW(rc) / 2;
    rc.bottom = (float)rc.right * weather.bmHeight / weather.bmWidth;

    mHotPiece *imagePiece = (mHotPiece *)NEWPIECE(mImagePiece);

    _c(imagePiece)->setRect(imagePiece, &rc);
    _c(imagePiece)->setProperty(imagePiece, NCSP_IMAGEPIECE_IMAGE, (DWORD)&weather);
    _c(imagePiece)->setProperty(imagePiece, NCSP_IMAGEPIECE_DRAWMODE, NCS_DM_SCALED);
    _c(view)->addContent(view, imagePiece, HEADER_LOGO_MARGIN, HEADER_LOGO_MARGIN);


    _c(imagePiece)->getRect(imagePiece, &rc);
    mHotPiece *textPiece = (mHotPiece *)NEWPIECE(mTextPiece);

    _c(view)->addContent(view, textPiece, rc.right + 2, RECTH(rc) / 2 - BRIEF_TMP_HEIGHT / 2);
    rc.left = rc.top = 2;
    rc.right = rc.right - 2; //RECTW(rc) / 2 -4;
    rc.bottom = BRIEF_TMP_HEIGHT;
    _c(textPiece)->setRect(textPiece, &rc);
    _c(textPiece)->setProperty(textPiece, NCSP_LABELPIECE_LABEL, (DWORD)tmp);
    _c(textPiece)->setProperty(textPiece, NCSP_TEXTPIECE_TEXTCOLOR, (DWORD)0xFF0000FF);
    _c(textPiece)->setProperty(textPiece, NCSP_TEXTPIECE_TEXTSHADOWCOLOR, (DWORD)0xFF0000DD);
    _c(textPiece)->setProperty(textPiece, NCSP_TEXTPIECE_LOGFONT, (DWORD)createLogFont(30));
    _c(textPiece)->setProperty(textPiece, NCSP_LABELPIECE_AUTOWRAP, (DWORD)TRUE);

    return TRUE;
}

static BOOL _mymain_onCreateTime(mMainWnd *self, DWORD dwAddData)
{
    mContainerCtrl *ctnr;
    mTextPiece *textPiece;
    RECT rc;

    GetClientRect(self->hwnd, &rc);
    ctnr = (mContainerCtrl *)ncsCreateWindow(NCSCTRL_CONTAINERCTRL,
                                             "time",
                                             WS_VISIBLE, 0, 0,
                                             HEADER_LOGO_WIDTH, 2, RECTW(rc) - HEADER_LOGO_WIDTH, HEADER_LOGO_HEIGHT - 2 * HEADER_LOGO_MARGIN,
                                             self->hwnd,
                                             NULL, NULL, NULL, 0);
#if HIDE_CURSOR
    ShowCursor(FALSE);
#endif

    mPanelPiece *view = (mPanelPiece *)NEWPIECE(mPanelPiece);
    textPiece = (mTextPiece *)NEWPIECE(mTextPiece);
    _c(ctnr)->setBody(ctnr, (mHotPiece *)view);

    rc.top = rc.left = 0;
    rc.right = RECTW(rc) - HEADER_LOGO_WIDTH;
    rc.bottom = HEADER_LOGO_HEIGHT - 2 * HEADER_LOGO_MARGIN;
    _c(textPiece)->setRect(textPiece, &rc);
    _c(textPiece)->setProperty(textPiece, NCSP_LABELPIECE_LABEL, (DWORD) "--:--:--");
    _c(textPiece)->setProperty(textPiece, NCSP_TEXTPIECE_TEXTCOLOR, (DWORD)0xFF0000FF);
    _c(textPiece)->setProperty(textPiece, NCSP_LABELPIECE_ALIGN, (DWORD)(NCS_ALIGN_RIGHT));
    _c(textPiece)->setProperty(textPiece, NCSP_TEXTPIECE_TEXTSHADOWCOLOR, (DWORD)0xFF0000DD);
    _c(textPiece)->setProperty(textPiece, NCSP_TEXTPIECE_LOGFONT, (DWORD)createLogFont(30));
    _c(textPiece)->setProperty(textPiece, NCSP_LABELPIECE_AUTOWRAP, (DWORD)TRUE);

    _c(view)->setRect(view, &rc);
    _c(view)->addContent(view, (mHotPiece*)textPiece, 0, 0);

    SetTimer((mWidget *)ctnr->hwnd, 100, 100);
    ncsSetComponentHandlers((mComponent *)ctnr, g_timerhandles, -1);
    return TRUE;
}


#if 0
static BOOL mymain_onCreateCalendar(mMainWnd *self, DWORD dwAddData)
{
    mContainerCtrl *ctnr;
    mTextPiece *textPiece;
    RECT rc;

    GetClientRect(self->hwnd, &rc);
    ctnr = (mContainerCtrl *)ncsCreateWindow(NCSCTRL_MONTHCALENDAR,
            "calendar",
            WS_VISIBLE, 0, 0,
            RECTW(rc) / 2, HEADER_LOGO_HEIGHT, RECTW(rc) / 2, RECTH(rc) - HEADER_LOGO_HEIGHT - (RECTH(rc) >> 2),
            self->hwnd,
            NULL, NULL, NULL, 0);
    return TRUE;

}
#endif
static BOOL mymain_onCreateCalendar(mMainWnd *self, DWORD dwAddData)
{
    mContainerCtrl *ctnr;
    mTextPiece *textPiece;
    RECT rc;

    GetClientRect(self->hwnd, &rc);
    SetWindowFont(self->hwnd,createLogFont(30));
    ctnr = (mContainerCtrl *)ncsCreateWindow(NCSCTRL_MONTHCALENDAR,
            "calendar",
            WS_VISIBLE, WS_EX_USEPARENTFONT, 0,
            RECTW(rc) / 2, HEADER_LOGO_HEIGHT, RECTW(rc) / 2, RECTH(rc) - HEADER_LOGO_HEIGHT - (RECTH(rc) >> 2),
            self->hwnd,
            NULL, NULL, NULL, 0);
#if HIDE_CURSOR
    ShowCursor(FALSE);
#endif
    return TRUE;

}

static BOOL mymain_onCreate(mMainWnd *self, DWORD dwAddData)
{
    BOOL ret = FALSE;
    ret = _mymain_onCreate_naviBar(self, dwAddData);
    ret = _mymain_onCreate_Header(self, dwAddData);
    ret = _mymain_onCreateBriefInfo(self, dwAddData);
    ret = _mymain_onCreateTime(self, dwAddData);
    ret = mymain_onCreateCalendar(self, dwAddData);
#if HIDE_CURSOR
    ShowCursor(FALSE);
#endif
    return ret;
}

static BOOL mymain_onClose(mWidget *self, int message)
{
    free(bmp);
    DestroyMainWindow(self->hwnd);
    hMainWnd = HWND_INVALID;
    return TRUE;
}

static BOOL my_main_hittest(mWidget *self, int message)
{
    POINT pt = {0, 0};
    GetCursorPos(&pt);

    printf("hit pos: (%d, %d)\n", pt.x, pt.y);
    return TRUE;
}

static BOOL doorcallme(mWidget *self, int message)
{
    CalledMainWindow(self->hwnd);
    return TRUE;
}

static NCS_EVENT_HANDLER mymain_handlers[] =
{
    {MSG_CREATE, (void *)mymain_onCreate},
    {MSG_CLOSE, (void *)mymain_onClose},
    {MSG_LBUTTONDOWN, (void *)SpeedMeterMessageHandler},
    {MSG_LBUTTONUP, (void *)SpeedMeterMessageHandler},
    {MSG_MOUSEMOVE, (void *)SpeedMeterMessageHandler},
    {MSG_USER_DOORCALLME, (void *)doorcallme},
    {0, NULL}
};

int ST_CreateMainWindow_New(int s32DispW, int s32DispH)
{
    MSG Msg;
    mWidget *mymain;

    g_s32UiMainWnd_W = s32DispW;
    g_s32UiMainWnd_H = s32DispH;
    if (0 != LoadProjectPicture())
    {
        printf("open res file fail\n");
        return -1;
    }
    logfont = CreateLogFont (NULL, "song", "GB2312",
                FONT_WEIGHT_BLACK, FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
                FONT_SPACING_CHARCELL, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE,
                24, 0);
    logfontgb12 = CreateLogFont (NULL, "song", "GB2312",
                FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
                FONT_SPACING_CHARCELL, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE,
                12, 0);
    logfontbig24 = CreateLogFont (NULL, "ming", "BIG5",
                FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_SETWIDTH_NORMAL,
                FONT_SPACING_CHARCELL, FONT_UNDERLINE_LINE, FONT_STRUCKOUT_NONE,
                24, 0);

    mymain = ncsCreateMainWindow(NCSCTRL_MAINWND, "home",
                                 WS_NONE | WS_VISIBLE,
                                 WS_EX_AUTOSECONDARYDC,
                                 1,
                                 0, 0, g_s32UiMainWnd_W, g_s32UiMainWnd_H,
                                 HWND_DESKTOP,
                                 0, 0,
                                 NULL,
                                 NULL,
                                 mymain_handlers,
                                 0);
    hMainWnd = mymain->hwnd;
    mGEffInit();
    sem_init(&g_toUI_sem,0,0);
    g_run = TRUE;
    g_tid_msg = pthread_create(&g_tid_msg, NULL, msg_toUIcmd_process, NULL);
    SetMemDCColorKey((HDC)HDC_SCREEN, MEMDC_FLAG_SRCCOLORKEY, MakeRGBA(0x80, 0x80, 0x80, 0x80));

    while (GetMessage (&Msg, mymain->hwnd)) {
        TranslateMessage (&Msg);
        DispatchMessage (&Msg);
    }

    MainWindowThreadCleanup (mymain->hwnd);

    return 0;
}

int ST_InitMiniGui(int argc, const char **args)
{
    if (InitGUI (argc ,args) != 0) {
        return 1;
    }
#ifdef _MGRM_PROCESSES
    if (!ServerStartup (0 , 0 , 0)) {
        fprintf (stderr,
                 "Can not start the server of MiniGUI-Processes: mginit.\n");
        return 1;
    }
#endif
    ncsInitialize();
    ncs4TouchInitialize();

    INIT_LIST_HEAD(&g_btnProcListHead);
    INIT_LIST_HEAD(&g_trackBarProcListHead);

    return 0;
}

int ST_DeinitMiniGui(int not_used)
{
    CLEAR_TRACKBAR_OLD_PROC_LIST(g_trackBarProcListHead);
    CLEAR_BUTTON_OLD_PROC_LIST(g_btnProcListHead);
    mGEffDeinit();

    ncs4TouchUninitialize();
    ncsUninitialize();

    TerminateGUI (not_used);//deinit GUI

    return 0;
}

