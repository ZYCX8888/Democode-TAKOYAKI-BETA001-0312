#ifndef __DRAW_H_
#define __DRAW_H_

#include <linux/fb.h>
#include <stdio.h>
#include "mstarFb.h"

void drawRect (int x0, int y0, int width, int height, int color, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, char *fb);

void drawPixel (int x0, int y0, int color, struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo, char *fb);
#endif
