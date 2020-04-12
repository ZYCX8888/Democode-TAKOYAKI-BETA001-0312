#ifndef __GPIO_OPERATION_H
#define __GPIO_OPERATION_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void _user_set_gpio_value(unsigned int gpio_num, unsigned char value);
void _user_export_gpio(unsigned int gpio_num);
void _user_set_gpio_direction(unsigned int gpio_num, char *pdirection);

#endif
