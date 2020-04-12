/*
* mid_md.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef __MID_MD_H__
#define __MID_MD_H__

#define RAW_W            352
#define RAW_H            288
#define MD_DIV_W        12
#define MD_DIV_H        10
#define MD1_DIV_W        16
#define MD1_DIV_H        12
#define OD_DIV_W        3
#define OD_DIV_H        3
#define OD1_DIV_W        2
#define OD1_DIV_H        2


int mid_md_Param_Change(MI_S8 *param, MI_S32 paramLen);
int mid_md_Initial(int param);
int mid_md_Uninitial(void);
int mid_md_UnBind_DIVP_To_VDF(void);
void SetMdUseVdfChannelValue(MI_U8 value);
MI_U8 GetMdUseVdfChannelValue(void);

#endif /* __MID_MD_H__ */

