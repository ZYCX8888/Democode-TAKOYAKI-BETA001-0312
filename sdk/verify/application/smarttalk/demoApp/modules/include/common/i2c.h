#ifndef _I2C_H
#define _I2C_H

typedef enum {
    I2C_FMT_A8D8, /**< 8 bits Address, 8 bits Data */
    I2C_FMT_A16D8,/**< 16 bits Address 8 bits Data */
    I2C_FMT_A8D16,/**< 8 bits Address 16 bits Data */
    I2C_FMT_A16D16,/**< 16 bits Address 16 bits Data */
    I2C_FMT_END/**< Reserved */
} ISP_I2C_FMT;

int vif_i2c_init();
int vif_i2c_deinit();
int vif_i2c_write(int slaveAddr, short reg, unsigned short value, ISP_I2C_FMT fmt);
int vif_i2c_read(int slaveAddr, unsigned int reg, unsigned short *val, ISP_I2C_FMT fmt);
#endif
