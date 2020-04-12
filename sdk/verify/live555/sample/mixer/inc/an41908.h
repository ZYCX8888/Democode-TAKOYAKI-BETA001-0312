/*
* an41908.h- Sigmastar
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

#ifndef __AN_41908_A_H__
#define __AN_41908_A_H__

#if 1
#define LENSDRV_FOCUS_STEPS_REG		0x24 	//define AB motor to Focus
#define LENSDRV_FOCUS_INTCT_REG		0x25	//
#define LENSDRV_ZOOM_STEPS_REG		0x29   //define CD motor to Zoom
#define LENSDRV_ZOOM_INTCT_REG		0x2A	//
#else
#define LENSDRV_FOCUS_STEPS_REG		0x29 	//define CD motor to Focus 
#define LENSDRV_ZOOM_STEPS_REG		0x24	//define AB motor to Zoom
#endif

#define MOTOR_IDLE_BIT				(0x1 << 10)


#define AN41908A_UNKNOWN_CMD 0x00
#define AN41908A_READ_CMD	0xF0
#define AN41908A_WRITE_CMD	0xF1
#define AN41908A_ZOOM_CMD	0x2F
#define AN41908A_FOCUS_CMD	0x3F

#define AN41908A_ZOOM_IN	0x20
#define AN41908A_ZOOM_OUT	0x21
#define AN41908A_ZOOM_STOP	0x22
#define AN41908A_ZOOM_RESET	0x23

#define AN41908A_ZOOM_SET_SPEED	0x25
#define AN41908A_ZOOM_IS_MOVING	0x26
#define AN41908A_ZOOM_CUR_POS	0x2A

#define AN41908A_FOCUS_FAR		0x30
#define AN41908A_FOCUS_NEAR		0x31
#define AN41908A_FOCUS_STOP		0x32
#define AN41908A_FOCUS_SET_SPEED 0x33
#define AN41908A_FOCUS_CUR_POS	0x3D
#define AN41908A_FOCUS_RESET	0x3E

#define AN41908A_ZOOM_FOCUS_RESET 0x4E

#define IRIS_LENS_ENABLE
#define LENS_SUNNY	(0)

#define OSCIN_FREQ		(27000000)
#define VD_FREQ			(34)
#define VD_DELAY_MS		(30)
#define FULLSTEP_TO_SUBSTEP(x)	(x<<3)
#define SUBSTEP_TO_FULLSTEP(x)	(x>>3)

#define INTCT_VALUE(psum)		(OSCIN_FREQ/(VD_FREQ*24*psum) + 1)

#define FOCUS_MAX_STEPS	(1864)
#define ZOOM_MAX_STEPS	(710)
//#define FOCUS_MAX_STEPS	(3318)
//#define ZOOM_MAX_STEPS	(1352)
#define MAX_STEP_LONG	(30)

#define ZOOM_LOST_STEPS_CORRECT	(8)
#define FOCUS_LOST_STEPS_CORRECT	(8)

enum SPEED_LEVEL
{
	SPEED_FAST = 0x0 ,
	SPEED_NORMAL ,
	SPEED_SLOW ,
};

enum MFZ_STATE
{
	MFZ_IDLE = 0x0,
	MFZ_BUSY,	
};

enum MOTOR_TYPE
{
	MOTOR_FOCUS = 0,
	MOTOR_ZOOM,
	MOTOR_UNKNOW,
};

const int zoom_speed_range[][2] = {
	{ SPEED_FAST, 25 },
	{ SPEED_NORMAL, 15},
	{ SPEED_SLOW, 5},
};

const int focus_speed_range[][2] = {
	{ SPEED_FAST, 30},
	{ SPEED_NORMAL, 15},
	{ SPEED_SLOW, 5},
};

#if 0
typedef struct _MFZ_LENS_
{
    int zoom_pos;
    int focus_pos;	
	int zoom_speed;
	int zoom_speed_level;
	int focus_speed;
	int focus_speed_level;
	int is_moving;
	int zoom_steps;
	int focus_steps;
	
	int reg_addr;
	int reg_value;
	int zoom_dir;
	int focus_dir;
	
} MFZ_LENS;
#else
typedef struct _MFZ_MOTOR_INFO
{
	int pos;
	int speed;
	int speed_level;
	int is_moving;
	int steps;
	int dir;
}MOTOR_INFO;

typedef struct _MFZ_LENS_
{
	MOTOR_INFO  Motor[MOTOR_UNKNOW];
	int reg_addr;
	int reg_value;
	
} MFZ_LENS;
#endif

enum MOTOR_DIR
{
	BACKWARD_DIR = -1,
	UNKNOWN_DIR = 0,
	FORWARD_DIR = 1,	
};

/* ============================================================================================
 *
 * REG[0x00 to 0x0E]: mainly for auto iris
 * REG[0x20 to 0x0E]: mainly for auto focus and zoom
 *
 * ============================================================================================ */
//#define IRIS_LENS_ENABLE
const int iris_lens_tbl[][2] = {
#if 0
	{ 0x00,0x0000 },  //Set Iris Target
	{ 0x01,0x6000 },  
	{ 0x02,0x66F0 },
	{ 0x03,0x0E10 },
	{ 0x04,0xD640 },
	{ 0x05,0x0024 },
	{ 0x0A,0x0000 },
	{ 0x0B,0x0400 },
	{ 0x0E,0x0300 },
#endif

#if 0
    { 0x00,0x0100 },  //Set Iris Target
	{ 0x01,0x688A },  
	{ 0x02,0x6610 },
	{ 0x03,0x0E10 },
	{ 0x04,0x8028 },
	{ 0x05,0x0D24 },
	{ 0x0B,0x8480 },
	{ 0x0E,0xFFFF },
#endif

#if 0
    { 0x00,0x00F0 },  //Set Iris Target
	{ 0x01,0x3E00 },  
	{ 0x02,0x1000 },
	{ 0x03,0x0E10 },
	{ 0x04,0xD640 },
	{ 0x05,0x0004 },
	{ 0x0B,0x0400 },
	{ 0x0A,0x0000 },
	{ 0x0E,0x0300 },
#endif

#if 1
    { 0x00,0x0100 },  //Set Iris Target
	{ 0x01,0x688A },  
	{ 0x02,0x66F0 },
	{ 0x03,0x0E10 },
	{ 0x04,0x8028 },
	{ 0x05,0x0D24 },
	{ 0x0B,0x05FF },
	{ 0x0E,0xFFFF },
#endif
};

const int focus_zoom_tbl[][2] = {
    { 0x0B, 0x0080 },
	{ 0x20, 0x1E10 },		/* DT1=3.03ms    PWM=30.1KHZ 		*/
	{ 0x21, 0x0087 },
	{ 0x22, 0x0001 },		/* DT2A=0.91ms   AB phase 90 degree */
	{ 0x23, 0xC8C8},		/* PWMA PWMB max duty 0.89			*/
	//{ 0x23, 0xA8A8 },		/* PWMA PWMB max duty 0.75	3.75V	*/
	//{ 0x24, 0x0400 },		/* 256 Subdivision & enable			*/
	{ 0x24, 0x1C08 },		/* 256 Subdivision & disable		*/
	{ 0x25, 0x02BF },      /* motor step cycle        			*/
	{ 0x27, 0x0001 },		/* DT2B=0.91ms   CD phase 90 degree */
	{ 0x28, 0xC8C8 },		/* PWMC PWMD max duty 0.89			*/
	//{ 0x28, 0xA8A8 },		/* PWMC PWMD max duty 0.75	3.75V	*/
	//{ 0x29, 0x0400 },		/* 256 Subdivision & enable			*/
	{ 0x29, 0x1C08 },		/* 256 Subdivision & disable		*/
	{ 0x2A, 0x02BF },		/* motor step cycle  		        */
};

#define ZOOM_GAIN	(5)
typedef struct Track_Curve
{
	int zoom_vs_focus[ZOOM_MAX_STEPS/ZOOM_GAIN + 1];
	
}TRACK_CURVE_T;

#endif

