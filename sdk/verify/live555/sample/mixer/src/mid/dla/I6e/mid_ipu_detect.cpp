#include "mid_ipu_detect.h"
#include "mid_system_config.h"
#include "mid_VideoEncoder.h"
#include "mid_dla.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/time.h>
#include <unistd.h>

#define INNER_MOST_ALIGNMENT     (8)
#define MID_DETECT_THRESHOLD    (0.45)

#define MAX_DETECT_NUM            (10)

extern int DLAtoRECT(MI_VENC_CHN s32VencChn, int recCnt, ST_DlaRectInfo_T* pRecInfo, MI_BOOL bShow, MI_BOOL bShowBorder);
extern MI_U32 g_rotation;
extern pthread_mutex_t g_mutex_UpadteOsdState;  
extern pthread_cond_t  g_cond_UpadteOsdState;  
extern MI_U32 g_UpadteOsdState;


CMidIPUDetect::CMidIPUDetect(IPU_InitInfo_S &stInitInfo) : CMidIPUInterface(stInitInfo)
{
    m_vpeChn            = 0;
    m_vpePort            = 2;
    m_divpChn            = Mixer_Divp_GetChannleNum();
    m_ipuChn             = 0;
    m_u32OutBufDepth     = 1;
    m_u32InBufDepth        = 1;
    m_u32Width            = 640;
    m_u32Height            = 360;
    m_s32Fd                = -1;
    memset(&m_stNPUDesc, 0, sizeof(MI_IPU_SubNet_InputOutputDesc_t));
    m_s32LabelCount        = 0;

    (void)InitResource();
}

CMidIPUDetect::~CMidIPUDetect()
{
    Mixer_Divp_PutChannleNum(m_divpChn);
    (void)ReleaseResource();
}

int CMidIPUDetect::InitResource()
{
    MI_S32 s32Ret = MI_SUCCESS;
	
    std::ifstream LabelFile;

    if(MI_SUCCESS != IPUCreateDevice(m_stIPUInfo.szIPUfirmware, MAX_VARIABLE_BUF_SIZE))
    {
        MIXER_ERR("IPUCreateDevice error, %s, ret:[0x%x]\n", m_stIPUInfo.szIPUfirmware, s32Ret);
        return -1;
    }

    if(MI_SUCCESS != IPUCreateChannel(&m_ipuChn, m_stIPUInfo.szModelFile, m_u32InBufDepth, m_u32OutBufDepth))
    {
        MIXER_ERR("IPUCreateChannel error, chn:%d, model:%s, ret:[0x%x]\n", m_ipuChn, m_stIPUInfo.szModelFile, s32Ret);
        IPUDestroyDevice();
        return -1;
    }

    if (MI_SUCCESS != (s32Ret = MI_IPU_GetInOutTensorDesc(m_ipuChn, &m_stNPUDesc)))
    {
        MIXER_ERR("MI_IPU_GetInOutTensorDesc error, chn:%d, ret:[0x%x]\n", m_ipuChn, s32Ret);
        IPUDestroyChannel(m_ipuChn, m_u32InBufDepth, m_u32OutBufDepth);
        IPUDestroyDevice();
        return -1;
    }

    MIXER_DBG("H:%d,W:%d,C:%d,format:%s\n", m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1],
                m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2], m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[3],
                (m_stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_U8) ? "MI_IPU_FORMAT_U8" :
                (m_stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_NV12) ? "MI_IPU_FORMAT_NV12" :
                (m_stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_INT16) ? "MI_IPU_FORMAT_INT16" :
                (m_stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_INT32) ? "MI_IPU_FORMAT_INT32" :
                (m_stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_INT8) ? "MI_IPU_FORMAT_INT8" :
                (m_stNPUDesc.astMI_InputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_FP32) ? "MI_IPU_FORMAT_FP32" : "UNKNOWN");

    LabelFile.open(m_stIPUInfo.u.ExtendInfo1.szLabelFile);
    while(1)
    {
        if(LabelFile.eof() || (m_s32LabelCount >= LABEL_CLASS_COUNT))
        {
            break;
        }

        LabelFile.getline(&m_szLabelName[m_s32LabelCount][0], LABEL_NAME_MAX_SIZE);

        MIXER_DBG("m_szLabelName[%d]=%s\n", m_s32LabelCount, m_szLabelName[m_s32LabelCount]);

        m_s32LabelCount++;
    }

    (void)InitStreamSource();

    return MI_SUCCESS;
}

int CMidIPUDetect::ReleaseResource()
{
    (void)IPUDestroyChannel(m_ipuChn, m_u32InBufDepth, m_u32OutBufDepth);
    (void)IPUDestroyDevice();

    (void)ReleaseStreamSource();

    return MI_SUCCESS;
}

void CMidIPUDetect::DealDataProcess()
{
    fd_set read_fds;
    struct timeval tv;
    MI_S32 s32Ret = 0;
    MI_SYS_ChnPort_t stChnOutputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE stBufHandle;

    stChnOutputPort.eModId       = E_MI_MODULE_ID_DIVP;
    stChnOutputPort.u32DevId     = 0;
    stChnOutputPort.u32ChnId     = m_divpChn;
    stChnOutputPort.u32PortId     = 0;

    FD_ZERO(&read_fds);
    FD_SET(m_s32Fd, &read_fds);

    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000;
    s32Ret = select(m_s32Fd + 1, &read_fds, NULL, NULL, &tv);
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
        if(FD_ISSET(m_s32Fd, &read_fds))
        {
            memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
            if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort, &stBufInfo, &stBufHandle))
            {
                usleep(1*1000);
            }
            else
            {
                (void)DoDetect(&stBufInfo);

                if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(stBufHandle))
                {
                    MIXER_ERR("MI_SYS_ChnOutputPortPutBuf error\n");
                }
            }
        }
    }
}

int CMidIPUDetect::InitStreamSource()
{
    MI_VPE_PortMode_t stVpeMode;
    MI_SYS_WindowRect_t stCropWin;
    MI_DIVP_OutputPortAttr_t stDivpPortAttr;
    MI_S32 s32Ret = MI_SUCCESS;
    Mixer_Sys_BindInfo_T stBindInfo;
    MI_SYS_ChnPort_t stChnOutputPort;

    memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
    ExecFunc(MI_VPE_GetPortMode(m_vpeChn, m_vpePort, &stVpeMode), MI_VPE_OK);

    stCropWin.u16Width = stVpeMode.u16Width;
    stCropWin.u16Height = stVpeMode.u16Height;
    stCropWin.u16X = 0;
    stCropWin.u16Y = 0;
    MIXERCHECKRESULT(Mixer_Divp_CreatChannel(m_divpChn, (MI_SYS_Rotate_e)0x0, &stCropWin));

    if(((g_rotation & 0xFFFF) == 90) || ((g_rotation & 0xFFFF) == 270))
    {
        stDivpPortAttr.u32Width      = m_u32Height;
        stDivpPortAttr.u32Height     = m_u32Width;
    }
    else
    {
        stDivpPortAttr.u32Width      = m_u32Width;
        stDivpPortAttr.u32Height     = m_u32Height;
    }

    stDivpPortAttr.eCompMode     = E_MI_SYS_COMPRESS_MODE_NONE;
	stDivpPortAttr.ePixelFormat	= E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;//E_MI_SYS_PIXEL_FRAME_ARGB8888;

    MIXERCHECKRESULT(Mixer_Divp_SetOutputAttr(m_divpChn, &stDivpPortAttr));
    MIXERCHECKRESULT(Mixer_Divp_StartChn(m_divpChn));

    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId  = 0;
    stBindInfo.stSrcChnPort.u32ChnId  = m_vpeChn;
    stBindInfo.stSrcChnPort.u32PortId = m_vpePort;

    stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId  = 0;
    stBindInfo.stDstChnPort.u32ChnId  = m_divpChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stBindInfo.stDstChnPort, 3, 3), MI_SUCCESS);

    stBindInfo.u32SrcFrmrate = 30; //MI_VideoEncoder::vpeframeRate;
    stBindInfo.u32DstFrmrate = 30; //MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#endif
    MIXERCHECKRESULT(Mixer_Sys_Bind(&stBindInfo));

    stChnOutputPort.eModId       = E_MI_MODULE_ID_DIVP;
    stChnOutputPort.u32DevId     = 0;
    stChnOutputPort.u32ChnId     = m_divpChn;
    stChnOutputPort.u32PortId     = 0;
    s32Ret = MI_SYS_GetFd(&stChnOutputPort, &m_s32Fd);
    if (s32Ret < 0)
    {
        MIXER_ERR("divp ch: %d, get fd. err\n", stChnOutputPort.u32ChnId);
        return -1;
    }

    MIXER_DBG("m_s32Fd:%d\n", m_s32Fd);

    return MI_SUCCESS;
}

int CMidIPUDetect::ReleaseStreamSource()
{
    Mixer_Sys_BindInfo_T stBindInfo;

    if (m_s32Fd > 0)
    {
        MI_SYS_CloseFd(m_s32Fd);
        m_s32Fd = -1;
    }

    memset(&stBindInfo, 0x00, sizeof(Mixer_Sys_BindInfo_T));

    stBindInfo.stSrcChnPort.eModId    = E_MI_MODULE_ID_VPE;
    stBindInfo.stSrcChnPort.u32DevId  = 0;
    stBindInfo.stSrcChnPort.u32ChnId  = m_vpeChn;
    stBindInfo.stSrcChnPort.u32PortId = m_vpePort;

    stBindInfo.stDstChnPort.eModId    = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId  = 0;
    stBindInfo.stDstChnPort.u32ChnId  = m_divpChn;
    stBindInfo.stDstChnPort.u32PortId = 0;

    ExecFunc(MI_SYS_SetChnOutputPortDepth(&stBindInfo.stDstChnPort, 3, 3), MI_SUCCESS);

    stBindInfo.u32SrcFrmrate = 30; //MI_VideoEncoder::vpeframeRate;
    stBindInfo.u32DstFrmrate = 15; //MI_VideoEncoder::vpeframeRate;
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E
    stBindInfo.eBindType = E_MI_SYS_BIND_TYPE_FRAME_BASE;
#endif
    MIXERCHECKRESULT(Mixer_Sys_UnBind(&stBindInfo));

    MIXERCHECKRESULT(Mixer_Divp_StopChn(m_divpChn));
    MIXERCHECKRESULT(Mixer_Divp_DestroyChn(m_divpChn));

    return MI_SUCCESS;
}
MI_S32 CMidIPUDetect:: SetIeParam(IeParamInfo tmp,MI_U8 scop)
{
  return 0;
}
int CMidIPUDetect::DoDetect(MI_SYS_BufInfo_t* pstBufInfo)
{
    MI_IPU_TensorVector_t stInputTensorVector;
    MI_IPU_TensorVector_t stOutputTensorVector;
    MI_S32 s32Ret = MI_SUCCESS;

    if (pstBufInfo == NULL)
    {
        return MI_SUCCESS;
    }

    memset(&stInputTensorVector, 0, sizeof(MI_IPU_TensorVector_t));
	memset(&stOutputTensorVector, 0, sizeof(MI_IPU_TensorVector_t));
	if(E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420 != pstBufInfo->stFrameData.ePixelFormat)
    {
    if (MI_SUCCESS != (s32Ret = MI_IPU_GetInputTensors(m_ipuChn, &stInputTensorVector)))
    {
        MIXER_ERR("MI_IPU_GetInputTensors error, ret[0x%x]\n", s32Ret);
        return -1;
    }

    (void)ScaleToModelSize(pstBufInfo, &stInputTensorVector);
    }
	else
	{
	    stInputTensorVector.u32TensorCount = m_stNPUDesc.u32InputTensorCount;
	    if (pstBufInfo->eBufType == E_MI_SYS_BUFDATA_RAW)
	    {
	        //MIXER_DBG("E_MI_SYS_BUFDATA_RAW\n");
	        stInputTensorVector.astArrayTensors[0].phyTensorAddr[0] = pstBufInfo->stRawData.phyAddr;
	        stInputTensorVector.astArrayTensors[0].ptTensorData[0] = pstBufInfo->stRawData.pVirAddr;
	    }
	    else if(pstBufInfo->eBufType == E_MI_SYS_BUFDATA_FRAME)
	    {
	        //MIXER_DBG("E_MI_SYS_BUFDATA_FRAME\n");
	        stInputTensorVector.astArrayTensors[0].phyTensorAddr[0] = pstBufInfo->stFrameData.phyAddr[0];
	        stInputTensorVector.astArrayTensors[0].ptTensorData[0] = pstBufInfo->stFrameData.pVirAddr[0];

	        stInputTensorVector.astArrayTensors[0].phyTensorAddr[1] = pstBufInfo->stFrameData.phyAddr[1];
	        stInputTensorVector.astArrayTensors[0].ptTensorData[1] = pstBufInfo->stFrameData.pVirAddr[1];
	        //save_buffer_to_file(stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32BufSize);
	    }
	}
    if (MI_SUCCESS != (s32Ret = MI_IPU_GetOutputTensors(m_ipuChn, &stOutputTensorVector)))
    {
        MIXER_ERR("MI_IPU_GetOutputTensors error, ret[0x%x]\n", s32Ret);
        MI_IPU_PutInputTensors(m_ipuChn, &stInputTensorVector);
        return -1;
    }

    if(MI_SUCCESS != (s32Ret = MI_IPU_Invoke(m_ipuChn, &stInputTensorVector, &stOutputTensorVector)))
    {
        MIXER_ERR("MI_IPU_Invoke error, ret[0x%x]\n", s32Ret);
        MI_IPU_PutOutputTensors(m_ipuChn,&stOutputTensorVector);
        MI_IPU_PutInputTensors(m_ipuChn, &stInputTensorVector);
        return -1;
    }

    PrintResult(&stOutputTensorVector);

    MI_IPU_PutInputTensors(m_ipuChn, &stInputTensorVector);
    MI_IPU_PutOutputTensors(m_ipuChn,&stOutputTensorVector);

    return MI_SUCCESS;
}
void CMidIPUDetect::PrintResult(MI_IPU_TensorVector_t* pstOutputTensorVector)
{
    size_t j = 0;
    MI_S32 count = 0;
	 std::vector <std::vector<TrackBBox>> detFrameDatas;
	std::vector <TrackBBox> detFrameData;
	
	ST_DlaRectInfo_T stRectInfo[MAX_DETECT_NUM];
	
    float *pfBBox = (float *)pstOutputTensorVector->astArrayTensors[0].ptTensorData[0];
    float *pfClass = (float *)pstOutputTensorVector->astArrayTensors[1].ptTensorData[0];
    float *pfScore = (float *)pstOutputTensorVector->astArrayTensors[2].ptTensorData[0];
    float *pfDetect = (float *)pstOutputTensorVector->astArrayTensors[3].ptTensorData[0];

    std::vector<DetectionBBoxInfo >detections = GetDetections(pfBBox,pfClass,  pfScore,  pfDetect);

    MIXER_DBG(" size:%d\n", detections.size());
	

     for (j = 0; j < detections.size(); j++)
     {
        DetectionBBoxInfo bbox;
		TrackBBox cur_box;
		memset(&cur_box,0x00,sizeof(TrackBBox));
        const int label = detections[j].classID;
        const float score = detections[j].score;
        if (score < MID_DETECT_THRESHOLD /*|| label == 0*/)
        {
            continue;
        }

        bbox.xmin = detections[j].xmin * m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2]; // width/col
        bbox.xmin = bbox.xmin < 0 ? 0 : bbox.xmin;

        bbox.ymin = detections[j].ymin * m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1]; // height/row
        bbox.ymin = bbox.ymin < 0 ? 0 : bbox.ymin;

        bbox.xmax = detections[j].xmax * m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2];
        bbox.xmax = (bbox.xmax > m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2]) ?
                    m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2] : bbox.xmax;

        bbox.ymax = detections[j].ymax * m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1];
        bbox.ymax = (bbox.ymax > m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1]) ?
                    m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1] : bbox.ymax ;

        bbox.score = score;
		cur_box.x = bbox.xmin;
		cur_box.y = bbox.ymin;
		cur_box.w = bbox.xmax-bbox.xmin;
		cur_box.h = bbox.ymax-bbox.ymin;
		cur_box.score = bbox.score;
	    cur_box.classID =  label;

		detFrameData.push_back(cur_box);
		
		MIXER_DBG("label = %d, score=%f [%.2f %.2f %.2f %.2f] \n",label, score, cur_box.x, cur_box.y, cur_box.w, cur_box.h);
		//detFrameDatas[label].push_back(cur_box);
      }
	  count = 0;
      string strLabelName = "";
      if(detFrameData.size())
      {
	     detFrameDatas.push_back(detFrameData);
      }
	  MIXER_DBG("detFrameDatas.size=%d\n",detFrameDatas.size());
      m_DetectTrackBBoxs = m_DetectBBoxTracker.track_iou(detFrameDatas);
	  for(MI_U8 i=0; i < m_DetectTrackBBoxs.size() && detFrameDatas.size(); i++)
	  {
	      int index = m_DetectTrackBBoxs[i].boxes.size()-1;
		  
		  TrackBBox & trackBox =  m_DetectTrackBBoxs[i].boxes[index];
		  TrackBBox bboxes = trackBox;
		  if ((bboxes.classID >= 0) && (bboxes.classID < m_s32LabelCount))
          {
            strLabelName = m_szLabelName[bboxes.classID];
          }
		  if (count > (MAX_DETECT_NUM-1))
		  {
			 break;
		  }
		memset(&stRectInfo[count], 0, sizeof(ST_DlaRectInfo_T));
		// convert to 1920x1080
		stRectInfo[count].rect.u32X = bboxes.x * 1920 / m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2];
		stRectInfo[count].rect.u32Y = bboxes.y * 1080 / m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1];
		stRectInfo[count].rect.u16PicW = (bboxes.w) * 1920 / m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2];
		stRectInfo[count].rect.u16PicH = (bboxes.h) * 1080 / m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1];
		// snprintf(stRectInfo[count].szObjName, sizeof(stRectInfo[count].szObjName) - 1, "%s: %.2f", strLabelName.c_str(), bboxes.score);
        snprintf(stRectInfo[count].szObjName, sizeof(stRectInfo[count].szObjName) - 1, "%s ", strLabelName.c_str());

		 count++;
		MIXER_DBG("bboxes.classID=%d  name:%s\n",bboxes.classID,strLabelName.c_str());
	   }
	  	DLAtoRECT(0, count, stRectInfo, TRUE, TRUE);
	    pthread_mutex_lock(&g_mutex_UpadteOsdState);  	
	    g_UpadteOsdState++;
	    pthread_cond_signal(&g_cond_UpadteOsdState);  
        pthread_mutex_unlock(&g_mutex_UpadteOsdState);  
/*  	count = 0; 
	for (map<int, vector<DetectionBBoxInfo> >::iterator it =
         detectionsInImage.begin(); it != detectionsInImage.end(); ++it)
    {
        int label = it->first;
        string strLabelName = "Unknown";
        const vector<DetectionBBoxInfo>& bboxes = it->second;

        if (label >= 0 && label < m_s32LabelCount)
        {
            strLabelName = m_szLabelName[label];
        }

        for (j = 0; j < bboxes.size(); ++j)
        {
            if (count >= (MAX_DETECT_NUM-1))
            {
                break;
            }

            memset(&stRectInfo[count], 0, sizeof(ST_DlaRectInfo_T));
            // convert to 1920x1080
            stRectInfo[count].rect.u32X = bboxes[j].xmin * 1920 / m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2];
            stRectInfo[count].rect.u32Y = bboxes[j].ymin * 1080 / m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1];
            stRectInfo[count].rect.u16PicW = (bboxes[j].xmax - bboxes[j].xmin) * 1920 / m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2];
            stRectInfo[count].rect.u16PicH = (bboxes[j].ymax - bboxes[j].ymin) * 1080 / m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1];
            snprintf(stRectInfo[count].szObjName, sizeof(stRectInfo[count].szObjName) - 1,
                    "%s: %.2f", strLabelName.c_str(), bboxes[j].score);
        }

        count ++;
    }
    DLAtoRECT(0, count, stRectInfo, TRUE, TRUE);
*/
}

int CMidIPUDetect::ScaleToModelSize(MI_SYS_BufInfo_t* pstBufInfo, MI_IPU_TensorVector_t* pstInputTensorVector)
{
    unsigned char *pSrcImage = NULL;
    unsigned char *pDst = NULL;

    #if 0
    static MI_S32 debug_i = 0;
    char szDebugName[64] = {0, };
    #endif

    if ((pstBufInfo == NULL) ||
        (pstInputTensorVector == NULL))
    {
        return -1;
    }

    // MIXER_DBG("eBufType:%d, ePixelFormat:%d\n", pstBufInfo->eBufType, pstBufInfo->stFrameData.ePixelFormat);
    switch(pstBufInfo->eBufType)
    {
        case E_MI_SYS_BUFDATA_RAW:
        {
            // stInputTensorVector.stArrayTensors[0].phyTensorAddr[0] = pstBufInfo->stRawData.phyAddr;
            // stInputTensorVector.stArrayTensors[0].pstTensorData[0] = pstBufInfo->stRawData.pVirAddr;
        }
        break;

        case E_MI_SYS_BUFDATA_FRAME:
        {
            if ((pstBufInfo->stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_ABGR8888) ||
                (pstBufInfo->stFrameData.ePixelFormat == E_MI_SYS_PIXEL_FRAME_ARGB8888))
            {
                pSrcImage = (unsigned char *)pstBufInfo->stFrameData.pVirAddr[0];
            }
        }
        break;
        default:
        {
            MIXER_ERR("invalid buf type, %d\n", (int)pstBufInfo->eBufType);
            return -1;
        }
    }

    // model channel
	if ((m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[3] == 3)&&(NULL != pSrcImage))
    {
        cv::Mat srcMat(pstBufInfo->stFrameData.u16Height, pstBufInfo->stFrameData.u16Width ,CV_8UC4, pSrcImage);
        cv::Mat resizeMat;

        if (((MI_U32)srcMat.size().width != m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2]) &&
            ((MI_U32)srcMat.size().height != m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1]))
        {
            cv::resize(srcMat, resizeMat, cv::Size(m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[2],
                                                    m_stNPUDesc.astMI_InputTensorDescs[0].u32TensorShape[1]));
        }
        else
        {
            resizeMat = srcMat;
        }

        #if 0
        memset(szDebugName, 0, sizeof(szDebugName));
        snprintf(szDebugName, sizeof(szDebugName) - 1, "resizeMat_%04d.bmp", debug_i ++);
        cv::imwrite(szDebugName, resizeMat);
        #endif

        pSrcImage = (unsigned char *)resizeMat.data;
        pDst = (unsigned char *)pstInputTensorVector->astArrayTensors[0].ptTensorData[0];
        memcpy(pDst, pSrcImage, resizeMat.size().width * resizeMat.size().height * 4);

        #if 0
        int fd = -1;
        memset(szDebugName, 0, sizeof(szDebugName));
        snprintf(szDebugName, sizeof(szDebugName) - 1, "%04d.raw", debug_i ++);
        fd = open(szDebugName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd > 0)
        {
            write(fd, pSrcImage, resizeMat.size().width * resizeMat.size().height * 4);
            close(fd);
        }
        #endif
    }

    return MI_SUCCESS;
}

std::vector<DetectionBBoxInfo > CMidIPUDetect::GetDetections(float *pfBBox, float *pfClass, float *pfScore, float *pfDetect)
{
    // show bbox
    int s32DetectCount = round(*pfDetect);
    std::vector<DetectionBBoxInfo > detections(s32DetectCount);

    for(int i=0;i<s32DetectCount;i++)
    {
        DetectionBBoxInfo  detection;
        memset(&detection,0,sizeof(DetectionBBoxInfo));
        //box coordinate
        detection.ymin =  *(pfBBox+(i*ALIGN_UP(4,INNER_MOST_ALIGNMENT))+0);
        detection.xmin =  *(pfBBox+(i*ALIGN_UP(4,INNER_MOST_ALIGNMENT))+1);
        detection.ymax =  *(pfBBox+(i*ALIGN_UP(4,INNER_MOST_ALIGNMENT))+2);
        detection.xmax =  *(pfBBox+(i*ALIGN_UP(4,INNER_MOST_ALIGNMENT))+3);

        //box class
        detection.classID = round(*(pfClass+i));

        //score
        detection.score = *(pfScore+i);
        detections.push_back(detection);
    }

    return detections;
}

