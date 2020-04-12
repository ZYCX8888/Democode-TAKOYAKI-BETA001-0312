#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "mi_common.h"
#include "mi_sys.h"

#include "fdfr.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<stdio.h>
#include<sys/time.h>

#include<unistd.h>
#include "FaceDatabase.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
static int Initial = 0;


#define INNER_MOST_ALIGNMENT (8)
#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))

Fdfr::Fdfr()
{
    FdaChn = 0;
    FrChn = 0;

    Threshold = 0.5;
}
Fdfr::~Fdfr()
{
}

void Fdfr::Init()
{
    int Depth = 3, BaseBufSize = 1024*1024*2, s32Ret;

    intImgWidth = 1920;
    intImgHeight = 1080;
    fScoreThreshold = 0.0f;


    if (Initial)
        return;
    Initial = 1;

    stDevAttr.u32MaxVariableBufSize = 2097152; // fr + fda
    stDevAttr.u32YUV420_W_Pitch_Alignment = 16;
    stDevAttr.u32YUV420_H_Pitch_Alignment = 2;
    stDevAttr.u32XRGB_W_Pitch_Alignment = 16;
    s32Ret = MI_IPU_CreateDevice(&stDevAttr, NULL, "ipu_firmware.bin.shrink", 0);

    if (s32Ret != MI_SUCCESS) {
        printf("fail to create ipu device\n");
        return;
    }

    memset(&FdaChnAttr, 0, sizeof(FdaChnAttr));
    FdaChnAttr.u32InputBufDepth = 0;
    FdaChnAttr.u32OutputBufDepth = Depth;
    s32Ret = MI_IPU_CreateCHN(&FdaChn, &FdaChnAttr, NULL, "caffe_fda_fixed.tflite_sgsimg.img");

    if (s32Ret != MI_SUCCESS) {
        printf("fail to create ipu channel%d\n", FdaChn);
        MI_IPU_DestroyDevice();
        return;
    }

    memset(&FrChnAttr, 0, sizeof(FrChnAttr));
    FrChnAttr.u32InputBufDepth = Depth;
    FrChnAttr.u32OutputBufDepth = Depth;
    s32Ret = MI_IPU_CreateCHN(&FrChn, &FrChnAttr, NULL, "caffe_fr_fixed.tflite_sgsimg.img");

    if (s32Ret != MI_SUCCESS) {
        printf("fail to create ipu channel%d\n", FrChn);
        MI_IPU_DestroyCHN(FdaChn);
        MI_IPU_DestroyDevice();
        return;
    }

    s32Ret = MI_IPU_GetInOutTensorDesc(FdaChn, &FdaDesc);
    if (s32Ret) {
        FDAFR_FUNC_INFO("fail to get network(%d) description\n", FdaChn);
        MI_IPU_DestroyCHN(FdaChn);
        MI_IPU_DestroyCHN(FrChn);
        MI_IPU_DestroyDevice();
        return;
    }

    s32Ret = MI_IPU_GetInOutTensorDesc(FrChn, &FrDesc);
    if (s32Ret) {
        printf("fail to get network(%d) description\n", FrChn);
        MI_IPU_DestroyCHN(FdaChn);
        MI_IPU_DestroyCHN(FrChn);
        MI_IPU_DestroyDevice();
        return;
    }

    face_db.LoadFromFileBinay("feat.bin", "name.list");
    int num = face_db.persons.size();
    FDAFR_FUNC_INFO("%d \n", num);
    for (int i = 0; i < num; i++) {
        FDAFR_FUNC_INFO("%s \n", face_db.persons[i].name.c_str());
        int feat_num = face_db.persons[i].features.size();
        for (int j = 0; j < feat_num; j++) {
            FDAFR_FUNC_INFO("%d %f %f \n", face_db.persons[i].features[j].length, face_db.persons[i].features[j].pData[0], face_db.persons[i].features[j].pData[1]);
            FDAFR_FUNC_INFO("%s \n", face_db.persons[i].name.c_str());
        }
    }
    FDAFR_FUNC_INFO("%d %f %f \n", face_db.persons[0].features[0].length, face_db.persons[0].features[0].pData[0], face_db.persons[0].features[0].pData[1]);

}
void Fdfr::Deinit()
{

    MI_IPU_DestroyCHN(FdaChn);
    MI_IPU_DestroyCHN(FrChn);
    MI_IPU_DestroyDevice();
}
void Fdfr::Start()
{
    ST_TEM_ATTR stTemAttr;
    PTH_RET_CHK(pthread_attr_init(&stTemAttr.thread_attr));

    memset(&stTemAttr, 0, sizeof(ST_TEM_ATTR));
    stTemAttr.fpThreadDoSignal = NULL;
    stTemAttr.fpThreadWaitTimeOut = DataMonitor;
    stTemAttr.u32ThreadTimeoutMs = 10;
    stTemAttr.bSignalResetTimer = 0;
    stTemAttr.stTemBuf.pTemBuffer = (void *)this;
    stTemAttr.stTemBuf.u32TemBufferSize = 0;
    TemOpen(stModDesc.modKeyString.c_str(), stTemAttr);
    TemStartMonitor(stModDesc.modKeyString.c_str());
}
void Fdfr::Stop()
{
    TemStop(stModDesc.modKeyString.c_str());
    TemClose(stModDesc.modKeyString.c_str());
}
void Fdfr::BindBlock(stModInputInfo_t &stIn)
{
    stFdfrInputInfo_t stFdfrInputInfo;
    stModDesc_t stPreModDesc;
    MI_SYS_ChnPort_t stChnPort;

    memset(&stChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    memset(&stFdfrInputInfo, 0, sizeof(stFdfrInputInfo_t));
    GetInstance(stIn.stPrev.modKeyString)->GetModDesc(stPreModDesc);
    stFdfrInputInfo.intPreChn = (unsigned int)stPreModDesc.chnId;
    stFdfrInputInfo.intPreDev= (unsigned int)stPreModDesc.devId;
    stFdfrInputInfo.intPreMod = (unsigned int)stPreModDesc.modId;
    stFdfrInputInfo.intPreOutPort = (unsigned int)stIn.stPrev.portId;
    stFdfrInputInfo.intInPort = stIn.curPortId;
    vectFdFrInput.push_back(stFdfrInputInfo);
    stChnPort.eModId = (MI_ModuleId_e)stFdfrInputInfo.intPreMod;
    stChnPort.u32ChnId = (MI_U32)stFdfrInputInfo.intPreChn;
    stChnPort.u32PortId = (MI_U32)stFdfrInputInfo.intPreOutPort;
    stChnPort.u32DevId = (MI_U32)stFdfrInputInfo.intPreDev;
    MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 3);

}
void Fdfr::UnBindBlock(stModInputInfo_t &stIn)
{
    std::vector<stFdfrInputInfo_t>::iterator it;

    for (it = vectFdFrInput.begin(); it != vectFdFrInput.end(); it++)
    {
        if (it->intInPort == stIn.curPortId)
        {
            vectFdFrInput.erase(it);

            break;
        }
    }
}


std::string Fdfr::getFaceNamebyFeature(void* feat1) {
    return faceRecognizer.find_name(face_db, (float *)feat1);
}

int g_new_add_userid = -1;
char g_new_add_name[256]={0};
char g_new_del_name[256]={0};

void Fdfr::DoRecognition(MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *inputVector, MI_IPU_TensorVector_t *outputVector)
{
    void* pimg;
    int s32Ret, count = vecTrackBBoxs.size();
    std::map<int, stCountName_t> mapLastIDName;
    mapLastIDName.swap(mapCurIDName);
    vectResult.clear();
    bool bDataBaseChanged = FALSE;
    static char id_string[128];

    if (stBufInfo.eBufType == E_MI_SYS_BUFDATA_RAW) {
        pimg = stBufInfo.stRawData.pVirAddr;
    } else if (stBufInfo.eBufType == E_MI_SYS_BUFDATA_FRAME) {
        pimg = stBufInfo.stFrameData.pVirAddr[0];
    }

    for(int i=0;i<count;i++)
    {
        int id = vecTrackBBoxs[i].id;
        stCountName_t countname = SearchNameByID(mapLastIDName, id);
        int index = vecTrackBBoxs[i].boxes.size()-1;
        TrackBBox & trackBox = vecTrackBBoxs[i].boxes[index];
        bool bForceFR = FALSE;

        if(id==g_new_add_userid && strlen(g_new_add_name))
        {
            bForceFR = TRUE;
        }

        if(countname.Name.empty() == FALSE && bForceFR==FALSE)
        {
            mapCurIDName.insert({id, countname});
        }
        else
        {
            FDAFR_FUNC_INFO("3==%d==+++++++++  X: %.2f  Y: %.2f  H: %.2f  W: %.2f  ++++++++++\n", i, trackBox.x, trackBox.y, trackBox.h, trackBox.w);

            if (trackBox.h < 120 && trackBox.w < 120) {
                goto SHOWFACE;
            }

            if(fr_ePixelFormat==E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
            {

                cv::Mat frame(intImgHeight*3/2, intImgWidth ,CV_8UC1, pimg,ALIGN_UP(intImgWidth,16));
                unsigned char* pimg112 = (unsigned char* )inputVector->astArrayTensors[0].ptTensorData[0];
                cv::Mat crop(112*3/2, 112, CV_8UC1, pimg112,ALIGN_UP(112,16));
                float face5point[10] =
                {
                    trackBox.lm1_x, trackBox.lm1_y,
                    trackBox.lm2_x, trackBox.lm2_y,
                    trackBox.lm3_x, trackBox.lm3_y,
                    trackBox.lm4_x, trackBox.lm4_y,
                    trackBox.lm5_x, trackBox.lm5_y
                };


                FaceRecognizeUtils::CropImage_112x112_YUV420_NV12(frame, face5point, crop);
                #if 0
                {
                    cv::Mat BGR_crop;
                    char char_crop[32];
                    sprintf(char_crop, "crop/crop_%d.jpg", nnncrop++);
                    cv::cvtColor(crop, BGR_crop, cv::COLOR_YUV2BGR_NV12);

                    cv::imwrite(char_crop,BGR_crop);
                    if (nnncrop == 50) {
                      nnncrop = 0;
                    }
                }
                #endif
            }
            else if(fr_ePixelFormat==E_MI_SYS_PIXEL_FRAME_ARGB8888 ||
                    fr_ePixelFormat==E_MI_SYS_PIXEL_FRAME_BGRA8888)
            {
                cv::Mat frame(intImgHeight, intImgWidth ,CV_8UC4, pimg);
                unsigned char* pimg112 = (unsigned char* )inputVector->astArrayTensors[0].ptTensorData[0];
                cv::Mat crop(112, 112, CV_8UC4, pimg112);

                float face5point[10] =
                {
                    trackBox.lm1_x, trackBox.lm1_y,
                    trackBox.lm2_x, trackBox.lm2_y,
                    trackBox.lm3_x, trackBox.lm3_y,
                    trackBox.lm4_x, trackBox.lm4_y,
                    trackBox.lm5_x, trackBox.lm5_y
                };


                FaceRecognizeUtils::CropImage_112x112<float>(frame, face5point, crop);
                #if 0
                {
                    char char_crop[32];
                    sprintf(char_crop, "crop/crop_%d.jpg", nnncrop++);
                    cv::imwrite(char_crop, crop);
                    //printf("\n====save codes\n");
                    if (nnncrop == 50) {
                       nnncrop = 0;
                    }
                }
                #endif
           }




            s32Ret = MI_IPU_Invoke(FrChn, inputVector, outputVector);
            if (s32Ret != MI_SUCCESS) {
                FDAFR_FUNC_INFO("FR invoke error:%d\n", s32Ret);
                continue;
            }

            for (unsigned int i = 0; i < outputVector->u32TensorCount; i++) {
                countname.Name = getFaceNamebyFeature((float*)outputVector->astArrayTensors[0].ptTensorData[0]);
                if (countname.Name.empty() == FALSE )
                {
                    mapCurIDName.insert({id, countname});
                }
                else
                {
                    countname.Name = "unknown";
                    mapCurIDName.insert({id, countname});
                }

                if((bForceFR && !strcmp(countname.Name.c_str(), "unknown"))||((strcmp(countname.Name.c_str(),g_new_add_name)==0)&&(strlen(g_new_add_name)!=0)))
                 {
                    face_db.AddPersonFeature(g_new_add_name, (float*)outputVector->astArrayTensors[0].ptTensorData[0]);
                    printf("add %s to database\n",g_new_add_name);
                    bDataBaseChanged = TRUE;
                 }
            }
        }
SHOWFACE:
        //save result info
        stFaceInfo_t stFaceInfo;
        memset(&stFaceInfo, 0, sizeof(stFaceInfo_t));
        // box

        stFaceInfo.faceH = trackBox.h;
        stFaceInfo.faceW = trackBox.w;
        stFaceInfo.xPos  = trackBox.x;
        stFaceInfo.yPos  = trackBox.y;
        stFaceInfo.winWid = intImgWidth;
        stFaceInfo.winHei = intImgHeight;
        vectResult.push_back(stFaceInfo);
        if(countname.Name.empty())
        {
            sprintf(id_string,"%sID :%d","unknown",id);
            strncpy(stFaceInfo.faceName,id_string,sizeof(stFaceInfo.faceName)-1);
        } else
        {
            sprintf(id_string,"%sID :%d", countname.Name.c_str(),id);
            strncpy(stFaceInfo.faceName,id_string,sizeof(stFaceInfo.faceName)-1);
        }

        vectResult.push_back(stFaceInfo);

    }


//show fps
//    static   struct  timeval    tv_start;
//    struct timeval tv_end;
//     static int frame_count = 0;
//     if(frame_count==0)
//     {
//        gettimeofday(&tv_start,NULL);
//     }
//
//     frame_count ++ ;
//      if (frame_count==20)
//      {
//         gettimeofday(&tv_end,NULL);
//         int elasped_time = (tv_end.tv_sec-tv_start.tv_sec)*1000+(tv_end.tv_usec-tv_start.tv_usec)/1000;
//         float fps = (1000.0/elasped_time)*20;
//         TIME_FUNC_INFO("====fps is %f\n",fps);
//         frame_count = 0;
//      }




    g_new_add_userid = -1;
    g_new_add_name[0] = 0;
    if(g_new_del_name[0])
    {
         face_db.DelPerson(g_new_del_name);
         printf("del persion %s\n",g_new_del_name);
         g_new_del_name[0] = 0;
         bDataBaseChanged = TRUE;
    }


    if (bDataBaseChanged)
    {
        face_db.SaveToFileBinary("feat.bin", "name.list");
        mapCurIDName.clear();

    }

    std::map<int, stCountName_t>::iterator iter_end = mapCurIDName.begin();
    for (; iter_end != mapCurIDName.end(); ) {
        iter_end->second.Count++;
        if ((iter_end->second.Count % 10 == 0)  && (!strcmp(iter_end->second.Name.c_str(), "unknown"))) {
            mapCurIDName.erase(iter_end++);
        }
        else
        {
            iter_end++;
        }
    }



}

stCountName_t Fdfr::SearchNameByID(std::map<int, stCountName_t> &mapLastIDName, int id)
{
    std::map<int, stCountName_t>::iterator iter;
    iter = mapLastIDName.find(id);
    if(iter==mapLastIDName.end())
    {
       stCountName_t countname;
       countname.Count = 0;
       countname.Name = "";
       return countname;
    }
    else
    {
        return iter->second;
    }
}

void Fdfr::DoTrack(stFdaOutputDataDesc_t *pstFdaOutputData)
{
    std::vector<TrackBBox> vecTrackBoxs;
    std::vector<DetBBox> tempDetboxes;

    std::vector <std::vector<TrackBBox>> detFrameDatas;
    std::vector <TrackBBox> detFrameData;

    SaveFdaOutData(pstFdaOutputData,tempDetboxes);
    vecDetBBoxs.clear();
    int count = tempDetboxes.size();
    FDAFR_FUNC_INFO("demo count = %d \n", count);

    for (int i = 0; i < count; i++)
    {
        DetBBox detbox = tempDetboxes[i];
        FDAFR_FUNC_INFO("2==%d==+++++++++  X1: %.2f  Y1: %.2f  X2: %.2f  Y2: %.2f  ++++++++++\n", i, detbox.x1, detbox.y1, detbox.x2, detbox.y2);
        if (detbox.score > 0.7)
        {
            if (detbox.x1 < 0 || detbox.y1< 0 || detbox.x2 > intImgWidth|| detbox.y2> intImgHeight)
                continue;
            TrackBBox cur_box;
            memcpy(&cur_box, (TrackBBox *)&detbox, sizeof(TrackBBox));
            cur_box.x = detbox.x1;
            cur_box.y = detbox.y1;
            cur_box.w = detbox.x2 - detbox.x1;
            cur_box.h = detbox.y2 - detbox.y1;
            cur_box.score = detbox.score;
            detFrameData.push_back(cur_box);
            vecDetBBoxs.push_back(detbox);

       }
   }

   detFrameDatas.push_back(detFrameData);
   vecTrackBBoxs = BBoxTracker.track_iou(detFrameDatas);
}


std::vector<DetBBox> &Fdfr::SaveFdaOutData(stFdaOutputDataDesc_t *pstFdaOutputData,std::vector<DetBBox> & detboxes)
{
    float fcount = *(pstFdaOutputData->pDectCount);
    int count = fcount;


    FDAFR_FUNC_INFO("=============face count%d============\n", count);
    detboxes.clear();
    for (int i = 0; i < count; i++)
    {
        DetBBox box;
        if (*(pstFdaOutputData->pfScores + i) > fScoreThreshold) {
            box.y1 = *(pstFdaOutputData->pfBBox + i * ALIGN_UP(4, INNER_MOST_ALIGNMENT) ) * intImgHeight;
            box.x1 = *(pstFdaOutputData->pfBBox + i * ALIGN_UP(4, INNER_MOST_ALIGNMENT) + 1) * intImgWidth;
            box.y2 = *(pstFdaOutputData->pfBBox + i * ALIGN_UP(4, INNER_MOST_ALIGNMENT) + 2) * intImgHeight;
            box.x2 = *(pstFdaOutputData->pfBBox + i * ALIGN_UP(4, INNER_MOST_ALIGNMENT) + 3) * intImgWidth;
            box.score = *(pstFdaOutputData->pfScores+ i);
            box.lm1_x = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT)) * intImgWidth;
            box.lm1_y = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 1) * intImgHeight;
            box.lm2_x = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 2) * intImgWidth;
            box.lm2_y = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 3) * intImgHeight;
            box.lm3_x = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 4) * intImgWidth;
            box.lm3_y = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 5) * intImgHeight;
            box.lm4_x = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 6) * intImgWidth;
            box.lm4_y = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 7) * intImgHeight;
            box.lm5_x = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 8) * intImgWidth;
            box.lm5_y = *(pstFdaOutputData->pfLms + i * ALIGN_UP(10,INNER_MOST_ALIGNMENT) + 9) * intImgHeight;
            FDAFR_FUNC_INFO("1==%d==+++++++++  X1: %.2f  Y1: %.2f  X2: %.2f  Y2: %.2f  ++++++++++\n", i, box.x1, box.y1, box.x2, box.y2);
            FDAFR_FUNC_INFO("2==%d==+++++++++ ( X1: %.2f  Y1: %.2f)   ++++++++++\n", i, box.lm1_x, box.lm1_y);
            FDAFR_FUNC_INFO("2==%d==+++++++++ ( X2: %.2f  Y2: %.2f)   ++++++++++\n", i, box.lm2_x, box.lm2_y);
            FDAFR_FUNC_INFO("2==%d==+++++++++ ( X3: %.2f  Y3: %.2f)   ++++++++++\n", i, box.lm3_x, box.lm3_y);
            FDAFR_FUNC_INFO("2==%d==+++++++++ ( X4: %.2f  Y4: %.2f)   ++++++++++\n", i, box.lm4_x, box.lm4_y);
            FDAFR_FUNC_INFO("2==%d==+++++++++ ( X5: %.2f  Y5: %.2f)   ++++++++++\n", i, box.lm5_x, box.lm5_y);
            detboxes.push_back(box);
        }
    }

    return detboxes;

}



int save_buffer_to_file(void *buffer, int len)
{
    int fd,ret;

    fd = open("/customer/dla/buffer.bin", O_RDWR |O_TRUNC);
    if (fd < 0) {
        FDAFR_FUNC_INFO("fail to open /customer/dla/buffer.bin:%s\n", strerror(errno));
        return fd;
    }

    ret = write(fd, buffer, len);
    FDAFR_FUNC_INFO("write %d bytes\n", ret);
    close (fd);

    return ret;
}


static void GetTimeCost(clock_t start, clock_t end, char *msg)
{
    double seconds  =(double)(end - start)/CLOCKS_PER_SEC;

    FDAFR_FUNC_INFO("%s cost time is: %.8fs\n", msg, seconds);
}

int Fdfr::InferenceFDANetwork(MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *Vector4Affine)
{
    int s32Ret;
    struct timeval timeout;
    MI_IPU_TensorVector_t inputVector, outputVector;

    // prepare input vector
    if (FdaDesc.u32InputTensorCount != 1) {
        FDAFR_FUNC_INFO("error: FDA network input count isn't 1\n");
        return E_IPU_ERR_ILLEGAL_INPUT_OUTPUT_PARAM;
    }
    inputVector.u32TensorCount = FdaDesc.u32InputTensorCount;
    if (stBufInfo.eBufType == E_MI_SYS_BUFDATA_RAW) {
        FDAFR_FUNC_INFO("InferenceFDANetwork E_MI_SYS_BUFDATA_RAW\n");
        inputVector.astArrayTensors[0].phyTensorAddr[0] = stBufInfo.stRawData.phyAddr;
        inputVector.astArrayTensors[0].ptTensorData[0] = stBufInfo.stRawData.pVirAddr;
    } else if (stBufInfo.eBufType == E_MI_SYS_BUFDATA_FRAME) {
        FDAFR_FUNC_INFO("InferenceFDANetwork E_MI_SYS_BUFDATA_FRAME\n");
        inputVector.astArrayTensors[0].phyTensorAddr[0] = stBufInfo.stFrameData.phyAddr[0];
        inputVector.astArrayTensors[0].ptTensorData[0] = stBufInfo.stFrameData.pVirAddr[0];

        inputVector.astArrayTensors[0].phyTensorAddr[1] = stBufInfo.stFrameData.phyAddr[1];
        inputVector.astArrayTensors[0].ptTensorData[1] = stBufInfo.stFrameData.pVirAddr[1];
        //save_buffer_to_file(stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32BufSize);
    }

    //prepare output vector
    do {
            s32Ret = MI_IPU_GetOutputTensors(FdaChn, &outputVector);
            if (s32Ret != MI_SUCCESS) {
                timeout.tv_sec = 0;
                timeout.tv_usec = 100*1000;
                select(0, NULL, NULL, NULL, &timeout);
            }
    } while (s32Ret != MI_SUCCESS);


//    struct  timeval    tv_start;
//    struct timeval tv_1;
//
//
//    gettimeofday(&tv_start,NULL);
//    clock_t start = clock();
    s32Ret = MI_IPU_Invoke(FdaChn, &inputVector, &outputVector);
//    clock_t end = clock();
//    gettimeofday(&tv_1,NULL);
//
//    int dofda_inms = (tv_1.tv_sec-tv_start.tv_sec)*1000+(tv_1.tv_usec-tv_start.tv_usec)/1000;
//
//    TIME_FUNC_INFO("========================%d==================invoke fd in ms\n",dofda_inms);
//
//    GetTimeCost(start,end,"fd invoke time is");
    if (s32Ret != MI_SUCCESS) {
        FDAFR_FUNC_INFO("IPU invoke error:%d\n", s32Ret);
        return s32Ret;
    }

    *Vector4Affine = outputVector;

    //MI_IPU_PutOutputTensors(FdaChn, &outputVector);
    FDAFR_FUNC_INFO("InferenceFDANetwork done\n");
    return MI_SUCCESS;
}
int Fdfr::DoFd(MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *Vector4Affine)
{
    FDAFR_FUNC_INFO("FD get data: buf type %d format %d addr %llx w %d h %d\n", stBufInfo.eBufType, stBufInfo.stFrameData.ePixelFormat, stBufInfo.stFrameData.phyAddr[0], stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);
    return InferenceFDANetwork(stBufInfo, Vector4Affine);
}
#define RAMDOM(start, end) ((end > start)?((rand()%(end - start)) + start):0)
void Fdfr::DoFr(MI_SYS_BufInfo_t &stBufInfo, MI_IPU_TensorVector_t *Vector4Affine)
{
    int s32Ret;
    struct timeval timeout;
    stFdaOutputDataDesc_t data;
    MI_IPU_TensorVector_t inputVector, outputVector;

    //prepare FR input vector
    do {
            s32Ret = MI_IPU_GetInputTensors(FrChn, &inputVector);
            if (s32Ret != MI_SUCCESS) {
                timeout.tv_sec = 0;
                timeout.tv_usec = 100*1000;
                select(0, NULL, NULL, NULL, &timeout);
            }
    } while (s32Ret != MI_SUCCESS);

    //prepare FR output vector
    do {
            s32Ret = MI_IPU_GetOutputTensors(FrChn, &outputVector);
            if (s32Ret != MI_SUCCESS) {
                timeout.tv_sec = 0;
                timeout.tv_usec = 100*1000;
                select(0, NULL, NULL, NULL, &timeout);
            }
    } while (s32Ret != MI_SUCCESS);

    data.pfBBox = (float*)(Vector4Affine->astArrayTensors[0].ptTensorData[0]);
    data.pfLms = (float*)(Vector4Affine->astArrayTensors[1].ptTensorData[0]);
    data.pfScores = (float*)(Vector4Affine->astArrayTensors[2].ptTensorData[0]);
    data.pDectCount = (float*)(Vector4Affine->astArrayTensors[3].ptTensorData[0]);
    DoTrack(&data);
    MI_IPU_PutOutputTensors(FdaChn, Vector4Affine);

    DoRecognition(stBufInfo, &inputVector, &outputVector);
    MI_IPU_PutInputTensors(FrChn, &inputVector);
    MI_IPU_PutOutputTensors(FrChn, &outputVector);
#if 0
    stFaceInfo_t stFaceInfo;
    const char *NameList[] = {"Albert", "Nancy", "Peter", "Bryce", "Laura", "Mary", "Sam", "Tommy", "Abel"};
    unsigned int i = 0;
    unsigned int j = 0;

    // FR network
    InferenceFRNetwork(stBufInfo, Vector4Affine);

    memset(&stFaceInfo, 0, sizeof(stFaceInfo_t));
    // 1 crop by divp
    //printf("FR get data: buf type %d format %d addr %llx w %d h %d\n", stBufInfo.eBufType, stBufInfo.stFrameData.ePixelFormat, stBufInfo.stFrameData.phyAddr[0], stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);

    // do recognition

    // insert the result
    for (i = 0; i < sizeof(NameList) / sizeof(char *); i++)
    {
        strcpy(stFaceInfo.faceName, NameList[i]);
        stFaceInfo.faceW = RAMDOM(116, 400);
        stFaceInfo.faceH = RAMDOM(116, 400);
        stFaceInfo.xPos = RAMDOM(0, 1920);
        stFaceInfo.yPos = RAMDOM(0, 1080);
        stFaceInfo.winWid = 1920;
        stFaceInfo.winHei = 1080;
        vectResult.push_back(stFaceInfo);
    }
    for (j = i; j < 15; j++)
    {
        memset(stFaceInfo.faceName, 0, 50);
        stFaceInfo.faceW = RAMDOM(116, 400);
        stFaceInfo.faceH = RAMDOM(116, 400);
        stFaceInfo.xPos = RAMDOM(0, 1920);
        stFaceInfo.yPos = RAMDOM(0, 1080);
        stFaceInfo.winWid = 1920;
        stFaceInfo.winHei = 1080;
        vectResult.push_back(stFaceInfo);
    }
#endif
}
void Fdfr::SendResult()
{
    ST_TEM_USER_DATA stUsrData;
    std::map<unsigned int, stModOutputInfo_t>::iterator itFdfrOut;
    std::vector<stFaceInfo_t>::iterator itVectFace;
    char *pTempData = NULL;

    stUsrData.u32UserDataSize = sizeof(stFaceInfo_t) * vectResult.size();
    if (stUsrData.u32UserDataSize)
    {
        stUsrData.pUserData = malloc(stUsrData.u32UserDataSize);
        if (!stUsrData.pUserData) {
            FDAFR_FUNC_INFO("fail to allocate memory for stUsrData.pUserData\n");
            return;
        }
        pTempData = (char *)stUsrData.pUserData;
        assert(stUsrData.pUserData);
        for (itVectFace = vectResult.begin(); itVectFace != vectResult.end(); itVectFace++)
        {
            memcpy(pTempData, &(*itVectFace), sizeof(stFaceInfo_t));
            pTempData += sizeof(stFaceInfo_t);
        }
        for(itFdfrOut = mapModOutputInfo.begin(); itFdfrOut != mapModOutputInfo.end(); itFdfrOut++)
        {
            TemSend(mapModOutputInfo[itFdfrOut->second.curPortId].curIoKeyString.c_str(), stUsrData);
            TemStartOneShot(mapModOutputInfo[itFdfrOut->second.curPortId].curIoKeyString.c_str());
        }
        free(stUsrData.pUserData);
    }
    vectResult.clear();
}
void *Fdfr::DataMonitor(ST_TEM_BUFFER stBuf)
{
    MI_SYS_ChnPort_t stChnOutputPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32 s32Fd;
    MI_S32 s32Ret;
    fd_set read_fds;
    struct timeval tv;
    Fdfr *pClass = (Fdfr *)stBuf.pTemBuffer;
    std::vector<stFdfrInputInfo_t>::iterator it;
    std::vector<MI_SYS_BUF_HANDLE> vectHandle;
    std::vector<MI_SYS_BUF_HANDLE>::iterator itHandle;
    std::vector<MI_S32>::iterator itFd;
    MI_SYS_BufInfo_t astBufInfo[2]; //0 : fd, 1 : fr
    MI_IPU_TensorVector_t Vector4Affine;

    memset(astBufInfo, 0, sizeof(MI_SYS_BufInfo_t) * 2);


    struct  timeval    tv_start;
    struct timeval tv_1;
    struct timeval tv_2;
    int dofda_inms = 0;
    int dofdafr_inms = 0;

    for (it = pClass->vectFdFrInput.begin(); it != pClass->vectFdFrInput.end(); it++)
    {
        stChnOutputPort.eModId = (MI_ModuleId_e)it->intPreMod;
        stChnOutputPort.u32ChnId = (MI_U32)it->intPreChn;
        stChnOutputPort.u32PortId = (MI_U32)it->intPreOutPort;
        stChnOutputPort.u32DevId = (MI_U32)it->intPreDev;
        switch (stChnOutputPort.eModId)
        {
            case E_MI_MODULE_ID_VPE:
            case E_MI_MODULE_ID_DIVP:
            {
                s32Ret = MI_SYS_GetFd(&stChnOutputPort, &s32Fd);
                if (s32Ret < 0)
                {
                    goto EXIT1;
                }
                FD_ZERO(&read_fds);
                FD_SET(s32Fd, &read_fds);
                tv.tv_sec = 0;
                tv.tv_usec = 10 * 1000;
                s32Ret = select(s32Fd + 1, &read_fds, NULL, NULL, &tv);
                MI_SYS_CloseFd(s32Fd);
                if (s32Ret < 0)
                {
                    goto EXIT1;
                }
                else if (0 == s32Ret)
                {
                    goto EXIT1;
                }
            }
            break;
            default:
                assert(0);
        }
    }

    for (it = pClass->vectFdFrInput.begin(); it != pClass->vectFdFrInput.end(); it++)
    {
        stChnOutputPort.eModId = (MI_ModuleId_e)it->intPreMod;
        stChnOutputPort.u32ChnId = (MI_U32)it->intPreChn;
        stChnOutputPort.u32PortId = (MI_U32)it->intPreOutPort;
        stChnOutputPort.u32DevId = (MI_U32)it->intPreDev;
        switch (stChnOutputPort.eModId)
        {
            case E_MI_MODULE_ID_VPE:
            case E_MI_MODULE_ID_DIVP:
            {
                if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnOutputPort, &stBufInfo,&hHandle))
                {
                    if (it->intInPort == 0)
                        astBufInfo[0] = stBufInfo;
                    else if (it->intInPort == 1)
                        astBufInfo[1] = stBufInfo;
                    else
                        assert(0);

                    vectHandle.push_back(hHandle);
                }
                else
                {
                    goto EXIT;
                }
            }
            break;
            default:
                assert(0);
        }
    }


    gettimeofday(&tv_start,NULL);
    pClass->fd_ePixelFormat = astBufInfo[0].stFrameData.ePixelFormat;
    pClass->fr_ePixelFormat = astBufInfo[0].stFrameData.ePixelFormat;
    FDAFR_FUNC_INFO("fd format :%d \n",pClass->fd_ePixelFormat);
    FDAFR_FUNC_INFO("fr format :%d \n",pClass->fr_ePixelFormat);

    pClass->DoFd(astBufInfo[0], &Vector4Affine);
    gettimeofday(&tv_1,NULL);
    pClass->DoFr(astBufInfo[1], &Vector4Affine);
    pClass->SendResult();
    gettimeofday(&tv_2,NULL);

    dofda_inms = (tv_1.tv_sec-tv_start.tv_sec)*1000+(tv_1.tv_usec-tv_start.tv_usec)/1000;
    dofdafr_inms = (tv_2.tv_sec-tv_start.tv_sec)*1000+(tv_2.tv_usec-tv_start.tv_usec)/1000;
    TIME_FUNC_INFO("================================fda time is %d ms\n",dofda_inms);
    TIME_FUNC_INFO("=================================fdafr time is %d ms\n",dofdafr_inms);

EXIT:
    for (itHandle = vectHandle.begin(); itHandle != vectHandle.end(); ++itHandle)
    {
        MI_SYS_ChnOutputPortPutBuf(*itHandle);
    }
    vectHandle.clear();
EXIT1:
    return NULL;
}

void Fdfr::DataReceiver(void *pData, unsigned int dataSize, void *pUsrData)
{
    MI_SYS_BufInfo_t *pstBufInfo;

    pstBufInfo = (MI_SYS_BufInfo_t *)pData;
    assert(sizeof(MI_SYS_BufInfo_t) == dataSize);
    assert(pstBufInfo->eBufType == E_MI_SYS_BUFDATA_FRAME);

    switch (pstBufInfo->stFrameData.ePixelFormat)
    {
        case E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420:
        {
            //do yuv420 copy...
        }
        break;
        case E_MI_SYS_PIXEL_FRAME_YUV422_YUYV:
        case E_MI_SYS_PIXEL_FRAME_ARGB8888:
        {
        }
        break;
        default:
            FDAFR_FUNC_INFO("Not support!!\n");
            assert(0);
            break;
    }
}
void * Fdfr::SenderSignal(ST_TEM_BUFFER stBuf, ST_TEM_USER_DATA stUsrData)
{
    stReceiverDesc_t *pReceiver = (stReceiverDesc_t *)stBuf.pTemBuffer;
    Fdfr *pClass = dynamic_cast<Fdfr *>(pReceiver->pSysClass);

    pClass->Send(pReceiver->uintPort, stUsrData.pUserData, stUsrData.u32UserDataSize);

    return NULL;
}
void * Fdfr::SenderSignalEnd(ST_TEM_BUFFER stBuf)
{
    stReceiverDesc_t *pReceiver = (stReceiverDesc_t *)stBuf.pTemBuffer;
    Fdfr *pClass = dynamic_cast<Fdfr *>(pReceiver->pSysClass);

    pClass->Send(pReceiver->uintPort, NULL, 0);

    return NULL;
}

int Fdfr::CreateSender(unsigned int outPortId)
{
    ST_TEM_ATTR stTemAttr;
    stReceiverDesc_t stDest;

    PTH_RET_CHK(pthread_attr_init(&stTemAttr.thread_attr));
    memset(&stTemAttr, 0, sizeof(ST_TEM_ATTR));
    stTemAttr.fpThreadDoSignal = SenderSignal;
    stTemAttr.fpThreadWaitTimeOut = SenderSignalEnd;
    stTemAttr.u32ThreadTimeoutMs = 500;
    stTemAttr.bSignalResetTimer = TRUE;
    stTemAttr.stTemBuf.pTemBuffer = (void *)&(mapRecevier[outPortId]);
    stTemAttr.stTemBuf.u32TemBufferSize = 0;
    TemOpen(mapModOutputInfo[outPortId].curIoKeyString.c_str(), stTemAttr);

    return 0;
}
int Fdfr::StartSender(unsigned int outPortId, stReceiverPortDesc_t &stRecvPortDesc)
{
    return 0;
}
int Fdfr::StopSender(unsigned int outPortId, stReceiverPortDesc_t &stRecvPortDesc)
{
    return 0;
}
int Fdfr::DestroySender(unsigned int outPortId)
{
    TemClose(mapModOutputInfo[outPortId].curIoKeyString.c_str());

    return 0;
}

