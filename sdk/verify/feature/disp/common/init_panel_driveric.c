/*
* init_panel_driveric.c- Sigmastar
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
#include "init_panel_driveric.h"
#include "spi_cmd_480x854_v2.h"

#if 0 //customer
#define CS_GPIO_PIN   (107)
#define CLK_GPIO_PIN   (108)
#define SDO_GPIO_PIN   (110)
#define RST_GPIO_PIN   (126)
#define PANEL_EN (148)
#endif

#if 0 //common
#define CS_GPIO_PIN   (107)
#define CLK_GPIO_PIN   (108)
#define SDO_GPIO_PIN   (110)
#define RST_GPIO_PIN   (15)
#define PANEL_EN (148)
#endif

#if 1 //i2m demo board
#define CS_GPIO_PIN   (9)
#define CLK_GPIO_PIN   (6)
#define SDO_GPIO_PIN   (7)
#define RST_GPIO_PIN   (10)
#define PANEL_EN (UNNEEDED_CONTROL)
#endif

typedef struct
{
    char path[100];
    MI_U8 gpio_num;
}stgpio_value_path;

stgpio_value_path gstgpio_value_path[5];

static void WaitTime(long ms)
{
    int i;
    for (i = 0; i < ms; i++)
    {
        usleep(1000);
    }
}

static void init_cmd(unsigned short data)
{
    MI_PANEL_SetGpioStatus(CS_GPIO_PIN, 0);
    MI_PANEL_SetCmd(0x00, 1);
    MI_PANEL_SetCmd(data, 8);
    MI_PANEL_SetGpioStatus(CS_GPIO_PIN, 1);
}

static void init_data(unsigned short data)
{
    MI_PANEL_SetGpioStatus(CS_GPIO_PIN, 0);
    MI_PANEL_SetCmd(0x01, 1);
    MI_PANEL_SetCmd(data, 8);
    MI_PANEL_SetGpioStatus(CS_GPIO_PIN, 1);
}

void init_panel_ST7796S(void)
{
    printf("init panel start\n");

    MI_PANEL_GpioConfig_t stGpioCfg;

    stGpioCfg.u16GpioBL = UNNEEDED_CONTROL;
    stGpioCfg.u16GpioRST = RST_GPIO_PIN;//15;
    stGpioCfg.u16GpioCS = CS_GPIO_PIN;
    stGpioCfg.u16GpioSCL = CLK_GPIO_PIN;
    stGpioCfg.u16GpioSDO = SDO_GPIO_PIN;
    stGpioCfg.u16GpioEN = PANEL_EN;
    MI_PANEL_GPIO_Init(&stGpioCfg);

    MI_PANEL_SetGpioStatus(PANEL_EN, 1);
    MI_PANEL_SetGpioStatus(CS_GPIO_PIN, 1);
    MI_PANEL_SetGpioStatus(CLK_GPIO_PIN, 1);
    MI_PANEL_SetGpioStatus(SDO_GPIO_PIN, 1);

    //panel reset
    MI_PANEL_SetGpioStatus(RST_GPIO_PIN, 1);
    WaitTime(2);
    MI_PANEL_SetGpioStatus(RST_GPIO_PIN, 0);
    WaitTime(2);
    MI_PANEL_SetGpioStatus(RST_GPIO_PIN, 1);

    WaitTime(20);
    init_cmd(0x11);     //Sleep Out
    WaitTime(20);

    init_cmd(0x36);
    init_data(0x58);

    init_cmd(0x3A);
    init_data(0x55);

    init_cmd(0xF0);
    init_data(0xC3);

    init_cmd(0xF0);
    init_data(0x96);

    init_cmd(0xB0);
    init_data(0x8C);

    init_cmd(0xB4);// Display Inversion Control
    init_data(0x01);//1-dot inversion

    init_cmd(0xB5);
    init_data(0x02); //RF Modify
    init_data(0x02); //RF Modify
    init_data(0x00);
    init_data(0x04); //RF Modify

    init_cmd(0xB6);
    init_data(0xA0);
    init_data(0x02);
    init_data(0x3B);

    init_cmd(0xE8);
    init_data(0x40);
    init_data(0x8A);
    init_data(0x00);
    init_data(0x00);
    init_data(0x29);
    init_data(0x0F); //RF
    init_data(0x32); //RF
    init_data(0x33);

    init_cmd(0xC0);
    init_data(0x80);
    init_data(0x31);   //VGH=14.968V, VGL=-11.385

    init_cmd(0xC1);   //GVDD
    init_data(0x18);   //4.75V

    init_cmd(0xC2);
    init_data(0xA7);

    init_cmd(0xC5);   //VCOM
    init_data(0x12);   //0.725V  //10

    init_cmd(0xE0);
    init_data(0x78);
    init_data(0x0C);
    init_data(0x17);
    init_data(0x11);
    init_data(0x11);
    init_data(0x1B);
    init_data(0x3D);
    init_data(0x44);
    init_data(0x4C);
    init_data(0x38);
    init_data(0x12);
    init_data(0x10);
    init_data(0x1C);
    init_data(0x1F);

    init_cmd(0xE1);
    init_data(0xF0);
    init_data(0x06);
    init_data(0x11);
    init_data(0x0C);
    init_data(0x0E);
    init_data(0x19);
    init_data(0x3B);
    init_data(0x44);
    init_data(0x4E);
    init_data(0x3B);
    init_data(0x16);
    init_data(0x16);
    init_data(0x22);
    init_data(0x26);

    init_cmd(0xF0);
    init_data(0x3C);

    init_cmd(0xF0);
    init_data(0x69);
    WaitTime(20);

    init_cmd(0x29);
    init_cmd(0x2c);

    printf("init panel end\n");
}

void init_panel_ST7796S_N(void)
{
	printf("init panel start\n");

    MI_PANEL_GpioConfig_t stGpioCfg;

    stGpioCfg.u16GpioBL = UNNEEDED_CONTROL;
    stGpioCfg.u16GpioRST = RST_GPIO_PIN;//15;
    stGpioCfg.u16GpioCS = CS_GPIO_PIN;
    stGpioCfg.u16GpioSCL = CLK_GPIO_PIN;
    stGpioCfg.u16GpioSDO = SDO_GPIO_PIN;
    stGpioCfg.u16GpioEN = PANEL_EN;
    MI_PANEL_GPIO_Init(&stGpioCfg);

    MI_PANEL_SetGpioStatus(PANEL_EN, 1);
    MI_PANEL_SetGpioStatus(CLK_GPIO_PIN, 1);
    MI_PANEL_SetGpioStatus(SDO_GPIO_PIN, 1);
    MI_PANEL_SetGpioStatus(CS_GPIO_PIN, 1);

    //panel reset
    MI_PANEL_SetGpioStatus(RST_GPIO_PIN, 1);
    WaitTime(5);
    MI_PANEL_SetGpioStatus(RST_GPIO_PIN, 0);
    WaitTime(5);
    MI_PANEL_SetGpioStatus(RST_GPIO_PIN, 1);
    //WaitTime(50);

    WaitTime(50);
    init_cmd(0x11);     //Sleep Out
    WaitTime(50);

    init_cmd(0x36);
    init_data(0xc8);

    init_cmd(0x3A);
    init_data(0x55);

    init_cmd(0xF0);
    init_data(0xC3);

    init_cmd(0xF0);
    init_data(0x96);

	init_cmd(0xB4);// Display Inversion Control
    init_data(0x01);//1-dot inversion

    init_cmd(0xB0);
    init_data(0x82);

	init_cmd(0xB6);
    init_data(0xA0);
	init_data(0x02); //RF Modify
	init_data(0x3b);

	init_cmd(0xC1);
	init_data(0x1D);

	init_cmd(0xC2);
	init_data(0xA7);

	init_cmd(0xC5);
	init_data(0x23);

	init_cmd(0xE8);
	init_data(0x40);
	init_data(0x8A);
	init_data(0x00);
	init_data(0x00);
	init_data(0x29);
	init_data(0x19);
	init_data(0xA5);
	init_data(0x33);

  	init_cmd(0xE0);
	init_data(0xF0);
	init_data(0x03);
	init_data(0x0A);
	init_data(0x12);
	init_data(0x15);
	init_data(0x1D);
	init_data(0x42);
	init_data(0x44);
	init_data(0x50);
	init_data(0x28);
	init_data(0x16);
	init_data(0x15);
	init_data(0x20);
	init_data(0x21);

	init_cmd(0xE1);
	init_data(0xF0);
	init_data(0x03);
	init_data(0x0A);
	init_data(0x12);
	init_data(0x15);
	init_data(0x1C);
	init_data(0x42);
	init_data(0x44);
	init_data(0x52);
	init_data(0x28);
	init_data(0x16);
	init_data(0x15);
	init_data(0x20);
	init_data(0x23);

	init_cmd(0xF0);
	init_data(0x3C);

	init_cmd(0xF0);
	init_data(0x69);

	WaitTime(50);
	init_cmd(0x29);
	init_cmd(0x20);

    printf("init panel end\n");
}

void init_panel_ST7701S(void)
{
    printf("init panel start\n");

    MI_PANEL_GpioConfig_t stGpioCfg;

    stGpioCfg.u16GpioBL = UNNEEDED_CONTROL;
    stGpioCfg.u16GpioRST = RST_GPIO_PIN;//15;
    stGpioCfg.u16GpioCS = CS_GPIO_PIN;
    stGpioCfg.u16GpioSCL = CLK_GPIO_PIN;
    stGpioCfg.u16GpioSDO = SDO_GPIO_PIN;
    stGpioCfg.u16GpioEN = UNNEEDED_CONTROL;
    MI_PANEL_GPIO_Init(&stGpioCfg);

    //MI_PANEL_SetGpioStatus(PANEL_EN, 1);
    MI_PANEL_SetGpioStatus(CS_GPIO_PIN, 1);
    MI_PANEL_SetGpioStatus(CLK_GPIO_PIN, 1);
    MI_PANEL_SetGpioStatus(SDO_GPIO_PIN, 1);

    //panel reset
    MI_PANEL_SetGpioStatus(RST_GPIO_PIN, 1);
    WaitTime(2);
    MI_PANEL_SetGpioStatus(RST_GPIO_PIN, 0);
    WaitTime(2);
    MI_PANEL_SetGpioStatus(RST_GPIO_PIN, 1);

    WaitTime(20);
    init_cmd(0x11);
    WaitTime(20);

    init_cmd(0xFF);
    init_data(0x77);
    init_data(0x01);
    init_data(0x00);
    init_data(0x00);
    init_data(0x10);

    init_cmd(0xC0);
    init_data(0xE9);
    init_data(0x03);

    init_cmd(0xC1);
    init_data(0x11);
    init_data(0x02);

    init_cmd(0xC2);
    init_data(0x31);
    init_data(0x08);

    init_cmd(0xC3);
    init_data(0x0c);
    init_data(0x10);
    init_data(0x08);

    init_cmd(0xCC);
    init_data(0x10);

    init_cmd(0xB0);
    init_data(0x00);//1
    init_data(0x0D);
    init_data(0x14);
    init_data(0x0D);
    init_data(0x10);
    init_data(0x05);
    init_data(0x02);
    init_data(0x08);
    init_data(0x08);
    init_data(0x1E);
    init_data(0x05);
    init_data(0x13);
    init_data(0x11);
    init_data(0xA3);
    init_data(0x29);
    init_data(0x18);

    init_cmd(0xB1);
    init_data(0x00);
    init_data(0x0C);
    init_data(0x14);
    init_data(0x0C);
    init_data(0x10);
    init_data(0x05);
    init_data(0x03);
    init_data(0x08);
    init_data(0x07);
    init_data(0x20);
    init_data(0x05);
    init_data(0x13);
    init_data(0x11);
    init_data(0xA4);
    init_data(0x29);
    init_data(0x18);

    init_cmd(0xFF);
    init_data(0x77);
    init_data(0x01);
    init_data(0x00);
    init_data(0x00);
    init_data(0x11);

    init_cmd(0xB0);
    init_data(0x6C);

    init_cmd(0xB1);
    init_data(0x43);

    init_cmd(0xB2);
    init_data(0x07);

    init_cmd(0xB3);
    init_data(0x80);

    init_cmd(0xB5);
    init_data(0x47);

    init_cmd(0xB7);
    init_data(0x85);

    init_cmd(0xB8);
    init_data(0x20);

    init_cmd(0xB9);
    init_data(0x10);

    init_cmd(0xC1);
    init_data(0x78);

    init_cmd(0xC2);
    init_data(0x78);

    init_cmd(0xD0);
    init_data(0x88);
    WaitTime(20);

    init_cmd(0xE0);
    init_data(0x00);
    init_data(0x00);
    init_data(0x02);

    init_cmd(0xE1);
    init_data(0x08);
    init_data(0x00);
    init_data(0x0A);
    init_data(0x00);
    init_data(0x07);
    init_data(0x00);
    init_data(0x09);
    init_data(0x00);
    init_data(0x00);
    init_data(0x33);
    init_data(0x33);

    init_cmd(0xE2);
    init_data(0x00);
    init_data(0x00);
    init_data(0x00);
    init_data(0x00);
    init_data(0x00);
    init_data(0x00);
    init_data(0x00);
    init_data(0x00);
    init_data(0x00);
    init_data(0x00);
    init_data(0x00);
    init_data(0x00);
    init_data(0x00);

    init_cmd(0xE3);
    init_data(0x00);
    init_data(0x00);
    init_data(0x33);
    init_data(0x33);

    init_cmd(0xE4);
    init_data(0x44);
    init_data(0x44);

    init_cmd(0xE5);
    init_data(0x0E);
    init_data(0x60);
    init_data(0xA0);
    init_data(0xA0);
    init_data(0x10);
    init_data(0x60);
    init_data(0xA0);
    init_data(0xA0);
    init_data(0x0A);
    init_data(0x60);
    init_data(0xA0);
    init_data(0xA0);
    init_data(0x0C);
    init_data(0x60);
    init_data(0xA0);
    init_data(0xA0);

    init_cmd(0xE6);
    init_data(0x00);
    init_data(0x00);
    init_data(0x33);
    init_data(0x33);

    init_cmd(0xE7);
    init_data(0x44);
    init_data(0x44);

    init_cmd(0xE8);
    init_data(0x0D);
    init_data(0x60);
    init_data(0xA0);
    init_data(0xA0);
    init_data(0x0F);
    init_data(0x60);
    init_data(0xA0);
    init_data(0xA0);
    init_data(0x09);
    init_data(0x60);
    init_data(0xA0);
    init_data(0xA0);
    init_data(0x0B);
    init_data(0x60);
    init_data(0xA0);
    init_data(0xA0);

    init_cmd(0xEB);
    init_data(0x02);
    init_data(0x01);
    init_data(0xE4);
    init_data(0xE4);
    init_data(0x44);
    init_data(0x00);
    init_data(0x40);

    init_cmd(0xEC);
    init_data(0x02);
    init_data(0x01);

    init_cmd(0xED);
    init_data(0xAB);
    init_data(0x89);
    init_data(0x76);
    init_data(0x54);
    init_data(0x01);
    init_data(0xFF);
    init_data(0xFF);
    init_data(0xFF);
    init_data(0xFF);
    init_data(0xFF);
    init_data(0xFF);
    init_data(0x10);
    init_data(0x45);
    init_data(0x67);
    init_data(0x98);
    init_data(0xBA);

    init_cmd(0xFF);
    init_data(0x77);
    init_data(0x01);
    init_data(0x00);
    init_data(0x00);
    init_data(0x00);

    init_cmd(0x29);

    printf("init panel end\n");
}

void init_panel_LX50FWB(void)
{
    int n;
    MI_U32 u32SpiCmdCnt;
    MI_PANEL_GpioConfig_t stGpioCfg;

    printf("init panel start\n");

    stGpioCfg.u16GpioBL = UNNEEDED_CONTROL;
    stGpioCfg.u16GpioRST = RST_GPIO_PIN;//15;
    stGpioCfg.u16GpioCS = CS_GPIO_PIN;
    stGpioCfg.u16GpioSCL = CLK_GPIO_PIN;
    stGpioCfg.u16GpioSDO = SDO_GPIO_PIN;
    stGpioCfg.u16GpioEN = UNNEEDED_CONTROL;
    MI_PANEL_GPIO_Init(&stGpioCfg);

    //MI_PANEL_SetGpioStatus(PANEL_EN, 1);
    MI_PANEL_SetGpioStatus(CS_GPIO_PIN, 1);
    MI_PANEL_SetGpioStatus(CLK_GPIO_PIN, 1);
    MI_PANEL_SetGpioStatus(SDO_GPIO_PIN, 1);

    //panel reset
    MI_PANEL_SetGpioStatus(RST_GPIO_PIN, 0);
    WaitTime(20);
    MI_PANEL_SetGpioStatus(RST_GPIO_PIN, 1);

    u32SpiCmdCnt = sizeof(Initialization_CmdTable)/sizeof(MI_PANEL_SpiCmdTable_t);
    for(n = 0; n < u32SpiCmdCnt; n++)
    {
        if(REGFLAG_DELAY == Initialization_CmdTable[n].cmd)
            usleep(Initialization_CmdTable[n].count*1000);
        else if(REGFLAG_END_OF_TABLE == Initialization_CmdTable[n].cmd)
            break;
        else
        {
            MI_PANEL_SetGpioStatus(CS_GPIO_PIN, 0);
            MI_PANEL_SetCmd(Initialization_CmdTable[n].cmd, 16);
            MI_PANEL_SetGpioStatus(CS_GPIO_PIN, 1);
        }
    }
    printf("init panel end\n");
}
