#include "gpio_operation.h"

void _user_set_gpio_value(unsigned int gpio_num, unsigned char value)
{
    unsigned char n = 0;
    char set_value[4];
    char path[100];
    FILE *fp;

    sprintf(path, "/sys/class/gpio/gpio%d/value", gpio_num);
    if ((fp = fopen(path, "rb+")) == NULL)
    {
        printf("Cannot open gpio value file.\n");
        exit(1);
    }
    if (value)
    {
        rewind(fp);
        strcpy(set_value,"1");
        fwrite(&set_value, sizeof(char), 1, fp);
        fclose(fp);
    }
    else
    {
        rewind(fp);
        strcpy(set_value,"0");
        fwrite(&set_value, sizeof(char), 1, fp);
        fclose(fp);
    }
}
void _user_export_gpio(unsigned int gpio_num)
{
    FILE *fp;
    unsigned char u8strlen;
    char str[10];

    if ((fp = fopen("/sys/class/gpio/export", "ab")) == NULL)
    {
        printf("Cannot open export file.\n");
        exit(1);
    }
    rewind(fp);
    sprintf(str, "%d", gpio_num);
    u8strlen = strlen(str);
    fwrite(str, sizeof(char), u8strlen, fp);
    fclose(fp);
    printf("user export gpio:%d\n",gpio_num);
}

void _user_set_gpio_direction(unsigned int gpio_num, char *pdirection)
{
    char path[100];
    FILE *fp;
    unsigned char u8strlen;

    sprintf(path, "/sys/class/gpio/gpio%d/direction", gpio_num);
    if ((fp = fopen(path, "rb+")) == NULL)
    {
        printf("Cannot open direction file.\n");
        exit(1);
    }
    rewind(fp);
    u8strlen = strlen(pdirection);
    fwrite(pdirection, sizeof(char), u8strlen, fp);
    fclose(fp);
    printf("gpio:%d direction set ok\n",gpio_num);
}

