//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2011  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//		This file should be modified by some customers according to their SOC configuration.
//--=========================================================================--

#ifndef _VPU_CONFIG_H_
#define _VPU_CONFIG_H_
#include "../config.h"


#define ENC_STREAM_BUF_SIZE  0xF00000
#define ENC_STREAM_BUF_COUNT 5


#define BODA950_CODE                    0x9500
#define CODA960_CODE                    0x9600
#define CODA980_CODE                    0x9800

#define WAVE512_CODE                    0x5120
#define WAVE520_CODE                    0x5200
#define WAVE515_CODE                    0x5150
#define WAVE525_CODE                    0x5250

#define WAVE511_CODE                    0x5110
#define WAVE521_CODE                    0x5210
#define WAVE521C_CODE                   0x521c

#define DEFAULT_TYPE_CODE WAVE511_CODE // for test
#define PRODUCT_CODE_W_SERIES(x) (x == WAVE512_CODE || x == WAVE520_CODE || x == WAVE515_CODE || x == WAVE525_CODE || x == WAVE511_CODE || x == WAVE521_CODE || x == WAVE521C_CODE)
#define PRODUCT_CODE_NOT_W_SERIES(x) (x == BODA950_CODE || x == CODA960_CODE || x == CODA980_CODE)
#ifdef SUPPORT_MEMORY_SIZE_OPTIMIZE
#define WAVE5_MAX_CODE_BUF_SIZE             (512*1024)
#else
#define WAVE5_MAX_CODE_BUF_SIZE             (1024*1024)
#endif
#define WAVE520ENC_WORKBUF_SIZE             (128*1024)
#define WAVE525ENC_WORKBUF_SIZE             (2*1024*1024)	// svac encoder needs 2MB for workbuffer
#define WAVE521ENC_WORKBUF_SIZE             (128*1024)      //HEVC 128K, AVC 40K

#define WAVE512DEC_WORKBUF_SIZE             (2*1024*1024)
#define WAVE515DEC_WORKBUF_SIZE             (2*1024*1024)
#define WAVE525DEC_WORKBUF_SIZE             (1.5*1024*1024)
#ifdef SUPPORT_MEMORY_SIZE_OPTIMIZE
#define WAVE521DEC_WORKBUF_SIZE             (172*1024) // (((174336/4096) + 1)*4096)/1024
// avc
// [FW] sizeof(instance_t)=10688, sizeof(avc_dec_t)=56256, sizeof(user_buf)=0, sizeof(userdata_t)=41232, SIZE_USERDATA_BUF=40960, SIZE_USERDATA_BUF=32
// [FW] sizeof(pool.slist)=17248, sizeof(pool.sps)=13880, sizeof(pool.pps)=21120, AVC_DEC_MAX_NUM_SLIST_BUF=77. AVC_DEC_MAX_NUM_SPS_BUF=10, AVC_DEC_MAX_NUM_PPS_BUF=66
// hevc
// [FW] sizeof(instance_t)=10688, sizeof(hevc_t)=163648, sizeof(user_buf)=0, sizeof(userdata_t)=41232, SIZE_USERDATA_BUF=40960, SIZE_USERDATA_BUF=32, SIZE_PRESCAN_TEMP_BUF=0
// [FW] MAX_NUM_SPS=4, MAX_NUM_PPS=16, MAX_PRESCAN=6, SIZE_PRESCAN_TEMP_BUF=0
// [FW] sizeof(pool.slist)=25600, sizeof(pool.sps)=60960, sizeof(pool.pps)=6912, MAX_NUM_SLIST_BUF=25. MAX_NUM_SPS_BUF=6, MAX_NUM_PPS_BUF=18
// avc => 10688 + 56256 = 66944
// hevc => 10688 + 163648 = 174336
#else
#define WAVE521DEC_WORKBUF_SIZE             (1.5*1024*1024)
#endif
#define WAVE525_SVAC_DEC_WORKBUF_SIZE       (7*1024*1024) // max mvcol buffer included in workbuffer due to sequence change.


#define MAX_INST_HANDLE_SIZE            48              /* DO NOT CHANGE THIS VALUE */
#define MAX_NUM_INSTANCE                32
#define MAX_NUM_VPU_CORE                1
#define MAX_NUM_VCORE                   1

    #define MAX_ENC_AVC_PIC_WIDTH           4096
    #define MAX_ENC_AVC_PIC_HEIGHT          2304
#define MAX_ENC_PIC_WIDTH               4096
#define MAX_ENC_PIC_HEIGHT              2304
#define MIN_ENC_PIC_WIDTH               96
#define MIN_ENC_PIC_HEIGHT              16

// for WAVE420
#define W4_MIN_ENC_PIC_WIDTH            256
#define W4_MIN_ENC_PIC_HEIGHT           128
#define W4_MAX_ENC_PIC_WIDTH            8192
#define W4_MAX_ENC_PIC_HEIGHT           8192

#define MAX_DEC_PIC_WIDTH               4096
#define MAX_DEC_PIC_HEIGHT              2304

#define MAX_CTU_NUM                     0x4000      // CTU num for max resolution = 8192x8192/(64x64)
#define MAX_SUB_CTU_NUM	                (MAX_CTU_NUM*4)
#define MAX_MB_NUM                      0x40000     // MB num for max resolution = 8192x8192/(16x16)

//  Application specific configuration
#define VPU_ENC_TIMEOUT                 60000
#define VPU_DEC_TIMEOUT                 20000
#define VPU_BUSY_CHECK_TIMEOUT          5000

// codec specific configuration
#define VPU_REORDER_ENABLE              1   // it can be set to 1 to handle reordering DPB in host side.
#define CBCR_INTERLEAVE			        1 //[default 1 for BW checking with CnMViedo Conformance] 0 (chroma separate mode), 1 (chroma interleave mode) // if the type of tiledmap uses the kind of MB_RASTER_MAP. must set to enable CBCR_INTERLEAVE
#define VPU_ENABLE_BWB			        1

#define HOST_ENDIAN                     VDI_128BIT_LITTLE_ENDIAN
#define VPU_FRAME_ENDIAN                HOST_ENDIAN
#define VPU_STREAM_ENDIAN               HOST_ENDIAN
#define VPU_USER_DATA_ENDIAN            HOST_ENDIAN
#define VPU_SOURCE_ENDIAN               HOST_ENDIAN
#define DRAM_BUS_WIDTH                  16


// for WAVE520
#define USE_SRC_PRP_AXI         0
#define USE_SRC_PRI_AXI         1
#define DEFAULT_SRC_AXI         USE_SRC_PRP_AXI

/************************************************************************/
/* VPU COMMON MEMORY                                                    */
/************************************************************************/
#define COMMAND_QUEUE_DEPTH             3

#define ENC_SRC_BUF_NUM                     2

#define ONE_TASKBUF_SIZE_FOR_W511DEC_CQ     (20*1024*1024)   /*  for temp*/
#define ONE_TASKBUF_SIZE_FOR_W5DEC_CQ       (8*1024*1024)   /* upto 8Kx4K, need 8Mbyte per task*/
#define ONE_TASKBUF_SIZE_FOR_W5ENC_CQ       (8*1024*1024)  /* upto 8Kx8K, need 8Mbyte per task.*/

#ifdef SUPPORT_MEMORY_SIZE_OPTIMIZE
#define WAVE5_TEMPBUF_OFFSET                WAVE5_MAX_CODE_BUF_SIZE
#define WAVE5_TEMPBUF_SIZE                  ((396*1024)+(128*1024)) // if secondary AXI option is disabled. this size can be 0
// #define WAVE5_TEMPBUF_SIZE                  (213*1024) // if secondary AXI option is disabled. this size can be 0 for 4K
#define WAVE5_TASK_BUF_OFFSET               (WAVE5_MAX_CODE_BUF_SIZE + WAVE5_TEMPBUF_SIZE)   // common mem = | codebuf(1M) | tempBuf(1M) | taskbuf0x0 ~ 0xF |
#else
#define WAVE5_TEMPBUF_OFFSET                (1024*1024)
#define WAVE5_TEMPBUF_SIZE                  (1024*1024)
#define WAVE5_TASK_BUF_OFFSET               (2*1024*1024)   // common mem = | codebuf(1M) | tempBuf(1M) | taskbuf0x0 ~ 0xF |
#endif

#define ONE_PARAMBUF_SIZE_FOR_CQ            827392    // default PARAM buffer size = CEIL(4K, 0.8M) with SUPPORT_SLICE_NUM_OPT in firmware
#define SIZE_COMMON                         (WAVE5_MAX_CODE_BUF_SIZE + WAVE5_TEMPBUF_SIZE + (COMMAND_QUEUE_DEPTH*ONE_PARAMBUF_SIZE_FOR_CQ))

//=====4. VPU REPORT MEMORY  ======================//
#define SIZE_REPORT_BUF                 (0x10000)

#define STREAM_END_SIZE                 0
#define STREAM_END_SET_FLAG             0
#define STREAM_END_CLEAR_FLAG           -1
#define EXPLICIT_END_SET_FLAG           -2

#define USE_BIT_INTERNAL_BUF            1
#define USE_IP_INTERNAL_BUF             1
#define USE_DBKY_INTERNAL_BUF           1
#define USE_DBKC_INTERNAL_BUF           1
#define USE_OVL_INTERNAL_BUF            1
#define USE_BTP_INTERNAL_BUF            1
#define USE_ME_INTERNAL_BUF             1

/* WAVE410 only */
#define USE_BPU_INTERNAL_BUF            1
#define USE_VCE_IP_INTERNAL_BUF         1
#define USE_VCE_LF_ROW_INTERNAL_BUF     1

/* WAVE420 only */
#define USE_IMD_INTERNAL_BUF            1
#define USE_RDO_INTERNAL_BUF            1
#define USE_LF_INTERNAL_BUF             1


#define SCALER_STEP                     2

#endif  /* _VPU_CONFIG_H_ */

