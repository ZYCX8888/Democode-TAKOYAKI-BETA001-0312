#include "mid_system_config.h"
#include "mi_vpe_datatype.h"

static ModuleAoInfo_s AoDevInfo_t;
static ModuleAiChnInfo_s AiChnInfo_t;
static ModuleAiInfo_s AiDevInfo_t;
static ModuleRgnInfo_s RgnInfo_t;
static ModuleVencInfo_s VencInfo_t;
static ModuleVpeInfo_s  VpeBaseInfo_t;
static ModuleVpeTopInfo_s VpeTopInfo_t;
static ModuleLdcInfo_s  LdcInfo_t;
static ModuleVifInfo_s  VifInfo_t;
static ModuleSensorInfo_s      SensorInfo_t;
static ModuleRecInfo_s  RecInfo_t;

const static char *RecPath = "/tmp/mixer_rec/";

const static char *ModuleCfgName[DevMax] = {\
	"DevSensor",\
	"DevVif",\
	"DevVpeTop",\
	"DevVpeBase",\
	"DevLdc",\
	"DevVenc",\
	"DevRgn",\
	"DevAi",\
	"DevAo",\
	"DevAiChn",\
	"DevRec",
};

static MI_S8 CreateSensorCfg(cJSON **targetJson)
{
	cJSON *item = NULL;
	cJSON *child_item[SENSOR_REINDEX_NUM] = {NULL, NULL};
	cJSON * child_arr = NULL;
	MI_U16 i = 0x0, j = 0x0;

	if(NULL == targetJson)
	{
		MIXER_ERR("targetJson is null!\n");
		return CREATE_ITEM_FAIL;
	}

	item = cJSON_CreateObject();
	if(NULL == item)
	{
		printf("Creat the json is fail!\n");
		return CREATE_ITEM_FAIL;
	}

	cJSON_AddStringToObject(item,"name","imx307");
	cJSON_AddNumberToObject(item,"workResIndex",0);

	child_arr = cJSON_CreateArray();
	if(NULL == child_arr)
	{
		MIXER_ERR("create sensorResIndex attr item config is fail !\n");
		goto exit;
	}

	for(i = 0; i < SENSOR_REINDEX_NUM; i++)
	{
		child_item[i] = cJSON_CreateObject();
		if(NULL == child_item[i])
		{
			MIXER_ERR("can not create child_item. err\n");
			goto exit;
		}

		if(child_item[i])
		{
			cJSON_AddNumberToObject(child_item[i], "maxFps", 30);
			cJSON_AddNumberToObject(child_item[i], "minFps", 3);
			cJSON_AddNumberToObject(child_item[i], "cropX", 0);
			cJSON_AddNumberToObject(child_item[i], "cropY", 0);
			cJSON_AddNumberToObject(child_item[i], "sensor_w",1920);
			cJSON_AddNumberToObject(child_item[i], "sensor_h",1080);
			cJSON_AddItemToArray(child_arr, child_item[i]);
		}
	}

	MIXER_DBG("create sensorResIndex attr item config is OK !\n");
	cJSON_AddItemToObject(item, "sensorResIndex", child_arr);

	*targetJson = item;

	return CREAT_OK;

exit:

	printf("create sensorResIndex attr item config is fail !\n");
	for(j = 0; j < i && child_item[j]; j++)
	{
		cJSON_Delete(child_item[j]);
	}

	if(NULL != child_arr)
		cJSON_Delete(child_arr);

	if(NULL != item)
		cJSON_Delete(item);

	return CREATE_ITEM_FAIL;
}

static MI_S8 CreateVifCfg(cJSON **targetJson)
{
	cJSON *item = NULL;

	if(NULL == targetJson)
	{
		MIXER_ERR("targetJson is null!\n");
		return CREATE_ITEM_FAIL;
	}

	item = cJSON_CreateObject();
	if(NULL == item)
	{
		MIXER_ERR("Creat the json is fail!\n");
		return CREATE_ITEM_FAIL;
	}
	cJSON_AddNumberToObject(item, "MaxFps", 30);
	cJSON_AddNumberToObject(item, "MinFps", 3);
	cJSON_AddNumberToObject(item, "MaxWidth", 1920);
	cJSON_AddNumberToObject(item, "MaxHeight", 1080);
	cJSON_AddNumberToObject(item, "CurWidth", 1920);
	cJSON_AddNumberToObject(item, "CurHeight", 1080);
	cJSON_AddNumberToObject(item, "IsHDR", 0);
	cJSON_AddNumberToObject(item, "VifChn", 0);
	MIXER_DBG("create vif attr itemconfig is OK !\n");

	*targetJson = item;

	return CREAT_OK;
}

static MI_S8 CreateVpeTopCfg(cJSON **targetJson)
{
	cJSON *item = NULL;

	if(NULL == targetJson)
	{
		MIXER_ERR("targetJson is null!\n");
		return CREATE_ITEM_FAIL;
	}

	item = cJSON_CreateObject();
	if(NULL == item)
	{
		MIXER_ERR("Creat the json is fail!\n");
		return CREATE_ITEM_FAIL;
	}

	cJSON_AddNumberToObject(item, "MaxFps", 30);
	cJSON_AddNumberToObject(item, "MinFps", 3);
	cJSON_AddNumberToObject(item, "MaxWidth", VPE_OUTPORT0_W);
	cJSON_AddNumberToObject(item, "MaxHeight", VPE_OUTPORT0_H);
	cJSON_AddNumberToObject(item, "PortId", 0);
	cJSON_AddNumberToObject(item, "DevId", 0);
	cJSON_AddNumberToObject(item, "ChnId", 0);
	cJSON_AddNumberToObject(item, "3DnrLevel", 0);
	cJSON_AddNumberToObject(item, "Rotation", 0);

	MIXER_DBG("create VpeTop attr itemconfig is OK !\n");

	*targetJson = item;

	return CREAT_OK;
}

static MI_S8 CreateVpeBaseCfg(cJSON **targetJson)
{
	cJSON *item = NULL;
	cJSON *child_item[VPE_MAX_PORT] = {NULL};
	cJSON *child_arr = NULL;
    MI_U32 tmpW = 1920;
	MI_U32 tmpH = 1080;
	MI_U32 curVpeH = 1920; 
	MI_U32 curVpeW = 1080;
	MI_U8 port = 0x0, i = 0x0;
	
#if TARGET_CHIP_I5
	MI_U8 Channel = 1;
#elif TARGET_CHIP_I6
	MI_U8 Channel = 0;
#elif TARGET_CHIP_I6E
	MI_U8 Channel = 0x0;
#elif TARGET_CHIP_I6B0
	MI_U8 Channel = 0x0;
#endif

	if(NULL == targetJson)
	{
		MIXER_ERR("targetJson is null!\n");
		return CREATE_ITEM_FAIL;
	}

	item = cJSON_CreateObject();
	if(NULL == item)
	{
		MIXER_ERR("Creat the json is fail!\n");
		goto exit;
	}

	cJSON_AddNumberToObject(item, "channel", Channel);
	cJSON_AddNumberToObject(item, "Rota", 0);
	cJSON_AddNumberToObject(item, "3dnrLevel", 1);

	child_arr = cJSON_CreateArray();
	if(NULL == child_arr)
	{
		MIXER_ERR("Creat child_arr is fail!\n");
		goto exit;
	}

	for(port = 0; port < VPE_MAX_PORT; port++)
	{     
		child_item[port] = cJSON_CreateObject();
		if(NULL == child_item[port])
		{
			MIXER_ERR("Creat child_item is fail!\n");
			goto exit;	
		}
	    if(0 == port)//jpeg
	    {
	      tmpW = VPE_OUTPORT0_W > JPEG_STREAM_W ? VPE_OUTPORT0_W:JPEG_STREAM_W;
		  tmpH = VPE_OUTPORT0_H > JPEG_STREAM_H ? VPE_OUTPORT0_H:JPEG_STREAM_H;
		  curVpeW = JPEG_STREAM_W;
		  curVpeH = JPEG_STREAM_H;
	    }
		else if(1 == port)//main stream
		{
		  tmpW = VPE_OUTPORT1_W > MAIN_STREAM_W ? VPE_OUTPORT1_W:MAIN_STREAM_W;
		  tmpH = VPE_OUTPORT1_H > MAIN_STREAM_H ? VPE_OUTPORT1_H:MAIN_STREAM_H;
		  curVpeW = MAIN_STREAM_W; 
		  curVpeH = MAIN_STREAM_H;
		}
		else if(2 == port)//second/thired.. stream
		{
		  tmpW = VPE_OUTPORT2_W > SECOND_STREAM_W ? VPE_OUTPORT2_W : SECOND_STREAM_W;
		  tmpH = VPE_OUTPORT2_H > SECOND_STREAM_H ? VPE_OUTPORT2_H : SECOND_STREAM_H;
		  curVpeW = SECOND_STREAM_W; 
		  curVpeH = SECOND_STREAM_H;
		}
		else
		{
		  tmpW = VPE_OUTPORT2_W > THIRD_STREAM_W ? VPE_OUTPORT2_W : THIRD_STREAM_W;
		  tmpH = VPE_OUTPORT2_H > THIRD_STREAM_H ? VPE_OUTPORT2_H : THIRD_STREAM_H;
		  curVpeW = THIRD_STREAM_W; 
		  curVpeH = THIRD_STREAM_H;
		}
		cJSON_AddNumberToObject(child_item[port], "fps", 30);
		cJSON_AddNumberToObject(child_item[port], "minFps", 3);
		cJSON_AddNumberToObject(child_item[port], "maxWidth", tmpW);
		cJSON_AddNumberToObject(child_item[port], "maxHeight", tmpH);
		cJSON_AddNumberToObject(child_item[port], "curWidth", curVpeW);
		cJSON_AddNumberToObject(child_item[port], "curHeight", curVpeH);
		cJSON_AddNumberToObject(child_item[port], "outDepth", 3);

		cJSON_AddItemToArray(child_arr, child_item[port]);
	}
	cJSON_AddItemToObject(item, "vpebase_attr", child_arr);
	
	*targetJson = item;
	MIXER_DBG("create vpe base config is ok\n");
	
	return CREAT_OK;

exit:
	MIXER_ERR("create vpe port%d item Attr config is failed!\n",port);

	for(i = 0x0; i < port && child_item[i]; i++)
	{
		if(NULL != child_item[i])
		{
			cJSON_Delete(child_item[i]);
			child_item[i] = NULL;
		}
	}

	if(NULL != child_arr)
		cJSON_Delete(child_arr);   

	if(NULL != item)
		cJSON_Delete(item);

	return CREATE_ITEM_FAIL;
}

static MI_S8 CreateLDCCfg(cJSON **targetJson)
{
	cJSON * item = NULL;

	if(NULL == targetJson)
	{
		MIXER_ERR("targetJson is null!\n");
		return CREATE_ITEM_FAIL;
	}
	
	item = cJSON_CreateObject();
	if(NULL == item)
	{
		MIXER_ERR("Creat the json is fail!\n");
		return CREATE_ITEM_FAIL;
	}
	
	cJSON_AddNumberToObject(item, "MaxFps", 30);
	cJSON_AddNumberToObject(item, "MinFps", 3);
	cJSON_AddNumberToObject(item, "maxWidth", VPE_OUTPORT0_W);
	cJSON_AddNumberToObject(item, "maxHeight", VPE_OUTPORT0_H);
#if TARGET_CHIP_I6E
	cJSON_AddNumberToObject(item, "portId", OPEN_LDC_FROM_PORT);
#else
    cJSON_AddNumberToObject(item, "portId", 0);
#endif
	cJSON_AddNumberToObject(item, "devId", 0);
	cJSON_AddNumberToObject(item, "chnId", 0);
	cJSON_AddNumberToObject(item, "3dnrLevel", 0);
	cJSON_AddNumberToObject(item, "rotation", 0);

	MIXER_DBG("create Ldc item config is OK !\n");

	*targetJson = item;

	return CREAT_OK;
}

static MI_S8 CreateVencCfg(cJSON **targetJson)
{
	cJSON *item = NULL;
	cJSON *child_arr = NULL, *child[MAX_VIDEO_NUMBER] ={NULL};
	MI_U8 VencNum = 0x0;
	MI_U8 j = 0x0;

	if(NULL == targetJson)
	{
		MIXER_ERR("targetJson is null!\n");
		return CREATE_ITEM_FAIL;
	}

	item = cJSON_CreateObject();
	if(NULL == item)
	{
		MIXER_ERR("Creat the json is fail!\n");
		return CREATE_ITEM_FAIL;
	}
	
	{
		child_arr = cJSON_CreateArray();
		if(NULL == child_arr)
		{
			printf("create venc chn is fail!\n" );

			cJSON_Delete(item);
			goto exit;
		}

		for(VencNum = 0; VencNum < MAX_VIDEO_NUMBER; VencNum++)
		{
			child[VencNum] = cJSON_CreateObject();
			if(NULL == child[VencNum])
			{
				goto exit;
			}

			if(0 == VencNum)	//main stream
			{
				#if TARGET_CHIP_I6	|| TARGET_CHIP_I6B0
				cJSON_AddNumberToObject(child[VencNum],"FromPort", M_STREAM_FROM_PORT);
				cJSON_AddNumberToObject(child[VencNum],"StreamWidth",MAIN_STREAM_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamHeight",MAIN_STREAM_H); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxWidth",MAIN_STREAM_MAX_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxHeight",MAIN_STREAM_MAX_H); 
				cJSON_AddNumberToObject(child[VencNum],"BindModuleType", Mixer_Venc_Bind_Mode_HW_RING);
				#elif TARGET_CHIP_I6E 
				cJSON_AddNumberToObject(child[VencNum],"FromPort", M_STREAM_FROM_PORT);
				cJSON_AddNumberToObject(child[VencNum],"StreamWidth",MAIN_STREAM_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamHeight",MAIN_STREAM_H); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxWidth",MAIN_STREAM_MAX_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxHeight",MAIN_STREAM_MAX_H);
				cJSON_AddNumberToObject(child[VencNum],"BindModuleType", Mixer_Venc_Bind_Mode_FRAME);
				#else
				cJSON_AddNumberToObject(child[VencNum],"FromPort", M_STREAM_FROM_PORT);
				cJSON_AddNumberToObject(child[VencNum],"StreamWidth",MAIN_STREAM_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamHeight",MAIN_STREAM_H); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxWidth",MAIN_STREAM_MAX_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxHeight",MAIN_STREAM_MAX_H);
				//cJSON_AddNumberToObject(child[VencNum],"BindModuleType", Mixer_Venc_Bind_Mode_FRAME);
				#endif
				cJSON_AddNumberToObject(child[VencNum],"EncodeType", VE_AVC); 
				cJSON_AddNumberToObject(child[VencNum],"MmaModuleID",MODULE_ID_VENC_H264_5);
				
				cJSON_AddNumberToObject(child[VencNum],"MaxBitrate", M_STREAM_MAX_BITRATE);
				cJSON_AddNumberToObject(child[VencNum],"Bitrate", M_STREAM_CUR_BITRATE);
				cJSON_AddNumberToObject(child[VencNum],"MinBitrate",M_STREAM_MIN_BITRATE); 
			}
			else if(1 == VencNum)		//sub stream
			{
				cJSON_AddNumberToObject(child[VencNum],"FromPort", S_STREAM_FROM_PORT);
				cJSON_AddNumberToObject(child[VencNum],"StreamWidth", SECOND_STREAM_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamHeight", SECOND_STREAM_H);
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxWidth",SECOND_STREAM_MAX_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxHeight",SECOND_STREAM_MAX_H); 
				
				cJSON_AddNumberToObject(child[VencNum],"EncodeType", VE_AVC ); 
				cJSON_AddNumberToObject(child[VencNum],"MmaModuleID", MODULE_ID_VENC_H264_5);

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0           
				cJSON_AddNumberToObject(child[VencNum],"BindModuleType", Mixer_Venc_Bind_Mode_FRAME);
#endif
				cJSON_AddNumberToObject(child[VencNum],"MaxBitrate", S_STREAM_MAX_BITRATE);
				cJSON_AddNumberToObject(child[VencNum],"Bitrate", S_STREAM_CUR_BITRATE);
				cJSON_AddNumberToObject(child[VencNum],"MinBitrate",S_STREAM_MIN_BITRATE); 
			}
			else if(2 == VencNum)	// four  jpeg
			{
				#if TARGET_CHIP_I6	
				cJSON_AddNumberToObject(child[VencNum],"FromPort", J_STREAM_FROM_PORT);
				cJSON_AddNumberToObject(child[VencNum],"StreamWidth",VPE_OUTPORT0_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamHeight",VPE_OUTPORT0_H);
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxWidth",VPE_OUTPORT0_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxHeight",VPE_OUTPORT0_H); 
				cJSON_AddNumberToObject(child[VencNum],"BindModuleType", J_VENC_BIND_TYPE);
				#elif TARGET_CHIP_I6E || TARGET_CHIP_I6B0		// jpeg's res must == main stream
				cJSON_AddNumberToObject(child[VencNum],"FromPort", J_STREAM_FROM_PORT);
				cJSON_AddNumberToObject(child[VencNum],"StreamWidth",VPE_OUTPORT0_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamHeight",VPE_OUTPORT0_H);
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxWidth",VPE_OUTPORT0_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxHeight",VPE_OUTPORT0_H); 
				cJSON_AddNumberToObject(child[VencNum],"BindModuleType", J_VENC_BIND_TYPE);
				#else	
				cJSON_AddNumberToObject(child[VencNum],"FromPort", J_STREAM_FROM_PORT);
				cJSON_AddNumberToObject(child[VencNum],"StreamWidth",VPE_OUTPORT1_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamHeight",VPE_OUTPORT1_H);
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxWidth",VPE_OUTPORT1_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxHeight",VPE_OUTPORT1_H); 
				//cJSON_AddNumberToObject(child[VencNum],"BindModuleType", Mixer_Venc_Bind_Mode_FRAME);
				#endif
				cJSON_AddNumberToObject(child[VencNum],"EncodeType", VE_JPG); 
				cJSON_AddNumberToObject(child[VencNum],"MmaModuleId",MODULE_ID_VENC_JPG);
				
				cJSON_AddNumberToObject(child[VencNum],"MaxBitrate", J_STREAM_MAX_BITRATE);
				cJSON_AddNumberToObject(child[VencNum],"Bitrate", J_STREAM_CUR_BITRATE);
				cJSON_AddNumberToObject(child[VencNum],"MinBitrate",J_STREAM_MIN_BITRATE); 
				
			}
#if TARGET_CHIP_I6B0
			else if(3 == VencNum) 
			{
				cJSON_AddNumberToObject(child[VencNum],"FromPort", VpePort3 );
				cJSON_AddNumberToObject(child[VencNum],"StreamWidth",THIRD_STREAM_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamHeight",THIRD_STREAM_H); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxWidth",THIRD_STREAM_MAX_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxHeight",THIRD_STREAM_MAX_H);
				cJSON_AddNumberToObject(child[VencNum],"EncodeType", VE_YUV420); 
				cJSON_AddNumberToObject(child[VencNum],"MmaModuleId",MODULE_ID_DIVP_FRAME_OUT);
				cJSON_AddNumberToObject(child[VencNum],"BindModuleType", Mixer_Venc_Bind_Mode_FRAME);
				cJSON_AddNumberToObject(child[VencNum],"MaxBitrate", T_STREAM_MAX_BITRATE);
				cJSON_AddNumberToObject(child[VencNum],"Bitrate",T_STREAM_CUR_BITRATE);
				cJSON_AddNumberToObject(child[VencNum],"MinBitrate",T_STREAM_MIN_BITRATE);
			}
#endif
			else 		//third
			{
#if TARGET_CHIP_I6B0
				cJSON_AddNumberToObject(child[VencNum],"FromPort", DivpCPort1 + (VencNum-4));
#else
				cJSON_AddNumberToObject(child[VencNum],"FromPort", DivpCPort1 + (VencNum-3));
#endif
				cJSON_AddNumberToObject(child[VencNum],"StreamWidth",THIRD_STREAM_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamHeight",THIRD_STREAM_H); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxWidth",THIRD_STREAM_MAX_W); 
				cJSON_AddNumberToObject(child[VencNum],"StreamMaxHeight",THIRD_STREAM_MAX_H);
				cJSON_AddNumberToObject(child[VencNum],"EncodeType", VE_YUV420); 
				cJSON_AddNumberToObject(child[VencNum],"MmaModuleId",MODULE_ID_DIVP_FRAME_OUT);
                
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0              
				cJSON_AddNumberToObject(child[VencNum],"BindModuleType", Mixer_Venc_Bind_Mode_FRAME);
#endif

				cJSON_AddNumberToObject(child[VencNum],"MaxBitrate", T_STREAM_MAX_BITRATE);
				cJSON_AddNumberToObject(child[VencNum],"Bitrate",T_STREAM_CUR_BITRATE);
				cJSON_AddNumberToObject(child[VencNum],"MinBitrate",T_STREAM_MIN_BITRATE); 
			}
			
			cJSON_AddNumberToObject(child[VencNum],"Fps",30); 
			cJSON_AddNumberToObject(child[VencNum],"MinFps",3); 

			cJSON_AddItemToArray(child_arr,child[VencNum]);
		}
	}
	cJSON_AddItemToObject(item, "venc_attr", child_arr);
	*targetJson = item;
	MIXER_DBG("create venc chn item is OK!\n");

	return CREAT_OK;
exit:

	for(j = 0x0;j < VencNum;j++)
	{
		if(child[j])
		{
			cJSON_Delete(child[j]);
			child[j] = NULL;
		}
	}		  	

	if(NULL != child_arr)
		cJSON_Delete(child_arr);

	if(NULL != item)
		cJSON_Delete(item);

	MIXER_ERR("create venc chn is fail!\n" );

	return CREATE_ITEM_FAIL;
}

static MI_S8 CreateRgnCfg(cJSON **targetJson)
{
	cJSON *item = NULL;

	if(NULL == targetJson)
	{
		MIXER_ERR("targetJson is null!\n");
		return CREATE_ITEM_FAIL;
	}

	item = cJSON_CreateObject();
	if(NULL == item)
	{
		MIXER_ERR("can not create obj. err\n");
		goto exit;
	}
	
	MIXER_DBG("creat The RGN item is OK!\n");
	cJSON_AddNumberToObject(item,"IsPraM", 1);

	*targetJson = item;

	return CREAT_OK;

exit:

	MIXER_ERR("create rgn attr item config is fail !\n");

	//if(NULL != item)
	//	cJSON_Delete(item);

	return CREATE_ITEM_FAIL;
}

static MI_S8 CreateAiDevCfg(cJSON **targetJson)
{
	cJSON *item = NULL;

	if(NULL == targetJson)
	{
		MIXER_ERR("targetJson is null!\n");
		return CREATE_ITEM_FAIL;
	}

	item = cJSON_CreateObject();
	if(NULL == item)
	{
		MIXER_ERR("can not create obj. err\n");
		return CREATE_ITEM_FAIL;
	}

	{
		cJSON_AddNumberToObject(item, "Chnt", 1);
		cJSON_AddNumberToObject(item, "MdiaType", 1);
		cJSON_AddNumberToObject(item, "Samplerate", 16000);
		cJSON_AddNumberToObject(item, "bindMod", 16);
		cJSON_AddNumberToObject(item, "bitWidthByte", 16);
		cJSON_AddNumberToObject(item, "VolumeInDb" ,13);

		MIXER_DBG("create aiDev item config is OK !\n");
	}
	*targetJson = item;

	return CREAT_OK;
}

static MI_S8 CreateAoDevCfg(cJSON **targetJson)
{
	cJSON *item = NULL;

	if(NULL == targetJson)
	{
		MIXER_ERR("targetJson is null!\n");
		return CREATE_ITEM_FAIL;
	}

	item = cJSON_CreateObject();
	if(NULL == item)
	{
		MIXER_ERR("can not create obj. err\n");
		return CREATE_ITEM_FAIL;
	}

	cJSON_AddNumberToObject(item, "MdiaType", 1);
	cJSON_AddNumberToObject(item, "Samplerate", 16000);
	cJSON_AddNumberToObject(item, "bitWidthByte", 16);
	cJSON_AddNumberToObject(item, "bindMod", 14);
	cJSON_AddNumberToObject(item, "VolumeOutDb", 10);

	cJSON_AddNumberToObject(item, "bindMod", 14);

	MIXER_DBG("create aoDev item config is OK !\n");

	*targetJson = item;

	return CREAT_OK;
}

static MI_S8 CreateAiChnCfg(cJSON **targetJson)
{
	cJSON *item = NULL;

	if(NULL == targetJson)
	{
		MIXER_ERR("targetJson is null!\n");
		return CREATE_ITEM_FAIL;
	}
	item = cJSON_CreateObject();
	if(NULL == item)
	{
		MIXER_ERR("can not create obj. err\n");
		return CREATE_ITEM_FAIL;
	}

	cJSON_AddNumberToObject(item, "Chnt", 1);
	cJSON_AddNumberToObject(item, "pktNum", 2);
	cJSON_AddNumberToObject(item, "bitWidthByte", 16);
	cJSON_AddNumberToObject(item, "bindMod", 15);

	MIXER_DBG("create aiChn item config is OK !\n");

	*targetJson = item;

	return CREAT_OK;
}

static MI_S8 CreateRecInfo(cJSON **targetJson)
{
	cJSON *item = NULL;

	if(NULL == targetJson)
	{
		MIXER_ERR("targetJson is null!\n");
		return CREATE_ITEM_FAIL;
	}
	item = cJSON_CreateObject();
	if(NULL == item)
	{
		MIXER_ERR("can not create obj. err\n");
		return CREATE_ITEM_FAIL;
	}

	cJSON_AddStringToObject(item, "RecPath", RecPath);
	cJSON_AddNumberToObject(item, "Storage", 0);
	
	MIXER_DBG("create Rec item config is OK !\n");

	*targetJson = item;

	return CREAT_OK;
}

MI_S8 CreateDefaultConfig(const MI_S8 *MixerConfigPath)
{ 
	MI_S8 i = 0;
	FILE * pFile = NULL;
	cJSON * father[DevMax] = {NULL};
	cJSON * item = NULL;
	MI_S8 ret = 0x0;

	if(NULL == MixerConfigPath)
	{
		MIXER_ERR("wrong param\n");
		return INVALID_FILEPATH;
	}

	ret = CreateSensorCfg(&father[0]);
	ret |= CreateVifCfg(&father[1]);
	ret |= CreateVpeTopCfg(&father[2]);
	ret |= CreateVpeBaseCfg(&father[3]);
	ret |= CreateLDCCfg(&father[4]);
	ret |= CreateVencCfg(&father[5]);
	ret |= CreateRgnCfg(&father[6]);
	ret |= CreateAiDevCfg(&father[7]);
	ret |= CreateAoDevCfg(&father[8]);
	ret |= CreateAiChnCfg(&father[9]);
	ret |= CreateRecInfo(&father[10]);
	
	if(CREAT_OK != ret)
	{
		for(i = 0x0; i<DevMax; i++)
		{
			if(NULL  != father[i])
			{
				cJSON_Delete(father[i]);
				father[i] = NULL;
			}
		}
		return CREATE_ITEM_FAIL;
	}

	item = cJSON_CreateObject();
	if(NULL == item)
	{
		printf("Creat the json is fail!\n");
		return CREATE_ITEM_FAIL;
	}
	
	i = DevSensor; 
	cJSON_AddItemToObject(item,ModuleCfgName[DevSensor], father[i]);
	
	cJSON_AddItemToObject(item,ModuleCfgName[DevVif], father[++i]);

	cJSON_AddItemToObject(item,ModuleCfgName[DevVpeTop], father[++i]);
	cJSON_AddItemToObject(item,ModuleCfgName[DevVpeBase], father[++i]);
	cJSON_AddItemToObject(item,ModuleCfgName[DevLdc], father[++i]);
	cJSON_AddItemToObject(item,ModuleCfgName[DevVenc], father[++i]);

	cJSON_AddItemToObject(item,ModuleCfgName[DevRgn], father[++i]);	
	cJSON_AddItemToObject(item,ModuleCfgName[DevAi], father[++i]);
	cJSON_AddItemToObject(item,ModuleCfgName[DevAo],father[++i]);
	cJSON_AddItemToObject(item,ModuleCfgName[DevAiChn],father[++i]);
	cJSON_AddItemToObject(item,ModuleCfgName[DevRec],father[++i]);
	
	printf("=DefaultConfig is %s\n",cJSON_Print(item));
	pFile = fopen((const char *)MixerConfigPath, "w+");
	if(NULL == pFile)
	{
		MIXER_ERR( "Could not creat the FILE[%s]", MixerConfigPath);
		return CREAT_FILE_FAIL;
	}
	fwrite(cJSON_Print(item), BUFFER_LEN, 1, pFile);
	fclose(pFile);
	if(item)
	{
		cJSON_Delete(item);
	}	
	return CREAT_OK;
}

MI_S32 Get_AllCustomerConfigInfo(MI_S8 *MixerConfigPath)
{
	char *filestr = NULL;
	char tmpItem[DevMax] = {0};
	cJSON *Item = NULL;
	cJSON *fatherJson = NULL;
	cJSON *childJson = NULL;
	MI_U8  itemArrSize = 0;
	MI_S32 i = 0;
	MI_S8  ret = CREAT_OK;
	if(NULL == MixerConfigPath)
	{
		MIXER_ERR("wrong param\n");
		return INVALID_FILEPATH;
	}
	memset(&VifInfo_t, 0, sizeof(VifInfo_t));
	memset(&SensorInfo_t,0, sizeof(SensorInfo_t));
	memset(&VpeBaseInfo_t, 0, sizeof(VpeBaseInfo_t));
	memset(&LdcInfo_t, 0, sizeof(LdcInfo_t));
	memset(&VpeTopInfo_t, 0x0, sizeof(VpeTopInfo_t));
	memset(&VencInfo_t, 0, sizeof(VencInfo_t));
	memset(&RgnInfo_t,0, sizeof(RgnInfo_t));
	memset(&AiDevInfo_t, 0, sizeof(AiDevInfo_t));
	memset(&AiChnInfo_t, 0, sizeof(AiChnInfo_t));
	memset(&AoDevInfo_t, 0, sizeof(AoDevInfo_t));
	
	filestr = read_json_file_delete_comment((char *)"/customer/sysConfig.json");
	if (NULL == filestr)
	{
	   filestr = read_json_file_delete_comment((char *)MixerConfigPath);
	   if (NULL == filestr)
	   {
		 printf("\n ERROR read file ! \n" );
		 ret = CreateDefaultConfig((const signed char *)MixerConfigPath);
		 filestr = read_json_file_delete_comment((char *)MixerConfigPath);
		 if (NULL == filestr && CREAT_OK != ret)
		 {
			return NOT_FOUND_FILE;
		 }
	   }
	}
	if ((Item = cJSON_Parse(filestr)) == NULL)
	{
		printf("\n ERROR file string to JSON !\n" );
		free(filestr);
		return ERR_JSON_FILE;
	}
	free(filestr);

	//sensor cfg	
	fatherJson = cJSON_GetObjectItem(Item, ModuleCfgName[DevSensor]);
	if(!fatherJson)
	{
		MIXER_ERR(" ERR_JSON_ITEM\n!");
		return ERR_JSON_ITEM;
	}
	SensorInfo_t.sensor_name =(MI_S8 *)mcJSON_getSTRING(fatherJson, NULL, "name",NULL);
	SensorInfo_t.workResIndex = (MI_U32)mcJSON_getINT(fatherJson,-1, "workResIndex", NULL);
	
	MIXER_DBG("sensor_name=%s, workPad=%d\n",SensorInfo_t.sensor_name, SensorInfo_t.workResIndex);
	
	itemArrSize =  (MI_U8)cJSON_GetArraySize(mcJSON_getJSON(fatherJson,"sensorResIndex",NULL));	
	for(i = 0; i < SENSOR_MAX_RES_INDEX && i < itemArrSize; i++)
	{
		childJson = cJSON_GetArrayItem(mcJSON_getJSON(fatherJson,"sensorResIndex",NULL),i);
		if(!childJson)
		{
			MIXER_ERR(" ERR_JSON_ITEM\n!");
			return ERR_JSON_ITEM;
		}
		SensorInfo_t.ResIndex[i].maxFps = (MI_U8)cJSON_GetObjectItem(childJson,"maxFps")->valueint;
		SensorInfo_t.ResIndex[i].minFps = (MI_U8)cJSON_GetObjectItem(childJson,"minFps")->valueint;
		SensorInfo_t.ResIndex[i].cropX = (MI_U16)cJSON_GetObjectItem(childJson,"cropX")->valueint;
		SensorInfo_t.ResIndex[i].cropY = (MI_U16)cJSON_GetObjectItem(childJson,"cropY")->valueint;
		SensorInfo_t.ResIndex[i].sensor_w = (MI_U16)cJSON_GetObjectItem(childJson,"sensor_w")->valueint;
		SensorInfo_t.ResIndex[i].sensor_h = (MI_U16)cJSON_GetObjectItem(childJson,"sensor_h")->valueint;
		
		MIXER_DBG("=[%d]=maxFps=%d,minfps=%d, cropX(%d), cropY(%d), sensor_h=%d sensor_w=%d\n",i,\
						SensorInfo_t.ResIndex[i].maxFps,\
						SensorInfo_t.ResIndex[i].minFps,\
						SensorInfo_t.ResIndex[i].cropX,\
						SensorInfo_t.ResIndex[i].cropY,\
						SensorInfo_t.ResIndex[i].sensor_h,\
						SensorInfo_t.ResIndex[i].sensor_w);
	}

	MIXER_DBG("get vif info.\n");
	fatherJson = cJSON_GetObjectItem(Item, ModuleCfgName[DevVif]);
	if(!fatherJson)
	{
		MIXER_ERR(" ERR_JSON_ITEM\n!");
		return ERR_JSON_ITEM;
	}
	VifInfo_t.MaxFps = (MI_U8)mcJSON_getJSON(fatherJson,"MaxFps",NULL)->valueint;
	VifInfo_t.MinFps = (MI_U8)mcJSON_getJSON(fatherJson,"MinFps",NULL)->valueint;
	VifInfo_t.MaxWidth = (MI_U16)mcJSON_getJSON(fatherJson,"MaxWidth",NULL)->valueint;
	VifInfo_t.MaxHeight = (MI_U16)mcJSON_getJSON(fatherJson,"MaxHeight",NULL)->valueint;
	VifInfo_t.CurWidth = (MI_U16)mcJSON_getJSON(fatherJson,"CurWidth",NULL)->valueint;
	VifInfo_t.CurHeight = (MI_U16)mcJSON_getJSON(fatherJson,"CurHeight",NULL)->valueint;
	VifInfo_t.IsHDR = (MI_U8)mcJSON_getJSON(fatherJson,"IsHDR",NULL)->valueint;
	VifInfo_t.VifChn = (MI_U8)mcJSON_getJSON(fatherJson,"VifChn",NULL)->valueint;
		
	MIXER_DBG("get vpe top info\n");
	fatherJson = cJSON_GetObjectItem(Item, ModuleCfgName[DevVpeTop]);
	if(!fatherJson)
	{
		MIXER_ERR(" ERR_JSON_ITEM\n!");
		return ERR_JSON_ITEM;
	}
	VpeTopInfo_t.MaxFps = (MI_U8)mcJSON_getJSON(fatherJson,"MaxFps",NULL)->valueint;
	VpeTopInfo_t.MinFps = (MI_U8)mcJSON_getJSON(fatherJson,"MinFps",NULL)->valueint;
	VpeTopInfo_t.MaxWidth = (MI_U16)mcJSON_getJSON(fatherJson,"MaxWidth",NULL)->valueint;
	VpeTopInfo_t.MaxHeight = (MI_U16)mcJSON_getJSON(fatherJson,"MaxHeight",NULL)->valueint;
	VpeTopInfo_t.DevId = (MI_U8)mcJSON_getJSON(fatherJson,"DevId",NULL)->valueint;
	VpeTopInfo_t.PortId = (MI_U8)mcJSON_getJSON(fatherJson,"PortId",NULL)->valueint;
	VpeTopInfo_t.ChnId = (MI_U8)mcJSON_getJSON(fatherJson,"ChnId",NULL)->valueint;
	VpeTopInfo_t._3DnrLevel = (MI_U16)mcJSON_getJSON(fatherJson,"3DnrLevel",NULL)->valueint;
	VpeTopInfo_t.Rotation = (MI_U8)mcJSON_getJSON(fatherJson,"Rotation",NULL)->valueint;

	MIXER_DBG("get vpe base info\n");
	memset(tmpItem, 0x0, sizeof(tmpItem));
	memcpy(tmpItem, ModuleCfgName[DevVpeBase], strlen(ModuleCfgName[DevVpeBase]));
	fatherJson = cJSON_GetObjectItem(Item, tmpItem);
	if(!fatherJson)
	{
		MIXER_ERR(" ERR_JSON_ITEM\n!");
		return ERR_JSON_ITEM;
	}
	VpeBaseInfo_t.Channel = (MI_U8)mcJSON_getINT(fatherJson,-1, "channel",NULL);	 
	VpeBaseInfo_t.roatValue = (MI_U16)mcJSON_getINT(fatherJson,-1, "Rota",NULL);
	VpeBaseInfo_t._3dnrLevel = (MI_U8)mcJSON_getINT(fatherJson,-1, "3dnrLevel",NULL);
	VpeBaseInfo_t.Vpe_OutPortNum = itemArrSize =  (MI_U8)cJSON_GetArraySize(mcJSON_getJSON(fatherJson,"vpebase_attr",NULL));	
	for(i = 0 ; i < itemArrSize; i++)
	{
		childJson = cJSON_GetArrayItem(mcJSON_getJSON(fatherJson, "vpebase_attr", NULL), i);
		if(!childJson)
		{
			MIXER_ERR(" ERR_JSON_ITEM\n!");
			return ERR_JSON_ITEM;
		}
	
		VpeBaseInfo_t.Outdepth[i] = (MI_U8)cJSON_GetObjectItem(childJson,"outDepth")->valueint;	   
		VpeBaseInfo_t.in_modle_height[i] = (MI_U16)cJSON_GetObjectItem(childJson,"maxHeight")->valueint;
		VpeBaseInfo_t.in_modle_width[i] =(MI_U16)cJSON_GetObjectItem(childJson,"maxWidth")->valueint;	   
		VpeBaseInfo_t.CurHeight[i] = (MI_U16)cJSON_GetObjectItem(childJson,"curHeight")->valueint;
		VpeBaseInfo_t.CurWidth[i] = (MI_U16)cJSON_GetObjectItem(childJson,"curWidth")->valueint;	   
		VpeBaseInfo_t.fps[i] = (MI_U8)cJSON_GetObjectItem(childJson,"fps")->valueint;
		VpeBaseInfo_t.minFps[i] = (MI_U8)cJSON_GetObjectItem(childJson,"minFps")->valueint;	   
	}
	
	MIXER_DBG("get ldc info\n");
	fatherJson = cJSON_GetObjectItem(Item, ModuleCfgName[DevLdc]);
	if(!fatherJson)
	{
		MIXER_ERR(" ERR_JSON_ITEM\n!");
		return ERR_JSON_ITEM;
	}
	LdcInfo_t.maxFps = (MI_U8)mcJSON_getJSON(fatherJson,"MaxFps",NULL)->valueint;
	LdcInfo_t.minFps = (MI_U8)mcJSON_getJSON(fatherJson,"MinFps",NULL)->valueint;
	LdcInfo_t.maxWidth = (MI_U16)mcJSON_getJSON(fatherJson,"maxWidth",NULL)->valueint;
	LdcInfo_t.maxHeight = (MI_U16)mcJSON_getJSON(fatherJson,"maxHeight",NULL)->valueint;
	LdcInfo_t.devId = (MI_U8)mcJSON_getJSON(fatherJson,"devId",NULL)->valueint;
	LdcInfo_t.portId = (MI_U8)mcJSON_getJSON(fatherJson,"portId",NULL)->valueint;
	LdcInfo_t.chnId = (MI_U8)mcJSON_getJSON(fatherJson,"chnId",NULL)->valueint;
	LdcInfo_t.rotation = (MI_U8)mcJSON_getJSON(fatherJson,"rotation",NULL)->valueint;

	MIXER_DBG("get venc info\n");
	fatherJson = cJSON_GetObjectItem(Item,ModuleCfgName[DevVenc]);
	if(!fatherJson)
	{
		MIXER_ERR(" ERR_JSON_ITEM\n!");
		return ERR_JSON_ITEM;
	}

	itemArrSize =  (MI_U8)cJSON_GetArraySize(mcJSON_getJSON(fatherJson,"venc_attr",NULL));	
	for(i = 0 ; i < itemArrSize; i++)
	{
		childJson = cJSON_GetArrayItem(mcJSON_getJSON(fatherJson,"venc_attr",NULL), i);
		if(!childJson)
		{
			MIXER_ERR(" ERR_JSON_ITEM\n!");
			return ERR_JSON_ITEM;
		}
		VencInfo_t.StreamHeight[i] = (MI_U16)cJSON_GetObjectItem(childJson, "StreamHeight")->valueint;
		VencInfo_t.StreamWidth[i] = (MI_U16)cJSON_GetObjectItem(childJson, "StreamWidth")->valueint;
		
		VencInfo_t.StreamMaxHeight[i] = (MI_U16)cJSON_GetObjectItem(childJson, "StreamMaxHeight")->valueint;
		VencInfo_t.StreamMaxWidth[i] = (MI_U16)cJSON_GetObjectItem(childJson, "StreamMaxWidth")->valueint;
	
		VencInfo_t.Minfps[i] = (MI_U8)cJSON_GetObjectItem(childJson, "Minfps")->valueint;
		VencInfo_t.Fps[i] = (MI_U8)cJSON_GetObjectItem(childJson, "Fps")->valueint;

		VencInfo_t.EncodeType[i] = (MI_U8)cJSON_GetObjectItem(childJson, "EncodeType")->valueint; 
	 	VencInfo_t.MmaModuleId[i] = (MI_U8)cJSON_GetObjectItem(childJson, "MmaModuleId")->valueint; 
		VencInfo_t.BindModuleType[i] = (MI_U8)cJSON_GetObjectItem(childJson, "BindModuleType")->valueint; 
		
		VencInfo_t.FromPort[i] = (MI_U8)cJSON_GetObjectItem(childJson, "FromPort")->valueint;
	
		VencInfo_t.MaxBitrate[i] = (MI_U32)cJSON_GetObjectItem(childJson, "MaxBitrate")->valueint;
		VencInfo_t.Bitrate[i] = (MI_U32)cJSON_GetObjectItem(childJson, "Bitrate")->valueint;
		VencInfo_t.MinBitrate[i] = (MI_U32)cJSON_GetObjectItem(childJson, "MinBitrate")->valueint;

	/*	  printf("=itemArrSize=%d=====VencInfo_t.MmaModuleId[%d]=%d===\n",itemArrSize,i,VencInfo_t.MmaModuleId[i]);
		  printf("==VencInfo_t.streamWidth[%d]=%d==\n",i,VencInfo_t.StreamWidth[i]);
		  printf("==VencInfo_t.streamHeight[%d]=%d==\n",i,VencInfo_t.StreamHeight[i]);
		  printf("==VencInfo_t.Minfps[%d]=%d.\n",i,VencInfo_t.Minfps[i]);*/
		 
	}

	MIXER_DBG("get rgn info\n");
		fatherJson = cJSON_GetObjectItem(Item, ModuleCfgName[DevRgn]);
		if(!fatherJson)
		{
			MIXER_ERR(" ERR_JSON_ITEM\n!");
			return ERR_JSON_ITEM;
		}
		
		RgnInfo_t.IsPraM = (MI_U8)mcJSON_getJSON(fatherJson,"IsPraM", NULL)->valueint;; 
		
		MIXER_DBG("get ai info\n");
		memset(tmpItem, 0x0, sizeof(tmpItem));
		memcpy(tmpItem, ModuleCfgName[DevAi], strlen(ModuleCfgName[DevAi]));
		fatherJson = cJSON_GetObjectItem(Item, tmpItem);
		if(!fatherJson)
		{
			MIXER_ERR(" ERR_JSON_ITEM\n!");
			return ERR_JSON_ITEM;
		}
		AiDevInfo_t.Chnt = (MI_U8)mcJSON_getINT(fatherJson,-1, "Chnt",NULL);
		AiDevInfo_t.AiMediaType[0] = (MI_U8)mcJSON_getINT(fatherJson,-1, "MdiaType",NULL); 
		AiDevInfo_t.Samplerate = (MI_U32)mcJSON_getINT(fatherJson,-1, "samplerate",NULL); 
		AiDevInfo_t.BitWidthByte = (MI_U32)mcJSON_getINT(fatherJson,-1, "bitWidthByte",NULL); 
		AiDevInfo_t.VolumeInDb = (MI_U8)mcJSON_getINT(fatherJson,-1, "VolumeInDb",NULL); 

		MIXER_DBG("get ao info\n");
		memset(tmpItem, 0x0, sizeof(tmpItem));
		memcpy(tmpItem, ModuleCfgName[DevAo], strlen(ModuleCfgName[DevAo]));
		fatherJson = cJSON_GetObjectItem(Item, tmpItem);
		if(!fatherJson)
		{
			MIXER_ERR(" ERR_JSON_ITEM\n!");
			return ERR_JSON_ITEM;
		}
		AoDevInfo_t.AoMediaType[0] = (MI_U8)mcJSON_getINT(fatherJson,-1, "MdiaType",NULL); 
		AoDevInfo_t.Samplerate = (MI_U32)mcJSON_getINT(fatherJson,-1, "samplerate",NULL); 
		AoDevInfo_t.BitWidthByte = (MI_U32)mcJSON_getINT(fatherJson,-1, "bitWidthByte",NULL);  
		AoDevInfo_t.VolumeOutDb = (MI_U8)mcJSON_getINT(fatherJson,-1, "VolumeOutDb",NULL); 

		MIXER_DBG("get aichn info\n");
		memset(tmpItem, 0x0, sizeof(tmpItem));
		memcpy(tmpItem, ModuleCfgName[DevAiChn], strlen(ModuleCfgName[DevAiChn]));
		fatherJson = cJSON_GetObjectItem(Item, tmpItem);
		if(!fatherJson)
		{
			MIXER_ERR(" ERR_JSON_ITEM\n!");
			return ERR_JSON_ITEM;
		}
		AiChnInfo_t.BitWidthByte = (MI_U32)mcJSON_getINT(fatherJson,-1, "bitWidthByte",NULL);
		AiChnInfo_t.pktnum   = (MI_U32)mcJSON_getINT(fatherJson,-1, "pktNum",NULL);

		memset(tmpItem, 0x0, sizeof(tmpItem));
		memcpy(tmpItem, ModuleCfgName[DevRec], strlen(ModuleCfgName[DevRec]));
		fatherJson = cJSON_GetObjectItem(Item, tmpItem);
		if(!fatherJson)
		{
			MIXER_ERR(" ERR_JSON_ITEM\n!");
			return ERR_JSON_ITEM;
		}
		char* rTmp = (char *  )mcJSON_getSTRING(fatherJson, NULL, "RecPath", NULL);
		memcpy(RecInfo_t.RecPath, rTmp, strlen(rTmp));
		RecInfo_t.Storage =(MI_U8)mcJSON_getINT(fatherJson,-1, "Storage",NULL);
		MIXER_DBG("rec path is %s\n", RecInfo_t.RecPath);

		
		if(Item)
		{
			cJSON_Delete(Item);
		}
		return 0;
}

ModuleAoInfo_s * GetAoDevInfo()
{
	return &AoDevInfo_t;	
}

ModuleAiChnInfo_s * GetAiCHNInfo()
{
	return &AiChnInfo_t;
}

ModuleAiInfo_s *GetAiDevInfo()
{
	return &AiDevInfo_t;
}

ModuleRgnInfo_s *GetRgnInfo()
{
	return &RgnInfo_t;
}

ModuleVencInfo_s *GetVencInfo()
{
	return &VencInfo_t;
}

ModuleVpeInfo_s  *GetVpeBaseInfo()
{
	return &VpeBaseInfo_t;
}

ModuleVpeTopInfo_s *GetVpeTopInfo()
{	
	return &VpeTopInfo_t;
}

ModuleLdcInfo_s  *GetLdcInfo()
{	
	return &LdcInfo_t;
}

ModuleVifInfo_s  *GetVifInfo()
{
	return &VifInfo_t;
}

ModuleSensorInfo_s  *GetSensorInfo()
{	
	return &SensorInfo_t;
}

ModuleRecInfo_s *GetRecInfo()
{
	return &RecInfo_t;
}

