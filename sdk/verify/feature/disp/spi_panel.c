#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include "mi_panel.h"
#include "init_panel_driveric.h"
#include "spi_operation.h"
#include "gpio_operation.h"

#define CS_PANEL_0   (47)
#define CS_PANEL_1   (48)

#define RS_GPIO_PIN    (7)
#define RST_GPIO_PIN   (6)

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
    _user_set_gpio_value(CS_PANEL_0,0);
    _user_set_gpio_value(CS_PANEL_1,0);

    _user_set_gpio_value(RS_GPIO_PIN,0);
    SPI_Write((char*)&data,1);

    _user_set_gpio_value(CS_PANEL_0,1);
    _user_set_gpio_value(CS_PANEL_1,1);
}

static void init_param(unsigned short data)
{
    _user_set_gpio_value(CS_PANEL_0,0);
    _user_set_gpio_value(CS_PANEL_1,0);

    _user_set_gpio_value(RS_GPIO_PIN,1);
    SPI_Write((char*)&data,1);

    _user_set_gpio_value(CS_PANEL_0,1);
    _user_set_gpio_value(CS_PANEL_1,1);
}

void init_panel_ST7789V(void)
{
    printf("init panel start\n");

    _user_export_gpio(CS_PANEL_0);
    _user_export_gpio(CS_PANEL_1);
    _user_export_gpio(RST_GPIO_PIN);
    _user_export_gpio(RS_GPIO_PIN);

    _user_set_gpio_direction(CS_PANEL_0,"out");
    _user_set_gpio_direction(CS_PANEL_1,"out");
    _user_set_gpio_direction(RS_GPIO_PIN,"out");
    _user_set_gpio_direction(RST_GPIO_PIN,"out");

    _user_set_gpio_value(CS_PANEL_0,1);
    _user_set_gpio_value(CS_PANEL_1,1);
    _user_set_gpio_value(RS_GPIO_PIN,1);

    _user_set_gpio_value(RST_GPIO_PIN,1);
    WaitTime(2);
    _user_set_gpio_value(RST_GPIO_PIN,0);
    WaitTime(2);
    _user_set_gpio_value(RST_GPIO_PIN,1);
    WaitTime(20);

    //*** ST7789V + CTC2.8 ***//
    //===============================================
    init_cmd(0x11); 
    WaitTime(120); //WaitTime 120ms 
    //--------------------------------ST7789S Frame rate setting----------------------------------// 
    init_cmd(0xb2); 
    init_param(0x0c); 
    init_param(0x0c); 
    init_param(0x00); 
    init_param(0x33); 
    init_param(0x33); 

    init_cmd(0xb7); 
    init_param(0x35);   //VGH=13V, VGL=-10.4V
    //---------------------------------ST7789S Power setting--------------------------------------// 
    init_cmd(0xbb); 
    init_param(0x22); 	   //28

    init_cmd(0xc0); 
    init_param(0x2c); 

    init_cmd(0xc2); 
    init_param(0x01); 

    init_cmd(0xc3); 
    init_param(0x28);		 //调深浅   0b	  20

    init_cmd(0xc4); 
    init_param(0x22); 		//20

    init_cmd(0xc6); 
    init_param(0x1e); 

    init_cmd(0xd0); 
    init_param(0xa4); 
    init_param(0xa1); 

    init_cmd(0x3A);
    init_param(0x05);

    //--------------------------------ST7789S gamma setting---------------------------------------// 
    init_cmd(0xe0); 
    init_param(0xd0); 
    init_param(0x03); 
    init_param(0x08); 
    init_param(0x0b); 
    init_param(0x0f); 
    init_param(0x2c); 
    init_param(0x41); 
    init_param(0x54); 
    init_param(0x4e); 
    init_param(0x07); 
    init_param(0x0e); 
    init_param(0x0c); 
    init_param(0x1e); 
    init_param(0x23); 

    init_cmd(0xe1); 
    init_param(0xd0); 
    init_param(0x03); 
    init_param(0x09); 
    init_param(0x0b); 
    init_param(0x0d); 
    init_param(0x19); 
    init_param(0x3c); 
    init_param(0x54); 
    init_param(0x4f); 
    init_param(0x0e); 
    init_param(0x1d); 
    init_param(0x1c); 
    init_param(0x20); 
    init_param(0x22); 

    init_cmd(0x29); 
}


pthread_t send_data_pt;
#define panel_w 240
#define panel_h 320
#define bytesperpixel 2 
#define buffsize (panel_w*panel_h*bytesperpixel)

void *imagedata = NULL;
bool stop_pthread = false;

static void mlswap(void *pdata,unsigned int panelW,unsigned int panelH)
{
    int row,column;
    unsigned char dataH,dataL;
    unsigned short pixel;
    unsigned short *pos = (unsigned short*)pdata;

    for(row=0;row<panelW;row++)
        for(column=0;column<panelH;column++)
        {
            pixel=*pos; 
            dataH = (pixel&0xff00)>>8;
            dataL = pixel&0xff;
            *pos= dataL<<8|dataH;
            pos++;
        }
}

static void *panel_send_data_pt(void *pData)
{
    int srcfd;
    int readsize = 0;
    unsigned int sendbytes = 0;
    unsigned short *pdata = (unsigned short*)imagedata;
    unsigned char *buff = (unsigned char*)imagedata;

    srcfd = open("./240_320_rgb565_20.bin",O_RDONLY);
    if(srcfd < 0)
    {
        perror("open src file err\n");
    }

    readsize = read(srcfd, imagedata, buffsize);
    if(readsize != buffsize)
    {
        printf("read src file err,read size:%d\n",readsize); 
    }

    mlswap(pdata,panel_w,panel_h);

    init_cmd(0x2c); 
    _user_set_gpio_value(CS_PANEL_0,0);
    _user_set_gpio_value(CS_PANEL_1,0);
    _user_set_gpio_value(RS_GPIO_PIN,1);

    while(!stop_pthread)
    {
        if(sendbytes<buffsize)
        {
#if 0
            unsigned char dataH = 0;
            unsigned char dataL = 0;
            unsigned short data = 0;
            data = *pdata;
            dataH = (data&0xff00)>>8;
            dataL = data&0xff;
            send_disp_data(dataH,dataL);
            pdata++;
            sendbytes += 2;

            //MI_PANEL_SetGpioStatus(RS_GPIO_PIN, 1);
            //SPI_Write((char*)&dataH,1);
            //MI_PANEL_SetGpioStatus(RS_GPIO_PIN, 0);

            //MI_PANEL_SetGpioStatus(RS_GPIO_PIN, 1);
            //SPI_Write((char*)&dataL,1);
            //MI_PANEL_SetGpioStatus(RS_GPIO_PIN, 0);
#endif

#if 1 
            {
                int row = 4,column = 240;
                SPI_Write(buff,column*row*2);
                buff+=column*row*2; 
                sendbytes+=column*row*2;
            }
#endif
        }
        else
        {
            readsize = read(srcfd, imagedata, buffsize);
            if(readsize < buffsize)
            {
                lseek(srcfd, 0, SEEK_SET);
                readsize = read(srcfd, imagedata, buffsize);
                if(readsize != buffsize)
                {
                    printf("read src file failed,read size:%d\n",readsize); 
                }
            }
            sendbytes = 0; 
            pdata = (unsigned short*)imagedata;
            buff = (unsigned char*)imagedata;
            mlswap(pdata,panel_w,panel_h);
        }
    }

    _user_set_gpio_value(CS_PANEL_0,1);
    _user_set_gpio_value(CS_PANEL_1,1);
    _user_set_gpio_value(RS_GPIO_PIN,0);
    init_cmd(0x29); 

    close(srcfd);

    return 0;
}

int main(void)
{
    int i = 0;
    char *data = NULL;

    SPI_Open();
    init_panel_ST7789V();

    imagedata = malloc(buffsize);
    if(imagedata)
        data = (char*)imagedata;
    else
        printf("malloc image data buff failed\n");

    pthread_create(&send_data_pt, NULL, panel_send_data_pt, NULL);

    for(;;)
    {
        fd_set stFdSet;
        char buf[50];
        FD_ZERO(&stFdSet);
        FD_SET(0,&stFdSet);
        printf("input 'q' exit\n\n");
        select(1, &stFdSet, NULL, NULL, NULL);
        if(FD_ISSET(0, &stFdSet))
        {
            int i = read(0, buf, sizeof(buf));
            if(i>0 && (buf[0] == 'q'))
            {
                printf("pthread exit\n");
                stop_pthread = true;
                pthread_join(send_data_pt,NULL);
                break;
            }
        }
    }
    

    if(imagedata)
    {
        free(imagedata);
        imagedata = NULL;
    }
    SPI_Close();

    printf("show spi panel exit\n");

    return 0;
}
