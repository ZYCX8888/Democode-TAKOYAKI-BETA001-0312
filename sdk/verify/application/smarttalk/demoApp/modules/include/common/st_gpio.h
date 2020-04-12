#ifndef __ST_GPIO_H__
#define __ST_GPIO_H__

#ifdef __cplusplus
extern "C"  {
#endif

MI_S32 ST_Gpio_Export(MI_S32 s32Gpio);
MI_S32 ST_Gpio_SetDirection(MI_S32 s32Gpio, MI_S32 s32Direction);
MI_S32 ST_Gpio_GetDirection(MI_S32 s32Gpio, MI_S8 *ps8Direction, MI_S32 s32Len);
MI_S32 ST_Gpio_SetValue(MI_S32 s32Gpio, MI_S8 *s8Value, MI_S32 s32Len);
MI_S32 ST_Gpio_GetValue(MI_S32 s32Gpio, MI_S8 *s8Value, MI_S32 s32Len);

#ifdef __cplusplus
}
#endif

#endif
