#include <stdio.h>
#include <fcntl.h>  //open
#include <unistd.h> //getopt
#include <string.h> //memset
#include <stdlib.h> //strtol

typedef unsigned char u8;
typedef unsigned char bool;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
#include "mhal_pnl_datatype.h"
#include "pnl_table_RM68200.h"
#include "SAT070CP50_1024x600.h"
#include "ADT07016BR50_1024x600.h"
#include "SAT070BO30I21Y0_1024x600_MIPI.h"
#include "LX50FWB4001_RM68172_480x854_v2.h"
#include "T30P133_480x854.h"

#define DISP_DEVICE_NULL     0
#define DISP_DEVICE_HDMI     1
#define DISP_DEVICE_VGA      2
#define DISP_DEVICE_LCD      4

typedef struct __attribute__((packed)) {
    u8     magic_prefix[4];       //"LOGO"
    u32 dipslay_buffer_size;   //Hex
    u64 dipslay_buffer_start;  //Hex
    u16 width;            //Dec
    u16 high;             //Dec
    u8 pixel_clock;      //Dec
    u8 padding[11];     //padding[0] map to BootlogoImgConfig_t.interface
    u8 panelname[20];
    MhalPnlParamConfig_t stPnlPara;
    MhalPnlMipiDsiConfig_t stMipiDsiCfg;
}ST_LOGO_HEADER;

ST_LOGO_HEADER logo_header;

static unsigned Atoi(const char * pStr)
{
    int intStrLen = strlen(pStr);
    unsigned short bUseHex = 0;
    unsigned int intRetNumber = 0;
    int i = 0;

    if (pStr == NULL)
    {
        return 0xFFFFFFFF;
    }

    if (intStrLen > 2)
    {
        if (pStr[0] == '0' &&(pStr[1] == 'X' || pStr[1] == 'x'))
        {
            bUseHex = 1;
            pStr += 2;
        }
    }
    if (bUseHex == 1)
    {
        for (i = 0; i < intStrLen - 2; i++)
        {
            if ((pStr[i] > '9' || pStr[i] < '0')    \
                && (pStr[i] > 'f' || pStr[i] < 'a') \
                && (pStr[i] > 'F' || pStr[i] < 'A'))
            {
                return 0xFFFFFFFF;
            }
        }
        sscanf(pStr, "%x", &intRetNumber);
    }
    else
    {
        for (i = 0; i < intStrLen; i++)
        {
            if (pStr[i] > '9' || pStr[i] < '0')
            {
                return 0xFFFFFFFF;
            }
        }
        intRetNumber =  atoi(pStr);
    }
    return intRetNumber;
}
#ifdef __x86_64__
    #You must use gcc xxx -m32 for 32bit cpu!!!!!
#endif


int main(int argc, char *argv[]) {
    int fd, fd_logo;
    int result;
    int i=0;
	int read_bytes = 0;
    char bufLogo[32];
    memset(&logo_header, 0, sizeof(ST_LOGO_HEADER));
    memcpy(logo_header.magic_prefix,"LOGO",4);

    while((result = getopt(argc, argv, "w:h:t:a:s:p:")) != -1 )
    {
        switch(result) {
        case 'w': {
            logo_header.width=Atoi(optarg);
        }
        break;
        case 'h': {
            logo_header.high=Atoi(optarg);
        }
        break;
        case 't': {
            logo_header.pixel_clock=Atoi(optarg);
        }
        break;
        case 'a': {
            logo_header.dipslay_buffer_start=Atoi(optarg);
        }
        break;
        case 's': {
            logo_header.dipslay_buffer_size=Atoi(optarg);
        }
        break;
        case 'p': {
            strcpy(logo_header.panelname, optarg);
        }
        break;
        default:
            printf("no argv");
        }
    }
	printf("pnl: %s\n",logo_header.panelname);
    if (!strcmp(logo_header.panelname, "RM68200"))
    {
        memcpy(&logo_header.stPnlPara, &stPanel_720x1280_60_RM68200, sizeof(MhalPnlParamConfig_t));
        memcpy(&logo_header.stMipiDsiCfg, &tPanel_RM68200_720x1280_4Lane_Sync_Pulse_RGB888, sizeof(MhalPnlMipiDsiConfig_t));
		logo_header.padding[0] = DISP_DEVICE_LCD;
    }
	else if(!strcmp(logo_header.panelname, "SAT070CP50"))
	{
		printf("null mipi cfg\n");
		memcpy(&logo_header.stPnlPara, &stPanel_SAT070CP50_1024x600, sizeof(MhalPnlParamConfig_t));
		logo_header.padding[0] = DISP_DEVICE_LCD;
	}
    else if(!strcmp(logo_header.panelname, "ADT07016BR50"))
	{
		printf("null mipi cfg\n");
		memcpy(&logo_header.stPnlPara, &stPanel_ADT07016BR50_1024x600, sizeof(MhalPnlParamConfig_t));
		logo_header.padding[0] = DISP_DEVICE_LCD;
	}
    else if(!strcmp(logo_header.panelname, "LX50_RM68172_V2"))
	{
		printf("spi cfg PNL para size(%d)\n",(int)sizeof(MhalPnlParamConfig_t));
		memcpy(&logo_header.stPnlPara, &stPanel_LX50_RM68172_V2, sizeof(MhalPnlParamConfig_t));
        memcpy(&logo_header.stMipiDsiCfg, &stMipiDsiConfig_RM68172_V2, sizeof(MhalPnlMipiDsiConfig_t));
		logo_header.padding[0] = DISP_DEVICE_LCD;
	}
    else if(!strcmp(logo_header.panelname, "T30P133"))
	{
		memcpy(&logo_header.stPnlPara, &stPanel_T30P133, sizeof(MhalPnlParamConfig_t));
        memcpy(&logo_header.stMipiDsiCfg, &stMipiDsiConfig_T30P133, sizeof(MhalPnlMipiDsiConfig_t));
		logo_header.padding[0] = DISP_DEVICE_LCD;
	}
    else if(!strcmp(logo_header.panelname, "SAT070BO30I21Y0"))
	{
		printf("null mipi cfg\n");
		memcpy(&logo_header.stPnlPara, &stPanel_SAT070BO30I21Y0_1024x60, sizeof(MhalPnlParamConfig_t));
        memcpy(&logo_header.stMipiDsiCfg, &stMipiDsiConfig_SAT070BO30I21Y0_1024x600, sizeof(MhalPnlMipiDsiConfig_t));
		logo_header.padding[0] = DISP_DEVICE_LCD;
	}
	else
	{
		logo_header.padding[0] = DISP_DEVICE_HDMI;
	}
	//printf("MhalPnlParamConfig_t: %d,MhalPnlMipiDsiConfig_t: %d\n",sizeof(MhalPnlParamConfig_t),sizeof(MhalPnlMipiDsiConfig_t));
    fd = open(argv[argc-2],  O_CREAT | O_RDWR | O_TRUNC, 0777);
    if(fd ==-1)
    {
        printf("open %s failed\n", argv[argc-2]);
        return 0;
    }
    fd_logo = open(argv[argc-1], O_RDONLY);
    if(fd_logo ==-1)
    {
        printf("open %s failed\n", argv[argc-1]);
        return 0;
    }

    write(fd, &logo_header, sizeof(ST_LOGO_HEADER));
    if (logo_header.stMipiDsiCfg.u32CmdBufSize != 0)
    {
        printf("spi cfg PNL para size(%d) spi cmd size(%d )\n",(int)sizeof(MhalPnlParamConfig_t),logo_header.stMipiDsiCfg.u32CmdBufSize);
        write(fd, logo_header.stMipiDsiCfg.pu8CmdBuf, logo_header.stMipiDsiCfg.u32CmdBufSize);
    }

    do
    {
		read_bytes = read(fd_logo, bufLogo, 32);
		if (read_bytes)
		{
			write(fd, bufLogo, read_bytes);
		}
    }while(read_bytes);
    close(fd_logo);
    close(fd);

    printf("dipslay_start=0x%08llx\n", (logo_header.dipslay_buffer_start));
    printf("dipslay_size=0x%08x\n", (unsigned int)(logo_header.dipslay_buffer_size));
    printf("width=%d \thex:0x%04x\n", logo_header.width, logo_header.width);
    printf("high=%d \thex:0x%04x\n", logo_header.high, logo_header.high);
    printf("pixel_clock=%d \thex:0x%02x\n", logo_header.pixel_clock, logo_header.pixel_clock);

    return 0;
}
