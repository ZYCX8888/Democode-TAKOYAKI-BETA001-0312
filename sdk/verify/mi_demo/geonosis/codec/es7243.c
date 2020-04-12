#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "es7243.h"
#include "i2c.h"

int ES7243_WriteByte(unsigned char reg, unsigned char val)
{
    int s32Ret;
    unsigned char data[2];
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages;

    if (-1 == I2C_GetFd())
    {
        printf("ES7243 hasn't been call I2C_Init.\n");
        return -1;
    }

    memset((&packets), 0x0, sizeof(packets));
    memset((&messages), 0x0, sizeof(messages));

    // send one message
    packets.nmsgs = 1;
    packets.msgs = &messages;

    // fill message
    messages.addr = ES7243_CHIP_ADDR;   // codec reg addr
    messages.flags = 0;                 // read/write flag, 0--write, 1--read
    messages.len = 2;                   // size
    messages.buf = data;                // data addr

    // fill data
    data[0] = reg;
    data[1] = val;

    s32Ret = ioctl(I2C_GetFd(), I2C_RDWR, (unsigned long)&packets);
    if (s32Ret < 0)
    {
        printf("Failed to write byte to ES7243: %s.\n", strerror(errno));
        return -1;
    }

    return 0;
}

int ES7243_ReadByte(unsigned char reg, unsigned char *val)
{
    int s32Ret;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];
    unsigned char tmpReg, tmpVal;

    if (-1 == I2C_GetFd())
    {
        printf("ES7243 hasn't been call I2C_Init.\n");
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

    messages[0].addr = ES7243_CHIP_ADDR;
    messages[0].flags = 0;
    messages[0].len = 1;
    messages[0].buf = &tmpReg;

    tmpVal = 0;
    messages[1].addr = ES7243_CHIP_ADDR;
    messages[1].flags = 1;
    messages[1].len = 1;
    messages[1].buf = &tmpVal;

    s32Ret = ioctl(I2C_GetFd(), I2C_RDWR, (unsigned long)&packets);
    if (s32Ret < 0)
    {
        printf("Failed to read byte from ES7243: %s.\n", strerror(errno));
        return -1;
    }

    *val = tmpVal;
    return 0;
}

int ES7243_Init(void)
{
    int s32Ret;
    unsigned char val = 0;

    s32Ret = I2C_Init(10, 5);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

#if 1
    s32Ret = ES7243_WriteByte(0x00, 0x09);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7243_ReadByte(0x00, &val);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    printf("=============== [0x00]:0x%x ======================.\n", val);

    s32Ret = ES7243_WriteByte(0x06, 0x00);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7243_WriteByte(0x05, 0x1B);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7243_WriteByte(0x01, 0x0C);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7243_WriteByte(0x04, 0x01);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7243_WriteByte(0x08, 0x43);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7243_WriteByte(0x05, 0x13);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

#endif

    printf("================Init ES7210 success.======================\n");

    return 0;
}

int ES7243_Deinit(void)
{
    int s32Ret;
    unsigned char val = 0;

    s32Ret = I2C_Init(10, 5);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7243_WriteByte(0x06, 0x05);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7243_WriteByte(0x05, 0x1B);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7243_WriteByte(0x06, 0x5C);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7243_WriteByte(0x07, 0x3F);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7243_WriteByte(0x08, 0x4B);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7243_WriteByte(0x09, 0x9F);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

	printf("Suspend ES7243 success.\n");
	return 0;
}

