//#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "mid_json.h"
/*****
 *　参数包含需要获取的json数据
 *	获取object , string ,int
 * ****/
 /* Predeclare these prototypes. */
cJSON *mcJSON_getJSON(cJSON *pj, const char *father,...);
char *mcJSON_getSTRING(cJSON *pj,char *return_defvalue, const char *father,...);
int mcJSON_getINT(cJSON *pj,int return_defint, const char *father,...);
/* Parse the input text into an unescaped cstring, and populate item. */
//static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static cJSON *_mjson_getobject(cJSON *my_json, const char *father,va_list p);
/* Add item to array/object. */
//检查是否注释行，即含有//
static int check_comment(char *str){
	int is_comment=0;
	int j=0;

	for(;str[j] !='\0';j++){
		if(str[j]==' '){
			continue;
		}else if(str[j] =='/' && str[j+1] == '/'){
			is_comment=1;
		}else{
			break;
		}
	}
	return is_comment;
}
/*
*	读取整个文件到字符串中
*	清除注释：
*/
char *read_json_file_delete_comment(char *filepath)
{
    FILE * pFile;
    long lSize;
    char * buffer,tmp[1024];


    if(access(filepath,F_OK) !=0){
    	printf( "Could not find the FILE[%s]", filepath);
    	return NULL;
    }

    /* 若要一个byte不漏地读入整个文件，只能采用二进制方式打开 */
    pFile = fopen (filepath, "rb" );
    if (pFile==NULL)
    {
    	printf( "Could not open the FILE[%s]", filepath);
        return NULL;
    }

    /* 获取文件大小 */
    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
	if(0 >= lSize)
	{
	  fclose(pFile);
	  return NULL;
	}
    rewind (pFile);

    /* 分配内存存储整个文件*/
    buffer = (char*) malloc (sizeof(char)*lSize+2);
	if(NULL == buffer)
	{
	  printf( "Could not open the FILE[%s]: Memory! ", filepath);
	  fclose(pFile);
	  return NULL;
	}
    memset(buffer,0,lSize);

    int index=0,j=0;
    while(fgets(tmp,1024,pFile) != NULL){
    	j=0;

    	//检查是否注释行
    	if(check_comment(tmp) == 1) break;

    //	j=0;
    	while(tmp[j] != '\0'){
    		buffer[index]=tmp[j];
    		index++;
    		if(index>lSize) break;
    		if(1023 < (j++)) break;
    	}
    }
    buffer[index]='\0';

	fclose(pFile);
	return buffer;

}



/****
 * 获取子json对象
 * 最多10个，最后一个必须为NULL
 * ***/
cJSON *_mjson_getobject(cJSON *my_json,const char *father,va_list p){
	cJSON *fathj = cJSON_GetObjectItem(my_json,father);
	if((fathj = cJSON_GetObjectItem(my_json,father)) == NULL){
		return NULL;
	}
	const char *child=father;
	int i=0;
	for(;child || i>10;i++){
		if((child = va_arg(p,char *))==NULL) {
			break;
		}
		if((fathj = cJSON_GetObjectItem(fathj,child))==NULL){
			break;
		}
		father=child;
	}
	return fathj;
}

/*****
 *　参数包含需要获取的json数据
 *	获取object , string ,int
 *
 *
 * ****/
cJSON *mcJSON_getJSON(cJSON *my_json,const char *father,...){
	if(father == NULL){
		return my_json; //  :(
	}
	va_list p;
	va_start(p,father);
	va_end(p);
	return _mjson_getobject(my_json,father,p);

}

char *mcJSON_getSTRING(cJSON *my_json,char *return_defvalue,const char *father,...){
	if(father == NULL){
		return my_json->valuestring;
	}

	va_list p;
	va_start(p,father);
	va_end(p);
	cJSON *j =  _mjson_getobject(my_json,father,p);

	return j ? j->valuestring:return_defvalue;
}
int mcJSON_getINT(cJSON *my_json,int return_defint, const char *father,...){

	if(father == NULL){
		return my_json->valuedouble; 
	}

	va_list p;
	va_start(p,father);
	va_end(p);
	cJSON *j =  _mjson_getobject(my_json,father,p);
	return j ?j->valuedouble: return_defint;
}
int mcjson_setstring(cJSON *my_json,char *setvalue,char *father,...){
	if(father == NULL){
		my_json->valuestring = setvalue;
		my_json->type = cJSON_String;
		return 1;
	}
	va_list p;
	va_start(p,father);
	va_end(p);
	cJSON *j =  _mjson_getobject(my_json,father,p);
	if(j){
		j->type = cJSON_String;
		j->valuestring=setvalue;
		return 1;
	}else{
		return -1;
	}
}




int mcjson_setint(cJSON *my_json,int setvalue,char *father,...){

	if(father== NULL){
		my_json->valuedouble = setvalue;
		my_json->type = cJSON_Number;
		return 1;
	}

	va_list p;
	va_start(p,father);
	va_end(p);
	cJSON *j =  _mjson_getobject(my_json,father,p);

	if(j){
		j->type = cJSON_Number;
		j->valuedouble=setvalue;
		return 1;
	}else{
		return -1;
	}
}
/**
 * father 为NULL，放置在NEXT
 * 
*/
int mcjson_set_obj(cJSON *my_json,char *obj_key,cJSON *obj,char *father,...){
	if(father == NULL && obj_key != NULL){
		my_json->string = obj_key;
		my_json->type = cJSON_Object;
		my_json->next = obj;
		obj->prev = my_json;
		return 1;
	}else if(father == NULL && obj_key == NULL){
		my_json->type = cJSON_Array;
		my_json->next = obj;
		obj->prev = my_json;
		return 1;	
	}

	va_list p;
	va_start(p,father);
	va_end(p);
	cJSON *j =  _mjson_getobject(my_json,father,p);

	if(j){
		j->child= obj;
		return 1;
	}else{
		return -1;
	}
}

