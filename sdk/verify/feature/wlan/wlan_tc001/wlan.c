#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mi_sys.h"
#include "mi_wlan.h"



int main()
{
#if 1
    WLAN_HANDLE wlanHdl = -1;
    MI_WLAN_ScanResult_t stRst;
    MI_WLAN_OpenParams_t stOpenParam = {E_MI_WLAN_NETWORKTYPE_INFRA};
    MI_WLAN_Status_t  status;
    MI_WLAN_InitParams_t stParm = {"/config/wifi/wlan.json"};
    int i = 0;
    MI_WLAN_ConnectParam_t	stConnectParam =  \
    {E_MI_WLAN_SECURITY_WPA2, "overlord", "12345678", 5000};


    MI_WLAN_Init(&stParm);
    sleep(5);

    MI_WLAN_Open(&stOpenParam);
    //return 0;
    //stRst.u8APNumber = 64;
    //MI_WLAN_Scan(NULL, &stRst);
    sleep(5);
    MI_WLAN_Connect(&wlanHdl, &stConnectParam);

    sleep(60);
    MI_WLAN_GetStatus(&status);

    if(status.stStaStatus.state == WPA_COMPLETED)
        printf("%s %s\n", status.stStaStatus.ip_address, status.stStaStatus.ssid);
    else
        printf("sta inconnected\n");

    MI_WLAN_Disconnect(wlanHdl);

    sleep(3);
    MI_WLAN_Close();

    sleep(3);
    MI_WLAN_DeInit();
    return 0;
#endif
#if 0
    WLAN_HANDLE wlanHdl = -1;
    MI_WLAN_OpenParams_t stOpenParam = {E_MI_WLAN_NETWORKTYPE_AP};
    MI_WLAN_Status_t  status;
    int i = 0;
    MI_WLAN_InitParams_t stParm = {"/mnt/sdk/interface/src/wlan/wlan.json", 0};
    MI_WLAN_ConnectParam_t stConnectParam =  \
    {E_MI_WLAN_SECURITY_WPA2, "overlord", "12345678", 5000};


    MI_WLAN_Init(&stParm);
    sleep(5);

    MI_WLAN_Open(&stOpenParam);
    //return 0;
    //stRst.u8APNumber = 64;
    //MI_WLAN_Scan(NULL, &stRst);
    sleep(5);
    MI_WLAN_Connect(&wlanHdl, &stConnectParam);

    sleep(60);

    MI_WLAN_GetStatus(&status);

    for(i = 0; i < status.stApStatus.u16HostNum; i++)
    {
        printf("%s %s %s %lld\n", status.stApStatus.astHosts[i].hostname, \
               status.stApStatus.astHosts[i].ipaddr, status.stApStatus.astHosts[i].macaddr, \
               status.stApStatus.astHosts[i].connectedtime);
    }

    sleep(10);

    MI_WLAN_Disconnect(wlanHdl);

    sleep(3);
    MI_WLAN_Close();

    sleep(3);
    MI_WLAN_DeInit();
    return 0;
#endif

}

