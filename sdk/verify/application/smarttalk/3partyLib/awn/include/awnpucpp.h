#ifndef __AWNPU_CPP_H__
#define __AWNPU_CPP_H__
#include "awnpu.h"
#include <vector>


class AWResult;

class API_EXPORT AWNPU
{
public:
	static AWNPU *Create(int action, int flag = 0);
	virtual ~AWNPU() = 0;

public:
	typedef void (*ResultCallback)(void *context, AWNPU *aw, AWResult *result);
	
public:
	virtual int Process(int action, int frame_id, void *data, int width, int height, int flag, ResultCallback callback = NULL, void *context = NULL) = 0;
	virtual int Process(int action, int frame_id, void *data, int width, int height, void *data2, int width2, int height2, int flag, ResultCallback callback = NULL, void *context = NULL) = 0;
	virtual AWResult *GetResult() = 0;
	virtual int FreezeFR(int tracking_id) = 0;

public:
	template<typename T>
	int SetParameter(int param_id, T param_value){return SetParameter(param_id, param_value);}

	template<typename T>
	T GetParameter(int param_id){return GetParameter((T)0, param_id);}

	virtual int SetParameter(int param_id, int param_value) = 0;
	virtual int SetParameter(int param_id, float param_value) = 0;
	virtual int SetParameter(int param_id, char *param_value) = 0;
	virtual int SetParameter(int param_id, bool param_value) = 0;
	virtual int GetParameter(int placeholder, int param_id) = 0;
	virtual float GetParameter(float placeholder, int param_id) = 0;
	virtual const char *GetParameter(char *placeholder, int param_id) = 0;
	virtual bool GetParameter(bool placeholder, int param_id) = 0;
};

class API_EXPORT FDResult
{
public:
	int code;
	std::vector<AW_RECT> box;
	std::vector<int> tracking_id;
};

class API_EXPORT FRResult
{
public:
	std::vector<bool> valid;
	std::vector<std::vector<unsigned char>> feature;
};

class API_EXPORT LandmarkResult
{
public:
	std::vector<bool> valid;
	std::vector<std::vector<float>> landmark;
};

class API_EXPORT HeadResult
{
public:
	typedef struct _RotateAngle
	{
		float x;	// rotate on x axis
		float y;	// rotate on y axis
		float z;	// rotate on z axis
	} RotateAngle;

public:
	std::vector<bool> valid;
	std::vector<int> state;
	std::vector<RotateAngle> angle;
};

class API_EXPORT LiveResult
{
public:
	std::vector<bool> valid;
	std::vector<bool> live;
};

class API_EXPORT PDResult
{
public:
	int code;
	std::vector<AW_RECT> box;
	std::vector<int> tracking_id;
};

class API_EXPORT GenderResult
{
public:
	std::vector<bool> valid;
	std::vector<bool> male;
};

class API_EXPORT AgeResult
{
public:
	std::vector<bool> valid;
	std::vector<int> age;
};

class API_EXPORT AWResult
{
public:
	int tag;
	int flag;
	int frame_id;
	FDResult *fd_result;
	FRResult *fr_result;
	LandmarkResult *landmark;
	HeadResult *head_result;
	LiveResult *live_result;
	PDResult *pd_result;
	GenderResult *gd_result;
	AgeResult *age_result;

public:
	AWResult();
	AWResult(AWResult &b);
	virtual ~AWResult();
	virtual AWResult& operator =(AWResult &b);
	virtual void Clear();
};


class API_EXPORT AWFeatManage
{
public:
	static AWFeatManage *Create(AWNPU *aw = NULL, int featureType = 0, int featureSize = 0, int flag = 0);
	static AWFeatManage *Create(const char *dir, int extraDataSize, int maxFeatureNumber, AWNPU *aw = NULL, int featureType = 0, int featureSize = 0, int flag = 0);
	virtual ~AWFeatManage() = 0;

public:
	virtual int Register(int id, void *feature, void *extraData, int extraLen, int flag) = 0;
	virtual int Deregister(int id) = 0;
	virtual int DeregisterAll() = 0;
	virtual float CalcSimilarity(void *feature1, void *feature2) = 0;
	virtual float CalcSimilarity(int featureType, int featureSize, void *feature1, void *feature2) = 0;
	virtual int FindSimilar(void *feature, float threshold, AW_FeatueResult *found, int foundSize, int *foundCount) = 0;
	virtual int GetFeatureInfoById(int id, AW_FeatueInfo *info) = 0;
	virtual int GetFeatureInfoByIndex(int index, AW_FeatueInfo *info) = 0;
	virtual int GetFeatureInfoCount() = 0;

public:
	virtual int Load(const char *filename) = 0;
	virtual int Save(const char *filename) = 0;
	virtual int Load(FILE *fp, int *readBytes) = 0;
	virtual int Save(FILE *fp, int *writeBytes) = 0;
};


#endif//__AWNPU_CPP_H__

