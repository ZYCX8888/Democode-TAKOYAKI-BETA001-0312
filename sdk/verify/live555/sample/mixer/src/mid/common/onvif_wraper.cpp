#include "dlfcn.h"
#include "mid_common.h"
#include "OnvifWraper.h"


typedef struct _OnvifWraperContext
{
    BOOL loaded;
    void *onvifLibHandle;

    void (*gfp_MST_ONVIF_Init)();
    void (*gfp_MST_ONVIF_StartTask)();
    void (*gfp_MST_ONVIF_StopTask)();
} OnvifWraperContext;

static OnvifWraperContext onvifWraperContext = {(BOOL)0};



int _OnvifWraper_Init()
{
    do
    {
        onvifWraperContext.onvifLibHandle = dlopen("libonvif.so", RTLD_NOW);

        if(NULL == onvifWraperContext.onvifLibHandle)
        {
            MIXER_ERR(" %s: load libonvif.so error \n", __func__);
            printf("%s:dlopen - %s \n", __func__, dlerror());
            return FALSE;
        }

        onvifWraperContext.gfp_MST_ONVIF_Init = (void (*)())dlsym(onvifWraperContext.onvifLibHandle, "MST_ONVIF_Init");

        if(NULL == onvifWraperContext.gfp_MST_ONVIF_Init)
        {
            MIXER_ERR(" %s: dlsym MST_ONVIF_Init failed, %s\n", __func__, dlerror());
            break;
        }

        onvifWraperContext.gfp_MST_ONVIF_StartTask = (void (*)())dlsym(onvifWraperContext.onvifLibHandle, "MST_ONVIF_StartTask");

        if(NULL == onvifWraperContext.gfp_MST_ONVIF_StartTask)
        {
            MIXER_ERR(" %s: dlsym MST_ONVIF_StartTask failed, %s\n", __func__, dlerror());
            break;
        }

        onvifWraperContext.gfp_MST_ONVIF_StopTask = (void (*)())dlsym(onvifWraperContext.onvifLibHandle, "MST_ONVIF_StopTask");

        if(NULL == onvifWraperContext.gfp_MST_ONVIF_StopTask)
        {
            MIXER_ERR(" %s: dlsym gfp_MST_ONVIF_StopTask failed, %s\n", __func__, dlerror());
            break;
        }

        onvifWraperContext.loaded = TRUE;
        MIXER_INFO(" %s: success\n", __func__);
        return TRUE;

    }
    while(0);

    MIXER_ERR(" %s: failed\n", __func__);

    if(onvifWraperContext.onvifLibHandle)
    {
        dlclose(onvifWraperContext.onvifLibHandle);
        onvifWraperContext.onvifLibHandle = NULL;
    }

    memset(&onvifWraperContext, 0, sizeof(OnvifWraperContext));
    return FALSE;
}


static OnvifWraperContext* _OnvifWraper_GetContext()
{
    if(FALSE == onvifWraperContext.loaded)
    {
        if(FALSE == _OnvifWraper_Init())
        {
            return NULL;
        }
    }

    return &onvifWraperContext;
}

int MST_ONVIF_StartTask()
{
    OnvifWraperContext* ctxt = _OnvifWraper_GetContext();

    if(NULL == ctxt)
    {
        MIXER_ERR(" %s: get Onvif interface failed\n", __func__);
        return FALSE;
    }

    ctxt->gfp_MST_ONVIF_StartTask();
    MIXER_INFO(" %s: success\n", __func__);
    return TRUE;
}

int MST_ONVIF_StopTask()
{
    OnvifWraperContext* ctxt = _OnvifWraper_GetContext();

    if(NULL == ctxt)
    {
        MIXER_ERR(" %s: get Onvif interface failed\n", __func__);
        return FALSE;
    }

    ctxt->gfp_MST_ONVIF_StopTask();
    MIXER_INFO(" %s: success\n", __func__);
    return TRUE;
}


int MST_ONVIF_Init()
{
    OnvifWraperContext* ctxt = _OnvifWraper_GetContext();

    if(NULL == ctxt)
    {
        MIXER_ERR(" %s: get Onvif interface failed\n", __func__);
        return FALSE;
    }

    ctxt->gfp_MST_ONVIF_Init();
    MIXER_INFO(" %s: success\n", __func__);
    return TRUE;
}

int MST_ONVIF_DeInit()
{
    OnvifWraperContext* ctxt = _OnvifWraper_GetContext();

    if(NULL == ctxt)
    {
        MIXER_ERR(" %s: get Onvif interface failed\n", __func__);
        return FALSE;
    }

    dlclose(ctxt->onvifLibHandle);
    memset(ctxt, 0, sizeof(OnvifWraperContext));
    ctxt->onvifLibHandle = NULL;

    onvifWraperContext.loaded = (BOOL)0;

    MIXER_INFO(" %s: success\n", __func__);
    return TRUE;
}
