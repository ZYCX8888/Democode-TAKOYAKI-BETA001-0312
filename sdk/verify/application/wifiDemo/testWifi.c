#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "mi_common_datatype.h"
#include "mi_wlan.h"


/*
 * 1.启动：做设备初始化，设置wifi模式，默认为STA模式。
 *
 *
 * 2. 接收操作指令：
 *      1) m 切换模式
 *      2) a 切换热点
 *      2) n [ssid passwd] 连接新的AP
 *      3) d 断开连接
 *      4）p 连接成功时，打印连接状态信息
 *      5) q 退出应用
 *
 * 3. STA模式
 *
 *
 * 4. AP模式
 *
*/

typedef struct
{
    WLAN_HANDLE *pHandle;
    MI_WLAN_ConnectParam_t *pConnParam;
} WlanConnectInfo_t;


// 连接热点信息, 可替换为其它指定的热点，如:ssid为"SKY"，密码为"12345678"
static MI_WLAN_ConnectParam_t g_stConnectParam[] = {
    {
        E_MI_WLAN_SECURITY_WPA,
        "YZCX888888",
        "YZCX8888",
        5000
    },
    {
        E_MI_WLAN_SECURITY_WPA,
        "TP-LINK_30C4",
        "YZCX8888",
        5000
    }
};

static WLAN_HANDLE g_wlanHdl = -1;
static MI_WLAN_OpenParams_t g_stOpenParam = {E_MI_WLAN_NETWORKTYPE_INFRA};
static MI_WLAN_InitParams_t g_stParam = {"/config/wifi/wlan.json"};
static MI_WLAN_ScanResult_t scanResult;
static MI_WLAN_Status_t  g_stStatus;
static bool g_bConnected = false;
static bool g_bThreadRun = false;
static int g_selectIdx = 0;
static pthread_t g_ptConn = NULL;
static pthread_mutex_t g_mutex;

void *_ConnectWorkThread(void *args)
{
    int nTimeoutCnt = 50;               // timeout 10s
    MI_WLAN_Status_t status;
    WlanConnectInfo_t *pstConnInfo = (WlanConnectInfo_t*)args;
    MI_WLAN_Connect(pstConnInfo->pHandle, pstConnInfo->pConnParam);
    printf("current conn info: handle=%d, ssid=%s, passwd=%s\n", *pstConnInfo->pHandle
            , (char*)pstConnInfo->pConnParam->au8SSId, (char*)pstConnInfo->pConnParam->au8Password);

    while (1)
    {
        if (nTimeoutCnt-- <= 0)
        {
            printf("connect failed, timeout\n");
            break;
        }

        MI_WLAN_GetStatus(&status);

        if (status.stStaStatus.state == WPA_COMPLETED)
        {
            g_bConnected = true;
            printf("connect success: ssid=%s, ip=%s\n", (char*)status.stStaStatus.ssid, (char*)status.stStaStatus.ip_address);
            break;
        }

        usleep(200000);
    }

    g_bThreadRun = false;

    return NULL;
}

void _InputCmdTips()
{
    printf("please input option command:\n");
    printf("1. switch STA/AP mode, input 'm'\n");
    printf("2. change wifi hotspot in list, input 'a'\n");
    printf("3. connect wifi hotspot, input 'n'\n");
    printf("4. disconnect wifi hotspot, input 'd'\n");
    printf("5. print wifi hotspot's info, input 'p'\n");
    printf("6. exit, input 'q'\n\n");
}


int main(int argc, char **argv)
{
    bool bExit = false;
    int nApNum = sizeof(g_stConnectParam)/sizeof(MI_WLAN_ConnectParam_t);
    WlanConnectInfo_t stConnInfo;

    // init wlan module
    if (MI_WLAN_Init(&g_stParam))
    {
        printf("Wlan init failed, please ensure wlan module is enabled.\n");
        return -1;
    }

    while (!bExit)
    {
        char inputChar = 0;
        _InputCmdTips();
        inputChar = getchar();

        switch (inputChar)
        {
            case 'm':
                printf("[CMD]:switch mode\n");

                switch (g_stOpenParam.eNetworkType)
                {
                    case E_MI_WLAN_NETWORKTYPE_INFRA:
                        g_stOpenParam.eNetworkType = E_MI_WLAN_NETWORKTYPE_AP;
                        break;
                    case E_MI_WLAN_NETWORKTYPE_AP:
                        g_stOpenParam.eNetworkType = E_MI_WLAN_NETWORKTYPE_INFRA;
                        break;
                    default:
                        break;
                }
                break;
            case 'a':
                printf("[CMD]:change AP\n");

                g_selectIdx = (g_selectIdx+1) % nApNum;
                g_wlanHdl = -1;
                break;
            case 'n':
                printf("[CMD]:open & connect\n");

                if (g_bConnected)
                {
                    printf("AP has already connected, please disconnect first\n");
                    break;
                }

                if (g_bThreadRun)
                {
                    printf("wlan is busy, another connection is trying to establish\n");
                    break;
                }

                if (MI_WLAN_Open(&g_stOpenParam))
                {
                    printf("wlan open failed\n");
                    break;
                }

                switch (g_stOpenParam.eNetworkType)
                {
                    case E_MI_WLAN_NETWORKTYPE_INFRA:
                        printf("try to connect in STA mode\n");
                        memset(&stConnInfo, 0, sizeof(stConnInfo));
                        stConnInfo.pHandle = &g_wlanHdl;
                        stConnInfo.pConnParam = &g_stConnectParam[g_selectIdx];
                        pthread_create(&g_ptConn, NULL, _ConnectWorkThread, &stConnInfo);
                        g_bThreadRun = true;
                        break;
                    case E_MI_WLAN_NETWORKTYPE_AP:
                        printf("try to connect in AP mode\n");
                        if (MI_SUCCESS == MI_WLAN_Connect(&g_wlanHdl, &g_stConnectParam[g_selectIdx]))
                            g_bConnected = true;
                        break;
                    default:
                        break;
                }
                break;
            case 'd':
                printf("[CMD]:disconnect & close\n");
                if (g_ptConn)
                {
                    pthread_join(g_ptConn, NULL);
                    g_ptConn = NULL;
                }

                if (g_bConnected)
                {
                    MI_WLAN_Disconnect(g_wlanHdl);
                    MI_WLAN_Close();
                    g_bConnected = false;
                }
                break;
            case 'p':
                printf("[CMD]:printf current status\n");

                if (!g_bConnected)
                {
                    printf("wlan is disconnected, please connect first\n");
                    break;
                }

                switch (g_stOpenParam.eNetworkType)
                {
                    case E_MI_WLAN_NETWORKTYPE_INFRA:
                        MI_WLAN_Scan(NULL, &scanResult);
                        printf("Scan result:\n");

                        for (int i = 0; i < scanResult.u8APNumber; i++)
                        {
                            char *pSsid = (char*)scanResult.stAPInfo[i].au8SSId;
                            if (pSsid && strcmp(pSsid, "\"\""))
                            {
                                char trimSsid[36];
                                memset(trimSsid, 0, sizeof(trimSsid));
                                strncpy(trimSsid, pSsid+1, strlen(pSsid));
                                printf("SSID: %s\n", trimSsid);
                                printf("MAC: %s\n", (char*)scanResult.stAPInfo[i].au8Mac);
                                printf("encrypt: %s\n", scanResult.stAPInfo[i].bEncryptKey?"true":"false");
                                printf("signalSTR: %d db\n", scanResult.stAPInfo[i].stQuality.signalSTR);
                                printf("frequency: %f GHz\n", scanResult.stAPInfo[i].fFrequency);
                                printf("bitrate: %f Mb/s\n", scanResult.stAPInfo[i].fBitRate);
                                printf("channel: %d\n", scanResult.stAPInfo[i].u8Channel);
                                printf("channel: %d\n", scanResult.stAPInfo[i].u16CellId);
                                printf("\n");
                            }
                        }
                        break;
                    case E_MI_WLAN_NETWORKTYPE_AP:
                        MI_WLAN_GetStatus(&g_stStatus);
                        printf("Access port:\n");

                        for (int i = 0; i < g_stStatus.stApStatus.u16HostNum; i++)
                        {
                            printf("HostName: %s\n", g_stStatus.stApStatus.astHosts[i].hostname);
                            printf("IP: %s\n", (char*)g_stStatus.stApStatus.astHosts[i].ipaddr);
                            printf("MAC: %s\n", (char*)g_stStatus.stApStatus.astHosts[i].macaddr);
                            printf("Connected time: %lld\n", g_stStatus.stApStatus.astHosts[i].connectedtime);
                            printf("\n");
                        }
                        break;
                    default:
                        break;
                }
                break;
            case 'q':
                printf("[CMD]:exit\n");
                if (g_ptConn)
                {
                    pthread_join(g_ptConn, NULL);
                    g_ptConn = NULL;
                }

                if (g_bConnected)
                {
                    MI_WLAN_Disconnect(g_wlanHdl);
                    MI_WLAN_Close();
                    g_bConnected = false;
                }

                bExit = true;
                break;
            default:
                break;
        }

    }

    MI_WLAN_DeInit();

    return 0;
}
