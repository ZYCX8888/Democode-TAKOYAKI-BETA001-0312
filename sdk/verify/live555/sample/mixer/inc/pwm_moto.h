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

#ifndef _PWM_MOTO_H_
#define _PWM_MOTO_H_


#ifdef __cplusplus

extern "C"
{
#endif /* __cplusplus */

void group_mode_enable_disable(int group_id,int enable);
void pwm_config_group_in(int          group_id);
void group_mode_stop(int group_id,int enable);
void group_mode_hold(int group_id,int enable);
void pwm_config_param(int group_id,char *param);

#ifdef __cplusplus

}

#endif /* __cplusplus */

#endif //_PWM_MOTO_H_




