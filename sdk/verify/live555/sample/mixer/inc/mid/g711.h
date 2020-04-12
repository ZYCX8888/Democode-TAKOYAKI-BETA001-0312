#ifndef __G711_H_
#define __G711_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void G711Encoder(short *pcm,unsigned char *code,int size,int lawflag);

void G711Decoder(short *pcm,unsigned char *code,int size,int lawflag);

void G711Covert(unsigned char *dst, unsigned char *src, int size, int flag);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //__G711_H_
