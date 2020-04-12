#ifndef __UTILITY_H__
#define __UTILITY_H__
#include "awnpu.h"
#include <string>
#include <vector>


#define DRAW_OSD_NUMBER 1


float IOU(AW_RECT r1, AW_RECT r2);
std::vector<AW_RECT>& refine_rects(std::vector<AW_RECT>& rects);

unsigned char *read_image(const char *filename, int *w, int *h, int *is_rgb);
std::vector<std::string> list_dir(std::string path);

int wait_key_q_or_break();

void SYS_CtrlGetMemInfo(const char *func, int line);


#endif//__UTILITY_H__
