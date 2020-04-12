#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "es7210.h"
#include "i2c.h"

int ES7210_WriteByte(unsigned char reg, unsigned char val)
{
    int s32Ret;
    unsigned char data[2];
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages;

    if (-1 == I2C_GetFd())
    {
        printf("ES7210 hasn't been call I2C_Init.\n");
        return -1;
    }

    memset((&packets), 0x0, sizeof(packets));
    memset((&messages), 0x0, sizeof(messages));

    // send one message
    packets.nmsgs = 1;
    packets.msgs = &messages;

    // fill message
    messages.addr = ES7210_CHIP_ADDR;   // codec reg addr
    messages.flags = 0;                 // read/write flag, 0--write, 1--read
    messages.len = 2;                   // size
    messages.buf = data;                // data addr

    // fill data
    data[0] = reg;
    data[1] = val;

    s32Ret = ioctl(I2C_GetFd(), I2C_RDWR, (unsigned long)&packets);
    if (s32Ret < 0)
    {
        printf("Failed to write byte to ES7210: %s.\n", strerror(errno));
        return -1;
    }

    return 0;
}

int ES7210_ReadByte(unsigned char reg, unsigned char *val)
{
    int s32Ret;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];
    unsigned char tmpReg, tmpVal;

    if (-1 == I2C_GetFd())
    {
        printf("ES7210 hasn't been call I2C_Init.\n");
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

    messages[0].addr = ES7210_CHIP_ADDR;
    messages[0].flags = 0;
    messages[0].len = 1;
    messages[0].buf = &tmpReg;

    tmpVal = 0;
    messages[1].addr = ES7210_CHIP_ADDR;
    messages[1].flags = 1;
    messages[1].len = 1;
    messages[1].buf = &tmpVal;

    s32Ret = ioctl(I2C_GetFd(), I2C_RDWR, (unsigned long)&packets);
    if (s32Ret < 0)
    {
        printf("Failed to read byte from ES7210: %s.\n", strerror(errno));
        return -1;
    }

    *val = tmpVal;
    return 0;
}

int ES7210_Init(void)
{
    int s32Ret;
    unsigned char val = 0;

    s32Ret = I2C_Init(10, 5);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

#if 0
    s32Ret = ES7210_WriteByte(0x00, 0xFF);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x00, 0x32);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x09, 0x30);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x0A, 0x30);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x23, 0x2A);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x22, 0x0A);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x21, 0x2A);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x20, 0x0A);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x40, 0xC3);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x41, 0x70);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x42, 0x70);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x43, 0x10);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x44, 0x10);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x45, 0x10);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x46, 0x10);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x47, 0x08);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x48, 0x08);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x49, 0x08);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x4A, 0x08);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x07, 0x20);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x02, 0x41);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_ReadByte(0x02, &val);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    printf("================================[0x02]:0x%x=========================================\n",
    val);

    s32Ret = ES7210_WriteByte(0x4B, 0x0F);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x4C, 0x0F);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x02, 0x41);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x06, 0x00);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x08, 0x12);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x11, 0x60);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x43, 0x1e);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x44, 0x1e);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x45, 0x1e);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x46, 0x1e);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x00, 0x71);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x00, 0x41);
    if (0 != s32Ret)
    {
        return s32Ret;
    }
#endif

#if 1
        s32Ret = ES7210_WriteByte(0x00, 0xFF);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x00, 0x32);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x09, 0x30);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x0A, 0x30);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x23, 0x2A);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x22, 0x0A);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x21, 0x2A);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x40, 0xC3);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x41, 0x70);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x42, 0x70);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x43, 0x10);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x44, 0x10);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x45, 0x10);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x46, 0x10);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x47, 0x08);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x48, 0x08);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x49, 0x08);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x4A, 0x08);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x07, 0x20);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x02, 0x41);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_ReadByte(0x02, &val);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x4B, 0x0F);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x4C, 0x0F);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x02, 0x41);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x06, 0x00);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x08, 0x12);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x11, 0x60);
        if (0 != s32Ret)
        {
            return s32Ret;
        }


        //0x20, mic1 mic2
        //0x10, mic3 mic4
        //mic1和mic3短接，所以无论怎么切拿到的Chn0都是mic1的数据
        s32Ret = ES7210_WriteByte(0x12, 0x10);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x43, 0x1e);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x44, 0x1e);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x45, 0x1e);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x46, 0x1e);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x00, 0x71);
        if (0 != s32Ret)
        {
            return s32Ret;
        }

        s32Ret = ES7210_WriteByte(0x00, 0x41);
        if (0 != s32Ret)
        {
            return s32Ret;
        }
#endif

    printf("================Init ES7210 success.======================\n");

    return 0;
}

int ES7210_Deinit(void)
{
    int s32Ret;
    unsigned char val = 0;

    s32Ret = I2C_Init(10, 5);
    if (0 != s32Ret)
    {
        return s32Ret;
    }
#if 0
    s32Ret = ES7210_WriteByte(0x04, 0x02);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x04, 0x01);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0xf7, 0x30);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0xf9, 0x01);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x16, 0xff);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x17, 0x00);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x01, 0x38);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x20, 0x00);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x21, 0x00);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x00, 0x00);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x00, 0x1e);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x01, 0x30);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x01, 0x00);
    if (0 != s32Ret)
    {
        return s32Ret;
    }
#endif

    s32Ret = ES7210_WriteByte(0x06, 0x00);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x4B, 0xFF);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x4C, 0xFF);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x0B, 0xD0);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x40, 0x80);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x01, 0x7F);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = ES7210_WriteByte(0x06, 0x07);
    if (0 != s32Ret)
    {
        return s32Ret;
    }
	printf("Suspend ES7210 success.\n");
	return 0;
}

