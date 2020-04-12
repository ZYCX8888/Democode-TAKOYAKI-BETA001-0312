


#include "mid_ipu_hc.h"
#include "mid_system_config.h"
#include "mi_sys.h"
#include "mi_venc.h"
#include "mi_divp.h"
#include "mi_vpe.h"

#include "mid_dla.h"

#include "mid_utils.h"
#include "mid_divp.h"
#include <math.h>

#define VPE_ORIGINAL_CROP_X 960
#define VPE_ORIGINAL_CROP_Y 540
#define VPE_ORIGINAL_CROP_W 1920
#define VPE_ORIGINAL_CROP_H 1080

extern MI_U32 g_rotation;

extern pthread_mutex_t g_mutex_UpadteOsdState;
extern pthread_cond_t  g_cond_UpadteOsdState;
extern MI_U32 g_UpadteOsdState;

struct _ImageSize
{
    MI_U16  ImageW;
    MI_U16  ImageH;
};
enum
{
    Res1280x720 = 0x0,
    Res1920x1080,
    Res2304x1296,
    Res2560x1440,
    Res2688x1512,
    Res3072x1728,
    Res3840x2160,
    ResMax,
};
typedef enum _CropState
{
    WaitForTargetSize = 0x0,
    Action,
} m_CropState;

const struct _ImageSize m_ResIndex[] = {{1280, 720},\
    {1920, 1080},\
    {2304, 1296},\
    {2560, 1440},\
    {2688, 1512},\
    {3072, 1728},\
    {3840, 2160}
};

extern int DLAtoRECT(MI_VENC_CHN s32VencChn, int recCnt, ST_DlaRectInfo_T* pRecInfo, MI_BOOL bShow, MI_BOOL bShowBorder);

MI_U16 volatile CHcRectAlgo::rCount = 0x0;
MI_U16 volatile CHcRectAlgo::wCount = 0x0;
HcRect_t *CHcRectAlgo::m_dataRects= NULL;
CHcRectAlgo::CHcRectAlgo()
{
    Init();
}

CHcRectAlgo::~CHcRectAlgo()
{
    UnInit();
}

MI_U16 CHcRectAlgo::Init()
{

    if(NULL == m_dataRects)
    {
        m_dataRects = new HcRect_t[HcRectAlgoArray];
        if(NULL == m_dataRects)
        {
            MIXER_ERR("can not alloc m_dataRects.\n");
            return -1;
        }
    }

    return 0;
}

MI_U16 CHcRectAlgo::UnInit()
{
    if(NULL != m_dataRects)
    {
        delete m_dataRects;
        m_dataRects = NULL;
    }

    return 0x0;
}

MI_U16 CHcRectAlgo::PushRect(const HcRect_t &mHcRectSize)
{
    if(NULL == m_dataRects)
    {
        MIXER_ERR("m_dataRects is null.\n");
        return -1;
    }

    HcRectLock.OnLock();

    while((wCount - rCount) >= HcRectAlgoArray)
    {
        rCount ++;
    }

    m_dataRects[wCount%HcRectAlgoArray] = mHcRectSize;
    wCount ++;

    HcRectLock.OnUnLock();

    return 0x0;
}

MI_U16 CHcRectAlgo::GetAlgoRect(HcRect_t &mHcRectSize)
{
    HcRect_t *CurRect = NULL, *tCurRect = NULL, tm_DataRects[HcRectAlgoArray];

    MI_U16 i = 0x0, j = 0x0;
    MI_S16 tmpx = 0x0, tmpy=0x0, tmpw= 0x0, tmph = 0x0;
    MI_U32 tSize[HcRectAlgoArray][4] = {{0x0,},};
    MI_S16 sCount = 0x0, tCount = 0x0, endCount = 0x0;
    MI_U16 sRet = 0x0;
    MI_U32 avgX=0x0, avgY=0x0, avgW=0x0, avgH=0x0;
    MI_U32 bitUpCount = 0x0;

    HcRectLock.OnLock();

    MI_U16 cSize = wCount - rCount;

    if(cSize < 4)
    {
        HcRectLock.OnUnLock();
        return 0x0;
    }
    for(i = 0; (i < cSize) && i < HcRectAlgoArray; i++)
    {
      if(NULL != (m_dataRects+rCount%HcRectAlgoArray))
      {
        tm_DataRects[i] = m_dataRects[rCount%HcRectAlgoArray];
        rCount++;
      }

    }
    HcRectLock.OnUnLock();

    for(i = 0; i < cSize-1; i++)
    {
         CurRect = &tm_DataRects[i];
        tCurRect = &tm_DataRects[i+1];

         tmpx = _abs(tCurRect->x -CurRect->x);
         tmpy = _abs(tCurRect->y -CurRect->y);
         tmpw = _abs(tCurRect->width -CurRect->width);
         tmph = _abs(tCurRect->height -CurRect->height);
        //MIXER_DBG("CurRect X:%d, Y:%d, W:%d, H:%d\n.", CurRect->x, CurRect->y, CurRect->width, CurRect->height);
        #if 0
         tSize[i][0] = tmpx * tmpx;
         tSize[i][1] = tmpy * tmpy;
         tSize[i][2] = tmpw * tmpw;
         tSize[i][3] = tmph * tmph;
        #else
        tSize[i][0] = tmpx ;
         tSize[i][1] = tmpy ;
         tSize[i][2] = tmpw;
         tSize[i][3] = tmph ;
        #endif
        //MIXER_DBG("tmpx:%d tmpy:%d tmpw:%d tmph:%d\n", tmpx, tmpy, tmpw, tmph);

        if(tSize[i][0] <= m_CurXWThreshold &&\
                   tSize[i][1] <= m_CurYHThreshold &&\
                   tSize[i][2] <= m_CurXWThreshold && \
                   tSize[i][3] <= m_CurYHThreshold)
        {
            bitUpCount &= ~(0x01 << i);
        }
        else
        {
            bitUpCount |= (0x01 << i);
        }
    }

    j=(cSize-1);
    do{
        if(0x0 == (bitUpCount & (0x1<<(j-1))))
            sCount++;
        else
        {
            if(sCount>=3)
            {
                break;
            }
            else
            {
                sCount = 0x0;
            }
        }
        j--;
    }while(j>0);

    if(sCount>=3)
    {
        endCount = sCount + j;
        j+=1;
        for(; j<=endCount; j++)
        {
            tCurRect = &tm_DataRects[j];
            tCount++;
            avgX += tCurRect->x;
            avgY += tCurRect->y;
            avgW += tCurRect->width;
            avgH += tCurRect->height;
        }

        mHcRectSize.x = avgX/tCount;
        mHcRectSize.y = avgY/tCount;
        mHcRectSize.width = avgW/tCount;
        mHcRectSize.height = avgH/tCount;
        sRet = sCount;
    }
    MIXER_DBG("X:%d, Y:%d, w:%d, h:%d\n.", mHcRectSize.x, mHcRectSize.y, mHcRectSize.width, mHcRectSize.height);

    return 1;
}


static inline MI_U16 GetMindex(MI_U16 w, MI_U16 h)
{

    MI_U16 tIndex = 0x0;
#if 0
    MI_U32 tSize = w * h;

    if(tSize >= m_ResIndex[Res3840x2160].ImageW * m_ResIndex[Res3840x2160].ImageH)
    {
        tIndex = Res3840x2160;
    }
    else if(tSize > m_ResIndex[Res3072x2160].ImageW * m_ResIndex[Res3072x2160].ImageH)
    {
        tIndex = Res3840x2160;
    }
    else if(tSize> m_ResIndex[Res2688x2160].ImageW * m_ResIndex[Res2688x2160].ImageH)
    {
        tIndex = Res3072x2160;
    }
    else if(tSize > m_ResIndex[Res2688x1520].ImageW * m_ResIndex[Res2688x1520].ImageH)
    {
        tIndex = Res2688x2160;
    }
    else if(tSize > m_ResIndex[Res2560x1440].ImageW * m_ResIndex[Res2560x1440].ImageH)
    {
        tIndex = Res2688x1520;
    }
    else if(tSize > m_ResIndex[Res2048x1520].ImageW * m_ResIndex[Res2048x1520].ImageH)
    {
        tIndex = Res2560x1440;
    }
    else if(tSize > m_ResIndex[Res1920x1080].ImageW * m_ResIndex[Res1920x1080].ImageH)
    {
        tIndex = Res2048x1520;
    }
    else if(tSize > m_ResIndex[Res1280x720].ImageW * m_ResIndex[Res1280x720].ImageH)
    {
        tIndex = Res1920x1080;
    }
    else
    {
        tIndex = Res1280x720;
    }
#else

    if(w >= m_ResIndex[Res3840x2160].ImageW || h >= m_ResIndex[Res3840x2160].ImageH)
    {
        tIndex = Res3840x2160;
    }
    else if(w >= m_ResIndex[Res3072x1728].ImageW || h >=  m_ResIndex[Res3072x1728].ImageH)
    {
        tIndex = Res3840x2160;
    }
    else if(w >= m_ResIndex[Res2688x1512].ImageW || h >= m_ResIndex[Res2688x1512].ImageH)
    {
        tIndex = Res3072x1728;
    }
    else if(w >= m_ResIndex[Res2560x1440].ImageW || h >= m_ResIndex[Res2560x1440].ImageH)
    {
        tIndex = Res2688x1512;
    }
    else if(w >= m_ResIndex[Res2304x1296].ImageW || h >= m_ResIndex[Res2304x1296].ImageH)
    {
        tIndex = Res2560x1440;
    }
    else if(w >= m_ResIndex[Res1920x1080].ImageW || h >= m_ResIndex[Res1920x1080].ImageH)
    {
        tIndex = Res2304x1296;
    }
    else if(w >= m_ResIndex[Res1280x720].ImageW || h >= m_ResIndex[Res1280x720].ImageH)
    {
        tIndex = Res1920x1080;
    }
    else
    {
        tIndex = Res1280x720;
    }
#endif

    return tIndex;
}


void *Hc2Vpe_Task(void *argc)
{
    if(NULL == argc)
    {
        MIXER_ERR("param err.\n");
        return NULL;
    }

    CMidIPUHc *pIpuHc = (CMidIPUHc *)argc;

    pIpuHc->Hc2Vpe_Task1();

    return NULL;
}

CMidIPUHc::CMidIPUHc(IPU_InitInfo_S &stInitInfo): CMidIPUInterface(stInitInfo)
{
    memset(&m_stHcModel, 0, sizeof(Model_Info_S));
    m_stHcModel.vpeChn          = 0;
    m_stHcModel.vpePort         = 2;
    m_stHcModel.divpChn         = Mixer_Divp_GetChannleNum();
    m_stHcModel.ipuChn          = 0;
    m_stHcModel.u32InBufDepth   = 1;
    m_stHcModel.u32OutBufDepth  = 1;
    m_stHcModel.s32Fd           = -1;
    m_stHcModel.pModelFile      = m_stIPUInfo.szModelFile;
    m_stHcModel.u32Width        = 640;
    m_stHcModel.u32Height       = 360;

    m_s32MaxFd = -1;

    InitResource();
}

CMidIPUHc::~CMidIPUHc()
{
	ReleaseStreamSource(m_stHcModel);
	ReleaseResource();
	//g_UpadteOsdState = TRUE;
    Mixer_Divp_PutChannleNum(m_stHcModel.divpChn);
}

void CMidIPUHc::DoCalcRectPoint(const HcRect_t &iTargetRectSize, \
                                     const MI_SYS_WindowRect_t &mCurRectSize, \
                                     MI_SYS_WindowRect_t &mResultRect)
{

    MI_S32 abstmpx = 0x0, abstmpy = 0x0, abstmpw = 0x0, abstmph = 0x0;
    MI_U16 setupbasex = 0x0, setupy = 0x0, setubasepw = 0x0, setuph = 0x0;
    MI_U16 ix = 0x0, iy = 0x0, iw = 0x0, ih = 0x0, ic = 0x0;
    MI_S32 tmp = 0x0;
    static MI_U8 iCount = 0x0, CurCount = 0x0;
    static MI_SYS_WindowRect_t iSetup[SETUPCOUNT];
    static HcRect_t mTargetRectSize;
    static MI_S32 tmpx = 0x0, tmpy = 0x0, tmpw = 0x0, tmph = 0x0;
    MI_U8 i = 0x0, j = 0x0, jj = 0x0;

    memset(&mResultRect, 0x0, sizeof(mResultRect));

    if(0x0 == iCount)
    {
        CurCount = 0x0;
        memset(iSetup, 0x0, sizeof(iSetup));
        tmpx = (iTargetRectSize.x - (MI_S16)mCurRectSize.u16X);
        tmpy = (iTargetRectSize.y - (MI_S16)mCurRectSize.u16Y);
        tmpw = ((MI_S32)iTargetRectSize.width - (MI_S32)mCurRectSize.u16Width);
        tmph = ((MI_S32)iTargetRectSize.height - (MI_S32)mCurRectSize.u16Height);

        mTargetRectSize = iTargetRectSize;

        #if 0
        MIXER_DBG("cx:%d, cy:%d, cw:%d, ch:%d. tx:%d, ty:%d, tw:%d, th:%d\n", \
                        iCurRectSize.u16X, iCurRectSize.u16Y, iCurRectSize.u16Width, iCurRectSize.u16Height,\
                        iTargetRectSize.x, iTargetRectSize.y, iTargetRectSize.width, iTargetRectSize.height);
        #endif
        abstmpx = (_abs(tmpx) >= 160) ? 160 : _abs(tmpx);
        abstmpy = (_abs(tmpy) >= 80) ? 80 : _abs(tmpy);
        abstmpw = ALIGN_8xDOWN((_abs(tmpw) >= 160) ? 160 : _abs(tmpw));
        abstmph = (_abs(tmph) >= 80) ? 80 : _abs(tmph);

        setupbasex = ((abstmpx)/8);
        setupy = ((abstmpy+9)/SETUPCOUNT);
        setubasepw = ((abstmpw)/8);
        setuph = ((abstmph+9)/SETUPCOUNT);

        for(i=0x0; i<SETUPCOUNT; i++)
        {
            if( ix < abstmpx)
            {
            #if 0
                if(setupx % 2)
                {
                    iSetup[i].u16X = i%2 ? (setupx-1) : (setupx+1);
                }
                else
                {
                    iSetup[i].u16X = setupx;
                }
            #else
                iSetup[i].u16X = 8;
                if( setupbasex >= SETUPCOUNT)
                {
                    if((setupbasex - SETUPCOUNT) > jj)
                    {
                        jj++;
                        iSetup[i].u16X = 8 * 2;
                    }
                }
            #endif
                ix += iSetup[i].u16X;
            }
            else
            {
                ic |= 0x01;
            }

            if( iy < abstmpy)
            {
                if(setupy % 2)
                {
                    iSetup[i].u16Y = i%2 ? (setupy-1) : (setupy+1);
                }
                else
                {
                    iSetup[i].u16Y = setupy;
                }
                iy += iSetup[i].u16Y;
            }
            else
            {
                ic |= 0x02;
            }

            if(iw < abstmpw)
            {
                iSetup[i].u16Width = 8;
                if( setubasepw >= SETUPCOUNT)
                {
                    if((setubasepw - SETUPCOUNT) > j)
                    {
                        j++;
                        iSetup[i].u16Width = 8 * 2;
                    }
                }

                iw += iSetup[i].u16Width;
            }
            else
            {
                ic |= 0x04;
            }


            if(ih < abstmph)
            {
                if(setuph % 2)
                {
                    iSetup[i].u16Height = i%2 ? (setuph-1) : (setuph+1);
                }
                else
                {
                    iSetup[i].u16Height = setuph;
                }

                ih += iSetup[i].u16Height;
            }
            else
            {
                ic |= 0x08;
            }

            iCount++;
            if(ic == 0x0f)
                break;
        }
    }

    //MIXER_DBG("iCount:%d, CurCount:%d, u16X:%d, tmpx:%d.\n", iCount, CurCount, iSetup[CurCount].u16X, tmpx);
    if(iCount > 0x0)
    {
        if(tmpx > 0)
        {
            mResultRect.u16X = mCurRectSize.u16X + iSetup[CurCount].u16X;
            if(mResultRect.u16X >= mTargetRectSize.x)
               mResultRect.u16X = mTargetRectSize.x;
        }
        else if(tmpx < 0)
        {
            tmp = (MI_S32)(mCurRectSize.u16X - iSetup[CurCount].u16X);
            if(tmp < 0)
               tmp = 0x0;

            mResultRect.u16X = tmp;

            if(mResultRect.u16X <= mTargetRectSize.x)
               mResultRect.u16X = mTargetRectSize.x;
        }
        else
        {
            mResultRect.u16X = mCurRectSize.u16X;
        }

        if(tmpw < 0)
        {
            tmp = (MI_S32)(mCurRectSize.u16Width - iSetup[CurCount].u16Width);
            if(tmp < 0)
               tmp = 0x0;
            mResultRect.u16Width = tmp;
            if(mResultRect.u16Width <= mTargetRectSize.width)
               mResultRect.u16Width = mTargetRectSize.width;
        }
        else if(tmpw > 0)
        {
            mResultRect.u16Width = mCurRectSize.u16Width + iSetup[CurCount].u16Width;
            if(mResultRect.u16Width >= mTargetRectSize.width)
               mResultRect.u16Width = mTargetRectSize.width;
        }
        else
        {
            mResultRect.u16Width = mCurRectSize.u16Width;
        }

        if(tmpy > 0)
        {
            mResultRect.u16Y = mCurRectSize.u16Y + iSetup[CurCount].u16Y;
            if(mResultRect.u16Y >= mTargetRectSize.y)
               mResultRect.u16Y = mTargetRectSize.y;
        }
        else if(tmpy < 0)
        {
            tmp = (MI_S32)mCurRectSize.u16Y - iSetup[CurCount].u16Y;
            if(tmp < 0)
               tmp = 0x0;

            mResultRect.u16Y = tmp;

            if(mResultRect.u16Y <= mTargetRectSize.y)
               mResultRect.u16Y = mTargetRectSize.y;
        }
        else
        {
            mResultRect.u16Y = mCurRectSize.u16Y;
        }

        if(tmph < 0)  //crop-> down
        {
            tmp = (MI_S32)(mCurRectSize.u16Height - iSetup[CurCount].u16Height);
            if(tmp < 0)
               tmp = 0x0;
            mResultRect.u16Height = tmp;

            if(mResultRect.u16Height <= mTargetRectSize.height)
                mResultRect.u16Height = mTargetRectSize.height;

        }
        else if(tmph > 0) //crop -> up
        {
            mResultRect.u16Height = mCurRectSize.u16Height + iSetup[CurCount].u16Height;
            if(mResultRect.u16Height >= mTargetRectSize.height)
                mResultRect.u16Height = mTargetRectSize.height;
        }
        else
        {
            mResultRect.u16Height = mCurRectSize.u16Height;
        }
        CurCount++;
        iCount--;

    }
    #if 0
    MIXER_DBG("rx:%d, ry:%d, rw:%d, rh:%d.\n", \
                        mResultRect.u16X, mResultRect.u16Y, mResultRect.u16Width, mResultRect.u16Height);
    #endif
}
#if 0
void CMidIPUHc::Hc2Vpe_Task1()
{
    HcRect_t tRectSize;

    MI_SYS_WindowRect_t stVpePortCropWinGet;
    MI_SYS_WindowRect_t tWin;

    MI_BOOL tick = 0x0;
    MI_U8 ntick = 0x0, nCick = 0x0;
    int sRet = 0x0;


    m_CropState tm_CropState = WaitForTargetSize;
    memset(&tRectSize, 0x0, sizeof(tRectSize));

    //get vpe channel crop info
    if(MI_VPE_OK != MI_VPE_GetChannelCrop(0, &stVpePortCropWinGet))
    {
        MIXER_DBG("can not get vpe channel crop info.\n");
        return ;
    }
    MIXER_DBG("cropX:%d, cropY:%d. cropW:%d, cropH:%d.\n", stVpePortCropWinGet.u16X,  \
              stVpePortCropWinGet.u16Y,  stVpePortCropWinGet.u16Width,  stVpePortCropWinGet.u16Height);

    while(TRUE == m_bThread)
    {
        MySystemDelay(31);
        switch(tm_CropState)
        {
            case WaitForTargetSize:

                tm_CropState = Action;
                break;

            case Action:
            {
                //get vpe port crop info
                if(tick)
                {
                    if(MI_VPE_OK != MI_VPE_GetPortCrop(0, 0, &stVpePortCropWinGet))
                    {
                        MIXER_DBG("can not get vpe port crop info.\n");
                        continue;
                    }
                }

                if(++ntick > 10)
                {
                   ntick = 0x0;
                   sRet = GetTargetSize(tRectSize);
                   if(sRet < 0)
                   {
                       //MIXER_DBG("can not get vpe port crop target size.\n");
                       continue;
                   }
                   else
                   {
                       nCick = 0x01;
                   }
                }

               if(nCick == 0x0)
                   continue;

               DoCalcRectPoint(tRectSize, stVpePortCropWinGet, tWin);

               if(0 != memcmp(&tWin, &stVpePortCropWinGet, sizeof(MI_SYS_WindowRect_t)))
               {
                   if(MI_VPE_OK != MI_VPE_SetPortCrop(0, 0, &tWin))
                   {
                       MIXER_DBG("can not set vpe port crop info.\n");
                       continue;
                   }
                   tick = 1;
               }

            }
            break;
        }
    }

}
#endif

BOOL IsPositionNeedToUpdate(HcRect_t CurHcPosition, HcRect_t NewHcPosition)
{
    #define CHANGE_THRESHOLD 128

    MI_U16 u16XDel = _abs(CurHcPosition.x - NewHcPosition.x);
    MI_U16 u16YDel = _abs(CurHcPosition.y - NewHcPosition.y);
    MI_U16 u16WDel = _abs(CurHcPosition.width - NewHcPosition.width);
    MI_U16 u16HDel = _abs(CurHcPosition.height - NewHcPosition.height);

    //MIXER_DBG("u16XDel:%d u16YDel:%d u16Wel:%d, u16HDel:%d\n", u16XDel, u16YDel, u16WDel, u16HDel);

    //Todo refine
    if((NewHcPosition.width < 64) || (NewHcPosition.width < 32))
    {
        return FALSE;
    }

    if((u16XDel > CHANGE_THRESHOLD) || (u16YDel > CHANGE_THRESHOLD) || (u16WDel > CHANGE_THRESHOLD) || (u16HDel > CHANGE_THRESHOLD))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void ConvertTo16v9Rect(HcRect_t OriRect, HcRect_t &ConvertedRect)
{
    MI_U16 u16wDif = 0;
    MI_U16 u16ModW = OriRect.width % 16;
    if(u16ModW)
    {
        u16wDif = 16 -  u16ModW;
    }
    else
    {
        u16wDif = 0;
    }

    MI_U16 u16hDif = 0;
    MI_U16 u16ModH = OriRect.height % 9;
    if(u16ModH)
    {
        u16hDif = 9 - u16ModH;
    }
    else
    {
        u16hDif = 0;
    }

    //宽可以被16整除，高可以被9整除
    MI_U16 u16DividedWidth = OriRect.width + u16wDif;
    MI_U16 u16DividedHeight = OriRect.height + u16hDif;


    //MIXER_DBG("u16wDif:%d u16hDif:%d u16DividedWidth:%d u16DividedHeight:%d\n", u16wDif, u16hDif, u16DividedWidth, u16DividedHeight);

    MI_U16 u16WidthExtendLendth = 0;
    MI_U16 u16HeightExtendLendth = 0;

    if((u16DividedWidth >= 1920) || (u16DividedHeight >= 1080))
    {
        BOOL bExtendWidth = FALSE;
        MI_U16 u16ExtendLength = 0;
        //计算应该延长宽还是高 以达到16：9的比例
        bExtendWidth = (u16DividedWidth * 9) < (16 * u16DividedHeight) ? TRUE:FALSE;

        //MIXER_DBG("bExtendWidth:%d\n", bExtendWidth);
        if(bExtendWidth) //extend width
        {
            u16ExtendLength = (u16DividedHeight*16)/9 - u16DividedWidth;

            u16WidthExtendLendth = u16wDif + u16ExtendLength;
            u16HeightExtendLendth = u16hDif;
        }
        else //extend height
        {
            u16ExtendLength = (u16DividedWidth*9)/16 - u16DividedHeight;

            u16HeightExtendLendth = u16hDif + u16ExtendLength;
            u16WidthExtendLendth = u16wDif;
        }

    }
    else //宽和高最小要为1920*1080，如果不是则要扩大到1080P
    {
        if(u16DividedWidth < 1920)
        {
            u16WidthExtendLendth = 1920 - OriRect.width;
        }
        else
        {
            u16WidthExtendLendth = 0;
        }

        if(u16DividedHeight < 1080)
        {
            u16HeightExtendLendth = 1080 - OriRect.height;
        }
        else
        {
            u16HeightExtendLendth = 0;
        }
    }

    //MIXER_DBG("u16WidthExtendLendth:%d u16HeightExtendLendth:%d\n", u16WidthExtendLendth, u16HeightExtendLendth);

    int TempX = 0;
    int TempY = 0;
    int TempW = 0;
    int TempH = 0;

    TempW = OriRect.width + u16WidthExtendLendth;
    TempH = OriRect.height + u16HeightExtendLendth;

    //考虑四个角出界的情况以及越界的情况
    TempX = OriRect.x - u16WidthExtendLendth/2;

    if((TempX + TempW) > 3840)
    {
        TempX = TempX - (TempX + TempW - 3840);
    }

    if(TempX < 0)
    {
        TempX = 0;
    }

    TempY = OriRect.y - u16HeightExtendLendth/2;

    if((TempY + TempH) > 2160)
    {
        TempY = TempY - (TempY + TempH - 2160);
    }

    if(TempY < 0)
    {
        TempY = 0;
    }

    if(((TempY + TempH) > 2160) || ((TempX + TempW) > 3840))
    {
        MIXER_ERR("error: x:%d y:%d w:%d h:%d\n", TempX, TempY, TempW, TempH);
    }

    MIXER_DBG("OriRect rect: x:%d y:%d w:%d h:%d\n", OriRect.x, OriRect.y, OriRect.width, OriRect.height);
    MIXER_DBG("converted rect: x:%d y:%d w:%d h:%d\n", TempX, TempY, TempW, TempH);
    ConvertedRect.x = TempX;
    ConvertedRect.y = TempY;
    ConvertedRect.width = TempW;
    ConvertedRect.height = TempH;

}

int CMidIPUHc::GetVpeCropRect(MI_SYS_WindowRect_t &stVpeTargetCrop)
{
    HcRect_t NewHcPosition;
    memset(&NewHcPosition, 0, sizeof(NewHcPosition));

    MI_U16 u16Ret = 0;

    u16Ret = m_dataRects.GetAlgoRect(NewHcPosition);
    if(u16Ret == 0)
    {
        return -1;
    }

    //MIXER_DBG("NewHcPosition rect: x:%d y:%d w:%d h:%d\n", NewHcPosition.x, NewHcPosition.y, NewHcPosition.width, NewHcPosition.height);

    if(FALSE == IsPositionNeedToUpdate(mCurHcPosition, NewHcPosition))
    {
        return 0;
    }

    mCurHcPosition = NewHcPosition;


    HcRect_t ConvertedRect;
    memset(&ConvertedRect, 0, sizeof(HcRect_t));

    ConvertTo16v9Rect(NewHcPosition, ConvertedRect);

    stVpeTargetCrop.u16X = ConvertedRect.x;
    stVpeTargetCrop.u16Y = ConvertedRect.y;
    stVpeTargetCrop.u16Width = ConvertedRect.width;
    stVpeTargetCrop.u16Height = ConvertedRect.height;

    return 1;
}
#if 1
void CMidIPUHc::Hc2Vpe_Task1()
{
    MI_SYS_WindowRect_t stVpeTargetCrop;
    MI_SYS_WindowRect_t stVpeToCrop;
    memset(&stVpeToCrop, 0, sizeof(MI_SYS_WindowRect_t));
    m_CropState State = WaitForTargetSize;

    #define VPE_CROP_STEPS 30
    #define VPE_CROP_STEP_THRESHOLD 128
    #define NOBODY_CHEK_COUNTS 150
    int XStep = 0;
    int YStep = 0;
    int WStep = 0;
    int HStep = 0;

    int XChange = 0;
    int YChange = 0;
    int WChange = 0;
    int HChange = 0;


    float fCropSteps = 0;
    BOOL bExistHuman = FALSE;
    MI_U8 u16NobodyCount = 0;//该值用于避免当有人存在的时候，GetVpeCropRect也会返回-1的情况
    BOOL bRecover = FALSE;

    MI_SYS_WindowRect_t stVpeCurCrop;

    stVpeCurCrop.u16X = VPE_ORIGINAL_CROP_X;
    stVpeCurCrop.u16Y = VPE_ORIGINAL_CROP_Y;
    stVpeCurCrop.u16Width = VPE_ORIGINAL_CROP_W;
    stVpeCurCrop.u16Height = VPE_ORIGINAL_CROP_H;

    MIXER_DBG("stVpeCurCrop:%d, y:%d, w:%d, h:%d\n", stVpeCurCrop.u16X, stVpeCurCrop.u16Y,
    stVpeCurCrop.u16Width, stVpeCurCrop.u16Height);

    MI_S32 s32Ret = 0;

    s32Ret = MI_VPE_SetPortCrop(0, 0, &stVpeCurCrop);
    if(s32Ret != MI_SUCCESS)
    {
        MIXER_ERR("MI_VPE_SetPortCrop failed\n");

    }

    while(TRUE == m_bThread)
    {
        switch(State)
        {
            case WaitForTargetSize:
            {
                memset(&stVpeTargetCrop, 0, sizeof(MI_SYS_WindowRect_t));
                s32Ret = GetVpeCropRect(stVpeTargetCrop);
                //MIXER_DBG("s32Ret:%d\n",s32Ret);
                if(s32Ret == 0)
                {
                    u16NobodyCount = 0;
                }
                else
                {
                    if(s32Ret < 0)
                    {
                        //MIXER_DBG("bExistHuman:%d u16NobodyCount:%d\n",bExistHuman,  u16NobodyCount);

                        if(bExistHuman && (u16NobodyCount > NOBODY_CHEK_COUNTS))
                        {
                            stVpeTargetCrop.u16X = VPE_ORIGINAL_CROP_X;
                            stVpeTargetCrop.u16Y = VPE_ORIGINAL_CROP_Y;
                            stVpeTargetCrop.u16Width = VPE_ORIGINAL_CROP_W;
                            stVpeTargetCrop.u16Height = VPE_ORIGINAL_CROP_H;

                            bExistHuman = FALSE;
                            bRecover = TRUE;
                            u16NobodyCount = 0;

                            MIXER_DBG("Revert to centre & 1080P!\n");
                        }
                        else if(bExistHuman)
                        {
                            u16NobodyCount++;
                            //MIXER_DBG("No one!\n");
                            break;
                        }
                    }
                    else
                    {
                        bExistHuman = TRUE;
                        u16NobodyCount = 0;
                        bRecover = FALSE;
                    }

                    if((s32Ret > 0) || bRecover)
                    {
                        bRecover = FALSE;


                        MIXER_DBG("Target crop x:%d, y:%d, w:%d, h:%d\n", stVpeTargetCrop.u16X,
                            stVpeTargetCrop.u16Y, stVpeTargetCrop.u16Width, stVpeTargetCrop.u16Height);

                        s32Ret = MI_VPE_GetPortCrop(0, 0, &stVpeCurCrop);
                        if(s32Ret != MI_SUCCESS)
                        {
                            MIXER_ERR("MI_VPE_GetPortCrop failed\n");

                        }

                        MIXER_DBG("Current Crop: x:%d, y:%d, w:%d, h:%d\n", stVpeCurCrop.u16X, stVpeCurCrop.u16Y,
                        stVpeCurCrop.u16Width, stVpeCurCrop.u16Height);

                        XChange = stVpeTargetCrop.u16X - stVpeCurCrop.u16X;
                        YChange = stVpeTargetCrop.u16Y - stVpeCurCrop.u16Y;
                        WChange = stVpeTargetCrop.u16Width - stVpeCurCrop.u16Width;
                        HChange = stVpeTargetCrop.u16Height - stVpeCurCrop.u16Height;

                        if((_abs(XChange) < VPE_CROP_STEP_THRESHOLD) && (_abs(YChange) < VPE_CROP_STEP_THRESHOLD) &&
                            (_abs(WChange) < VPE_CROP_STEP_THRESHOLD) && (_abs(HChange) < VPE_CROP_STEP_THRESHOLD))
                        {
                            MIXER_DBG("Change is too small! No need to crop\n");
                            break;
                        }

                        fCropSteps = VPE_CROP_STEPS;

                        if((_abs(XChange) >= 960) || (_abs(YChange) > 540) || (_abs(WChange) >= 960) || (_abs(HChange) > 540))
                        {
                            fCropSteps = fCropSteps * 1.2;
                        }

                        XStep = round(XChange / fCropSteps);
                        YStep = round(YChange / fCropSteps);
                        WStep = round(WChange / fCropSteps);
                        HStep = round(HChange / fCropSteps);

                        MIXER_DBG("Xstep:%d ystep:%d wstep:%d, hStep:%d\n", XStep, YStep, WStep, HStep);

                        stVpeToCrop = stVpeCurCrop;
                        //MIXER_DBG("stVpeCurCrop:%d, y:%d, w:%d, h:%d\n", stVpeCurCrop.u16X, stVpeCurCrop.u16Y,
                        //stVpeCurCrop.u16Width, stVpeCurCrop.u16Height);

                        State = Action;
                    }
                }
            }
            break;

            case Action:
            {

                if(fCropSteps)
                {
                    fCropSteps--;

                    if(XStep)
                    {
                        stVpeToCrop.u16X += XStep;
                        if(XStep > 0)
                        {
                            if(stVpeToCrop.u16X > stVpeTargetCrop.u16X)
                            {
                                stVpeToCrop.u16X = stVpeTargetCrop.u16X;

                                MIXER_DBG("x done!\n");
                            }

                        }
                        else
                        {
                            if((stVpeToCrop.u16X < stVpeTargetCrop.u16X) || (stVpeToCrop.u16X > 1920))
                            {
                                stVpeToCrop.u16X = stVpeTargetCrop.u16X;

                                MIXER_DBG("x done!\n");
                            }
                        }
                    }

                    if(YStep)
                    {
                        stVpeToCrop.u16Y += YStep;
                        //MIXER_DBG("stVpeToCrop.u16y:%d\n", stVpeToCrop.u16Y);
                        if(YStep > 0)
                        {
                            if((stVpeToCrop.u16Y > stVpeTargetCrop.u16Y))
                            {
                                stVpeToCrop.u16Y = stVpeTargetCrop.u16Y;

                                MIXER_DBG("y done!\n");
                            }
                        }
                        else
                        {
                            if((stVpeToCrop.u16Y < stVpeTargetCrop.u16Y) || (stVpeToCrop.u16Y > 1080))
                            {
                                stVpeToCrop.u16Y = stVpeTargetCrop.u16Y;

                                MIXER_DBG("y done!\n");
                            }
                        }
                    }

                    if(WStep)
                    {
                        stVpeToCrop.u16Width += WStep;
                        //MIXER_DBG("stVpeToCrop.u16w:%d\n", stVpeToCrop.u16Width);
                        if(WStep > 0)
                        {
                            if(stVpeToCrop.u16Width > stVpeTargetCrop.u16Width)
                            {
                                stVpeToCrop.u16Width = stVpeTargetCrop.u16Width;
                                MIXER_DBG("w done!\n");

                            }
                        }
                        else
                        {
                            if(stVpeToCrop.u16Width < stVpeTargetCrop.u16Width)
                            {
                                stVpeToCrop.u16Width = stVpeTargetCrop.u16Width;
                                MIXER_DBG("w done!\n");
                            }
                        }
                    }

                    if(HStep)
                    {
                        stVpeToCrop.u16Height += HStep;
                        //MIXER_DBG("stVpeToCrop.u16h:%d\n", stVpeToCrop.u16Height);
                        if(HStep > 0)
                        {
                            if(stVpeToCrop.u16Height > stVpeTargetCrop.u16Height)
                            {
                                stVpeToCrop.u16Height = stVpeTargetCrop.u16Height;
                                MIXER_DBG("h done!\n");
                            }
                        }
                        else
                        {
                            if(stVpeToCrop.u16Height < stVpeTargetCrop.u16Height)
                            {
                                stVpeToCrop.u16Height = stVpeTargetCrop.u16Height;
                                MIXER_DBG("h done!\n");
                            }
                        }
                    }

                    stVpeToCrop.u16X = ALIGN_DOWN(stVpeToCrop.u16X, 2);
                    stVpeToCrop.u16Y = ALIGN_DOWN(stVpeToCrop.u16Y, 2);
                    stVpeToCrop.u16Width = ALIGN_DOWN(stVpeToCrop.u16Width, 2);
                    stVpeToCrop.u16Height = ALIGN_DOWN(stVpeToCrop.u16Height, 2);

                    if((stVpeToCrop.u16X + stVpeToCrop.u16Width) > 3840)
                    {
                        MIXER_DBG("warning: Exceed max Width!\n");
                        MIXER_DBG("fCropSteps:%d CropRect:x:%d, w:%d\n", (int)fCropSteps, stVpeToCrop.u16X, stVpeToCrop.u16Width);
                        if(XStep <= 0)
                        {
                            stVpeToCrop.u16X += XStep;
                            if(XStep == 0)
                            {
                                stVpeToCrop.u16X -= (stVpeToCrop.u16X + stVpeToCrop.u16Width) - 3840;
                                XStep = -2;
                            }
                        }
                        else if(WStep <= 0)
                        {
                            stVpeToCrop.u16Width += WStep;
                            if(WStep == 0)
                            {
                                stVpeToCrop.u16Width -= (stVpeToCrop.u16X + stVpeToCrop.u16Width) - 3840;
                                WStep = -2;
                            }
                        }

                        MIXER_DBG("Fix:CropRect: x:%d, w:%d XStep:%d WStep:%d\n", stVpeToCrop.u16X, stVpeToCrop.u16Width, XStep, WStep);

                    }

                    if((stVpeToCrop.u16Y + stVpeToCrop.u16Height) > 2160)
                    {
                        MIXER_DBG("warning: Exceed max Height!\n");
                        MIXER_DBG("fCropSteps:%d CropRect: y:%d, h:%d\n", (int)fCropSteps, stVpeToCrop.u16Y,stVpeToCrop.u16Height);

                        if(YStep <= 0)
                        {
                            stVpeToCrop.u16Y += YStep;

                            if(YStep == 0)
                            {
                                stVpeToCrop.u16Y -= (stVpeToCrop.u16Y + stVpeToCrop.u16Height) - 2160;
                                YStep = -2;
                            }
                        }
                        else if(HStep <= 0)
                        {
                            stVpeToCrop.u16Height += HStep;

                            if(HStep == 0)
                            {
                                stVpeToCrop.u16Height -= (stVpeToCrop.u16Y + stVpeToCrop.u16Height) - 2160;
                                HStep = -2;
                            }
                        }

                        MIXER_DBG("Fix: CropRect: y:%d, h:%d YStep:%d HStep:%d\n", stVpeToCrop.u16Y,stVpeToCrop.u16Height, YStep, HStep);
                    }

                    if(((stVpeToCrop.u16Y + stVpeToCrop.u16Height) > 2160) || ((stVpeToCrop.u16X + stVpeToCrop.u16Width) > 3840))
                    {
                        MIXER_ERR("Fix failed!steps:%d :x %d, y:%d, w:%d, h:%d\n",(int)fCropSteps, stVpeToCrop.u16X, stVpeToCrop.u16Y,
                            stVpeToCrop.u16Width, stVpeToCrop.u16Height);
                        continue;
                    }

                   /* MIXER_DBG("fCropSteps:%d CropRect:x %d, y:%d, w:%d, h:%d\n", (int)fCropSteps, stVpeToCrop.u16X, stVpeToCrop.u16Y,
                    stVpeToCrop.u16Width, stVpeToCrop.u16Height);*/

                    if(MI_VPE_OK != MI_VPE_SetPortCrop(0, 0, &stVpeToCrop))
                    {
                       MIXER_ERR("can not set vpe port crop info.\n");
                    }

                }
                else
                {
                    MIXER_DBG("crop done! Crop steps:%d \n", (int)fCropSteps);
                    MIXER_DBG("Final Crop Rect:x %d, y:%d, w:%d, h:%d\n", stVpeToCrop.u16X, stVpeToCrop.u16Y,
                    stVpeToCrop.u16Width, stVpeToCrop.u16Height);
                    State = WaitForTargetSize;
                }

            }
            break;

            default:
            {

            }
            break;
        }

        MySystemDelay(31);
    }

}
#else

void CMidIPUHc::Hc2Vpe_Task1()
{
    MI_SYS_WindowRect_t stVpeTargetCrop;
    MI_SYS_WindowRect_t stVpeToCrop;
    memset(&stVpeToCrop, 0, sizeof(MI_SYS_WindowRect_t));
    m_CropState State = WaitForTargetSize;

    #define Y_STEP 4
    #define VPE_CROP_STEP_THRESHOLD 128
    #define NOBODY_CHEK_COUNTS 180

    const float r = 16.0/9;
    const MI_U8 yConstStep = Y_STEP;
    const MI_U8 xConstStep = r * Y_STEP;
    const MI_U8 wConstStep = xConstStep;
    const MI_U8 hConstStep = yConstStep;

    BOOL bExistHuman = FALSE;
    MI_U8 u16NobodyCount = 0;//该值用于避免当有人存在的时候，GetVpeCropRect也会返回-1的情况
    BOOL bRecover = FALSE;


    int YStep = 0;
    int XStep = 0;
    int WStep = 0;
    int HStep = 0;

    int XChange = 0;
    int YChange = 0;
    int WChange = 0;
    int HChange = 0;

    BOOL bXChangeDone = FALSE;
    BOOL bYChangeDone = FALSE;
    BOOL bWChangeDone = FALSE;
    BOOL bHChangeDone = FALSE;

    MI_S32 s32Ret = 0;
    MI_SYS_WindowRect_t stVpeCurCrop;

    stVpeCurCrop.u16X = VPE_ORIGINAL_CROP_X;
    stVpeCurCrop.u16Y = VPE_ORIGINAL_CROP_Y;
    stVpeCurCrop.u16Width = VPE_ORIGINAL_CROP_W;
    stVpeCurCrop.u16Height = VPE_ORIGINAL_CROP_H;

    MIXER_DBG("stVpeCurCrop:%d, y:%d, w:%d, h:%d\n", stVpeCurCrop.u16X, stVpeCurCrop.u16Y,
    stVpeCurCrop.u16Width, stVpeCurCrop.u16Height);

    s32Ret = MI_VPE_SetPortCrop(0, 0, &stVpeCurCrop);
    if(s32Ret != MI_SUCCESS)
    {
        MIXER_ERR("MI_VPE_SetPortCrop failed\n");

    }

    while(TRUE == m_bThread)
    {
        MySystemDelay(31);
        switch(State)
        {
            case WaitForTargetSize:
            {
                memset(&stVpeTargetCrop, 0, sizeof(MI_SYS_WindowRect_t));
                s32Ret = GetVpeCropRect(stVpeTargetCrop);
                //MIXER_DBG("s32Ret:%d\n",s32Ret);
                if(s32Ret == 0)
                {
                    u16NobodyCount = 0;
                }
                else
                {
                    if(s32Ret < 0)
                    {

                        //MIXER_DBG("bExistHuman:%d u16NobodyCount:%d\n",bExistHuman,  u16NobodyCount);

                        if(bExistHuman && (u16NobodyCount > NOBODY_CHEK_COUNTS))
                        {
                            stVpeTargetCrop.u16X = VPE_ORIGINAL_CROP_X;
                            stVpeTargetCrop.u16Y = VPE_ORIGINAL_CROP_Y;
                            stVpeTargetCrop.u16Width = VPE_ORIGINAL_CROP_W;
                            stVpeTargetCrop.u16Height = VPE_ORIGINAL_CROP_H;

                            bExistHuman = FALSE;
                            bRecover = TRUE;
                            u16NobodyCount = 0;

                            MIXER_DBG("Revert to centre & 1080P!\n");
                        }
                        else if(bExistHuman)
                        {
                            u16NobodyCount++;
                            //MIXER_DBG("No one!\n");
                            break;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        bExistHuman = TRUE;
                        u16NobodyCount = 0;
                        bRecover = FALSE;
                    }

                    if((s32Ret > 0) || bRecover)
                    {
                        bRecover = FALSE;
                        MIXER_DBG("get vpe cropx:%d, y:%d, w:%d, h:%d\n", stVpeTargetCrop.u16X,
                            stVpeTargetCrop.u16Y, stVpeTargetCrop.u16Width, stVpeTargetCrop.u16Height);
                        MIXER_DBG("stVpeCurCrop:%d, y:%d, w:%d, h:%d\n", stVpeCurCrop.u16X, stVpeCurCrop.u16Y,
                        stVpeCurCrop.u16Width, stVpeCurCrop.u16Height);

                        XChange = stVpeTargetCrop.u16X - stVpeCurCrop.u16X;
                        YChange = stVpeTargetCrop.u16Y - stVpeCurCrop.u16Y;
                        WChange = stVpeTargetCrop.u16Width - stVpeCurCrop.u16Width;
                        HChange = stVpeTargetCrop.u16Height - stVpeCurCrop.u16Height;

                        if((_abs(XChange) < VPE_CROP_STEP_THRESHOLD) && (_abs(YChange) < VPE_CROP_STEP_THRESHOLD) &&
                            (_abs(WChange) < VPE_CROP_STEP_THRESHOLD) && (_abs(HChange) < VPE_CROP_STEP_THRESHOLD))
                        {
                            MIXER_DBG("Change is too small! No need to crop\n");
                            break;
                        }

                        bXChangeDone = FALSE;
                        bYChangeDone = FALSE;
                        bWChangeDone = FALSE;
                        bHChangeDone = FALSE;

                        if(XChange > 0)
                        {
                            XStep = xConstStep;
                        }
                        else if(XChange < 0)
                        {
                            XStep = (-1) * xConstStep;
                        }
                        else
                        {
                            XStep = 0;
                            bXChangeDone = TRUE;
                        }

                        if(YChange > 0)
                        {
                            YStep = yConstStep;
                        }
                        else if(YChange < 0)
                        {
                            YStep = (-1) * yConstStep;
                        }
                        else
                        {
                            YStep = 0;
                            bYChangeDone = TRUE;
                        }

                        if(WChange > 0)
                        {
                            WStep = wConstStep;
                        }
                        else if(WChange < 0)
                        {
                            WStep = (-1) * wConstStep;
                        }
                        else
                        {
                            WStep = 0;
                            bWChangeDone = TRUE;
                        }

                        if(HChange > 0)
                        {
                            HStep = hConstStep;
                        }
                        else if(HChange < 0)
                        {
                            HStep = (-1) * hConstStep;
                        }
                        else
                        {
                            HStep = 0;
                            bHChangeDone = TRUE;
                        }

                        MIXER_DBG("Xstep:%d ystep:%d wstep:%d, hStep:%d \n", XStep, YStep, WStep, HStep);
                        MIXER_DBG("bXChangeDone:%d bYChangeDone:%d bWChangeDone:%d bHChangeDone:%d\n",bXChangeDone,
                            bYChangeDone, bWChangeDone, bHChangeDone );

                        stVpeToCrop = stVpeCurCrop;
                        stVpeCurCrop = stVpeTargetCrop;
                        MIXER_DBG("stVpeCurCrop:%d, y:%d, w:%d, h:%d\n", stVpeCurCrop.u16X, stVpeCurCrop.u16Y,
                        stVpeCurCrop.u16Width, stVpeCurCrop.u16Height);

                        State = Action;
                    }
                }
            }
            break;

            case Action:
            {
                if(bXChangeDone == FALSE)
                {
                    stVpeToCrop.u16X += XStep;
                    //MIXER_DBG("stVpeToCrop.u16X:%d\n", stVpeToCrop.u16X);
                    if(XStep > 0)
                    {
                        if(stVpeToCrop.u16X > stVpeTargetCrop.u16X)
                        {
                            stVpeToCrop.u16X = stVpeTargetCrop.u16X;
                            bXChangeDone = TRUE;
                            MIXER_DBG("x done!\n");
                        }
                    }
                    else
                    {
                        if((stVpeToCrop.u16X < stVpeTargetCrop.u16X) || (stVpeToCrop.u16X > 1920))
                        {
                            stVpeToCrop.u16X = stVpeTargetCrop.u16X;
                            bXChangeDone = TRUE;
                            MIXER_DBG("x done!\n");
                        }
                    }
                }

                if(bYChangeDone == FALSE)
                {
                    stVpeToCrop.u16Y += YStep;

                    //MIXER_DBG("stVpeToCrop.u16y:%d\n", stVpeToCrop.u16Y);
                    if(YStep > 0)
                    {
                        if((stVpeToCrop.u16Y > stVpeTargetCrop.u16Y))
                        {
                            stVpeToCrop.u16Y = stVpeTargetCrop.u16Y;
                            bYChangeDone = TRUE;
                            MIXER_DBG("y done!\n");
                        }
                    }
                    else
                    {
                        if((stVpeToCrop.u16Y < stVpeTargetCrop.u16Y) || (stVpeToCrop.u16Y > 1080))
                        {
                            stVpeToCrop.u16Y = stVpeTargetCrop.u16Y;
                            bYChangeDone = TRUE;
                            MIXER_DBG("y done!\n");
                        }
                    }
                }

                if(bWChangeDone == FALSE)
                {
                    stVpeToCrop.u16Width += WStep;

                    //MIXER_DBG("stVpeToCrop.u16w:%d\n", stVpeToCrop.u16Width);
                    if(WStep > 0)
                    {
                        if(stVpeToCrop.u16Width > stVpeTargetCrop.u16Width)
                        {
                            stVpeToCrop.u16Width = stVpeTargetCrop.u16Width;
                            bWChangeDone = TRUE;
                            MIXER_DBG("w done!\n");

                        }
                    }
                    else
                    {
                        if(stVpeToCrop.u16Width < stVpeTargetCrop.u16Width)
                        {
                            stVpeToCrop.u16Width = stVpeTargetCrop.u16Width;
                            bWChangeDone = TRUE;
                            MIXER_DBG("w done!\n");
                        }
                    }
                }

                if(bHChangeDone == FALSE)
                {
                    stVpeToCrop.u16Height += HStep;

                    //MIXER_DBG("stVpeToCrop.u16h:%d\n", stVpeToCrop.u16Height);
                    if(HStep > 0)
                    {
                        if(stVpeToCrop.u16Height > stVpeTargetCrop.u16Height)
                        {
                            stVpeToCrop.u16Height = stVpeTargetCrop.u16Height;
                            bHChangeDone = TRUE;
                            MIXER_DBG("h done!\n");
                        }
                    }
                    else
                    {
                        if(stVpeToCrop.u16Height < stVpeTargetCrop.u16Height)
                        {
                            stVpeToCrop.u16Height = stVpeTargetCrop.u16Height;
                            bHChangeDone = TRUE;
                            MIXER_DBG("h done!\n");
                        }
                    }
                }

                if(bXChangeDone && bYChangeDone && bWChangeDone && bHChangeDone)
                {
                    MIXER_DBG("Crop Change Done!\n");
                    State = WaitForTargetSize;

                    break;
                }

                MI_SYS_WindowRect_t stVpeAlignCrop;

                stVpeAlignCrop.u16X = ALIGN_UP((MI_U16)stVpeToCrop.u16X, 2);
                stVpeAlignCrop.u16Y = ALIGN_UP((MI_U16)stVpeToCrop.u16Y, 2);
                stVpeAlignCrop.u16Width = ALIGN_DOWN((MI_U16)stVpeToCrop.u16Width, 2);
                stVpeAlignCrop.u16Height = ALIGN_DOWN((MI_U16)stVpeToCrop.u16Height, 2);

                //MIXER_DBG("stVpeToCrop x:%d, y:%d, w:%d, h:%d\n", stVpeToCrop.u16X, stVpeToCrop.u16Y,
                //stVpeToCrop.u16Width, stVpeToCrop.u16Height);

                //MIXER_DBG("stVpeAlignCrop:%d, y:%d, w:%d, h:%d\n", stVpeAlignCrop.u16X, stVpeAlignCrop.u16Y,
                //stVpeAlignCrop.u16Width, stVpeAlignCrop.u16Height);

                if(MI_VPE_OK != MI_VPE_SetPortCrop(0, 0, &stVpeAlignCrop))
                {
                   MIXER_ERR("can not set vpe port crop info.\n");
                   break;
                }

            }
            break;

            default:
            {

            }
            break;
        }
    }

}

#endif
int CMidIPUHc::InitResource()
{
    MI_S32 s32Ret = MI_SUCCESS;

    if(MI_SUCCESS != (s32Ret =IPUCreateDevice(m_stIPUInfo.szIPUfirmware, MAX_VARIABLE_BUF_SIZE)))
    {
        MIXER_ERR("IPUCreateDevice error, %s, ret:[0x%x]\n", m_stIPUInfo.szIPUfirmware, s32Ret);
        return -1;
    }

    if (MI_SUCCESS != InitStreamSource(m_stHcModel))
    {
        MIXER_ERR("InitStreamSource for fd error\n");
        IPUDestroyDevice();
        return -1;
    }

    m_bThread = TRUE;
    pthread_create(&g_stPthreadHc2Vpe, NULL, Hc2Vpe_Task, (void*)this);
    pthread_setname_np(g_stPthreadHc2Vpe, "Hc2Vpe_Task");

    return MI_SUCCESS;
}

int CMidIPUHc::ReleaseResource()
{
    m_bThread = FALSE;

    pthread_join(g_stPthreadHc2Vpe, NULL);

    return MI_SUCCESS;
}
MI_S32 CMidIPUHc::SetIeParam(IeParamInfo tmp,MI_U8 scop)
{
  return 0;
}

void CMidIPUHc::DealDataProcess()
{
#if 0
    MI_S32 s32Ret = 0;
    MI_SYS_ChnPort_t stChnOutputPort;
    MI_IPU_TensorVector_t Vector4Affine;
    MI_U32 u32Count = 0;

    // get fd data
    memset(&stChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));
    stChnOutputPort.eModId      = E_MI_MODULE_ID_DIVP;
    stChnOutputPort.u32DevId    = 0;
    stChnOutputPort.u32ChnId    = m_stHcModel.divpChn;
    stChnOutputPort.u32PortId   = 0;

    do
    {
        memset(&m_stHcModel.stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
        s32Ret = MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort, &m_stHcModel.stBufInfo, &m_stHcModel.stBufHandle);
        if (s32Ret != MI_SUCCESS)
        {
            MySystemDelay(10 * 1000);
        }
        else
        {
            m_dataReady |= 0x01;
        }
        u32Count ++;
    }
    while ((MI_SUCCESS != s32Ret) && (u32Count <= 5));
#else
    fd_set read_fds;
    struct timeval tv;
    MI_S32 s32Ret = 0;
	MI_S32 s32GetBufRet = -1;
    MI_SYS_ChnPort_t stChnOutputPort;
    MI_IPU_TensorVector_t Vector4Affine;

    FD_ZERO(&read_fds);
    FD_SET(m_stHcModel.s32Fd, &read_fds);

    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000;
    s32Ret = select(m_s32MaxFd + 1, &read_fds, NULL, NULL, &tv);
    if (s32Ret < 0)
    {
        // fail
    }
    else if (0 == s32Ret)
    {
        // timeout
    }
    else
    {
        if(FD_ISSET(m_stHcModel.s32Fd, &read_fds))
        {
            stChnOutputPort.eModId      = E_MI_MODULE_ID_DIVP;
            stChnOutputPort.u32DevId    = 0;
            stChnOutputPort.u32ChnId    = m_stHcModel.divpChn;
            stChnOutputPort.u32PortId   = 0;

            memset(&m_stHcModel.stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
			s32GetBufRet = MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort, &m_stHcModel.stBufInfo,  \
                   															 &m_stHcModel.stBufHandle);
            if (MI_SUCCESS == s32GetBufRet)
            {
                m_dataReady |= 0x01;
            }
        }

    }
#endif

    if ((m_dataReady & 0x01) == 0x01)
    {
        HcRect_t mHcRectSize;
        MI_U8 recCnt = 0;
        if(MI_SUCCESS == DoHc(m_stHcModel.stBufInfo, &Vector4Affine))
        {
            recCnt = CalcHcRectSize(Vector4Affine, mHcRectSize);
            if(recCnt > 0)
            {
                UpdateResult(mHcRectSize);
#if 0
                memset(&pRecInfo,0x00,sizeof(ST_DlaRectInfo_T));
                pRecInfo.rect.u16PicH = mHcRectSize.height;
                pRecInfo.rect.u16PicW = mHcRectSize.width;
                pRecInfo.rect.u32X = mHcRectSize.x;
                pRecInfo.rect.u32Y = mHcRectSize.y;
                memcpy(pRecInfo.szObjName, "human", 5);
                DLAtoRECT(0, recCnt, &pRecInfo, TRUE, TRUE);
#endif
            }
            MI_IPU_PutOutputTensors(m_stHcModel.ipuChn, &Vector4Affine);
        }
    }

    if(m_dataReady & 0x01)
    {
       if(MI_SUCCESS == s32GetBufRet)
       {
         MI_SYS_ChnOutputPortPutBuf(m_stHcModel.stBufHandle);
       }
    }

    m_dataReady = 0;
}


int CMidIPUHc::InitStreamSource(Model_Info_S &stModelInfo)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VPE_PortMode_t stVpeMode;
    MI_SYS_WindowRect_t stCropWin;
    MI_DIVP_OutputPortAttr_t stDivpPortAttr;
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_SYS_ChnPort_t stChnOutputPort;

    if(MI_SUCCESS != IPUCreateChannel(&stModelInfo.ipuChn, stModelInfo.pModelFile, stModelInfo.u32InBufDepth,
                                      stModelInfo.u32OutBufDepth))
    {
        MIXER_ERR("IPUCreateChannel error, chn:%d, model:%s, ret:[0x%x]\n", stModelInfo.ipuChn, stModelInfo.pModelFile, s32Ret);
        return -1;
    }

    if (MI_SUCCESS != (s32Ret = MI_IPU_GetInOutTensorDesc(stModelInfo.ipuChn, &stModelInfo.stNPUDesc)))
    {
        MIXER_ERR("MI_IPU_GetInOutTensorDesc error, chn:%d, ret:[0x%x]\n", stModelInfo.ipuChn, s32Ret);
        IPUDestroyChannel(stModelInfo.ipuChn, stModelInfo.u32InBufDepth, stModelInfo.u32OutBufDepth);
        return -1;
    }

    MIXER_DBG("H:%d,W:%d,C:%d,format:%s, u32InputTensorCount:%d, u32OutputTensorCount:%d\n",
              stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1],
              stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2],
              stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[3],
              (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_U8) ? "MI_IPU_FORMAT_U8" :
              (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_NV12) ? "MI_IPU_FORMAT_NV12" :
              (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_INT16) ? "MI_IPU_FORMAT_INT16" :
              (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_INT32) ? "MI_IPU_FORMAT_INT32" :
              (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_INT8) ? "MI_IPU_FORMAT_INT8" :
              (stModelInfo.stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_FP32) ? "MI_IPU_FORMAT_FP32" : "UNKNOWN",
              stModelInfo.stNPUDesc.u32InputTensorCount, stModelInfo.stNPUDesc.u32OutputTensorCount);

    memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
    ExecFunc(MI_VPE_GetPortMode(stModelInfo.vpeChn, stModelInfo.vpePort, &stVpeMode), MI_VPE_OK);

    stCropWin.u16Width = stVpeMode.u16Width;
    stCropWin.u16Height = stVpeMode.u16Height;
    stCropWin.u16X = 0;
    stCropWin.u16Y = 0;
    MIXERCHECKRESULT(Mixer_Divp_CreatChannel(stModelInfo.divpChn, (MI_SYS_Rotate_e)0x0, &stCropWin));

    if(((g_rotation & 0xFFFF) == 90) || ((g_rotation & 0xFFFF) == 270))
    {
        stDivpPortAttr.u32Width     = stModelInfo.u32Height;
        stDivpPortAttr.u32Height    = stModelInfo.u32Width;
    }
    else
    {
        stDivpPortAttr.u32Width     = stModelInfo.u32Width;
        stDivpPortAttr.u32Height    = stModelInfo.u32Height;
    }

    stDivpPortAttr.eCompMode    = E_MI_SYS_COMPRESS_MODE_NONE;
    stDivpPortAttr.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;

    MIXERCHECKRESULT(Mixer_Divp_SetOutputAttr(stModelInfo.divpChn, &stDivpPortAttr));
    MIXERCHECKRESULT(Mixer_Divp_StartChn(stModelInfo.divpChn));

    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId  = 0;
    stBindInfo.stSrcChnPort.u32ChnId  = stModelInfo.vpeChn;
    stBindInfo.stSrcChnPort.u32PortId = stModelInfo.vpePort;

    stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId  = 0;
    stBindInfo.stDstChnPort.u32ChnId  = stModelInfo.divpChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stBindInfo.stDstChnPort, 3, 3), MI_SUCCESS);

    stBindInfo.u32SrcFrmrate = 30; //MI_VideoEncoder::vpeframeRate;
    stBindInfo.u32DstFrmrate = 15; //MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#endif
    MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

    stChnOutputPort.eModId      = E_MI_MODULE_ID_DIVP;
    stChnOutputPort.u32DevId    = 0;
    stChnOutputPort.u32ChnId    = stModelInfo.divpChn;
    stChnOutputPort.u32PortId   = 0;
    s32Ret = MI_SYS_GetFd(&stChnOutputPort, &stModelInfo.s32Fd);
    if (s32Ret < 0)
    {
        MIXER_ERR("divp ch: %d, get fd. err\n", stChnOutputPort.u32ChnId);
        return -1;
    }

    m_s32MaxFd = MAX(m_s32MaxFd, stModelInfo.s32Fd);

    MIXER_DBG("m_s32Fd:%d\n", stModelInfo.s32Fd);

    //get vpe capture res.
    ExecFunc(MI_VPE_GetChannelCrop(0, &m_stVpeCropWinGet), MI_SUCCESS);
    m_stVpeCropWinGet.u16Width = 3840;
    m_stVpeCropWinGet.u16Height = 2160;
    MIXER_DBG("vpe input width:%d,  height:%d.\n", m_stVpeCropWinGet.u16Width, m_stVpeCropWinGet.u16Height);
    MI_U32 tsize = m_stVpeCropWinGet.u16Height * m_stVpeCropWinGet.u16Width;
    if(tsize >= (3840 * 2160))
    {
        m_MaxIndex = Res3840x2160;
        m_MinIndex = Res2560x1440;
      m_dataRects.m_CurXWThreshold = 64 * 6;
      m_dataRects.m_CurYHThreshold = 36 * 6;
    }
    else if(tsize >= (3072 * 1728))
    {
        m_MaxIndex = Res3072x1728;
        m_MinIndex = Res2560x1440;
     m_dataRects.m_CurXWThreshold = 64 * 5;
     m_dataRects.m_CurYHThreshold = 36 * 5;
    }
    else if(tsize >= (2688 * 1512))
    {
        m_MaxIndex = Res2688x1512;
        m_MinIndex = Res1920x1080;
     m_dataRects.m_CurXWThreshold = 64 * 3;
     m_dataRects.m_CurYHThreshold = 36 * 3;
    }
    else if(tsize >= (2560 * 1440))
    {
        m_MaxIndex = Res2560x1440;
        m_MinIndex = Res1920x1080;
     m_dataRects.m_CurXWThreshold = 64 * 2;
     m_dataRects.m_CurYHThreshold = 96;
    }
    else if(tsize >= (2304 * 1296))
    {
        m_MaxIndex = Res2304x1296;
        m_MinIndex = Res1920x1080;
     m_dataRects.m_CurXWThreshold = 128;
     m_dataRects.m_CurYHThreshold = 96;
    }
    else if(tsize >= (1920 * 1080))
    {
        m_MaxIndex = Res1920x1080;
        m_MinIndex = Res1280x720;
     m_dataRects.m_CurXWThreshold = 96;
     m_dataRects.m_CurYHThreshold = 64;
    }
    else
    {
        m_MaxIndex = Res1280x720;
        m_MinIndex = Res1280x720;
     m_dataRects.m_CurXWThreshold = 64;
     m_dataRects.m_CurYHThreshold = 64;
    }

    MIXER_DBG("MaxIndex :%d,  MinIndex:%d, m_CurThreshold:%d.\n", m_MaxIndex, m_MinIndex, m_dataRects.m_CurXWThreshold);
    memset(&mCurHcPosition, 0, sizeof(mCurHcPosition));

    return MI_SUCCESS;
}

int CMidIPUHc::ReleaseStreamSource(Model_Info_S &stModelInfo)
{
    Mixer_Sys_BindInfo_T stBindInfo;

    (void)IPUDestroyChannel(stModelInfo.ipuChn, stModelInfo.u32InBufDepth, stModelInfo.u32OutBufDepth);

    if (stModelInfo.s32Fd > 0)
    {
        MI_SYS_CloseFd(stModelInfo.s32Fd);
        stModelInfo.s32Fd = -1;
    }

    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId  = 0;
    stBindInfo.stSrcChnPort.u32ChnId  = stModelInfo.vpeChn;
    stBindInfo.stSrcChnPort.u32PortId = stModelInfo.vpePort;

    stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId  = 0;
    stBindInfo.stDstChnPort.u32ChnId  = stModelInfo.divpChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stBindInfo.stDstChnPort, 3, 3), MI_SUCCESS);

    stBindInfo.u32SrcFrmrate = 30; //MI_VideoEncoder::vpeframeRate;
    stBindInfo.u32DstFrmrate = 15; //MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#endif
    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

    MIXERCHECKRESULT(Mixer_Divp_StopChn(stModelInfo.divpChn));
    MIXERCHECKRESULT(Mixer_Divp_DestroyChn(stModelInfo.divpChn));

    return MI_SUCCESS;
}

int CMidIPUHc::DoHc(const MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *Vector4Affine)
{
    MI_S32 s32Ret;
    MI_IPU_TensorVector_t stInputTensorVector, stOutputTensorVector;

    if (m_stHcModel.stNPUDesc.u32InputTensorCount != 1)
    {
        MIXER_ERR("error: FDA network input count isn't 1\n");
        return E_IPU_ERR_ILLEGAL_INPUT_OUTPUT_PARAM;
    }


    /*stInputTensorVector.u32TensorCount = m_stHcModel.stNPUDesc.u32InputTensorCount;
    if (MI_SUCCESS != (s32Ret = MI_IPU_GetInputTensors(m_stHcModel.ipuChn, &stInputTensorVector)))
    {
        MIXER_ERR("MI_IPU_GetInputTensors error, ret[0x%x]\n", s32Ret);
        return -1;
    }*/

    memset(&stOutputTensorVector, 0, sizeof(MI_IPU_TensorVector_t));
    if (MI_SUCCESS != (s32Ret = MI_IPU_GetOutputTensors(m_stHcModel.ipuChn, &stOutputTensorVector)))
    {
        MIXER_ERR("MI_IPU_GetOutputTensors error, ret[0x%x]\n", s32Ret);
        MI_IPU_PutInputTensors(m_stHcModel.ipuChn, &stInputTensorVector);
        return -1;
    }

    // prepare input vector
    memset(&stInputTensorVector, 0, sizeof(MI_IPU_TensorVector_t));
    stInputTensorVector.u32TensorCount = m_stHcModel.stNPUDesc.u32InputTensorCount;
    if (stBufInfo.eBufType == E_MI_SYS_BUFDATA_RAW)
    {
        //MIXER_DBG("E_MI_SYS_BUFDATA_RAW\n");
        stInputTensorVector.astArrayTensors[0].phyTensorAddr[0] = stBufInfo.stRawData.phyAddr;
        stInputTensorVector.astArrayTensors[0].ptTensorData[0] = stBufInfo.stRawData.pVirAddr;
    }
    else if (stBufInfo.eBufType == E_MI_SYS_BUFDATA_FRAME)
    {
        //MIXER_DBG("E_MI_SYS_BUFDATA_FRAME\n");
        stInputTensorVector.astArrayTensors[0].phyTensorAddr[0] = stBufInfo.stFrameData.phyAddr[0];
        stInputTensorVector.astArrayTensors[0].ptTensorData[0] = stBufInfo.stFrameData.pVirAddr[0];

        stInputTensorVector.astArrayTensors[0].phyTensorAddr[1] = stBufInfo.stFrameData.phyAddr[1];
        stInputTensorVector.astArrayTensors[0].ptTensorData[1] = stBufInfo.stFrameData.pVirAddr[1];
        //save_buffer_to_file(stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32BufSize);
    }

    if(MI_SUCCESS != (s32Ret = MI_IPU_Invoke(m_stHcModel.ipuChn, &stInputTensorVector, &stOutputTensorVector)))
    {
        MIXER_ERR("MI_IPU_Invoke error, ret[0x%x]\n", s32Ret);
        MI_IPU_PutOutputTensors(m_stHcModel.ipuChn,&stOutputTensorVector);
        MI_IPU_PutInputTensors(m_stHcModel.ipuChn, &stInputTensorVector);
        return -1;
    }

    *Vector4Affine = stOutputTensorVector;

    //release input vector
    //MI_IPU_PutOutputTensors(m_stHcModel.ipuChn,&stInputTensorVector);


    return MI_SUCCESS;
}

std::vector<DetBBox> CMidIPUHc::SaveFdaOutData(stFdaOutputDataDesc_t *pstFdaOutputData,std::vector<DetBBox> &detboxes)
{
    float fcount = *(pstFdaOutputData->pDectCount);
    int count = fcount;

    // MIXER_DBG("=============face count:%d============\n", count);
    detboxes.clear();
    for (int i = 0; i < count; i++)
    {
        DetBBox box;
        if (*(pstFdaOutputData->pfScores + i) > 0.0)
        {
            box.y1 = *(pstFdaOutputData->pfBBox + i * fdfr_ALIGN_UP(4, INNER_MOST_ALIGNMENT)) * (float)m_stHcModel.u32Height;
            box.x1 = *(pstFdaOutputData->pfBBox + i * fdfr_ALIGN_UP(4, INNER_MOST_ALIGNMENT) + 1) * (float)m_stHcModel.u32Width;
            box.y2 = *(pstFdaOutputData->pfBBox + i * fdfr_ALIGN_UP(4, INNER_MOST_ALIGNMENT) + 2) * (float)m_stHcModel.u32Height;
            box.x2 = *(pstFdaOutputData->pfBBox + i * fdfr_ALIGN_UP(4, INNER_MOST_ALIGNMENT) + 3) * (float)m_stHcModel.u32Width;
            box.score = *(pstFdaOutputData->pfScores+ i);
            box.lm1_x = *(pstFdaOutputData->pfLms + i * 10) * (float)m_stHcModel.u32Width;
            box.lm1_y = *(pstFdaOutputData->pfLms + i * 10 + 1) * (float)m_stHcModel.u32Height;
            box.lm2_x = *(pstFdaOutputData->pfLms + i * 10 + 2) * (float)m_stHcModel.u32Width;
            box.lm2_y = *(pstFdaOutputData->pfLms + i * 10 + 3) * (float)m_stHcModel.u32Height;
            box.lm3_x = *(pstFdaOutputData->pfLms + i * 10 + 4) * (float)m_stHcModel.u32Width;
            box.lm3_y = *(pstFdaOutputData->pfLms + i * 10 + 5) * (float)m_stHcModel.u32Height;
            box.lm4_x = *(pstFdaOutputData->pfLms + i * 10 + 6) * (float)m_stHcModel.u32Width;
            box.lm4_y = *(pstFdaOutputData->pfLms + i * 10 + 7) * (float)m_stHcModel.u32Height;
            box.lm5_x = *(pstFdaOutputData->pfLms + i * 10 + 8) * (float)m_stHcModel.u32Width;
            box.lm5_y = *(pstFdaOutputData->pfLms + i * 10 + 9) * (float)m_stHcModel.u32Height;
            MIXER_INFO("1==%d==+++++++++  X1: %.2f  Y1: %.2f  X2: %.2f  Y2: %.2f  ++++++++++\n", i, box.x1, box.y1, box.x2, box.y2);
            detboxes.push_back(box);
        }
    }

    return detboxes;
}

void CMidIPUHc::DoTrack(stFdaOutputDataDesc_t *pstFdaOutputData)
{
    std::vector<DetBBox> tempDetboxes;

    std::vector <std::vector<TrackBBox>> detFrameDatas;
    std::vector <TrackBBox> detFrameData;

    SaveFdaOutData(pstFdaOutputData,tempDetboxes);
    //m_detboxes.clear();
    int count = tempDetboxes.size();

    for (int i = 0; i < count; i++)
    {
        DetBBox detbox = tempDetboxes[i];

        if (detbox.score > 0.6)
        {

            if((detbox.x2 < 20.0) || ( detbox.x1 > 610.0) || ( detbox.y2 < 20.0) || ( detbox.y1 > 340.0)) continue;
            float det_box_w = detbox.x2 - detbox.x1;
            float det_box_h = detbox.y2 - detbox.y1;
            if((det_box_w < 30) || det_box_h < 40) continue;
            if((det_box_h / det_box_w) < 1.4) continue;

            // MIXER_DBG("=== %d==+++++++++  X1: %.2f  Y1: %.2f  X2: %.2f  Y2: %.2f h/w: %2f score :%.2f  ++++++++++\n", i,
                        //   detbox.x1, detbox.y1, detbox.x2, detbox.y2, det_box_h / det_box_w, detbox.score);
            if(detbox.x1 < 0)  detbox.x1 = 0.0;
            if(detbox.y1 < 0) detbox.y1 = 0.0;
            if(detbox.x2 > (float)m_stHcModel.u32Width) detbox.x2 = (float)m_stHcModel.u32Width;
            if(detbox.y2 > (float)m_stHcModel.u32Height) detbox.y2 = (float)m_stHcModel.u32Height;

            TrackBBox cur_box;
            memcpy(&cur_box, (TrackBBox *)&detbox, sizeof(TrackBBox));
            cur_box.x = detbox.x1;
            cur_box.y = detbox.y1;
            cur_box.w = detbox.x2 - detbox.x1;
            cur_box.h = detbox.y2 - detbox.y1;
            cur_box.score = detbox.score;
            detFrameData.push_back(cur_box);
            //m_detboxes.push_back(detbox);
        }
    }

    detFrameDatas.push_back(detFrameData);
    m_HcTrackBBoxs = m_BBoxTracker.track_iou(detFrameDatas);
}
MI_S8 CMidIPUHc::DataWaveFliter(HcRect_t &tRectSize,MI_U8 dataLen,MI_U8 fliterType)
{
    static MI_U16 x[WAVE_FLITER_QP]= {0};
    static MI_U16 y[WAVE_FLITER_QP]= {0};
    static MI_U16 height[WAVE_FLITER_QP]= {0};
    static MI_U16 width[WAVE_FLITER_QP]= {0};
    static MI_U32 sumX = 0;
    static MI_U32 sumY = 0;
    static MI_U32 sumH = 0;
    static MI_U32 sumW = 0;
    if(dataLen < WAVE_FLITER_QP)
    {
        x[dataLen] = tRectSize.x;
        y[dataLen] = tRectSize.y;
        height[dataLen] = tRectSize.height;
        width[dataLen] = tRectSize.width;
        if(0 == fliterType)
        {
            sumH += height[dataLen];
            sumW += width[dataLen];
            sumX += x[dataLen];
            sumY += y[dataLen];
        }
        return -1;
    }
    else
    {
        if(0 == fliterType)
        {
            tRectSize.x = sumX / WAVE_FLITER_QP;
            tRectSize.y  = sumY / WAVE_FLITER_QP;
            tRectSize.height = sumH / WAVE_FLITER_QP;
            tRectSize.width = sumW / WAVE_FLITER_QP;
        }
        else if(1 == fliterType)
        {
            static MI_U32 MaxX = 0;
            static MI_U32 MaxY = 0;
            static MI_U32 minH = height[0];
            static MI_U32 minW = width[0];
            for(MI_U8 len=0; len < WAVE_FLITER_QP; len++)
            {
                if(MaxX < x[len])
                {
                    MaxX = x[len];
                }
                if(MaxY < y[len])
                {
                    MaxY = y[len];
                }
                if(minH < height[len])
                {
                    minH = height[len];
                }
                if(minW < width[len])
                {
                    minW = width[len];
                }
            }
            tRectSize.x = MaxX;
            tRectSize.y  = MaxY;
            tRectSize.height = minH;
            tRectSize.width = minW;
        }
    }
    return 0;
}
int CMidIPUHc::CalcHcRectSize(const MI_IPU_TensorVector_t &Vector4Affine, HcRect_t &HcRectSize)
{
    MI_U16 i = 0x0;
    static MI_U32 RRCount = 0x0;
    MI_U32 RealDCount = 0x0;
    float startx = 10000.0, starty = 10000.0, endx = 0.0, endy = 0.0, xWidth = 0.0, xHeight = 0.0;

    stFdaOutputDataDesc_t data;

    data.pfBBox = (float*)(Vector4Affine.astArrayTensors[0].ptTensorData[0]);
    data.pfLms = (float*)(Vector4Affine.astArrayTensors[1].ptTensorData[0]);
    data.pfScores = (float*)(Vector4Affine.astArrayTensors[2].ptTensorData[0]);
    data.pDectCount = (float*)(Vector4Affine.astArrayTensors[3].ptTensorData[0]);
    DoTrack(&data);

    MI_U16 count = m_HcTrackBBoxs.size();

    ST_DlaRectInfo_T  pRecInfo[10];
    memset(&pRecInfo,0x00,sizeof(pRecInfo));

    //MIXER_DBG("count:%d.\n", count);

    for(i=0; i< count; i++)
    {
        int index = m_HcTrackBBoxs[i].boxes.size()-1;
        TrackBBox & trackBox = m_HcTrackBBoxs[i].boxes[index];
        TrackBBox box = trackBox;

        if(box.x < startx)
            startx = box.x * 0.8;
        if(box.y < starty)
            starty = box.y * 0.9;

        endx = (box.w + box.x) * 1.2;
        endy = (box.h + box.y) * 1.1;

        if(endx > xWidth)
            xWidth = endx;
        if(endy > xHeight)
            xHeight = endy;

        RealDCount++;
        RRCount = 0x0;
        // MIXER_DBG("HC==  x: %f,  y: %f,  w: %f,  h: %f.\n", box.x,  box.y, box.w, box.h);
    }

    if(RealDCount)
    {
        if(startx < 0)
            startx = 0;

        if(starty < 0)
            starty = 0;

        if(xWidth > (float)m_stHcModel.u32Width)
            xWidth = (float)m_stHcModel.u32Width;

        if(xHeight > (float)m_stHcModel.u32Height)
            xHeight = (float)m_stHcModel.u32Height;

        memset(&HcRectSize, 0x0, sizeof(HcRectSize));
        //
        if((g_rotation == E_MI_SYS_ROTATE_NONE) || (g_rotation == E_MI_SYS_ROTATE_180))
        {
            HcRectSize.x = ALIGN_8xDOWN((MI_U16)(startx * (float)m_stVpeCropWinGet.u16Width /(float)m_stHcModel.u32Width));
            // MIXER_DBG("starty:%d %d \n",  (MI_U16) starty, (MI_U16)(starty * (float)m_stVpeCropWinGet.u16Height  / (float)m_stHcModel.u32Height));
            HcRectSize.y = ALIGN_2xUP((MI_U16)(starty* (float)m_stVpeCropWinGet.u16Height  / (float)m_stHcModel.u32Height));
            // MIXER_DBG("HcRectSize.y:%d \n", HcRectSize.y);
            HcRectSize.width = ALIGN_8xUP((MI_U16)((xWidth -startx) * (float)m_stVpeCropWinGet.u16Width / (float) m_stHcModel.u32Width));
            HcRectSize.height = ALIGN_2xDOWN((MI_U16)((xHeight - starty) * (float)m_stVpeCropWinGet.u16Height  / (float)m_stHcModel.u32Height));
        }
        else
        {
            HcRectSize.x = ALIGN_8xDOWN((MI_U16)(startx * (float)m_stVpeCropWinGet.u16Height /(float)m_stHcModel.u32Width));
            HcRectSize.y= ALIGN_2xUP((MI_U16)(starty * (float)m_stVpeCropWinGet.u16Width / (float)m_stHcModel.u32Height));
            HcRectSize.width = ALIGN_8xUP((MI_U16)((xWidth - startx) * (float)m_stVpeCropWinGet.u16Height /(float) m_stHcModel.u32Width));
            HcRectSize.height = ALIGN_2xDOWN((MI_U16)((xHeight -starty) * (float)m_stVpeCropWinGet.u16Width / (float)m_stHcModel.u32Height));
        }

        HcRectSize.pts = MI_OS_GetTime();

        //MIXER_DBG("RealDCount:%d hc ==  x: %d  y: %d  w: %d  h: %d++++++\n",RealDCount, HcRectSize.x, HcRectSize.y, HcRectSize.width, HcRectSize.height);
    }
    else
    {
        RRCount++;
        if(RRCount >= 5)       //
        {
            RRCount = 0;
            HcRectSize.x = HcRectSize.y = 0x0;
            HcRectSize.width = m_ResIndex[m_MaxIndex].ImageW;
            HcRectSize.height = m_ResIndex[m_MaxIndex].ImageH;
            HcRectSize.pts = MI_OS_GetTime();
           // MIXER_DBG("======= w:%d, h:%d ====.\n", HcRectSize.width, HcRectSize.height);
            return 0;
        }
    }


    if (count){
        for(i=0; i< count; i++)
        {
            int index = m_HcTrackBBoxs[i].boxes.size()-1;
            TrackBBox & trackBox = m_HcTrackBBoxs[i].boxes[index];
            TrackBBox box = trackBox;

            pRecInfo[i].rect.u32X = ALIGN_8xDOWN((MI_U16)(box.x * 1920.0 /640.0));
            pRecInfo[i].rect.u32Y = ALIGN_2xUP((MI_U16)(box.y * 1080.0  / 360.0));
            pRecInfo[i].rect.u16PicH = ALIGN_2xDOWN((MI_U16)((box.h) * 1080.0 /360.0));
            pRecInfo[i].rect.u16PicW =  ALIGN_8xUP((MI_U16)((box.w) * 1920.0 /640.0));
            memcpy(pRecInfo[i].szObjName, "person", 7*sizeof(pRecInfo[i].szObjName[0]));
        }

        pRecInfo[count].rect.u32X = ALIGN_8xDOWN((MI_U16)(startx * 1920.0 /640.0));
        pRecInfo[count].rect.u32Y = ALIGN_2xUP((MI_U16)(starty * 1080.0  / 360.0));
        pRecInfo[count].rect.u16PicW =  ALIGN_8xUP((MI_U16)((xWidth-startx)* 1920.0 /640.0));
        pRecInfo[count].rect.u16PicH = ALIGN_2xDOWN((MI_U16)((xHeight - starty) * 1080.0  / 360.0));
        memcpy(pRecInfo[count].szObjName, "person", 7*sizeof(pRecInfo[count].szObjName[0]));
        // MIXER_DBG("======= x:%d, y:%d, w:%d, h:%d ====.\n", pRecInfo[count].rect.u32X, pRecInfo[count].rect.u32Y, pRecInfo[count].rect.u16PicW,  pRecInfo[count].rect.u16PicH);

        DLAtoRECT(0, count+1, pRecInfo, TRUE, TRUE);
        pthread_mutex_lock(&g_mutex_UpadteOsdState);
        g_UpadteOsdState++;
        pthread_cond_signal(&g_cond_UpadteOsdState);
        pthread_mutex_unlock(&g_mutex_UpadteOsdState);
    }else{
        g_UpadteOsdState++;
    }
    return RealDCount;
}

void CMidIPUHc::UpdateResult(const HcRect_t &mHcRectSize)
{

    m_dataRects.PushRect(mHcRectSize);
}


int CMidIPUHc::GetTargetSize( HcRect_t &mHcRectSize)
{

    MI_U16 TargetIndex = 0x0;

    static HcRect_t  lastVectRect, lastHcRectSize;
    static MI_BOOL state = FALSE;
    HcRect_t CurRect;

    MI_S16 CurPointX = 0x0, CurPointY = 0x0;
    static MI_S16 LastCurPointX = 0x0, LastCurPointY = 0x0;

    memset(&CurRect, 0x0, sizeof(CurRect));

    if(FALSE == state)
        memset(&lastHcRectSize, 0x0, sizeof(lastHcRectSize));

    if(0x0 == m_dataRects.GetAlgoRect(CurRect))
    {
        if(TRUE != state)
        {
            return -1;
        }
        else
        {
            CurRect = lastVectRect;
        }
    }
    else
    {
        state = TRUE;
    }

    lastVectRect = CurRect;
    if(TRUE == state)
    {
        //get max/min index
        TargetIndex = GetMindex(CurRect.width, CurRect.height);
        if(TargetIndex > m_MaxIndex)
            TargetIndex = m_MaxIndex;
        if(TargetIndex <= m_MinIndex)
            TargetIndex = m_MinIndex;

        CurPointX = CurRect.x + (MI_S16)CurRect.width/2;
        CurPointY = CurRect.y + (MI_S16)CurRect.height/2;

     if(_abs(LastCurPointX  - CurPointX) < 32)        //ignore
        CurPointX = LastCurPointX;
     if(_abs(LastCurPointY  - CurPointY) < 16)        //ignore
         CurPointY = LastCurPointY;

        if(CurPointX <= m_ResIndex[TargetIndex].ImageW/2)
        {
            mHcRectSize.x = 0x0;

        }
        else
        {
            if((CurPointX + m_ResIndex[TargetIndex].ImageW/2) > m_stVpeCropWinGet.u16Width)
            {
                mHcRectSize.x = ALIGN_8xDOWN((m_stVpeCropWinGet.u16Width - m_ResIndex[TargetIndex].ImageW));
            }
            else
            {
                mHcRectSize.x = ALIGN_8xDOWN((CurPointX - m_ResIndex[TargetIndex].ImageW/2));
            }
        }

        if(CurPointY <= m_ResIndex[TargetIndex].ImageH/2)
        {
            mHcRectSize.y = 0x0;

        }
        else
        {
            if((CurPointY + m_ResIndex[TargetIndex].ImageH/2) > m_stVpeCropWinGet.u16Height)
            {
                mHcRectSize.y = ALIGN_8xDOWN((m_stVpeCropWinGet.u16Height - m_ResIndex[TargetIndex].ImageH));
            }
            else
            {
                mHcRectSize.y = ALIGN_8xDOWN((CurPointY - m_ResIndex[TargetIndex].ImageH/2));
            }
        }

    LastCurPointX = CurPointX;
    LastCurPointY = CurPointY;

        mHcRectSize.width = m_ResIndex[TargetIndex].ImageW;
        mHcRectSize.height = m_ResIndex[TargetIndex].ImageH;

        //MIXER_DBG("SizeX:%d, Y:%d, W:%d, H:%d, TargetIndex:%d.\n", mHcRectSize.x, mHcRectSize.y, mHcRectSize.width, mHcRectSize.height, TargetIndex);
    }
    else
    {
        return -1;
    }
    return 0;
}
