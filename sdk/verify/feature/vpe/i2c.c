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
#include <linux/i2c-dev.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "i2c.h"
#include "memmap.h"
//#include "mi_vpe_test.h"
#define DBG_ENTER()
// I2C Linux device handle
static int g_i2cFile;

int vif_i2c_init()
{
    DBG_ENTER();

    /* RIU mapping*/
    MmapHandle* riu_base = devMemMMap(0x1F000000,0x207000);

    /*Configure PAD and Clock here*/

    *(unsigned short*)REG_ADDR(riu_base->virt_addr,0x1026,0x00) |= 0x00;
    *(unsigned short*)REG_ADDR(riu_base->virt_addr,0x1026,0x06) |= 0x04;
    *(unsigned short*)REG_ADDR(riu_base->virt_addr,0x1033,0x30) |= 0x40;
    *(unsigned short*)REG_ADDR(riu_base->virt_addr,0x1026,0x2B) |= 0xF0;

    *(unsigned short*)REG_ADDR(riu_base->virt_addr,0x1025,0x16) |= 0xF000;
    *(unsigned short*)REG_ADDR(riu_base->virt_addr,0x1026,0x02) |= 0x8D30;
    *(unsigned short*)REG_ADDR(riu_base->virt_addr,0x1026,0x03) |= 0x0061;

    *(unsigned short*)REG_ADDR(riu_base->virt_addr,0x1026,0x28) |= 0x03FF;
    *(unsigned short*)REG_ADDR(riu_base->virt_addr,0x1026,0x2E) |= 0x03FF;
    *(unsigned short*)REG_ADDR(riu_base->virt_addr,0x1026,0x34) |= 0x03FF;
    *(unsigned short*)REG_ADDR(riu_base->virt_addr,0x1026,0x3A) |= 0x03FF;

    devMemUmap(riu_base);
    //MI_VPE_TEST_INFO("Start.\n");


    g_i2cFile = open("/dev/i2c-1", O_RDWR);
    if (g_i2cFile < 0)
    {
        //MI_VPE_TEST_PRINT("open dev i2c err\n");
        return -1;;
    }

    //MI_VPE_TEST_INFO("%s success\n",__FUNCTION__);
    return 0;
}

int vif_i2c_deinit()
{
    close(g_i2cFile);
}


// set the I2C slave address for all subsequent I2C device transfers
static void i2cSetAddress(int address)
{
    if (ioctl(g_i2cFile, I2C_SLAVE_FORCE, address) < 0) {
        perror("i2cSetAddress");
        exit(1);
    }
}

// salveAddr : 8 bit savle address
int vif_i2c_write(int slaveAddr, short reg, unsigned short value, ISP_I2C_FMT fmt)
{
    unsigned char data[4];

    i2cSetAddress(slaveAddr >> 1);
    //printf("[%s]: slave Addr: %#x \n", __func__, slaveAddr);

    memset(data, 0, sizeof(data));
    switch(fmt) {
        default:
        case I2C_FMT_A8D8:
            //printf("[%s]: I2C_FMT_A8D8, reg = %#x, value = %#x \n", __func__, reg, value);
            data[0] = reg & 0xff;
            data[1] = value & 0xff;
            if (write(g_i2cFile, data, 2) != 2) {
                perror("Write Register");
                return -1;
            }
            break;
        case I2C_FMT_A16D8:
            //printf("[%s]: I2C_FMT_A16D8, reg = %#x, value = %#x \n", __func__, reg, value);
            data[0] = (reg >> 8) & 0xff;
            data[1] = reg & 0xff;
            data[2] = value & 0xff;
            if (write(g_i2cFile, data, 3) != 3) {
                perror("Write Register");
                return -1;
            }
            break;
        case I2C_FMT_A8D16:
            //printf("[%s]: I2C_FMT_A8D16, reg = %#x, value = %#x \n", __func__, reg, value);
            data[0] = reg & 0xff;
            data[1] = (value >> 8) & 0xff;
            data[2] = (value ) & 0xff;
            if (write(g_i2cFile, data, 3) != 3) {
                perror("Write Register");
                return -1;
            }
            break;
        case I2C_FMT_A16D16:
            //printf("[%s]: I2C_FMT_A16D16, reg = %#x, value = %#x \n", __func__, reg, value);
            data[0] = (reg >> 8) & 0xff;
            data[1] = (reg ) & 0xff;
            data[2] = (value >> 8) & 0xff;
            data[3] = (value ) & 0xff;
            if (write(g_i2cFile, data, 4) != 4) {
                perror("SetRegisterPair");
            }

            break;
    }

    return 0;
}

int vif_i2c_read(int slaveAddr, unsigned int reg, unsigned short *val, ISP_I2C_FMT fmt)
{
    unsigned char reg_addr[2];

    i2cSetAddress(slaveAddr >> 1);
    //printf("[%s]: slave Addr: %#x \n", __func__, slaveAddr);
    memset(reg_addr, 0, sizeof(unsigned char));

    switch(fmt) {
        default:
        case I2C_FMT_A8D8:
            //printf("[%s]: I2C_FMT_A8D8 \n", __func__);
            reg_addr[0] =  reg & 0xff;
            if (write(g_i2cFile, reg_addr, 1) != 1) {
                perror("Read RegisterPair set register");
                return -1;
            }
            if (read(g_i2cFile, val, 1) != 1) {
                perror("Read RegisterPair read value");
                return -1;
            }
            //printf("[%s]: read val[0] = %#x \n", __func__, val[0]);

            break;
        case I2C_FMT_A16D8:
            //printf("[%s]: I2C_FMT_A16D8 \n", __func__);
            reg_addr[0] = (reg >> 8) & 0xff;
            reg_addr[1] =  reg & 0xff;
            //printf("reg_addr[0]: %#x\n", reg_addr[0]);
            //printf("reg_addr[1]: %#x\n", reg_addr[1]);
            if (write(g_i2cFile, reg_addr, 2) != 2) {
                perror("Read RegisterPair set register");
                return -1;
            }
            if (read(g_i2cFile, val, 1) != 1) {
                perror("Read RegisterPair read value");
                return -1;
            }
            //printf("[%s]: read val[0] = %#x \n", __func__, val[0]);
            break;
        case I2C_FMT_A8D16:
            //printf("[%s]: I2C_FMT_A8D16 \n", __func__);
            reg_addr[0] =  reg & 0xff;
            //printf("reg_addr[0]: %#x\n", reg_addr[0]);
            if (write(g_i2cFile, reg_addr, 1) != 1) {
                perror("Read RegisterPair set register");
                return -1;
            }
            if (read(g_i2cFile, val, 2) != 2) {
                perror("Read RegisterPair read value");
                return -1;
            }
            //printf("[%s]: read val[0] = %#x \n", __func__, val[0]);

            break;
        case I2C_FMT_A16D16:
            //printf("[%s]: I2C_FMT_A16D16 \n", __func__);
            reg_addr[0] = (reg >> 8) & 0xff;
            reg_addr[1] =  reg & 0xff;
            if (write(g_i2cFile, reg_addr, 2) != 2) {
                perror("Read RegisterPair set register");
                return -1;
            }
            if (read(g_i2cFile, val, 2) != 2) {
                perror("Read RegisterPair read value");
                return -1;
            }
            //printf("[%s]: read val[0] = %#x \n", __func__, val[0]);

            break;
    }

    return 0;
}
