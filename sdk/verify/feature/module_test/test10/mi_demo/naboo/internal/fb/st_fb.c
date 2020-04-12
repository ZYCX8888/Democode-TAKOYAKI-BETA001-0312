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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>

#include "mi_sys.h"
#include "st_common.h"
#include "st_fb.h"
#include "mstarFb.h"
#include "mi_gfx.h"

#define FB_ROTATION    0

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

//Start of frame buffer mem
static MI_U8 *frameBuffer = NULL; // for user
static MI_U8 *frameBufferBack = NULL; // for GOP
static MI_U8 *frameBufferBlit = NULL; // for blit
static MI_PHY frameBufferPhy = 0;
static MI_PHY frameBufferBackPhy = 0;
static MI_PHY frameBufferBlitPhy = 0;
static MI_S32 g_fbFd = 0;
static MI_S32 g_screensize = 0;

/**
 *dump fix info of Framebuffer
 */
void printFixedInfo ()
{
    printf ("Fixed screen info:\n"
            "\tid: %s\n"
            "\tsmem_start: 0x%lx\n"
            "\tsmem_len: %d\n"
            "\ttype: %d\n"
            "\ttype_aux: %d\n"
            "\tvisual: %d\n"
            "\txpanstep: %d\n"
            "\typanstep: %d\n"
            "\tywrapstep: %d\n"
            "\tline_length: %d\n"
            "\tmmio_start: 0x%lx\n"
            "\tmmio_len: %d\n"
            "\taccel: %d\n"
            "\n",
            finfo.id, finfo.smem_start, finfo.smem_len, finfo.type,
            finfo.type_aux, finfo.visual, finfo.xpanstep, finfo.ypanstep,
            finfo.ywrapstep, finfo.line_length, finfo.mmio_start,
            finfo.mmio_len, finfo.accel);
}

/**
 *dump var info of Framebuffer
 */
void printVariableInfo ()
{
    printf ("Variable screen info:\n"
            "\txres: %d\n"
            "\tyres: %d\n"
            "\txres_virtual: %d\n"
            "\tyres_virtual: %d\n"
            "\tyoffset: %d\n"
            "\txoffset: %d\n"
            "\tbits_per_pixel: %d\n"
            "\tgrayscale: %d\n"
            "\tred: offset: %2d, length: %2d, msb_right: %2d\n"
            "\tgreen: offset: %2d, length: %2d, msb_right: %2d\n"
            "\tblue: offset: %2d, length: %2d, msb_right: %2d\n"
            "\ttransp: offset: %2d, length: %2d, msb_right: %2d\n"
            "\tnonstd: %d\n"
            "\tactivate: %d\n"
            "\theight: %d\n"
            "\twidth: %d\n"
            "\taccel_flags: 0x%x\n"
            "\tpixclock: %d\n"
            "\tleft_margin: %d\n"
            "\tright_margin: %d\n"
            "\tupper_margin: %d\n"
            "\tlower_margin: %d\n"
            "\thsync_len: %d\n"
            "\tvsync_len: %d\n"
            "\tsync: %d\n"
            "\tvmode: %d\n"
            "\n",
            vinfo.xres, vinfo.yres, vinfo.xres_virtual, vinfo.yres_virtual,
            vinfo.xoffset, vinfo.yoffset, vinfo.bits_per_pixel,
            vinfo.grayscale, vinfo.red.offset, vinfo.red.length,
            vinfo.red.msb_right, vinfo.green.offset, vinfo.green.length,
            vinfo.green.msb_right, vinfo.blue.offset, vinfo.blue.length,
            vinfo.blue.msb_right, vinfo.transp.offset, vinfo.transp.length,
            vinfo.transp.msb_right, vinfo.nonstd, vinfo.activate,
            vinfo.height, vinfo.width, vinfo.accel_flags, vinfo.pixclock,
            vinfo.left_margin, vinfo.right_margin, vinfo.upper_margin,
            vinfo.lower_margin, vinfo.hsync_len, vinfo.vsync_len,
            vinfo.sync, vinfo.vmode);
}

/**
 *get Color format fo framebuffer
 */
MI_FB_ColorFmt_e getFBColorFmt(struct fb_var_screeninfo *var)
{
    MI_FB_ColorFmt_e fmt = E_MI_FB_COLOR_FMT_INVALID;

    switch (var->bits_per_pixel)
    {
        case 16:
        {
            if (var->transp.length == 0) //RGB565
            {
                fmt = E_MI_FB_COLOR_FMT_RGB565;
            }
            else if (var->transp.length ==1) //ARGB 1555
            {
                fmt = E_MI_FB_COLOR_FMT_ARGB1555;
            }
            else if (var->transp.length == 4) //ARGB4444
            {
                fmt = E_MI_FB_COLOR_FMT_ARGB4444;
            }
        }
        break;
        //ARGB8888
        case 32:
        {
            fmt = E_MI_FB_COLOR_FMT_ARGB8888;
        }
        break;
        default:
            fmt = E_MI_FB_COLOR_FMT_INVALID;
            break;
    }
    return fmt;
}

/**
 * draw Rectangle. the colormat of Framebuffer is ARGB8888
 */
void drawRect_rgb32 (int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 4;
    const int stride = finfo.line_length / bytesPerPixel;

    int *dest = (int *) (frameBuffer)
        + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset);

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

/**
 * draw Rectangle. the colormat of Framebuffer is RGB565
 */
void drawRect_rgb16 (int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 2;
    const int stride = finfo.line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 2);
    const int blue = (color & 0xff) >> 3;
    const short color16 = blue | (green << 5) | (red << (5 + 6));

    short *dest = (short *) (frameBuffer)
        + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset);

    int x, y;
    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            dest[x] = color16;
        }
        dest += stride;
    }
}

/**
 *draw point. the color format of Framebuffer is ARGB1555
 */
void drawPoint_rgb15 (int x, int y, int color)
{
    const int bytesPerPixel = 2;
    const int stride = finfo.line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 3);
    const int blue = (color & 0xff) >> 3;
    const short color15 = blue | (green << 5) | (red << (5 + 5)) | 0x8000;

    short *dest = (short *) (frameBuffer)
        + (y + vinfo.yoffset) * stride + (x + vinfo.xoffset);

    dest[0] = color15;
}

void drawPoint_rgb15Ex (int x, int y, int color)
{
    const int bytesPerPixel = 2;
#if FB_ROTATION
    const int stride = vinfo.yres;
#else
    const int stride = finfo.line_length / bytesPerPixel;
#endif
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 3);
    const int blue = (color & 0xff) >> 3;
    const short color15 = blue | (green << 5) | (red << (5 + 5)) | 0x8000;

    short *dest = (short *) (frameBuffer)
        + (y + vinfo.yoffset) * stride + (x + vinfo.xoffset);

    dest[0] = color15;
}

void drawLine_rgb15(int x0, int y0, int x1, int y1, int color)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int ux = dx >0 ? 1 : -1;
    int uy = dx >0 ? 1 : -1;
    int dx2 = dx << 1;
    int dy2 = dy << 1;
    int e, x, y;

    if(abs(dx) > abs(dy))
    {
        e = -dx;
        x = x0;
        y = y0;
        for (x = x0; x < x1; x += ux)
        {
            // printf ("%d,%d\n",x, y);
            drawPoint_rgb15(x, y, color);
            e = e + dy2;
            if (e > 0)
            {
                y += uy;
                e = e - dx2;
            }
        }
    }
    else
    {
        e = -dy;
        x = x0;
        y = y0;
        for (y = y0; y < y1; y += uy)
        {
            // printf ("%d,%d\n",x, y);
            drawPoint_rgb15(x, y, color);
            e = e + dx2;
            if (e > 0)
            {
                x += ux;
                e= e - dy2;
            }
        }
    }
}

#if 0
void drawCircle_rgb15(int xc, int yc, int r, int color)
{
    int x, y, d;

    x = 0;
    y = r;

    d = 3 - 2 * r;

printf("ddddddddddddddddddddd, %d,%d - %d,%d\n", xc, yc, x, y);
    drawLine_rgb15(xc, yc, x, y, color);

    while(x < y)
    {
        if(d < 0)
        {
            d = d + 4 * x + 6;
        }
        else
        {
            d = d + 4 * ( x - y ) + 10;
            y --;
        }
        x ++;

        drawLine_rgb15(xc, yc, x, y, color);
    }
}
#endif

void drawCircle_rgb15(int cx, int cy, int r, int color)
{
	int r2 = r * r + r;
	int x = 0, x2 = 0, dx2 = 1;
	int y = r, y2 = y*y, dy2 = 2*y - 1;
	int sum = r2;

	while(x <= y)
    {
        drawLine_rgb15(cx - y, cy + x, cx + y, cy + x, color);
        if (x) drawLine_rgb15(cx - y, cy - x, cx + y, cy - x, color);

		sum -= dx2;
		if (sum <= y2)
        {
			if (x != y)
            {
                drawLine_rgb15(cx - x, cy - y, cx + x, cy - y, color);
				if (y) drawLine_rgb15(cx - x, cy + y, cx + x, cy + y, color);
			} /* if */
			y--; y2 -= dy2; dy2 -= 2;
		} /* if */
		x++;
		x2 += dx2;
		dx2 += 2;
	} /* while */
} /* bh */


/**
 *draw Rectangle. the color format of Framebuffer is ARGB1555
 */
void drawRect_rgb15 (int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 2;
    const int stride = finfo.line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 3);
    const int blue = (color & 0xff) >> 3;
    const short color15 = blue | (green << 5) | (red << (5 + 5)) | 0x8000;

    short *dest = (short *) (frameBuffer)
        + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset);

    int x, y;
    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            dest[x] = color15;
        }
        dest += stride;
    }
}

void drawRect_rgb15Ex (int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel = 2;
    const int stride = finfo.line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 3);
    const int blue = (color & 0xff) >> 3;
    const short color15 = blue | (green << 5) | (red << (5 + 5)) | 0x8000;

    short *dest = (short *) (frameBufferBack)
        + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset);

    int x, y;
    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            dest[x] = color15;
        }
        dest += stride;
    }
}

/**
 *draw Rectangle. the color format of Framebuffer is ARGB1444
 */
void  drawRect_rgb12(int x0, int y0, int width, int height, int color)
{
    const int bytesPerPixel =2;
    const int stride = finfo.line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 4);
    const int green = (color & 0xff00) >> (8 + 4);
    const int blue = (color & 0xff) >> 4;
    const short color16 = blue | (green << 4) | (red << (4+4)) |0xf000;
    short *dest = (short *) (frameBuffer)
        + (y0 + vinfo.yoffset) * stride + (x0 + vinfo.xoffset);

    int x, y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            dest[x] = color16;
        }
        dest += stride;
    }
}

void drawPoint (int x, int y, int color)
{
    MI_FB_ColorFmt_e fmt = getFBColorFmt(&vinfo);

    switch (fmt)
    {
        case E_MI_FB_COLOR_FMT_ARGB1555:
         {
            drawPoint_rgb15(x, y, color);
         }
         break;
         default:
             printf ("Warning: drawRect() not implemented for color Fmt %i\n", fmt);
    }
}

void drawLine (int x0, int y0, int x1, int y1, int color)
{
    MI_FB_ColorFmt_e fmt = getFBColorFmt(&vinfo);

    switch (fmt)
    {
        case E_MI_FB_COLOR_FMT_ARGB1555:
         {
            drawLine_rgb15(x0, y0, x1, y1, color);
         }
         break;
         default:
             printf ("Warning: drawRect() not implemented for color Fmt %i\n", fmt);
    }
}

void drawCircle(int xc, int yc, int r, int color)
{
    MI_FB_ColorFmt_e fmt = getFBColorFmt(&vinfo);

    switch (fmt)
    {
        case E_MI_FB_COLOR_FMT_ARGB1555:
         {
            drawCircle_rgb15(xc, yc, r, color);
         }
         break;
         default:
             printf ("Warning: drawRect() not implemented for color Fmt %i\n", fmt);
    }
}

/**
 *draw Rectangle. accroding to Framebuffer format
 */
void drawRect (int x0, int y0, int width, int height, int color)
{
     MI_FB_ColorFmt_e fmt = getFBColorFmt(&vinfo);

     switch (fmt)
     {
         case E_MI_FB_COLOR_FMT_ARGB8888:
         {
            drawRect_rgb32(x0, y0, width, height, color);
         }
         break;
         case E_MI_FB_COLOR_FMT_RGB565:
         {
            drawRect_rgb16(x0, y0, width, height, color);
         }
         break;
         case E_MI_FB_COLOR_FMT_ARGB4444:
         {
            drawRect_rgb12(x0, y0, width, height, color);
         }
         break;
         case E_MI_FB_COLOR_FMT_ARGB1555:
         {
            drawRect_rgb15(x0, y0, width, height, color);
         }
         break;
         default:
             printf ("Warning: drawRect() not implemented for color Fmt %i\n",
                fmt);
     }
}

void drawRectEx (int x0, int y0, int width, int height, int color)
{
     MI_FB_ColorFmt_e fmt = getFBColorFmt(&vinfo);

     switch (fmt)
     {
         case E_MI_FB_COLOR_FMT_ARGB1555:
         {
            drawRect_rgb15Ex(x0, y0, width, height, color);
         }
         break;
         default:
             printf ("Warning: drawRect() not implemented for color Fmt %i\n",
                fmt);
     }
}

/**
 *Conver color key value according to color format
 */
void convertColorKeyByFmt(MI_FB_ColorKey_t* colorkey)
{
        MI_FB_ColorFmt_e fmt = getFBColorFmt(&vinfo);
        MI_U8 red = colorkey->u8Red;
        MI_U8 green = colorkey->u8Green;
        MI_U8 blue = colorkey->u8Blue;
        switch (fmt)
        {
            case E_MI_FB_COLOR_FMT_RGB565:
            {
                colorkey->u8Red = (red >> 3)&(0x1f);
                colorkey->u8Green = (green >> 2)&(0x3f);
                colorkey->u8Blue = (blue >> 3)&(0x1f);
            }
            break;
            case E_MI_FB_COLOR_FMT_ARGB4444:
            {
                colorkey->u8Red = (red >> 4)&0xf;
                colorkey->u8Green = (green >> 4)&0xf;
                colorkey->u8Blue = (blue>>4)&0xf;
            }
            break;
            case E_MI_FB_COLOR_FMT_ARGB1555:
            {
                colorkey->u8Red = (red>>3) & 0x1f;
                colorkey->u8Green= (green >>3) & 0x1f;
                colorkey->u8Blue = (blue >>3) &0x1f;
            }
            break;
            default:
                printf("convertColorKeyByFmt colorfmt is %d\n",fmt);
            break;
        }
}

MI_S32 ST_Fb_Init()
{
    const MI_U8 *devfile = "/dev/fb0";
    MI_BOOL bShown;
    MI_FB_DisplayLayerAttr_t stDisplayAttr;

    /* Open the file for reading and writing */
    g_fbFd = open (devfile, O_RDWR);
    if (g_fbFd == -1)
    {
        perror("Error: cannot open framebuffer device");
        exit(0);
    }

    //get fb_fix_screeninfo
    if (ioctl(g_fbFd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perror("Error reading fixed information");
        exit(0);
    }
    //printFixedInfo();

    //get fb_var_screeninfo
    if (ioctl(g_fbFd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perror("Error reading variable information");
        exit(0);
    }
    //printVariableInfo();

    /* Figure out the size of the screen in bytes */
    g_screensize = finfo.smem_len;

#if FB_ROTATION
    /* Map the device to memory */
    frameBufferBack =
        (char *) mmap(0, g_screensize, PROT_READ | PROT_WRITE, MAP_SHARED, g_fbFd, 0);
    if (frameBufferBack == MAP_FAILED)
    {
        perror("Error: Failed to map framebuffer device to memory");
        exit(0);
    }

    frameBuffer = frameBufferBack + vinfo.xres * vinfo.yres * 2; // pixel 2
    frameBufferBlit = frameBuffer + vinfo.yres * vinfo.yres * 2;

    frameBufferBackPhy = (MI_PHY)finfo.smem_start;
    frameBufferPhy = (MI_PHY)(finfo.smem_start + vinfo.xres * vinfo.yres * 2);
    frameBufferBlitPhy = frameBufferPhy + vinfo.yres * vinfo.yres * 2;
#else
    /* Map the device to memory */
    frameBuffer =
        (char *) mmap(0, g_screensize, PROT_READ | PROT_WRITE, MAP_SHARED, g_fbFd, 0);
    if (frameBuffer == MAP_FAILED)
    {
        perror("Error: Failed to map framebuffer device to memory");
        exit(0);
    }
#endif
    // clear framebuffer
    drawRect(0, 0, vinfo.yres, vinfo.yres, ARGB888_BLUE);

#if FB_ROTATION
    // clear
    drawRectEx(0, 0, vinfo.xres, vinfo.yres, ARGB888_BLUE);
#endif

    return MI_SUCCESS;
}

void ST_Fb_SetColorFmt(MI_FB_ColorFmt_e enFormat)
{
    if (g_fbFd < 0)
    {
        return;
    }

    MI_FB_DisplayLayerAttr_t stDisplayAttr;

    // set color format
    memset(&stDisplayAttr, 0, sizeof(MI_FB_DisplayLayerAttr_t));
    if (ioctl(g_fbFd, FBIOGET_DISPLAYLAYER_ATTRIBUTES, &stDisplayAttr))
    {
        perror("Error: failed to FBIOGET_DISPLAYLAYER_ATTRIBUTES");
        return;
    }

    printf("%s %d, u32Xpos:%d,u32YPos:%d,u32dstWidth:%d,u32dstHeight:%d,u32DisplayWidth:%d,u32DisplayHeight:%d,u32ScreenWidth:%d,u32ScreenHeight:%d\n", __func__, __LINE__,
        stDisplayAttr.u32Xpos, stDisplayAttr.u32YPos, stDisplayAttr.u32dstWidth, stDisplayAttr.u32dstHeight,
        stDisplayAttr.u32DisplayWidth, stDisplayAttr.u32DisplayHeight, stDisplayAttr.u32ScreenWidth, stDisplayAttr.u32ScreenHeight);

    if (enFormat == E_MI_FB_COLOR_FMT_YUV422)
    {
        printf("not support this format, set default format, ARGB1555\n");
        enFormat = E_MI_FB_COLOR_FMT_ARGB1555;
    }
    stDisplayAttr.u32SetAttrMask = E_MI_FB_DISPLAYLAYER_ATTR_MASK_COLOR_FMB;
    stDisplayAttr.eFbColorFmt = enFormat;
    if (ioctl(g_fbFd, FBIOSET_DISPLAYLAYER_ATTRIBUTES, &stDisplayAttr))
    {
        perror("Error: failed to FBIOSET_DISPLAYLAYER_ATTRIBUTES");
        return;
    }
}

MI_S32 ST_Fb_DeInit()
{
    munmap (frameBuffer, g_screensize);
    frameBuffer = NULL;

    close(g_fbFd);
    g_fbFd = -1;

    return MI_SUCCESS;
}

MI_S32 ST_Fb_FillRect(const MI_SYS_WindowRect_t *pRect, MI_U32 u32ColorVal)
{
    drawRect((MI_S32)pRect->u16X, (MI_S32)pRect->u16Y, (MI_S32)pRect->u16Width, (MI_S32)pRect->u16Height, (MI_S32)u32ColorVal);

    return MI_SUCCESS;
}

MI_S32 ST_Fb_FillLine(MI_U16 x0, MI_U16 y0, MI_U16 x1, MI_U16 y1, MI_U32 u32ColorVal)
{
    drawLine(x0, y0, x1, y1, (MI_S32)u32ColorVal);

    return MI_SUCCESS;
}

MI_S32 ST_Fb_FillPoint(MI_U16 x, MI_U16 y, MI_U32 u32ColorVal)
{
    drawPoint(x, y, (MI_S32)u32ColorVal);

    return MI_SUCCESS;
}

MI_S32 ST_Fb_FillCircle(MI_U16 x, MI_U16 y, MI_U16 r, MI_U32 u32ColorVal)
{
    drawCircle(x, y, r, (MI_S32)u32ColorVal);

    return MI_SUCCESS;
}

MI_S32 ST_Fb_GetColorKey(MI_U32 *pu32ColorKeyVal)
{
    MI_FB_ColorKey_t colorKeyInfo;
    if (ioctl(g_fbFd, FBIOGET_COLORKEY,&colorKeyInfo) < 0) {
        ST_ERR("Error: failed to FBIOGET_COLORKEY\n");
        exit(0);
    }
    *pu32ColorKeyVal = (colorKeyInfo.bKeyEnable << 24)|(colorKeyInfo.u8Red << 16)|(colorKeyInfo.u8Green << 8)|(colorKeyInfo.u8Blue);

    return MI_SUCCESS;
}

MI_S32 ST_Fb_SetColorKey(MI_U32 u32ColorKeyVal)
{
    MI_FB_ColorKey_t colorKeyInfo;
    if (ioctl(g_fbFd, FBIOGET_COLORKEY,&colorKeyInfo) < 0) {
        ST_ERR("Error: failed to FBIOGET_COLORKEY\n");
        exit(0);
    }

    colorKeyInfo.bKeyEnable = TRUE;
    colorKeyInfo.u8Red = PIXEL8888RED(u32ColorKeyVal);
    colorKeyInfo.u8Green = PIXEL8888GREEN(u32ColorKeyVal);
    colorKeyInfo.u8Blue = PIXEL8888BLUE(u32ColorKeyVal);

    //convertColorKeyByFmt(&colorKeyInfo);
    if (ioctl(g_fbFd, FBIOSET_COLORKEY, &colorKeyInfo) < 0) {
        ST_ERR("Error: failed to FBIOGET_COLORKEY");
        exit(0);
    }

    return MI_SUCCESS;
}

MI_S32 ST_Fb_InitMouse(MI_S32 s32MousePicW, MI_S32 s32MousePicH, MI_S32 s32BytePerPixel, MI_U8 *pu8MouseFile)
{
    MI_FB_CursorAttr_t stCursorAttr;
    FILE *fp = NULL;
    MI_U8 *pbuff = NULL;
    pbuff = malloc(s32BytePerPixel * s32MousePicW * s32MousePicW);
    if (!g_fbFd)
    {
        ST_ERR("Please init fb first.\n");
        return -1;
    }
    fp = fopen(pu8MouseFile, "rb");
    if (fp)
    {
        fread(pbuff, 1, s32BytePerPixel * s32MousePicW * s32MousePicW, fp);
        fclose(fp);
    }
    //set curosr Icon && set positon
    stCursorAttr.stCursorImageInfo.u32Width = s32MousePicW;
    stCursorAttr.stCursorImageInfo.u32Height = s32MousePicW;
    stCursorAttr.stCursorImageInfo.u32Pitch = s32MousePicW; //?????
    stCursorAttr.stCursorImageInfo.eColorFmt = E_MI_FB_COLOR_FMT_ARGB8888;
    stCursorAttr.stCursorImageInfo.data = pbuff;
    stCursorAttr.u32HotSpotX = 18;
    stCursorAttr.u32HotSpotY = 9;
    stCursorAttr.u32XPos = 100;
    stCursorAttr.u32YPos = 1080;
    stCursorAttr.u16CursorAttrMask = E_MI_FB_CURSOR_ATTR_MASK_ICON
        | E_MI_FB_CURSOR_ATTR_MASK_SHOW | E_MI_FB_CURSOR_ATTR_MASK_POS;
    if (ioctl(g_fbFd, FBIOSET_CURSOR_ATTRIBUTE, &stCursorAttr)) {
        ST_ERR("Error FBIOSET_CURSOR_ATTRIBUTE\n");
        exit(0);
    }

    if (pbuff)
    {
        free(pbuff);
    }

    return MI_SUCCESS;
}

MI_S32 ST_Fb_MouseSet(MI_U32 u32X, MI_U32 u32Y)
{
    MI_FB_CursorAttr_t stCursorAttr;

    stCursorAttr.u32XPos = u32X;
    stCursorAttr.u32YPos = u32Y;
    stCursorAttr.u16CursorAttrMask = E_MI_FB_CURSOR_ATTR_MASK_POS;
    if (ioctl(g_fbFd, FBIOSET_CURSOR_ATTRIBUTE, &stCursorAttr)) {
        ST_ERR("Error FBIOSET_CURSOR_ATTRIBUTE\n");
        exit(0);
    }

    return MI_SUCCESS;
}

void ST_FB_Show(MI_BOOL bShow)
{
    if (g_fbFd < 0)
    {
        return;
    }

    if (ioctl(g_fbFd, FBIOPAN_DISPLAY, &vinfo) == -1)
    {
        perror("Error: failed to FBIOPAN_DISPLAY");
        exit(0);
    }

    if (ioctl(g_fbFd, FBIOSET_SHOW, &bShow)<0)
    {
        perror("Error: failed to FBIOSET_SHOW");
    }

    printf("%s fb\n", bShow ? "show" : "hide");
}

void ST_FB_SetAlphaInfo(MI_FB_GlobalAlpha_t *pstAlphaInfo)
{
    if (g_fbFd < 0)
    {
        return;
    }

    if (pstAlphaInfo == NULL)
    {
        return;
    }

    if (ioctl(g_fbFd, FBIOSET_GLOBAL_ALPHA, pstAlphaInfo) < 0)
    {
        perror("Error: failed to FBIOGET_GLOBAL_ALPHA");
    }
}

void ST_FB_GetAlphaInfo(MI_FB_GlobalAlpha_t *pstAlphaInfo)
{
    if (g_fbFd < 0)
    {
        return;
    }

    if (pstAlphaInfo == NULL)
    {
        return;
    }

    if (ioctl(g_fbFd, FBIOGET_GLOBAL_ALPHA, pstAlphaInfo) < 0)
    {
        perror("Error: failed to FBIOGET_GLOBAL_ALPHA");
    }
}

void ST_FB_ChangeResolution(int width, int height)
{
    if (g_fbFd < 0)
    {
        return;
    }

    printf("%s %d, width:%d,height:%d\n", __func__, __LINE__, width, height);
    MI_FB_DisplayLayerAttr_t stDisplayAttr;
    memset(&stDisplayAttr, 0, sizeof(MI_FB_DisplayLayerAttr_t));
    stDisplayAttr.u32SetAttrMask =  E_MI_FB_DISPLAYLAYER_ATTR_MASK_SCREEN_SIZE |
        E_MI_FB_DISPLAYLAYER_ATTR_MASK_BUFFER_SIZE | E_MI_FB_DISPLAYLAYER_ATTR_MASK_DISP_SIZE
        | E_MI_FB_DISPLAYLAYER_ATTR_MASK_DISP_POS;
    stDisplayAttr.u32Xpos = 0;
    stDisplayAttr.u32YPos = 0;
    stDisplayAttr.u32dstWidth = width;
    stDisplayAttr.u32dstHeight = height;
    stDisplayAttr.u32DisplayWidth = width;
    stDisplayAttr.u32DisplayHeight = height;
    stDisplayAttr.u32ScreenWidth = width;
    stDisplayAttr.u32ScreenHeight = height;
    //E_MI_FB_DISPLAYLAYER_ATTR_MASK_BUFFER_SIZE operaton will change
    //var info and fix.line_lingth, so Need Retrive fixinfo and varinfo
    if (ioctl(g_fbFd, FBIOSET_DISPLAYLAYER_ATTRIBUTES, &stDisplayAttr))
    {
        perror("Error: failed to FBIOSET_DISPLAYLAYER_ATTRIBUTES");
        return;
    }

    // get fb_fix_screeninfo
    if (ioctl(g_fbFd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perror("Error reading fixed information");
        exit(0);
    }
    // printFixedInfo();

    // get fb_var_screeninfo
    if (ioctl(g_fbFd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perror("Error reading variable information");
        exit(0);
    }
    // printVariableInfo();

#if FB_ROTATION
    frameBuffer = frameBufferBack + vinfo.xres * vinfo.yres * 2;
    frameBufferBlit = frameBuffer + vinfo.yres * vinfo.yres * 2;

    frameBufferBackPhy = (MI_PHY)finfo.smem_start;
    frameBufferPhy = (MI_PHY)(finfo.smem_start + vinfo.xres * vinfo.yres * 2);
    frameBufferBlitPhy = frameBufferPhy + vinfo.yres * vinfo.yres * 2;

    ST_DBG("xres:%d,yres:%d, frameBuffer:%p, frameBufferback:%p, frameBufferBlit:%p\n",
        vinfo.xres, vinfo.yres, frameBuffer, frameBufferBack, frameBufferBlit);

    ST_DBG("xres:%d,yres:%d, frameBufferPhy:%llx, frameBufferback:%llx, frameBufferBlit:%llx\n",
        vinfo.xres, vinfo.yres, frameBufferPhy, frameBufferBackPhy, frameBufferBlitPhy);
#else
    ST_DBG("xres:%d,yres:%d, frameBuffer:%p\n", vinfo.xres, vinfo.yres, frameBuffer);
#endif

    // clear framebuffer
    drawRect(0, 0, vinfo.yres, vinfo.yres, ARGB888_BLUE);

#if FB_ROTATION
    // clear
    drawRectEx(0, 0, vinfo.xres, vinfo.yres, ARGB888_BLUE);
#endif
}

MI_S32 ST_FB_Rotation(void)
{
    MI_GFX_Surface_t stSrcSurface;
    MI_GFX_Rect_t stSrcRect;
    MI_GFX_Surface_t stDstSurface;
    MI_GFX_Rect_t stDstRect;
    MI_GFX_Opt_t stGfxOpt;
    MI_U16 u16Fence;

    ExecFunc(MI_GFX_Open(), MI_SUCCESS);

    // rotation to blit buffer
    memset(&stSrcSurface, 0, sizeof(MI_GFX_Surface_t));
    stSrcSurface.eColorFmt = E_MI_GFX_FMT_ARGB1555;
    stSrcSurface.u32Width = vinfo.yres;
    stSrcSurface.u32Height = vinfo.yres;
    stSrcSurface.u32Stride = vinfo.yres * 2;
    stSrcSurface.phyAddr = frameBufferPhy;

    stSrcRect.s32Xpos = 0;
    stSrcRect.s32Ypos = 0;
    stSrcRect.u32Width = vinfo.yres;
    stSrcRect.u32Height = vinfo.yres;

    memset(&stDstSurface, 0, sizeof(MI_GFX_Surface_t));
    stDstSurface.eColorFmt = E_MI_GFX_FMT_ARGB1555;
    stDstSurface.u32Width = vinfo.yres;
    stDstSurface.u32Height = vinfo.yres;
    stDstSurface.u32Stride = vinfo.yres * 2;
    stDstSurface.phyAddr = frameBufferBlitPhy;

    stDstRect.s32Xpos = 0;
    stDstRect.s32Ypos = 0;
    stDstRect.u32Width = vinfo.yres;
    stDstRect.u32Height = vinfo.yres;

    memset(&stGfxOpt, 0, sizeof(MI_GFX_Opt_t));

    stGfxOpt.bEnGfxRop = FALSE;
    stGfxOpt.eRopCode = E_MI_GFX_ROP_NONE;
    stGfxOpt.eSrcDfbBldOp = E_MI_GFX_DFB_BLD_NONE;
    stGfxOpt.eDstDfbBldOp = E_MI_GFX_DFB_BLD_NONE;
    stGfxOpt.eMirror = E_MI_GFX_MIRROR_NONE;
    stGfxOpt.eRotate = E_MI_GFX_ROTATE_90;
    stGfxOpt.eSrcYuvFmt = E_MI_GFX_YUV_YVYU;
    stGfxOpt.eDstYuvFmt = E_MI_GFX_YUV_YVYU;
    stGfxOpt.stClipRect.s32Xpos = 0;
    stGfxOpt.stClipRect.s32Ypos = 0;
    stGfxOpt.stClipRect.u32Width  = vinfo.yres;
    stGfxOpt.stClipRect.u32Height = vinfo.yres;

    ST_DBG("src, (%dx%d), (%d,%d,%d,%d), phy:%llx\n", stSrcSurface.u32Width,
        stSrcSurface.u32Height, stSrcRect.s32Xpos, stSrcRect.s32Ypos, stSrcRect.u32Width, stSrcRect.u32Height,
        stSrcSurface.phyAddr);
    ST_DBG("dst, (%dx%d), (%d,%d,%d,%d), phy:%llx\n", stDstSurface.u32Width,
        stDstSurface.u32Height, stDstRect.s32Xpos, stDstRect.s32Ypos, stDstRect.u32Width, stDstRect.u32Height,
        stDstSurface.phyAddr);

    ExecFunc(MI_GFX_BitBlit(&stSrcSurface, &stSrcRect, &stDstSurface, &stDstRect, &stGfxOpt, &u16Fence), MI_SUCCESS);
    ExecFunc(MI_GFX_WaitAllDone(FALSE, u16Fence), MI_SUCCESS);

    // clip blit buffer to GOP buffer
    memset(&stSrcSurface, 0, sizeof(MI_GFX_Surface_t));
    stSrcSurface.eColorFmt = E_MI_GFX_FMT_ARGB1555;
    stSrcSurface.u32Width = vinfo.yres;
    stSrcSurface.u32Height = vinfo.yres;
    stSrcSurface.u32Stride = vinfo.yres * 2;
    stSrcSurface.phyAddr = frameBufferBlitPhy;

    stSrcRect.s32Xpos = vinfo.yres - vinfo.xres;
    stSrcRect.s32Ypos = 0;
    stSrcRect.u32Width = vinfo.xres;
    stSrcRect.u32Height = vinfo.yres;

    memset(&stDstSurface, 0, sizeof(MI_GFX_Surface_t));
    stDstSurface.eColorFmt = E_MI_GFX_FMT_ARGB1555;
    stDstSurface.u32Width = vinfo.xres;
    stDstSurface.u32Height = vinfo.yres;
    stDstSurface.u32Stride = vinfo.xres * 2;
    stDstSurface.phyAddr = frameBufferBackPhy;

    stDstRect.s32Xpos = 0;
    stDstRect.s32Ypos = 0;
    stDstRect.u32Width = vinfo.xres;
    stDstRect.u32Height = vinfo.yres;

    printf("======================================\n");

    ST_DBG("src, (%dx%d), (%d,%d,%d,%d), phy:%llx\n", stSrcSurface.u32Width,
        stSrcSurface.u32Height, stSrcRect.s32Xpos, stSrcRect.s32Ypos, stSrcRect.u32Width, stSrcRect.u32Height,
        stSrcSurface.phyAddr);
    ST_DBG("dst, (%dx%d), (%d,%d,%d,%d), phy:%llx\n", stDstSurface.u32Width,
        stDstSurface.u32Height, stDstRect.s32Xpos, stDstRect.s32Ypos, stDstRect.u32Width, stDstRect.u32Height,
        stDstSurface.phyAddr);

    ExecFunc(MI_GFX_BitBlit(&stSrcSurface, &stSrcRect, &stDstSurface, &stDstRect, NULL, &u16Fence), MI_SUCCESS);
    ExecFunc(MI_GFX_WaitAllDone(FALSE, u16Fence), MI_SUCCESS);

    ExecFunc(MI_GFX_Close(), MI_SUCCESS);
}

typedef struct __attribute__((packed))
{
    MI_U16 bfType;
    MI_U32 bfSize;
    MI_U16 bfReserved1;
    MI_U16 bfReserved2;
    MI_U32 bfOffBits;
} BITMAPFILEHEADER_S;

typedef struct __attribute__((packed))
{
    MI_U32 biSize;
    MI_U32 biWidth;
    MI_U32 biHeight;
    MI_U16 biPlanes;
    MI_U16 biBitCount;
    MI_U32 biCompression;
    MI_U32 biSizeImage;
    MI_U32 biXPelsPerMeter;
    MI_U32 biYPelsPerMeter;
    MI_U32 biClrUsed;
    MI_U32 biClrImportant;
}BITMAPINFOHEADER_S;

void ST_FB_ShowBMP(int x, int y, const char *szFile)
{
    int fd = -1;
    struct stat sbuf;
    unsigned char *ptr = NULL;
    BITMAPFILEHEADER_S *pstBMPFileHead;
    BITMAPINFOHEADER_S *pstBMPInfoHead;
    unsigned char *pBMPData = NULL;
    unsigned char *pStart = NULL;

    int i = 0, j = 0, bpp = 0;
    int stride = 0;

    if (szFile == NULL)
    {
        return;
    }

    fd = open (szFile, O_RDONLY|0);
    if (fd < 0)
    {
        ST_ERR("open %s error\n", szFile);
        return;
    }
    else
    {
        ST_DBG("open \"%s\" success, fd:%d\n", szFile, fd);
    }

    if (fstat(fd, &sbuf) < 0)
    {
        ST_ERR("fstar error\n");
        perror("fstat");
        goto END;
    }

    ptr = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED)
    {
        ST_ERR("mmap error\n");
        goto END;
    }

    pstBMPFileHead = (BITMAPFILEHEADER_S *)ptr;
    pstBMPInfoHead = (BITMAPINFOHEADER_S *)(ptr + sizeof(BITMAPFILEHEADER_S));
    ST_DBG("sizeof(BITMAPFILEHEADER_S)=%d, sizeof(BITMAPINFOHEADER_S)=%d, %x, width:%d,height:%d, sizeof(MI_U32)=%d,sizeof(MI_U16)=%d\n",
        sizeof(BITMAPFILEHEADER_S), sizeof(BITMAPINFOHEADER_S), pstBMPFileHead->bfType, pstBMPInfoHead->biWidth, pstBMPInfoHead->biHeight,
        sizeof(MI_U32), sizeof(MI_U16));

    pBMPData = ptr + sizeof(BITMAPFILEHEADER_S) + sizeof(BITMAPINFOHEADER_S);
    bpp = pstBMPInfoHead->biBitCount / 8;
    stride = pstBMPInfoHead->biWidth * bpp;

    ST_DBG("bpp:%d\n", bpp);

    for(i = 0; i < pstBMPInfoHead->biHeight; i ++)
    {
        for(j = 0; j < pstBMPInfoHead->biWidth; j++)
        {
            pStart = pBMPData + ((pstBMPInfoHead->biHeight - 1) - i) * stride + j * bpp;

            drawPoint_rgb15Ex(x + j, y +i, ARGB2PIXEL8888(128, *(pStart + 2), *(pStart + 1), *(pStart)));
        }
    }

#if FB_ROTATION
    ST_FB_Rotation();
#endif

END:
    if (ptr != NULL)
    {
        (void) munmap((void *)ptr, sbuf.st_size);
        ptr = NULL;
    }

    if (fd > 0)
    {
        close(fd);
        fd = -1;
    }

    return;
}

