#ifndef __CHAR_CONVERSION_H__
#define __CHAR_CONVERSION_H__

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

int utf8ToUnicodeChar (unsigned char *ch, int *unicode);
int utf8ToUnicode (unsigned char * utf8_str,
		unsigned short * unicode_str,
		int unicode_str_size);
int gb2312ToUtf8( char*  ptDestText,
		int  nDestLength,
		char*  ptSrcText,
		int  nSrcLength);

int utf8ToGb2312(char *gb2312_str,
		int len_gb2312,
		char *utf8_str,
		int len_utf8);
#ifdef __cplusplus
}
#endif	// __cplusplus

#endif //__CHAR_CONVERSION_H__