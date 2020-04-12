#include <stdio.h>
#include "sys.h"
#include "rtsp.h"
#include "venc.h"
#include "vpe.h"
#include "vif.h"
#include "divp.h"
#include "dla.h"
#include "fdfr.h"
#include "ui.h"
#include "iq.h"

extern int g_new_add_userid;
extern char g_new_add_name[];
extern char g_new_del_name[];

void Sys::Implement(std::string &strKey)
{
    unsigned int intId = 0;

    //printf("Connect key str %s\n", strKey.c_str());
    intId = Sys::FindBlockId(strKey);
    if (intId == (unsigned int)-1)
    {
        printf("Can't find key str %s\n", strKey.c_str());
        return;
    }
    if (!Sys::FindBlock(strKey))
    {
        switch (intId)
        {
            case E_SYS_MOD_RTSP:
            {
                SysChild<Rtsp> Rtsp(strKey);
            }
            break;
            case E_SYS_MOD_VENC:
            {
                SysChild<Venc> Venc(strKey);
            }
            break;
            case E_SYS_MOD_VPE:
            {
                SysChild<Vpe> Vpe(strKey);
            }
            break;
            case E_SYS_MOD_VIF:
            {
                SysChild<Vif> Vif(strKey);
            }
            break;
            case E_SYS_MOD_DIVP:
            {
                SysChild<Divp> Divp(strKey);
            }
            break;
            case E_SYS_MOD_DLA:
            {
                SysChild<Dla> Dla(strKey);
            }
            break;
            case E_SYS_MOD_FDFR:
            {
                SysChild<Fdfr> Fdfr(strKey);
            }
            break;
            case E_SYS_MOD_UI:
            {
                SysChild<Ui> Fdfr(strKey);
            }
            break;
            case E_SYS_MOD_IQ:
            {
                SysChild<Iq> Iq(strKey);
            }
            break;
            default:
                return;
        }
        GetInstance(strKey)->BuildModTree();
    }

    return;
}


MI_S32 InputProcessLoop()
{
	char custom_strings[100];

    printf("IPU Demo enter\n");

    while(1)
    {
        // when mixer process is backgroud running,getchar() will not block and return -1,so need sleep
        sleep(1);

        MI_S8 ch = getchar();
        if('q' == ch)
        {
            break;//exit
        }
        else if('a' == ch)
        {
            MI_U32 trackid;
            char  username[256];
            MI_S32 ret;

            printf("please input trackid:");
            fflush(stdin);
            ret = scanf("%d",&trackid);
            getchar();
            while((ret != 1) || trackid<0)
            {
                printf("input  parameter error.\n");
                printf("please input trackid:");
                fflush(stdin);
                ret = scanf("%d",&trackid);
                getchar();
            }
            printf("please input username:");
            fflush(stdin);
            username[0] = 0;
            ret = scanf("%s", username);
            getchar();
            while((ret != 1) || strlen(username)<=0||strlen(username)>=100)
            {
                printf("invalid usernamer.\n");
                printf("please input username:");
                fflush(stdin);
                ret = scanf("%s",username);
                getchar();
            }
            strcpy(g_new_add_name, username);
            g_new_add_userid = trackid;
            printf("input name :%s trackid:%d\n",g_new_add_name,g_new_add_userid);

        }
        else if('d' == ch)
        {
            char username[256];
            MI_S32 ret;

            printf("please input username to del:");
            fflush(stdin);
            username[0] = 0;
            ret = scanf("%s", username);
            getchar();
            while((ret != 1) || strlen(username)<=0||strlen(username)>=100)
            {
                printf("invalid usernamer.\n");
                printf("please input username to del:");
                fflush(stdin);
                ret = scanf("%s",username);
                getchar();
            }
            strcpy(g_new_del_name, username);
        }
    }
}
int main(int argc, char **argv)
{
    std::map<std::string, unsigned int> mapModId;
    char getC = 0;

    if (argc != 2)
    {
        printf("Usage: ./%s xxx_ini_path\n", argv[0]);

        return -1;
    }
    mapModId["RTSP"] = E_SYS_MOD_RTSP;
    mapModId["VENC"] = E_SYS_MOD_VENC;
    mapModId["VPE"] = E_SYS_MOD_VPE;
    mapModId["DIVP"] = E_SYS_MOD_DIVP;
    mapModId["DISP"] = E_SYS_MOD_DISP;
    mapModId["VDEC"] = E_SYS_MOD_VDEC;
    mapModId["VIF"] = E_SYS_MOD_VIF;
    mapModId["VDISP"] = E_SYS_MOD_VDISP;
    mapModId["LDC"] = E_SYS_MOD_LDC;
    mapModId["DLA"] = E_SYS_MOD_DLA;
    mapModId["FDFR"] = E_SYS_MOD_FDFR;
    mapModId["UI"] = E_SYS_MOD_UI;
    mapModId["IQ"] = E_SYS_MOD_IQ;
    Sys::InitSys(argv[1], mapModId);
   // do
    //{
    //    printf("Press 'q' to exit!\n");
     //   getC = getchar();
    //}while (getC != 'q');
    InputProcessLoop();
    Sys::DeinitSys();

    return 0;
}
