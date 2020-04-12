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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>

#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_ipu.h"

#include "maggie_mobilenet_pic.c"
#include "magpie_output.c"
#include "maggie_resnet_pic.c"

typedef struct invoke_package_s {
    MI_S32 u32ChnId;
    MI_IPU_TensorVector_t inVector, outVector;
} invoke_package_t;

static int buf_depth = 3;
static int pipefd_invoke_data[2];
pthread_mutex_t write_lock;

#define ExecFunc(func, _ret_) \
    if (func != _ret_)\
    {\
        printf("IPU_TEST [%d] %s exec function failed\n",__LINE__, #func);\
        return 1;\
    }\
    else\
    {\
        printf("IPU_TEST [%d] %s  exec function pass\n", __LINE__, #func);\
    }

void help(int argc,void **argv)
{
    putchar('\n');
    printf("this is ipu test app: \n");
    printf("--->%s -f num \n",argv[0]);
    printf("----->num 1: test01_for_ai_uac \n");
    printf("----->num 2: test02_for_uac_ao \n");
    printf("----->num 3: test03_for_ai_uac_ao \n");
    printf("----->num 4: test04_for_ai_uac_sendframe \n");
}

void stop_help()
{
    printf("\n\nenter 'q' to exit \n");
}


#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#define MAXLINE 4096
#define SERVER_IP	"172.19.21.15"
struct package {
    char file[256];
    int read_size;
}__attribute__((packed));
int get_resnet_network_file(void *dst_buf,int offset, int read_size, char *ctx)
{
    int    sockfd, n,rec_len, size, cnt;
    char    buf[MAXLINE];
    struct sockaddr_in    servaddr;
    char *file = ctx;
    char *modelAddr = dst_buf;
    struct package package;

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
    exit(0);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(2186);
    if( inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) <= 0){
	printf("inet_pton error for %s\n",SERVER_IP);
	exit(0);
    }

    if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

    memcpy(package.file, file, strlen(file));
    package.read_size = read_size;
    printf("send msg to server: \n");
    if( send(sockfd, &package, sizeof(package), 0) < 0)
    {
        printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    if((rec_len = recv(sockfd, buf, sizeof(size),0)) == -1) {
       perror("recv error");
       exit(1);
    }
    memcpy(&size, buf, sizeof(size));
    printf("Received : %s, size=%d \n", file, size);

    cnt = 0;
    while (1) {
    	rec_len = recv(sockfd, modelAddr+cnt, MAXLINE,0);
    	cnt += rec_len;
    	//printf("receive=%d, cnt=%d\n", rec_len, cnt);
    	if (cnt == size)
    		break;
    }
    printf("receive %d bytes\n", cnt);
    close(sockfd);
    return 0;
}

void *prepare_tensor_data(void *arg)
{
    MI_S32 s32Ret, cnt = 3;
    MI_IPU_TensorVector_t inputV;
    MI_IPU_TensorVector_t outputV;
    MI_IPU_CHN u32ChnId = *(MI_IPU_CHN*)arg;
    invoke_package_t package;

    printf("input thread start\n");
    do {
_get_input_buffer:
        s32Ret = MI_IPU_GetInputTensors(u32ChnId, &inputV);
        if (s32Ret != MI_SUCCESS) {
            sleep(1);
            goto _get_input_buffer;
        }
        // fill data
        if (u32ChnId == 0)
            memcpy(inputV.stArrayTensors[0].pstTensorData[0], au8MaggiePic, sizeof(au8MaggiePic));
        else if (u32ChnId == 1)
            memcpy(inputV.stArrayTensors[0].pstTensorData[0], resnet_au8MaggiePic, sizeof(resnet_au8MaggiePic));
        else
            ipu_err("unexpected error by u32ChnId\n");

_get_output_buffer:
        s32Ret = MI_IPU_GetOutputTensors(u32ChnId, &outputV);
        if (s32Ret != MI_SUCCESS) {
            sleep(1);
            goto _get_output_buffer;
        }

        package.u32ChnId = u32ChnId;
        package.inVector = inputV;
        package.outVector = outputV;
        //invoke
        pthread_mutex_lock(&write_lock);
        write(pipefd_invoke_data[1], &package, sizeof(package));
        pthread_mutex_unlock(&write_lock);
    } while (1);

    printf("input thread exit\n");
}

void check_result(int channel, void *buffer, int size)
{
        static int chn0_check = 0, chn1_check = 0;
        unsigned char *p =  buffer;
        int count;

        if (chn0_check && channel == 0)
            return;
        if (chn1_check && channel == 1)
            return;

        if (channel == 0) {
            chn0_check = 1;
        } else if (channel == 1) {
            chn1_check = 1;
        }
        for (count = 0; count < size; count++) {
            if (p[count] != output_result[count])
                printf("[channel%d]index%d values(0x%x, 0x%x) don't match\n", channel, p[count], output_result[count]);
        }
}

void *invoke(void *arg)
{
    MI_S32 s32Ret;
    invoke_package_t *package= (invoke_package_t*)arg;

    pthread_detach(pthread_self());

    printf("[channel%d]in-address=%llx, out-address=%llx\n",
        package->u32ChnId,
        package->inVector.stArrayTensors[0].phyTensorAddr[0],
        package->outVector.stArrayTensors[0].phyTensorAddr[0]);

    s32Ret = MI_IPU_Invoke(package->u32ChnId, &package->inVector, &package->outVector);
    MI_IPU_PutInputTensors(package->u32ChnId, &package->inVector);
    if (s32Ret == MI_SUCCESS) {
        // process output buffer data
        // ...
        printf("[channel%d] invoke done\n", package->u32ChnId);
        check_result(package->u32ChnId,  package->outVector.stArrayTensors[0].pstTensorData[0], 1001);
    } else {
        printf("[channel%d] invoke failed\n", package->u32ChnId);
    }
    MI_IPU_PutOutputTensors(package->u32ChnId, &package->outVector);
    free(package);
}

void *invoke_hub(void *arg)
{
    MI_S32 s32Ret;
    invoke_package_t *package;
    pthread_t tid;

    for ( ; ; ) {
        do {
            package = malloc(sizeof(*package));
        } while (!package);
        memset(package, 0, sizeof(*package));
        s32Ret = read(pipefd_invoke_data[0], package, sizeof(*package));
        if (s32Ret != sizeof(*package)) {
            printf("invoke data isn't completely, exit\n");
            return;
        }
        pthread_create(&tid, NULL, invoke, package);
    }
}

void debug(MI_S32 u32ChnId)
{
    MI_S32 s32Ret;
    MI_IPU_TensorVector_t inputV;
    MI_IPU_TensorVector_t outputV;

    for ( ; ; ) {
_get_input_buffer:
        s32Ret = MI_IPU_GetInputTensors(u32ChnId, &inputV);
        if (s32Ret != MI_SUCCESS) {
            sleep(1);
            goto _get_input_buffer;
        }
        // fill data
        memcpy(inputV.stArrayTensors[0].pstTensorData[0], au8MaggiePic, sizeof(au8MaggiePic));

_get_output_buffer:
        s32Ret = MI_IPU_GetOutputTensors(u32ChnId, &outputV);
        if (s32Ret != MI_SUCCESS) {
            sleep(1);
            goto _get_output_buffer;
        }

        MI_IPU_Invoke(u32ChnId, &inputV, &outputV);
        MI_IPU_PutInputTensors(u32ChnId, &inputV);
        MI_IPU_PutOutputTensors(u32ChnId, &outputV);
    }
}

int main(int argc,void **argv)
{
    MI_S32 s32Ret;
    MI_IPU_CHN u32ChnId = 0, chn = 1;
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;

    MI_IPU_DevAttr_t stDevAttr;
    MI_IPU_SubNet_InputOutputDesc_t desc, desc1;
    MI_IPUChnAttr_t chnAttr;

    pthread_t prepare_thread, prepare_thread1, invoke_hub_thread;

    memset(&stGlobalPrivPoolConf, 0 ,sizeof(stGlobalPrivPoolConf));
    stGlobalPrivPoolConf.eConfigType = E_MI_SYS_PER_CHN_PRIVATE_POOL;
    stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.eModule = E_MI_MODULE_ID_IPU;
    stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.u32Devid = 0;
    stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.u32Channel = u32ChnId;
    stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.u32PrivateHeapSize = buf_depth*1024*1024*2;
    stGlobalPrivPoolConf.bCreate = 1;
    s32Ret = MI_SYS_ConfigPrivateMMAPool(&stGlobalPrivPoolConf);
    if (s32Ret != MI_SUCCESS) {
        printf("config chn heap error[0x%x]\n", s32Ret);
    }

    memset(&stGlobalPrivPoolConf, 0 ,sizeof(stGlobalPrivPoolConf));
    stGlobalPrivPoolConf.eConfigType = E_MI_SYS_PER_CHN_PRIVATE_POOL;
    stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.eModule = E_MI_MODULE_ID_IPU;
    stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.u32Devid = 0;
    stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.u32Channel = chn;
    stGlobalPrivPoolConf.uConfig.stPreChnPrivPoolConfig.u32PrivateHeapSize = buf_depth*1024*1024*16;
    stGlobalPrivPoolConf.bCreate = 1;
    s32Ret = MI_SYS_ConfigPrivateMMAPool(&stGlobalPrivPoolConf);
    if (s32Ret != MI_SUCCESS) {
        printf("config chn heap error[0x%x]\n", s32Ret);
    }

    //stDevAttr.u32MaxVariableBufSize = 1605632*2;
    stDevAttr.u32MaxVariableBufSize = 7920000;
    s32Ret = MI_IPU_CreateDevice(&stDevAttr, NULL, NULL, 0);
    if (s32Ret != MI_SUCCESS) {
        printf("fail to create ipu device\n");
        return s32Ret;
    }
    chnAttr.u32InputBufDepth = buf_depth;
    chnAttr.u32OutputBufDepth = buf_depth;
    s32Ret = MI_IPU_CreateCHN(u32ChnId, &chnAttr, NULL, NULL);
    if (s32Ret != MI_SUCCESS) {
        printf("fail to create ipu channel%d\n", u32ChnId);
        MI_IPU_DestroyDevice();
        return s32Ret;
    }
    chnAttr.u32InputBufDepth = buf_depth;
    chnAttr.u32OutputBufDepth = buf_depth;
    s32Ret = MI_IPU_CreateCHN(chn, &chnAttr, get_resnet_network_file, "resnet_v2_50_fix_tflite_sgsimg.img");
    if (s32Ret != MI_SUCCESS) {
        printf("fail to create ipu channel%d\n", chn);
        MI_IPU_DestroyCHN(u32ChnId);
        MI_IPU_DestroyDevice();
        return s32Ret;
    }
    s32Ret = MI_IPU_GetInOutTensorDesc(u32ChnId, &desc);
    if (s32Ret) {
        printf("fail to get network(%d) description\n", u32ChnId);
        MI_IPU_DestroyCHN(u32ChnId);
        MI_IPU_DestroyCHN(chn);
        MI_IPU_DestroyDevice();
        return s32Ret;
    }
    s32Ret = MI_IPU_GetInOutTensorDesc(chn, &desc1);
    if (s32Ret) {
        printf("fail to get network(%d) description\n", chn);
        MI_IPU_DestroyCHN(u32ChnId);
        MI_IPU_DestroyCHN(chn);
        MI_IPU_DestroyDevice();
        return s32Ret;
    }

    if (pipe(pipefd_invoke_data)) {
        printf("fail to create pipe: %s\n", strerror(errno));
        MI_IPU_DestroyCHN(u32ChnId);
        MI_IPU_DestroyCHN(chn);
        MI_IPU_DestroyDevice();
        return -1;
    }

    pthread_mutex_init(&write_lock, NULL);

    pthread_create(&prepare_thread, NULL, prepare_tensor_data, &u32ChnId);
    pthread_create(&prepare_thread1, NULL, prepare_tensor_data, &chn);
    pthread_create(&invoke_hub_thread, NULL, invoke_hub, NULL);

    pthread_join(prepare_thread, NULL);
    pthread_join(prepare_thread1, NULL);
    pthread_join(invoke_hub_thread, NULL);

    MI_IPU_DestroyCHN(u32ChnId);
    MI_IPU_DestroyCHN(chn);
    MI_IPU_DestroyDevice();
    return 0;
}
