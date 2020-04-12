#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "es8156.h"
#include "i2c.h"

int ES8156_WriteByte(unsigned char reg, unsigned char val)
{
    int s32Ret;
    unsigned char data[2];
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages;

    if (-1 == I2C_GetFd())
    {
        printf("ES8156 hasn't been call I2C_Init.\n");
        return -1;
    }

    memset((&packets), 0x0, sizeof(packets));
    memset((&messages), 0x0, sizeof(messages));

    // send one message
    packets.nmsgs = 1;
    packets.msgs = &messages;

    // fill message
    messages.addr = ES8156_CHIP_ADDR;   // codec reg addr
    messages.flags = 0;                 // read/write flag, 0--write, 1--read
    messages.len = 2;                   // size
    messages.buf = data;                // data addr

    // fill data
    data[0] = reg;
    data[1] = val;

    s32Ret = ioctl(I2C_GetFd(), I2C_RDWR, (unsigned long)&packets);
    if (s32Ret < 0)
    {
        printf("Failed to write byte to ES8156: %s.\n", strerror(errno));
        return -1;
    }

    return 0;
}

int ES8156_ReadByte(unsigned char reg, unsigned char *val)
{
    int s32Ret;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];
    unsigned char tmpReg, tmpVal;

    if (-1 == I2C_GetFd())
    {
        printf("ES8156 hasn't been call I2C_Init.\n");
        return -1;
    }

    if (NULL == val)
    {
        printf("val param is NULL.\n");
        return -1;
    }

    tmpReg = reg;
    memset((&packets), 0x0, sizeof(packets));
    memset((&messages), 0x0, sizeof(messages));

    packets.nmsgs = 2;
    packets.msgs = &messages;

    messages[0].addr = ES8156_CHIP_ADDR;
    messages[0].flags = 0;
    messages[0].len = 1;
    messages[0].buf = &tmpReg;

    tmpVal = 0;
    messages[1].addr = ES8156_CHIP_ADDR;
    messages[1].flags = 1;
    messages[1].len = 1;
    messages[1].buf = &tmpVal;

    s32Ret = ioctl(I2C_GetFd(), I2C_RDWR, (unsigned long)&packets);
    if (s32Ret < 0)
    {
        printf("Failed to read byte from ES8156: %s.\n", strerror(errno));
        return -1;
    }

    *val = tmpVal;
    return 0;
}

int ES8156_Init(void)
{
    int s32Ret;
    unsigned char val = 0;

    s32Ret = I2C_Init(10, 5);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_ReadByte(REG_CHIP_ID0, &val);
    if (0 == s32Ret)
    {
        printf("ES8156 ID0:%x.\n", val);
    }
    else
    {
        return s32Ret;
    }

    s32Ret = ES8156_ReadByte(REG_CHIP_ID1, &val);
    if (0 == s32Ret)
    {
        printf("ES8156 ID1:%x.\n", val);
    }
    else
    {
        return s32Ret;
    }

    //val = MODE_CONFIG_1_VAL(1, 0, 0, 0, 0, 1, 0, 0);
    val = 0x84;
    s32Ret = ES8156_WriteByte(REG_MODE_CONFIG_1, val);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x00, 0x3c);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x00, 0x1c);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x02, 0x84);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x0A, 0x01);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x0B, 0x01);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x0D ,0x14);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x01, 0xE1);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x18, 0x00);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x08, 0x3F);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x09, 0x02);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x00, 0x01);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x22, 0x00);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x23, 0xCA);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x24, 0x00);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x25, 0x20);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x14, 0xbf);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    printf("======================Init ES8156 success.========================\n");

    return 0;
}

int ES8156_Deinit(void)
{
    int s32Ret;
    unsigned char val = 0;

    s32Ret = I2C_Init(10, 5);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x14, 0x00);
    if (0 != s32Ret)
    {
            return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x19, 0x02);
    if (0 != s32Ret)
    {
            return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x21, 0x1F);
    if (0 != s32Ret)
    {
            return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x22, 0x02);
    if (0 != s32Ret)
    {
            return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x25, 0x21);
    if (0 != s32Ret)
    {
            return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x25, 0x01);
    if (0 != s32Ret)
    {
            return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x25, 0x87);
    if (0 != s32Ret)
    {
            return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x18, 0x01);
    if (0 != s32Ret)
    {
            return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x09, 0x02);
    if (0 != s32Ret)
    {
            return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x09, 0x01);
    if (0 != s32Ret)
    {
            return s32Ret;
    }

    s32Ret = ES8156_WriteByte(0x08, 0x00);
    if (0 != s32Ret)
    {
            return s32Ret;
    }
	printf("Deinit ES8156 success.\n");
}
