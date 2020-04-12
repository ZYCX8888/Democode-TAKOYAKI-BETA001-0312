/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
/*
 * main.c
 *
 *  Created on: May 6, 2014
 *      Author: Xavier Tsai
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif

#include "mi_fd.h"

#ifdef __cplusplus
}
#endif


//Larry 20160107
#define FR_EN (1)
#define FD_NUM_OF_FACE_UNIT  (10)
#define	PFID_MAX_FACE_NUM    (20)  /* Max numbers of faces which can bedetected */
#define MILLION 1000000

typedef struct _pfid_face_position {
    signed short flag;      /* Setting flag(PFID_SFLG_*) */
    signed short conf;      /* Face confidence [low?F0?`high?F100] */
    signed short rotate;    /* Face rotation flag(PFID_ FACE_ROLL_*) */
    signed short rect_l;    /* Left face frame (X coord) / negative: invalid */
    signed short rect_r;    /* Right face frame (X coord) / negative: invalid */
    signed short rect_t;    /* Top face frame (Y coord) / negative: invalid */
    signed short rect_b;    /* Bottom face frame (Y coord) / negative: invalid */
    signed short eye_lx;    /* Left eye (X coord) / negative: invalid */
    signed short eye_ly;    /* Left eye (Y coord) / negative: invalid */
    signed short eye_rx;    /* Right eye(X coord) / negative: invalid */
    signed short eye_ry;    /* Right eye(Y coord) / negative: invalid */
    /* Other necessary information may be appended */
} PFID_FACE_POSITION;

typedef struct _pfid_rect {
    signed short sx;
    signed short sy;
    signed short ex;
    signed short ey;
} PFID_RECT;

typedef struct _pfid_face_detect {
    signed short num;                           /* Numbers of detected faces (Max:PFID_MAX_FACE_NUM) */
    PFID_FACE_POSITION pos[PFID_MAX_FACE_NUM];  /* Face position */
    signed char flg[PFID_MAX_FACE_NUM];         /* 0:don't care,1:face that should be tracked */
    PFID_RECT rect;                             /* Designated region (sx,sy,ex,ey) */
} PFID_FACE_DETECT;

typedef struct _pfdr_point {
    signed short x;  /* x coordinate   *Negative values are invalid */
    signed short y;  /* y coordinate   *Negative values are invalid */
} PFDR_POINT;

/* Structure for face direction */
typedef struct _pfdr_face_angle {
    signed short pitch;       /* Up-and-down(pitch)angle *up is positive(-180???`180??) */
    signed short yaw;         /* Right-and-left(yaw)angle *left is positive(-180???`180??) */
    signed short roll;        /* Roll(roll)angle *clockwise is positive(-180???`180??) */
    PFDR_POINT vector_front;  /* Face direction vector, 0 origin(x value?F- picture width ?` picture width)(y value?F- picture height ?` picture height) */
    PFDR_POINT vector_gaze;   /* Gazing vector, 0 origin(x value?F- picture width ?` picture width)(y value?F- picture height ?` picture height) */
} PFDR_FACE_ANGLE;


struct Param
{
    unsigned int width;
    unsigned int height;
    unsigned int pixFmt;
    int framerate;
    unsigned int frame_count;
    int fd_face_num;
    int face_detect_en;
    int face_angle_en;
    int face_smile_en;
    int face_blink_en;
    int face_age_gender_en;
    int face_recognize_en;
    int face_detect_track_mode;
    int face_detect_direction_mode;
};


unsigned int Get_OsCounterGetMs(void)
{
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    unsigned int T = (MILLION*(t1.tv_sec)+(t1.tv_nsec)/1000)/1000 ;
    return T;
}


int main(int argc, char **argv)
{
    S64 tx[5];
    struct Param SetParam;
    unsigned char  *frame_buf = NULL;
    S64 StartTime = 0, EndTime = 0;

    int fd_face_num = FD_NUM_OF_FACE_UNIT;

    int i = 0, x = 0, res = 0;
    int k=0;
    FILE * fin;

    // control variables
    char infileName[] = "./Img480X640.raw";

    MI_RET ret = MI_RET_FAIL;
    FD_HANDLE fd_handle;
#if FR_EN
    FR_HANDLE fr_handle;
#endif

    SetParam.width  = 480 ;
    SetParam.height = 640 ;
    SetParam.face_detect_en = 1;
    SetParam.face_angle_en = 0;
    SetParam.face_blink_en = 0;
    SetParam.face_smile_en = 0;
    SetParam.face_age_gender_en = 0;
    SetParam.face_recognize_en = FR_EN;
    SetParam.face_detect_track_mode = 1;
    SetParam.face_detect_direction_mode = 0;

    frame_buf = (unsigned char*)malloc(SetParam.width*SetParam.height);

    printf("1234567\n");  // FIXJASON, del later

    // open image files, and read data into buffer
    fin = fopen(infileName, "rb");
    if(!fin)
    {
        printf("the input file could not be open\n");
        return -1;
    }

    fread(frame_buf, sizeof(char), SetParam.width*SetParam.height, fin);
    fclose(fin);


    int fd_ind = 0;
    int FaceNum_Detected = 0;
    int FaceNum_Detected_pre = 0;
    int left[FD_NUM_OF_FACE_UNIT],top[FD_NUM_OF_FACE_UNIT],height[FD_NUM_OF_FACE_UNIT],width[FD_NUM_OF_FACE_UNIT];
    char fr_name[FD_NUM_OF_FACE_UNIT][32];
    char recog_str[128];

    unsigned char *fd_info_buf = NULL;
    unsigned char *fang_info_buf = NULL;
    unsigned char *blink_info_buf = NULL;
    unsigned char *smile_info_buf = NULL;
    unsigned char *age_gender_info_buf = NULL;
    unsigned char *recognize_info_buf = NULL;

    fd_handle = MI_FD_Init(SetParam.width, SetParam.height);
#if FR_EN
    fr_handle = MI_FR_Init(SetParam.width, SetParam.height);
#endif

    //fdip_ctl_SetFaceDetectionEnable(SetParam.face_detect_en);
    ret = MI_FD_EnableFD(fd_handle, SetParam.face_detect_en);
    //fdip_ctl_SetFaceAngleEnable(SetParam.face_angle_en);  // FIXJASON, no mi-api *****
    //fdip_ctl_SetFaceBlinkEnable(SetParam.face_blink_en);  // FIXJASON, no mi-api *****
    //fdip_ctl_SetFaceSmileEnable(SetParam.face_smile_en);  // FIXJASON, no mi-api *****
    //fdip_ctl_SetFaceAgeGenderEnable(SetParam.face_age_gender_en);  // FIXJASON, no mi-api *****
#if FR_EN
    //fdip_ctl_SetFaceRecognizeEnable(SetParam.face_recognize_en);
    ret = MI_FR_EnableFR(fr_handle, SetParam.face_recognize_en);
    //fdip_ctl_SetFrMode(2); //added this API at 20170119 by Agatha.
    MI_FR_SetFrMode(2);
    //fdip_ctl_SetFrFaceWidth(40); //added this API at 20170213 by Agatha.
    MI_FR_SetFrFaceWidth(40);
#endif

    int fd_info_buf_len = sizeof(PFID_FACE_DETECT);
    fd_info_buf = (unsigned char*)malloc(fd_info_buf_len);
    printf("fd_info_buf_len = %d\n", fd_info_buf_len);

    int recognize_info_buf_len = 2*PFID_MAX_FACE_NUM;

#if FR_EN
        recognize_info_buf = (unsigned char*)malloc(recognize_info_buf_len);
        printf("recognize_info_buf_len=%d \n", recognize_info_buf_len);
#endif

    //res = fdip_ctl_initial(romfiles);

#if FR_EN
    // load FR data base

        int fr_calculate_db_all_en = 0;
        if(fr_calculate_db_all_en==1)
        {
            // read DB_list.txt automatically
            fdip_ctl_GenFrDatabaseAll();  // FIXJASON, no mi-api *****
        }
        else
        {
            FILE* listfp;
            int store_num = 0;
            char filename[128];
            char feat_name[128];
            //int size = fdip_ctl_GetFeatureSizes();
            int size = MI_FR_GetFeatureSizes();
            char* feat_data;

            printf("FeatureSizes = %d \n", size);

            feat_data = (char*)malloc(size);
            listfp = fopen( "DB_list.txt", "rt" );
            if ( !listfp )
            {
                printf("Read DB_list.txt error!! \n");
                exit(0);
            }
            while ( !feof( listfp ) && !ferror( listfp ) )
            {
                memset( filename, 0, sizeof(char) * 128 );

                // read one line

                fscanf( listfp, "%s\n", filename );

                // calculate feature data from image
                //fdip_ctl_CalFeatureFromImg(filename, store_num);
                ret = MI_FR_CalFeatureFromImg(filename, store_num);

                // get feature data and name from previous stage
                //fdip_ctl_GetFeatureData(store_num, feat_data, feat_name);
                ret = MI_FR_GetFeatureData(store_num, feat_data, feat_name);

                sprintf(feat_name, "%s", filename); // assign feature name here (user name)

                //fdip_ctl_SetFeatureData(store_num, feat_data, feat_name);
                ret = MI_FR_SetFeatureData(store_num, feat_data, feat_name);

                store_num++;
            }

            free(feat_data);
        }

#endif
    printf("end fdip_ctl_initial = %d \n", res);

    PFID_FACE_DETECT* pfid_face_info = (PFID_FACE_DETECT*)fd_info_buf;
    memset(pfid_face_info, 0, sizeof(PFID_FACE_DETECT));

    /* Capture video frame */
    // while(1)
    {
        // for each frame, call this

        tx[0] = Get_OsCounterGetMs();
        //fdip_ctl_SetFaceDetectionOption(SetParam.face_detect_track_mode);
        ret = MI_FD_SetOption(fd_handle, FD_OPTION_DETECT_MODE, SetParam.face_detect_track_mode);
        //fdip_ctl_SetFaceDirectionDetectionOption(SetParam.face_detect_direction_mode);
        ret = MI_FD_SetOption(fd_handle, FD_OPTION_FACE_DIRECTION, SetParam.face_detect_direction_mode);
        //fdip_ctl_SetFaceDetectionFaceWidth(20);
        ret = MI_FD_SetOption(fd_handle, FD_OPTION_FACE_WIDTH, 20);
        //fdip_ctl_SetFacePartialDetectionFaceWidth(40);  // min face width in partial detection mode
        ret = MI_FD_SetOption(fd_handle, FD_OPTION_PARTIAL_WIDTH, 40);

        tx[1] = Get_OsCounterGetMs();
        //fd_ind = fdip_ctl_process((unsigned char*)frame_buf, (int)SetParam.width, (int)SetParam.height);
        fd_ind = MI_FD_Run(fd_handle, (const U8*)frame_buf);
        tx[2] = Get_OsCounterGetMs();
        //fdip_getfd_info((unsigned short*)fd_info_buf);
        FacePos_t*  face_position = NULL;
        FaceNum_Detected = MI_FD_GetFaceInfo(fd_handle, (FacePos_t **)&face_position);
        printf("FaceNum_Detected = %d \n", FaceNum_Detected);
        tx[3] = Get_OsCounterGetMs();
#if FR_EN
        //fdip_getrecognize_info((unsigned short*)recognize_info_buf);    // FIXJASON, *****, move lower
#endif
        tx[4] = Get_OsCounterGetMs();
        // get result
        unsigned short *mem_ptr = (unsigned short*)fd_info_buf;

        //FaceNum_Detected = pfid_face_info->num;  // FIXJASON
        if(FaceNum_Detected > FD_NUM_OF_FACE_UNIT)
        FaceNum_Detected = FD_NUM_OF_FACE_UNIT;
        printf("FaceNum_Detected = %d \n", FaceNum_Detected);

        if(SetParam.face_detect_en)
        {
            printf("F%d-FaceNum_Detected = %d\n", i, FaceNum_Detected);
            printf("%d  X %d\n", SetParam.width, SetParam.height);
            #if 0
            int cnt=1;
            for (x = 0; x < FaceNum_Detected; x++)
            {
                for(k=0;k<11;k++)
                {
                    cnt++;
                }

                left[x]   = pfid_face_info->pos[x].rect_l;
                top[x]    = pfid_face_info->pos[x].rect_t;
                width[x]  = pfid_face_info->pos[x].rect_r - pfid_face_info->pos[x].rect_l + 1;
                height[x] = pfid_face_info->pos[x].rect_b - pfid_face_info->pos[x].rect_t + 1;

                printf("flag(%d) conf(%d) rotate(%d)\n", pfid_face_info->pos[x].flag, pfid_face_info->pos[x].conf, pfid_face_info->pos[x].rotate);
                printf("rect_l(%d) rect_r(%d) rect_t(%d) rect_b(%d)\n", pfid_face_info->pos[x].rect_l, pfid_face_info->pos[x].rect_r, pfid_face_info->pos[x].rect_t, pfid_face_info->pos[x].rect_b);
                printf("eye_lx(%d) eye_ly(%d) eye_rx(%d) eye_ry(%d)\n", pfid_face_info->pos[x].eye_lx, pfid_face_info->pos[x].eye_ly, pfid_face_info->pos[x].eye_rx, pfid_face_info->pos[x].eye_ry);

                printf("track(%d)\n", pfid_face_info->flg[x]);
            }
            printf("sx(%d) sy(%d) ex(%d) ey(%d)\n", pfid_face_info->rect.sx, pfid_face_info->rect.sy, pfid_face_info->rect.ex, pfid_face_info->rect.ey);  // 0 0 0 0
            #else
            int cnt=1;
            for (x = 0; x < FaceNum_Detected; x++)
            {
                for(k=0;k<11;k++)
                {
                    cnt++;
                }

                left[x]   = face_position[x].posx;
                top[x]    = face_position[x].posy;
                width[x]  = face_position[x].posw;
                height[x] = face_position[x].posh;
            }
            #endif

#if 0

#if FR_EN
        fdip_getrecognize_info((unsigned short*)recognize_info_buf);    // FIXJASON, *****
#endif

#if FR_EN

                mem_ptr = (unsigned short*)recognize_info_buf;
                printf("-Recog_Detected");
                cnt=0;
                int str_len=0;
                short idx=0;
                for (x = 0; x < FaceNum_Detected; x++)
                {
                    for(k=0;k<2;k++)
                    {
                        printf("%04x", (mem_ptr[cnt]));
                        cnt++;
                    }
                    // get string
                    idx = (short)mem_ptr[cnt-2];
                    printf("\n idx(%d) \n", idx);  // FIXJASON, del later
                    if(idx>=0)
                    {
                        fdip_getrecognize_str(mem_ptr[cnt-2], recog_str);    // FIXJASON, *****
                        str_len = strlen(recog_str);
                        printf("str_len(%d) recog_str(%s)\n", str_len, recog_str);  // FIXJASON, del later

                        for(k=str_len-1;k>=0;k--)
                        {
                            if(recog_str[k] == '/')
                            break;
                        }
                        printf("k = %d \n", k);
                        memcpy(fr_name[x], &(recog_str[k+1]), str_len-k-5);
                        fr_name[x][str_len-k-5] = '\0';
                        printf("fr_name[x]=%s \n",fr_name[x]);
                        printf("mem_ptr[cnt-1]=(%d)\n", mem_ptr[cnt-1]);  // FIXJASON, del later
                        sprintf(fr_name[x], "%s (%d) \0", fr_name[x], mem_ptr[cnt-1]);
                        printf("fr_name[x]=%s \n",fr_name[x]);  // FIXJASON, del later
                    }
                    else
                    {
                        sprintf(fr_name[x], "Unknown User\0");
                    }

                }

#endif

#else

#if FR_EN
        FRFaceInfo_t frInfo[FD_NUM_OF_FACE_UNIT];
        MI_FR_GetFRInfo(fr_handle, FaceNum_Detected, (FRFaceInfo_t *)frInfo);
        for(x = 0; x < FaceNum_Detected; x++)
        {
            if(frInfo[x].store_idx == -1)
            {
                printf("FR: face not recognition\n");
            }
            else
            {
                printf("FR: face recognition:index=%d  strore_idx=%d percentage=%d name=%s \n", x, frInfo[x].store_idx, frInfo[x].percentage, frInfo[x].name);
            }
        }
#endif

#endif

            printf("\n");
            printf("--Time of seting parameters: %lld\n", (tx[1]-tx[0]));
            printf("--Time of function fdip_ctl_process: %lld\n", (tx[2]-tx[1]));
            printf("--Time of get information: %lld\n", (tx[3]-tx[2]));
            printf("--Time of FR: %lld\n", (tx[4]-tx[3]));

            printf("F%d-FaceNum_Detected=%d \n", i, FaceNum_Detected);
            for (x = 0; x < FaceNum_Detected; x++)
            {
#if FR_EN
                //printf("j=%d, left=%d, top=%d, width=%d height=%d , name=%s\n", x, left[x], top[x], width[x], height[x], fr_name[x]);
                printf("j=%d, left=%d, top=%d, width=%d height=%d , name=%s\n", x, left[x], top[x], width[x], height[x], frInfo[x].name);
#else
                printf("j=%d, left=%d, top=%d, width=%d height=%d\n", x, left[x], top[x], width[x], height[x]);
#endif
            }

        }
    }

    // free(fd_info_buf);

    /* Release buffer */
    // free(frame_buf);
    /* Close device */

    return 0;
}


