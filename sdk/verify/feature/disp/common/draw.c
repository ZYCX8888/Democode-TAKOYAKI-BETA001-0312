#include "draw.h"
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
void drawRect_rgb32 (int x0, int y0, int width, int height, int color, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, char *fb)
{
    const int bytesPerPixel = 4;
    const int stride = finfo->line_length / bytesPerPixel;

    int *dest = (int *) (fb)
        + (y0 + vinfo->yoffset) * stride + (x0 + vinfo->xoffset);

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
void drawRect_rgb16 (int x0, int y0, int width, int height, int color, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, char *fb)
{
    const int bytesPerPixel = 2;
    const int stride = finfo->line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 2);
    const int blue = (color & 0xff) >> 3;
    const short color16 = blue | (green << 5) | (red << (5 + 6));

    short *dest = (short *) (fb)
        + (y0 + vinfo->yoffset) * stride + (x0 + vinfo->xoffset);

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
 *draw Rectangle. the color format of Framebuffer is ARGB1555
 */
void drawRect_rgb15 (int x0, int y0, int width, int height, int color, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, char *fb)
{
    const int bytesPerPixel = 2;
    const int stride = finfo->line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 3);
    const int blue = (color & 0xff) >> 3;
    const short color15 = blue | (green << 5) | (red << (5 + 5)) | 0x8000;

    short *dest = (short *) (fb)
        + (y0 + vinfo->yoffset) * stride + (x0 + vinfo->xoffset);

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
void drawRect_rgb12(int x0, int y0, int width, int height, int color, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, char *fb)
{
    const int bytesPerPixel =2;
    const int stride = finfo->line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 4);
    const int green = (color & 0xff00) >> (8 + 4);
    const int blue = (color & 0xff) >> 4;
    const short color16 = blue | (green << 4) | (red << (4+4)) |0xf000;
    short *dest = (short *) (fb)
        + (y0 + vinfo->yoffset) * stride + (x0 + vinfo->xoffset);

    int x, y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            dest[x] = color16;
        }
        dest += stride;
    }
}
/**
 *draw Rectangle. accroding to Framebuffer format
 */
void drawRect (int x0, int y0, int width, int height, int color, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, char *fb)
{
     MI_FB_ColorFmt_e fmt = getFBColorFmt(vinfo);
     switch (fmt)
     {
         case E_MI_FB_COLOR_FMT_ARGB8888:
         {
            drawRect_rgb32(x0, y0, width, height, color, vinfo, finfo, fb);
         }
         break;
         case E_MI_FB_COLOR_FMT_RGB565:
         {
            drawRect_rgb16(x0, y0, width, height, color, vinfo, finfo, fb);
         }
         break;
         case E_MI_FB_COLOR_FMT_ARGB4444:
         {
            drawRect_rgb12(x0, y0, width, height, color, vinfo, finfo, fb);
         }
         break;
         case E_MI_FB_COLOR_FMT_ARGB1555:
         {
            drawRect_rgb15(x0, y0, width, height, color, vinfo, finfo, fb);
         }
         break;
         default:
             printf ("Warning: drawRect() not implemented for color Fmt %i\n",
                fmt);
     }
}

/**
 * draw Pixel. the colormat of Framebuffer is ARGB8888
 */
void drawPixel_rgb32 (int x0, int y0, int color, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, char *fb)
{
    const int bytesPerPixel = 4;
    const int stride = finfo->line_length / bytesPerPixel;

    int *dest = (int *) (fb)
        + (y0 + vinfo->yoffset) * stride + (x0 + vinfo->xoffset);

    *dest = color;
}

/**
 * draw Pixel. the colormat of Framebuffer is RGB565
 */
void drawPixel_rgb16 (int x0, int y0, int color, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, char *fb)
{
    const int bytesPerPixel = 2;
    const int stride = finfo->line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 2);
    const int blue = (color & 0xff) >> 3;
    const short color16 = blue | (green << 5) | (red << (5 + 6));

    short *dest = (short *) (fb)
        + (y0 + vinfo->yoffset) * stride + (x0 + vinfo->xoffset);

    *dest = color16;
}

/**
 *draw Pixel. the color format of Framebuffer is ARGB1555
 */
void drawPixel_rgb15 (int x0, int y0, int color, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, char *fb)
{
    const int bytesPerPixel = 2;
    const int stride = finfo->line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 3);
    const int green = (color & 0xff00) >> (8 + 3);
    const int blue = (color & 0xff) >> 3;
    const short color15 = blue | (green << 5) | (red << (5 + 5)) | 0x8000;

    short *dest = (short *) (fb)
        + (y0 + vinfo->yoffset) * stride + (x0 + vinfo->xoffset);

    *dest = color15;
}

/**
 *draw Pixel. the color format of Framebuffer is ARGB1444
 */
void drawPixel_rgb12(int x0, int y0, int color, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, char *fb)
{
    const int bytesPerPixel =2;
    const int stride = finfo->line_length / bytesPerPixel;
    const int red = (color & 0xff0000) >> (16 + 4);
    const int green = (color & 0xff00) >> (8 + 4);
    const int blue = (color & 0xff) >> 4;
    const short color16 = blue | (green << 4) | (red << (4+4)) |0xf000;
    short *dest = (short *) (fb)
        + (y0 + vinfo->yoffset) * stride + (x0 + vinfo->xoffset);

    *dest = color16;
}
void drawPixel (int x0, int y0, int color, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, char *fb)
{
     MI_FB_ColorFmt_e fmt = getFBColorFmt(vinfo);
     switch (fmt)
     {
         case E_MI_FB_COLOR_FMT_ARGB8888:
         {
            drawPixel_rgb32(x0, y0, color, vinfo, finfo, fb);
         }
         break;
         case E_MI_FB_COLOR_FMT_RGB565:
         {
            drawPixel_rgb16(x0, y0, color, vinfo, finfo, fb);
         }
         break;
         case E_MI_FB_COLOR_FMT_ARGB4444:
         {
            drawPixel_rgb12(x0, y0, color, vinfo, finfo, fb);
         }
         break;
         case E_MI_FB_COLOR_FMT_ARGB1555:
         {
            drawPixel_rgb15(x0, y0, color, vinfo, finfo, fb);
         }
         break;
         default:
             printf ("Warning: drawPixel() not implemented for color Fmt %i\n",fmt);
     }
}
