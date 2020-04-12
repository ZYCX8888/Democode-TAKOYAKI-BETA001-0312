//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include <common.h>
#include <command.h>
#include <stdlib.h>

#if defined(CONFIG_SSTAR_JPD)
#include "jinclude.h"
#include "jpeglib.h"
#endif


#if defined(CONFIG_SSTAR_DISP)
#include "mhal_common.h"
#include "mhal_disp_datatype.h"
#include "mhal_disp.h"
#endif

#if defined(CONFIG_SSTAR_PNL)
#include "mhal_pnl_datatype.h"
#include "mhal_pnl.h"
#include <../drivers/mstar/panel/PnlTbl.h>
#endif

#if defined(CONFIG_SSTAR_RGN)
#include "mhal_rgn_datatype.h"
#include "mhal_rgn.h"
#endif

#if defined(CONFIG_MS_PARTITION)
#include "part_mxp.h"
#endif

#if defined(CONFIG_CMD_MTDPARTS)
#include <jffs2/jffs2.h>
/* partition handling routines */
int mtdparts_init(void);
int find_dev_and_part(const char *id, struct mtd_device **dev,
		u8 *part_num, struct part_info **part);
#endif


#include "blit32.h"

typedef struct
{
    void *pInBuff;
    u64  u64InBuffAddr;
    u32  u32InBuffSize;
    u64  u64OutBuffAddr;
    u32  u32OutBuffSize;
    u16  u16DisplayWidth;
    u16  u16DisplayHeight;
    u8   u8DisplayRate;
    u8   u8Interface;
#if defined(CONFIG_SSTAR_PNL)
    u8 panelname[20];
    MhalPnlParamConfig_t stPnlPara;
    MhalPnlMipiDsiConfig_t stMipiDsiCfg;
#endif
}BootlogoImgConfig_t;



#define BOOTFB_DBG_LEVEL_INFO    0x01
#define BOOTFB_DBG_LEVEL_ERR     0x02

#define BOOTFB_DBG(dbglv, _fmt, _args...)    \
    do                                          \
    if(dbglv>=BOOTFB_DBG_LEVEL_ERR)              \
    {                                           \
            printf(_fmt, ## _args);             \
    }while(0)

#define MAKE_YUYV_VALUE(y,u,v)    ((y) << 24) | ((u) << 16) |((y) << 8) | (v)
#define YUYV_RED                MAKE_YUYV_VALUE(76,84,255)
#define FBDEV_GOP_NUM 2

#define DISP_DEVICE_NULL     0
#define DISP_DEVICE_HDMI     1
#define DISP_DEVICE_VGA      2
#define DISP_DEVICE_LCD      4


static MHAL_RGN_GopWindowConfig_t srcWincfg[FBDEV_GOP_NUM];
static MHAL_RGN_GopWindowConfig_t dstWincfg[FBDEV_GOP_NUM];
static BootlogoImgConfig_t  stBootlogoImgCfg;

int _BootFBGetImageConfig(BootlogoImgConfig_t *pCfg)
{
    //Length
    #define LOGO_HEADER_MAGIC_PREFIX_LEN                (4)
    #define LOGO_HEADER_OUT_BUFFER_SIZE_LEN             (4)
    #define LOGO_HEADER_OUT_BUFFER_ADDR_LEN             (8)
    #define LOGO_HEADER_DISPLAY_WIDTH_LEN               (2)
    #define LOGO_HEADER_DISPLAY_HEIGHT_LEN              (2)
    #define LOGO_HEADER_DISPLAY_RATE_LEN                (1)
    #define LOGO_HEADER_DISPLAY_INTERFACE_LEN           (1)
    #define LOGO_HEADER_RESERVED_LEN                    (10)

#if defined(CONFIG_SSTAR_PNL)
    #define LOGO_HEADER_PANEL_NAME_LEN                  (20)
    #define LOGO_HEADER_PANEL_INIT_PARA_LEN             (sizeof(MhalPnlParamConfig_t))
    #define LOGO_HEADER_PANEL_MIPI_DST_CONFIG_LEN       (sizeof(MhalPnlMipiDsiConfig_t))
    #define LOGO_HEADER_SIZE                            (52 + LOGO_HEADER_PANEL_INIT_PARA_LEN + LOGO_HEADER_PANEL_MIPI_DST_CONFIG_LEN)
#else
    #define LOGO_HEADER_SIZE                            (32)
#endif

    // Offset
    #define LOGO_HEADER_MAGIC_PREFIX_OFFSET             (0)
    #define LOGO_HEADER_OUT_BUFFER_SIZE_OFFSET          (LOGO_HEADER_MAGIC_PREFIX_OFFSET + LOGO_HEADER_MAGIC_PREFIX_LEN)
    #define LOGO_HEADER_OUT_BUFFER_ADDR_OFFSET          (LOGO_HEADER_OUT_BUFFER_SIZE_OFFSET + LOGO_HEADER_OUT_BUFFER_SIZE_LEN)
    #define LOGO_HEADER_DISPLAY_WIDTH_OFFSET            (LOGO_HEADER_OUT_BUFFER_ADDR_OFFSET + LOGO_HEADER_OUT_BUFFER_ADDR_LEN)
    #define LOGO_HEADER_DISPLAY_HEIGHT_OFFSET           (LOGO_HEADER_DISPLAY_WIDTH_OFFSET + LOGO_HEADER_DISPLAY_WIDTH_LEN)
    #define LOGO_HEADER_DISPLAY_FPS_OFFSET              (LOGO_HEADER_DISPLAY_HEIGHT_OFFSET + LOGO_HEADER_DISPLAY_HEIGHT_LEN)
    #define LOGO_HEADER_INTERFACE_ID_OFFSET             (LOGO_HEADER_DISPLAY_FPS_OFFSET + LOGO_HEADER_DISPLAY_RATE_LEN)
    #define LOGO_HEADER_RESERVED_OFFSET                 (LOGO_HEADER_INTERFACE_ID_OFFSET + LOGO_HEADER_DISPLAY_INTERFACE_LEN)
#if defined(CONFIG_SSTAR_PNL)
    #define LOGO_HEADER_PANEL_NAME_OFFSET               (LOGO_HEADER_RESERVED_OFFSET + LOGO_HEADER_RESERVED_LEN)
    #define LOGO_HEADER_PANEL_INIT_PARA_OFFSET          (LOGO_HEADER_PANEL_NAME_OFFSET + LOGO_HEADER_PANEL_NAME_LEN)
    #define LOGO_HEADER_PANEL_MIPI_DST_CONFIG_OFFSET    (LOGO_HEADER_PANEL_INIT_PARA_OFFSET + LOGO_HEADER_PANEL_INIT_PARA_LEN)
#endif

    #define LOGO_FLAHS_BASE     0x14000000

    u64     start, size;
	char strENVName[] = "LOGO";
    int idx;
#if defined(CONFIG_CMD_MTDPARTS) || defined(CONFIG_MS_SPINAND)
    struct mtd_device *dev;
    struct part_info *part;
    u8 pnum;
    int ret;

    ret = mtdparts_init();
    if (ret)
        return ret;

    ret = find_dev_and_part(strENVName, &dev, &pnum, &part);
    if (ret)
    {
        return ret;
    }

    if (dev->id->type != MTD_DEV_TYPE_NAND)
    {
        puts("not a NAND device\n");
        return -1;
    }

    start = part->offset;
    size = part->size;
#elif defined(CONFIG_MS_PARTITION)
    mxp_record rec;
	mxp_load_table();
	idx=mxp_get_record_index(strENVName);
	if(idx<0)
	{
        BOOTFB_DBG(BOOTFB_DBG_LEVEL_ERR, "can not found mxp record: %s\n", strENVName);
        return FALSE;
	}

	if(0 != mxp_get_record_by_index(idx,&rec))
    {
        BOOTFB_DBG(BOOTFB_DBG_LEVEL_ERR, "failed to get MXP record with name: %s\n", strENVName);
        return 0;
    }
    start = rec.start;
    size = rec.size;
#else
    start = 0;
    size = 0;
    return 0;
#endif

	{
		BOOTFB_DBG(BOOTFB_DBG_LEVEL_INFO, "%s in flash offset=0x%llx size=0x%llx\n",strENVName , start, size);

		pCfg->pInBuff = malloc(size);
		if(pCfg->pInBuff == NULL)
		{
		    BOOTFB_DBG(BOOTFB_DBG_LEVEL_ERR, "allocate buffer fail\n");
            return 0;
		}
    #if defined(CONFIG_CMD_MTDPARTS) || defined(CONFIG_MS_SPINAND)
        char  cmd_str[128];
        sprintf(cmd_str, "nand read.e 0x%p %s", pCfg->pInBuff, strENVName);
        run_command(cmd_str, 0);
    #else
        //sprintf(cmd_str, "sf probe; sf read 0x%p 0x%p 0x%p", pCfg->pInBuff, start, size);
        //run_command(cmd_str, 0);
        memcpy(pCfg->pInBuff, (void*)(u32)(start | LOGO_FLAHS_BASE), size);
        //printf("sf read dst:0x%08x src:0x%08x %08x", pCfg->pInBuff, start, size);
    #endif

        flush_cache((u32)pCfg->pInBuff, size);

        //Parsing Header
        for(idx=0; idx<4; idx++)
        {
            if( strENVName[idx] != *((u8 *)(pCfg->pInBuff+idx)))
            {
                BOOTFB_DBG(BOOTFB_DBG_LEVEL_ERR, "Header check fail\n");
                return 0;
            }
        }

        pCfg->u32OutBuffSize    = *((u32 *)(pCfg->pInBuff +  LOGO_HEADER_OUT_BUFFER_SIZE_OFFSET));
        pCfg->u64OutBuffAddr    = *((u64 *)(pCfg->pInBuff +  LOGO_HEADER_OUT_BUFFER_ADDR_OFFSET));
        pCfg->u16DisplayWidth   = *((u16 *)(pCfg->pInBuff +  LOGO_HEADER_DISPLAY_WIDTH_OFFSET));
        pCfg->u16DisplayHeight  = *((u16 *)(pCfg->pInBuff +  LOGO_HEADER_DISPLAY_HEIGHT_OFFSET));
        pCfg->u8DisplayRate     = *((u8 *) (pCfg->pInBuff +  LOGO_HEADER_DISPLAY_FPS_OFFSET));
        pCfg->u8Interface       = *((u8 *) (pCfg->pInBuff +  LOGO_HEADER_INTERFACE_ID_OFFSET));
#if defined(CONFIG_SSTAR_PNL)
        memcpy(pCfg->panelname, pCfg->pInBuff + LOGO_HEADER_PANEL_NAME_OFFSET, LOGO_HEADER_PANEL_NAME_LEN);
        memcpy(&pCfg->stPnlPara, pCfg->pInBuff + LOGO_HEADER_PANEL_INIT_PARA_OFFSET, LOGO_HEADER_PANEL_INIT_PARA_LEN);
        memcpy(&pCfg->stMipiDsiCfg, pCfg->pInBuff + LOGO_HEADER_PANEL_MIPI_DST_CONFIG_OFFSET, LOGO_HEADER_PANEL_MIPI_DST_CONFIG_LEN);
        pCfg->u64InBuffAddr     =  (u32)pCfg->pInBuff + LOGO_HEADER_SIZE + pCfg->stMipiDsiCfg.u32CmdBufSize;
        pCfg->u32InBuffSize     =  (u32)(size - LOGO_HEADER_SIZE + pCfg->stMipiDsiCfg.u32CmdBufSize);
        pCfg->stMipiDsiCfg.pu8CmdBuf = malloc(pCfg->stMipiDsiCfg.u32CmdBufSize);
        if (pCfg->stMipiDsiCfg.pu8CmdBuf == NULL)
        {
            printf("mipi dii cfg malloc error!!\n");
            return -1;
        }
        memcpy(pCfg->stMipiDsiCfg.pu8CmdBuf, pCfg->pInBuff + LOGO_HEADER_SIZE, pCfg->stMipiDsiCfg.u32CmdBufSize);
        printf("Panel name %s,pu8CmdBuf=0x%08x\n", pCfg->panelname,LOGO_HEADER_SIZE);
        BOOTFB_DBG(BOOTFB_DBG_LEVEL_INFO,"mipi cmd buf size %d\n", pCfg->stMipiDsiCfg.u32CmdBufSize);
        BOOTFB_DBG(BOOTFB_DBG_LEVEL_INFO,"PNL para size %d\n", sizeof(MhalPnlParamConfig_t));
        BOOTFB_DBG(BOOTFB_DBG_LEVEL_INFO,"PNL mipi size %d\n", sizeof(MhalPnlMipiDsiConfig_t));
        //_BootLogo_dump_buf(pCfg->stMipiDsiCfg.pu8CmdBuf,256);
#else
        pCfg->u64InBuffAddr     =  (u32)pCfg->pInBuff + LOGO_HEADER_SIZE;
        pCfg->u32InBuffSize     =  (u32)(size - LOGO_HEADER_SIZE);
#endif
        BOOTFB_DBG(BOOTFB_DBG_LEVEL_INFO, "InBuf:(%x), IN(%llx %x), OUT:(%llx, %x), DISP(%x %x %x), Interface:%x\n",
            (u32)pCfg->pInBuff,
            pCfg->u64InBuffAddr, pCfg->u32InBuffSize,
            pCfg->u64OutBuffAddr, pCfg->u32OutBuffSize,
            pCfg->u16DisplayWidth, pCfg->u16DisplayHeight,
            pCfg->u8DisplayRate, pCfg->u8Interface);
    }

    return 1;
}


static MS_S32 BootFBMemAlloc(MS_U8 *pu8Name, MS_U32 size, unsigned long long *pu64PhyAddr)
{
    return 0;
}

static MS_S32 BootFBMemRelease(unsigned long long u64PhyAddr)
{
    return 0;
}

void _BootFBPnlCtrl(BootlogoImgConfig_t *pstBootLogoImgCfg)
{
#if defined(CONFIG_SSTAR_PNL)
    void *pPnlDev;
    u32 u32DbgLevel = 0x0F;
    
	MhalPnlSetDebugLevel((void *)&u32DbgLevel);

    if(pstBootLogoImgCfg->u8Interface == DISP_DEVICE_LCD && pstBootLogoImgCfg->panelname[0] != 0)
    {
        BOOTFB_DBG(BOOTFB_DBG_LEVEL_INFO, "%s %d, PnlLink:%d\n",
            __FUNCTION__, __LINE__, pstBootLogoImgCfg->stPnlPara.eLinkType);

        MhalPnlSetDebugLevel((void *)&u32DbgLevel);

        if(MhalPnlCreateInstance(&pPnlDev, pstBootLogoImgCfg->stPnlPara.eLinkType) == FALSE)
        {
            BOOTFB_DBG(BOOTFB_DBG_LEVEL_ERR, "%s %d, PnlCreateInstance Fail\n", __FUNCTION__, __LINE__);
            return;
        }

        MhalPnlSetParamConfig(pPnlDev, &pstBootLogoImgCfg->stPnlPara);

        if(pstBootLogoImgCfg->stPnlPara.eLinkType == E_MHAL_PNL_LINK_MIPI_DSI)
        {
            MhalPnlSetMipiDsiConfig(pPnlDev, &pstBootLogoImgCfg->stMipiDsiCfg);
        }
        else if(pstBootLogoImgCfg->stPnlPara.eLinkType == E_MHAL_PNL_LINK_TTL_SPI_IF)
        {
            //_BootLogo_dump_buf(pstBootLogoImgCfg->stMipiDsiCfg.pu8CmdBuf,256);
            Lcm_init((unsigned char *)pstBootLogoImgCfg->stMipiDsiCfg.pu8CmdBuf, pstBootLogoImgCfg->stMipiDsiCfg.u32CmdBufSize);
        }
    }
#endif
}
void _BootFBDispCtrl(BootlogoImgConfig_t *pstBootLogoImgCfg)
{
#if defined(CONFIG_SSTAR_PNL)
	static void *pDevCtx = NULL;
	u32 u32Interface = MHAL_DISP_INTF_LCD;
	MHAL_DISP_DeviceTimingInfo_t stTimingInfo;
    MHAL_DISP_AllocPhyMem_t stPhyMem;
	
    stPhyMem.alloc = BootFBMemAlloc;
    stPhyMem.free  = BootFBMemRelease;

	if(pDevCtx == NULL)
	{
		if(MHAL_DISP_DeviceCreateInstance(&stPhyMem, 0, &pDevCtx) == FALSE)
		{
			BOOTFB_DBG(BOOTFB_DBG_LEVEL_ERR, "%s %d, CreateDevice Fail\n", __FUNCTION__, __LINE__);
			return ;
		}
	}
	MHAL_DISP_DeviceSetBackGroundColor(pDevCtx, YUYV_RED);
	MHAL_DISP_DeviceEnable(pDevCtx, 0);
	MHAL_DISP_DeviceAddOutInterface(pDevCtx, u32Interface);
	
	MHAL_DISP_SyncInfo_t stSyncInfo;
	stSyncInfo.u16Vact = pstBootLogoImgCfg->stPnlPara.u16Height;
	stSyncInfo.u16Vbb  = pstBootLogoImgCfg->stPnlPara.u16VSyncBackPorch;
	stSyncInfo.u16Vpw  = pstBootLogoImgCfg->stPnlPara.u16VSyncWidth;
	stSyncInfo.u16Vfb  = pstBootLogoImgCfg->stPnlPara.u16VTotal - stSyncInfo.u16Vact - stSyncInfo.u16Vbb - stSyncInfo.u16Vpw;
	stSyncInfo.u16Hact = pstBootLogoImgCfg->stPnlPara.u16Width;
	stSyncInfo.u16Hbb  = pstBootLogoImgCfg->stPnlPara.u16HSyncBackPorch;
	stSyncInfo.u16Hpw  = pstBootLogoImgCfg->stPnlPara.u16HSyncWidth;
	stSyncInfo.u16Hfb  = pstBootLogoImgCfg->stPnlPara.u16HTotal - stSyncInfo.u16Hact - stSyncInfo.u16Hbb - stSyncInfo.u16Hpw;
	stSyncInfo.u32FrameRate = pstBootLogoImgCfg->u8DisplayRate;
	
	BOOTFB_DBG(BOOTFB_DBG_LEVEL_INFO, "%s %d, H(%d %d %d %d) V(%d %d %d %d) Fps:%d\n",
		__FUNCTION__, __LINE__,
		stSyncInfo.u16Hfb, stSyncInfo.u16Hpw, stSyncInfo.u16Hbb, stSyncInfo.u16Hact,
		stSyncInfo.u16Vfb, stSyncInfo.u16Vpw, stSyncInfo.u16Vbb, stSyncInfo.u16Vact,
		stSyncInfo.u32FrameRate);
	
	stTimingInfo.eTimeType = E_MHAL_DISP_OUTPUT_USER;
	stTimingInfo.pstSyncInfo = &stSyncInfo;
	MHAL_DISP_DeviceSetOutputTiming(pDevCtx, MHAL_DISP_INTF_LCD, &stTimingInfo);
	MHAL_DISP_DeviceEnable(pDevCtx, 1);

#endif

}

static void _BootLogoJpdCtrl(BootlogoImgConfig_t  *pstBootlogoImgCfg)
{
#if defined(CONFIG_SSTAR_JPD)
    u32 u32JpgSize;
    u8 *pu8JpgBuffer;

    // Variables for the decompressor itself
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    u8 *framebuffer;
	u8* linebuffer;
    u16 u16RowStride, u16Width, u16PixelSize;
    int rc; //, i, j;

    u32JpgSize = pstBootlogoImgCfg->u32InBuffSize;
    pu8JpgBuffer = (unsigned char *)((u32)pstBootlogoImgCfg->u64InBuffAddr);

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, pu8JpgBuffer, u32JpgSize);
    rc = jpeg_read_header(&cinfo, TRUE);

    if (rc != 1)
    {
        return;
    }

	cinfo.out_color_space = JCS_RGB;

    jpeg_start_decompress(&cinfo);

    u16Width = cinfo.output_width;
    u16PixelSize = cinfo.output_components;

    framebuffer = (unsigned char *)((u32)pstBootlogoImgCfg->u64OutBuffAddr + 0x20000000);

	u16RowStride = u16Width * u16PixelSize;
	linebuffer = malloc(u16RowStride);
	if(!linebuffer)
		return;
	
	
	while (cinfo.output_scanline < cinfo.output_height)
	{
		unsigned char *buffer_array[1];
		buffer_array[0] = linebuffer ;
		u8* pixel=linebuffer;
		jpeg_read_scanlines(&cinfo, buffer_array, 1);
		for(int i = 0;i<u16Width;i++,pixel+=cinfo.output_components)
		{
			*(((int*)framebuffer)+i) = 0xFF<<24|(*(pixel))<<16|(*(pixel+1))<<8|(*(pixel+2));
		}
		framebuffer+=u16Width*4;
	}

    jpeg_finish_decompress(&cinfo);

    jpeg_destroy_decompress(&cinfo);
	free(linebuffer);
    flush_dcache_all();
#endif
}

int do_bootfb_disp(BootlogoImgConfig_t*  pstBootlogoImgCfg)
{
	_BootFBGetImageConfig(pstBootlogoImgCfg);
	_BootFBPnlCtrl(pstBootlogoImgCfg);
	_BootFBDispCtrl(pstBootlogoImgCfg);
	return 0;
}

/**
 * draw Rectangle. the colormat of Framebuffer is ARGB8888
 */
void drawRect_rgb32 (int x0, int y0, int width, 
	int height, int color,unsigned char* frameBuffer )
{
    const int stride = stBootlogoImgCfg.stPnlPara.u16Width;

    int *dest = (int *) (frameBuffer)
        + (y0) * stride + (x0);

    int x, y;
    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            dest[x] = color;
        }
        dest += stride;
    }
}

int do_bootfb_gop(int gopid,int gwinid,BootlogoImgConfig_t* pstBootlogoImgCfg)
{
	MHAL_RGN_GopInit();
    MHAL_RGN_GopSetBaseWindow(gopid, &srcWincfg[gopid], &dstWincfg[gopid]);
    MHAL_RGN_GopGwinSetPixelFormat(gopid, gwinid, E_MHAL_RGN_PIXEL_FORMAT_ARGB8888);
    MHAL_RGN_GopGwinSetWindow(gopid, gwinid,
        srcWincfg[gopid].u32Width, srcWincfg[gopid].u32Height,  srcWincfg[gopid].u32Width*4, 0, 0);
    MHAL_RGN_GopGwinSetBuffer(gopid, gwinid, pstBootlogoImgCfg->u64OutBuffAddr);
    MHAL_RGN_GopSetAlphaZeroOpaque(gopid, FALSE, FALSE, E_MHAL_RGN_PIXEL_FORMAT_ARGB8888);
    MHAL_RGN_GopSetAlphaType(gopid, gwinid, E_MHAL_GOP_GWIN_ALPHA_CONSTANT, 0xFF);
    MHAL_RGN_GopGwinEnable(gopid,gwinid);
	memset((unsigned char*)((u32)(pstBootlogoImgCfg->u64OutBuffAddr+0x20000000)),0,\
		srcWincfg[gopid].u32Height*srcWincfg[gopid].u32Width*4);
	return 0;
}
void do_bootfb_progress(u8 progress,char* message,BootlogoImgConfig_t* pstBootlogoImgCfg)
{
#define BGCOLOR 0xFF00FFFF
#define BARCOLOR 0xFF0000FF
#define BARCOLOR_DARK 0xFF000040

	static u8 bInited = 0;
	static u8 u8Completed = 0;
	static u8 u8PreMsgLen = 0;
	
	blit_props props;
	int completed = progress/10;
	u16 width = pstBootlogoImgCfg->stPnlPara.u16Width;
	u16 height = pstBootlogoImgCfg->stPnlPara.u16Height;
	unsigned char* framebuffer = (unsigned char*)((u32)(pstBootlogoImgCfg->u64OutBuffAddr+0x20000000));
	
	props.Buffer = (blit_pixel *)(framebuffer);
	props.BufHeight = height;
	props.BufWidth= width;
	props.Value = 0xFF000000;
	props.Wrap = 0;
	props.Scale = 4;

	
	int offset = 10+blit32_ADVANCE*props.Scale*strlen("[progress][100%]");
	int w = (width-offset)/20;
	if(!bInited)
	{
		drawRect_rgb32(0,0,width,height,BGCOLOR,framebuffer);
		char str[]="[progess]";
		blit32_TextProps(props,10,height/3,str);
		
		for(int i=0;i<completed;i++)
		{
			drawRect_rgb32(offset+w*i*2,height/3,w,blit32_ADVANCE*props.Scale,BARCOLOR_DARK,framebuffer);
		}
		for(int i=completed;i<10;i++)
		{
			drawRect_rgb32(offset+w*i*2,height/3,w,blit32_ADVANCE*props.Scale,BARCOLOR,framebuffer);
		}
		u8Completed = completed;
		bInited = 1;
	}
	else
	{
		for(int i=u8Completed;i<completed;i++)
		{
			drawRect_rgb32(offset+w*i*2,height/3,w,blit32_ADVANCE*props.Scale,BARCOLOR_DARK,framebuffer);
		}
	}

	char str[] = "[   %]";
	str[3] = '0'+progress%10;
	if(progress>=10)
	{
		str[2] = '0'+(progress%100)/10;
	}
	if(progress>=100)
	{
		str[1] = '0'+progress/100;
	}
	offset = 10+blit32_ADVANCE*props.Scale*strlen("[progress]");
	drawRect_rgb32(offset,height/3,blit32_ADVANCE*props.Scale*strlen(str),blit32_ADVANCE*props.Scale,BGCOLOR,framebuffer);
	blit32_TextProps(props,offset,height/3,str);
	if(message!=NULL)
	{
		props.Scale = 8;
		if(u8PreMsgLen>0)
		{
			offset = (width-blit32_ADVANCE*props.Scale*u8PreMsgLen)>>1;
			drawRect_rgb32(offset,height*2/3,blit32_ADVANCE*props.Scale*u8PreMsgLen,(blit32_ADVANCE+1)*props.Scale,BGCOLOR,framebuffer);
		}
		u8PreMsgLen  = strlen(message);
		offset = (width-blit32_ADVANCE*props.Scale*u8PreMsgLen)>>1;
		blit32_TextProps(props,offset,height*2/3,message);
	}
	flush_dcache_all();
}
void do_bootfb_logo(BootlogoImgConfig_t* pstBootlogoImgCfg)
{
	_BootLogoJpdCtrl(pstBootlogoImgCfg);
}
void do_bootfb_blank(BootlogoImgConfig_t* pstBootlogoImgCfg)
{
	memset((unsigned char*)((u32)(pstBootlogoImgCfg->u64OutBuffAddr+0x20000000)),0,\
		stBootlogoImgCfg.stPnlPara.u16Width*stBootlogoImgCfg.stPnlPara.u16Height*4);
}

int do_bootfb (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	static int gopid = 1;
	static int gwinid = 0;


	if(argc<2)
	{
		printf("usage\n");
		printf("1.bootframebuffer init [gopid] [gwinid]\n");
		printf("2.bootframebuffer progressbar <percentage> [message]\n");
		printf("3.bootframebuffer bootlogo \n");
		printf("4.bootframebuffer blank\n");
	}
	else
	{
		if(strcmp("init",argv[1])==0)
		{
			if(argc>=4)
			{
				gopid = simple_strtoul(argv[2], NULL, 10);
				gwinid = simple_strtoul(argv[3], NULL, 10);
			}
			
			do_bootfb_disp(&stBootlogoImgCfg);	
			//stBootlogoImgCfg.u64OutBuffAddr = 1024*1024*64;
			srcWincfg[gopid].u32X= 0;
			srcWincfg[gopid].u32Y = 0;
			srcWincfg[gopid].u32Width= stBootlogoImgCfg.stPnlPara.u16Width;
			srcWincfg[gopid].u32Height=stBootlogoImgCfg.stPnlPara.u16Height;
			memcpy(&dstWincfg[gopid],&srcWincfg[gopid],sizeof(MHAL_RGN_GopWindowConfig_t));
			do_bootfb_gop(gopid,gwinid,&stBootlogoImgCfg);
		}
		else if(strcmp("progressbar",argv[1])==0)
		{
			
			u8 progress = simple_strtoul(argv[2], NULL, 10);
			if(argc>=4)
			{
				do_bootfb_progress(progress,argv[3],&stBootlogoImgCfg);
			}
			else
			{
				do_bootfb_progress(progress,NULL,&stBootlogoImgCfg);
			}
		}
		else if(strcmp("bootlogo",argv[1])==0)
		{
			do_bootfb_logo(&stBootlogoImgCfg);
		}
		else if(strcmp("blank",argv[1])==0)
		{
			do_bootfb_blank(&stBootlogoImgCfg);
		}
	}

	return 0;
}


U_BOOT_CMD(
	bootframebuffer, CONFIG_SYS_MAXARGS, 1,    do_bootfb,
	"boot framebuffer \n" \
	"                 1.bootframebuffer init [gopid] [gwinid]\n" \
	"                 2.bootframebuffer progressbar <percentage> [message]\n" \
	"                 3.bootframebuffer bootlogo \n" \
	"                 4.bootframebuffer blank\n",
	NULL
);

