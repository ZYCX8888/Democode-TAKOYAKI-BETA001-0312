#ifndef _MID_JSON_H_
#define _MID_JSON_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include "cjson_utils.h"
/*****
 *    设置子节点string ,int
 * ****/
int mcjson_setstring(cJSON *my_json,char *setvalue,char *father,...);
int mcjson_setint(cJSON *my_json,int setvalue,char *father,...);
int mcjson_set_child_obj(cJSON *my_json,void *obj,char *father,...);

/*****
 *    获取节点的object , string ,int
 * ****/
cJSON *mcJSON_getJSON(cJSON *pj, const char *father,...);
char *mcJSON_getSTRING(cJSON *pj,char *return_defvalue,const char *father,...);
int mcJSON_getINT(cJSON *pj,int return_defint,const char *father,...);
/*
*    读取整个文件到字符串中
*    清除注释：
*/
char *read_json_file_delete_comment(char *filepath);

#ifdef __cplusplus
}
#endif

#endif
