#ifndef VERIFY_LIVE555_SAMPLE_MIXER_INC_MODULE_CUS3A_H_
#define VERIFY_LIVE555_SAMPLE_MIXER_INC_MODULE_CUS3A_H_

#include "isp_cus3a_if.h"

int mod1_isp_ae_init(void* pdata, ISP_AE_INIT_PARAM *init_state);
void mod1_isp_ae_release(void* pdata);
void mod1_isp_ae_run(void* pdata, const ISP_AE_INFO *info, ISP_AE_RESULT *result);
int mod1_isp_awb_init(void *pdata);
void mod1_isp_awb_run(void* pdata, const ISP_AWB_INFO *info, ISP_AWB_RESULT *result);
void mod1_isp_awb_release(void *pdata);
int mod1_isp_af_init(void *pdata, ISP_AF_INIT_PARAM *param);
void mod1_isp_af_release(void *pdata);
void mod1_isp_af_run(void *pdata, const ISP_AF_INFO *af_info, ISP_AF_RESULT *result);
int mod1_isp_af_ctrl(void *pdata, ISP_AF_CTRL_CMD cmd, void* param);

int ut_isp_ae_init(void* pdata, ISP_AE_INIT_PARAM *init_state);
void ut_isp_ae_release(void* pdata);
void ut_isp_ae_run(void* pdata, const ISP_AE_INFO *info, ISP_AE_RESULT *result);

int ut_isp_awb_init(void *pdata);
void ut_isp_awb_run(void* pdata, const ISP_AWB_INFO *info, ISP_AWB_RESULT *result);
void ut_isp_awb_release(void *pdata);

#endif /* VERIFY_LIVE555_SAMPLE_MIXER_INC_MODULE_CUS3A_H_ */
