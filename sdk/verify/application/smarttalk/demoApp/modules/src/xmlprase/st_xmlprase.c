#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>

#include "st_common.h"
#include "st_xmlprase.h"
#include "app_config.h"

#define MAIN_ID_STR_LEN 16
#define SUB_ID_STR_LEN 16
#define DEVICE_TYPE_LEN 4

static xmlNodePtr ST_XmlCreateNode(const SMART_BD_Machine_CFG_T *CfgNode)
{
    assert(CfgNode);

    MI_U8 MainID[MAIN_ID_STR_LEN] = {0};
    MI_U8 SubID[MAIN_ID_STR_LEN] = {0};
    MI_U8 DeviceType[MAIN_ID_STR_LEN] = {0};

    xmlNodePtr xml_CfgNode = NULL;

    xml_CfgNode = xmlNewNode(NULL, BAD_CAST"DeviceDescription");
    if (NULL == xml_CfgNode) {
        fprintf(stderr, "Failed to create new node.\n");
        return NULL;
    }

    snprintf(MainID, MAIN_ID_STR_LEN, "%d", CfgNode->s32MainID);
    xmlNewProp(xml_CfgNode, BAD_CAST"MainID", (xmlChar*)MainID);
    snprintf(SubID, SUB_ID_STR_LEN, "%d", CfgNode->s32SubID);
    xmlNewProp(xml_CfgNode, BAD_CAST"SubID", (xmlChar*)SubID);
    snprintf(DeviceType, DEVICE_TYPE_LEN, "%d", CfgNode->s32DeviceType);
    xmlNewProp(xml_CfgNode, BAD_CAST"DeviceType", (xmlChar*)DeviceType);

    xmlNewChild(xml_CfgNode, NULL, BAD_CAST"LocalID", (xmlChar *)CfgNode->u8LocalID);
    xmlNewChild(xml_CfgNode, NULL, BAD_CAST"IPaddr", (xmlChar *)CfgNode->u8IPaddr);
    xmlNewChild(xml_CfgNode, NULL, BAD_CAST"Telphone", (xmlChar *)CfgNode->u8Telphone);

    return xml_CfgNode;
}

static MI_S32 ST_XmlAddNodeToRoot(xmlNodePtr RootNode, const SMART_BD_Machine_CFG_T *CfgNode)
{
    xmlNodePtr xmlCfgNode = NULL;

    xmlCfgNode = ST_XmlCreateNode(CfgNode);
    if (NULL == xmlCfgNode)
    {
        fprintf(stderr, "Failed to create cfg node.\n");
        goto FAILED;
    }
    xmlAddChild(RootNode, xmlCfgNode);
    free(xmlCfgNode);

    return 0;
FAILED:
    return -1;
}

static MI_S32 ST_XmlCreateCfgFile(const MI_U8 *u8CfgFile, const SMART_BD_Machine_CFG_T *CfgNode)
{
    assert(u8CfgFile);

    xmlDocPtr docPtr = NULL;
    xmlNodePtr RootNode = NULL;
    SMART_BD_Machine_CFG_T stDefaultCfg;

    docPtr = xmlNewDoc(BAD_CAST"1.0");
    if (docPtr == NULL) {
        fprintf(stderr, "Failed to new doc.\n");
        return -1;
    }

    RootNode = xmlNewNode(NULL, BAD_CAST"SmartBD_CFG");
    if (NULL == RootNode) {
        fprintf(stderr, "Failed to new root node.\n");
        goto FAILED;
    }
    xmlDocSetRootElement(docPtr, RootNode);

    if (NULL == CfgNode)
    {
        stDefaultCfg.s32MainID = 1;
        stDefaultCfg.s32SubID = 0;
        stDefaultCfg.s32DeviceType = E_ST_DEV_ROOM;
        memset(&stDefaultCfg.u8LocalID, 0x0, CALL_ID_MAX_LEN);
        memcpy(&stDefaultCfg.u8LocalID, "01010101", strlen("01010101"));
        memset(&stDefaultCfg.u8IPaddr, 0x0, IPADDR_MAX_LEN);
        memcpy(&stDefaultCfg.u8IPaddr, "172.19.24.171", strlen("172.19.24.171"));
        memset(&stDefaultCfg.u8Telphone, 0x0, TELPHONE_LEN);
        memcpy(&stDefaultCfg.u8Telphone, "13912345678", strlen("13912345678"));
        if (ST_XmlAddNodeToRoot(RootNode, &stDefaultCfg) != 0) {
            fprintf(stderr, "Failed to add a config node.\n");
            goto FAILED;
        }
    }
    else
    {
        if (ST_XmlAddNodeToRoot(RootNode, CfgNode) != 0) {
            fprintf(stderr, "Failed to add a config node.\n");
            goto FAILED;
        }
    }
    xmlSaveFormatFileEnc(u8CfgFile, docPtr, "UTF-8", 1);
    xmlFreeDoc(docPtr);

    return 0;
    FAILED:
    if (docPtr) {
        xmlFreeDoc(docPtr);
    }

    return -1;
}

static MI_S32 ST_XmlAddNode(const MI_U8 *u8CfgFile, SMART_BD_Machine_CFG_T *pstDeviceCfg)
{
    assert(u8CfgFile);

    xmlDocPtr docPtr = NULL;
    xmlNodePtr RootNode = NULL;

    docPtr = xmlParseFile(u8CfgFile);
    if (NULL == docPtr) {
        fprintf(stderr, "Failed to parser xml file:%s\n", u8CfgFile);
        return -1;
    }

    RootNode = xmlDocGetRootElement(docPtr);
    if (RootNode == NULL) {
        fprintf(stderr, "Failed to get root node.\n");
        goto FAILED;
    }

    if (ST_XmlAddNodeToRoot(RootNode, pstDeviceCfg) != 0) {
        fprintf(stderr, "Failed to add a new phone node.\n");
        goto FAILED;
    }
    xmlSaveFormatFileEnc(u8CfgFile, docPtr, "UTF-8", 1);
    xmlFreeDoc(docPtr);

    return 0;
FAILED:
    if (docPtr) {
        xmlFreeDoc(docPtr);
    }

    return -1;
}

MI_S32 ST_XmlAddSmartBDCfg(SMART_BD_Machine_CFG_T *pstDeviceCfg)
{
    if (access(DEVICE_CFG_FILE, F_OK) == 0) {
        return ST_XmlAddNode(DEVICE_CFG_FILE, pstDeviceCfg);
    }
    else {
        if (NULL != pstDeviceCfg)
        {
            return ST_XmlCreateCfgFile(DEVICE_CFG_FILE, pstDeviceCfg);
        }
        else
        {
            return ST_XmlCreateCfgFile(DEVICE_CFG_FILE, NULL);
        }
    }
}

static int ST_XmlPraseDeviceCfg(xmlDocPtr doc, xmlNodePtr cur, MI_U8 *pu8CallID, MI_U8 *puIPaddr)
{
    assert(doc || cur);
    xmlChar *LocalIDkey = NULL;
    xmlChar *IPaddrIDkey = NULL;
    xmlChar *TelPhoneIDkey = NULL;
    MI_S32 s32FindCallID = 0;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"LocalID")))
        {
            LocalIDkey = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            printf("LocalID:pu8CallID %s %s\n", pu8CallID, LocalIDkey);
            if (0 == strncmp(pu8CallID, LocalIDkey, 8))
            {
                s32FindCallID = 1;
            }
            if (LocalIDkey)
            {
                xmlFree(LocalIDkey);
                LocalIDkey = NULL;
            }
        }
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"IPaddr")))
        {
            IPaddrIDkey = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if (s32FindCallID)
            {
                memcpy(puIPaddr, IPaddrIDkey, strlen(IPaddrIDkey));
            }
            printf("IPaddr: %s\n", IPaddrIDkey);
            if (IPaddrIDkey)
            {
                xmlFree(IPaddrIDkey);
                IPaddrIDkey = NULL;
            }
        }
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"Telphone")))
        {
            TelPhoneIDkey = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            printf("Telphone: %s\n", TelPhoneIDkey);
            if (TelPhoneIDkey)
            {
                xmlFree(TelPhoneIDkey);
                TelPhoneIDkey = NULL;
            }
        }
        cur = cur->next;
    }
    return s32FindCallID;
}

MI_S32 ST_XmlPraseUiCfg(const MI_U8 *pu8CfgFile, const MI_U8 *LayoutName, ST_Rect_T  stUiItemArea[])
{
    assert(pu8CfgFile);

    xmlDocPtr doc;
    xmlNodePtr cur;
    xmlChar *ItemIndex = NULL;
    xmlChar *Xpos = NULL;
    xmlChar *Ypos = NULL;
    xmlChar *Width = NULL;
    xmlChar *Height = NULL;

    MI_S32 s32PraseOk = -1;
    doc = xmlParseFile(pu8CfgFile);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse xml file:%s\n", pu8CfgFile);
        goto FAILED;
    }

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr, "Root is empty.\n");
        goto FAILED;
    }

    if ((xmlStrcmp(cur->name, (const xmlChar *)"UiSurface_LAYOUT"))) {
        fprintf(stderr, "The root is not UiSurface_LAYOUT.\n");
        goto FAILED;
    }
    cur = cur->xmlChildrenNode;
    while (cur != NULL)
    {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)LayoutName)))
        {
            cur = cur->xmlChildrenNode;
            while (cur != NULL)
            {
                if ((!xmlStrcmp(cur->name, (const xmlChar *)"ItemArea")))
                {
                    ItemIndex = xmlGetProp(cur, "ItemIndex");
                    Xpos = xmlGetProp(cur, "X");
                    Ypos = xmlGetProp(cur, "Y");
                    Width = xmlGetProp(cur, "W");
                    Height = xmlGetProp(cur, "H");
                    stUiItemArea[atoi(ItemIndex)].s32X = atoi(Xpos);
                    stUiItemArea[atoi(ItemIndex)].s32Y = atoi(Ypos);
                    stUiItemArea[atoi(ItemIndex)].s16PicW = atoi(Width);
                    stUiItemArea[atoi(ItemIndex)].s16PicH = atoi(Height);
                    //printf("itemindex(%d) x-y-w-h = (%d-%d-%d-%d)...\n", atoi(ItemIndex),
                    //    atoi(Xpos), atoi(Ypos), atoi(Width), atoi(Height));
                    s32PraseOk = 0;
                }
                cur = cur->next;
            }
            break;
        }
        cur = cur->next;
    }

    if (Xpos)
    {
        xmlFree(Xpos);
    }
    if (Ypos)
    {
        xmlFree(Ypos);
    }
    if (Width)
    {
        xmlFree(Width);
    }
    if (Height)
    {
        xmlFree(Height);
    }
    xmlFreeDoc(doc);
    return s32PraseOk;
FAILED:
    printf("Prase xml fail\n");
    if (doc) {
        xmlFreeDoc(doc);
    }
    return -1;
}

MI_S32 ST_XmlPraseBDCfg(const MI_U8 *pu8CfgFile, MI_U8 *pu8CallID, MI_U8 *puIPaddr)
{
    assert(pu8CfgFile);

    xmlDocPtr doc;
    xmlNodePtr cur;
    xmlChar *MainID, *SubID, *DeviceType;
    MI_S32 s32MatchId = 0;
    doc = xmlParseFile(pu8CfgFile);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse xml file:%s\n", pu8CfgFile);
        goto FAILED;
    }

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr, "Root is empty.\n");
        goto FAILED;
    }

    if ((xmlStrcmp(cur->name, (const xmlChar *)"SmartBD_CFG"))) {
        fprintf(stderr, "The root is not phone_books.\n");
        goto FAILED;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"DeviceDescription"))) {
            MainID = xmlGetProp(cur, "MainID");
            SubID = xmlGetProp(cur, "SubID");
            DeviceType = xmlGetProp(cur, "DeviceType");
            if (1 == ST_XmlPraseDeviceCfg(doc, cur, pu8CallID, puIPaddr))
            {
                s32MatchId = 1;
            }
        }
        cur = cur->next;
    }
    xmlFreeDoc(doc);
    return s32MatchId;
FAILED:
    if (doc) {
        xmlFreeDoc(doc);
    }
    return -1;
}

static int ST_XmlPraseDeviceID(xmlDocPtr doc, xmlNodePtr cur, MI_U8 *pu8DeviceID)
{
    assert(doc || cur);
    xmlChar *DeviceIDkey = NULL;
    MI_S32 s32FindDevID = 0;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"DeviceID")))
        {
            DeviceIDkey = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);

            memcpy(pu8DeviceID, DeviceIDkey, 8);
            printf("DeviceID:%s pu8DeviceID %s\n", pu8DeviceID, DeviceIDkey);
            if (DeviceIDkey)
            {
                xmlFree(DeviceIDkey);
                DeviceIDkey = NULL;
            }
        }
        cur = cur->next;
    }
    return 0;
}

MI_S32 ST_XmlPraseBD_MyInfo(const MI_U8 *pu8CfgFile, MI_U8 *pu8DeviceID)
{
    assert(pu8CfgFile);

    xmlDocPtr doc;
    xmlNodePtr cur;

    doc = xmlParseFile(pu8CfgFile);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse xml file:%s\n", pu8CfgFile);
        goto FAILED;
    }

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr, "Root is empty.\n");
        goto FAILED;
    }

    if ((xmlStrcmp(cur->name, (const xmlChar *)"SmartBD_CFG"))) {
        fprintf(stderr, "The root is not smart BD config.\n");
        goto FAILED;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"MachineInfo"))) {
            ST_XmlPraseDeviceID(doc, cur, pu8DeviceID);
        }
        cur = cur->next;
    }
    xmlFreeDoc(doc);
    return 0;
FAILED:
    if (doc) {
        xmlFreeDoc(doc);
    }
    return -1;
}


MI_S32 ST_XmlParseDevVolumeCfg(const MI_U8 *pu8DevVolume, MI_S32 *ps32VolValue, MI_BOOL *pbMute)
{
    xmlDocPtr doc;
    xmlNodePtr cur;
    xmlChar *pVolValue = NULL;
    xmlChar *pMute = NULL;

    doc = xmlParseFile(DEV_INFO_CFG_FILE);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse xml file:%s\n", DEV_INFO_CFG_FILE);
        printf("Failed to parse xml file:%s\n", DEV_INFO_CFG_FILE);
        goto FAILED;
    }

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr, "Root is empty.\n");
        goto FAILED;
    }

    if ((xmlStrcmp(cur->name, (const xmlChar *)"SmartDevInfo_CFG"))) {
        fprintf(stderr, "The root is not smart Devinfo config.\n");
        printf("The root is not smart Devinfo config.\n");
        goto FAILED;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        //printf("cur node name: %s\n", cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar *)pu8DevVolume)))
        {
            printf("find node %s\n", pu8DevVolume);
            pVolValue = xmlGetProp(cur, (const xmlChar *)"VolumeValue");
            pMute = xmlGetProp(cur, (const xmlChar *)"Mute");
            *ps32VolValue = atoi(pVolValue);
            *pbMute = (MI_BOOL)atoi(pMute);
            break;
        }

        cur = cur->next;
    }
    xmlFreeDoc(doc);
    return 0;
FAILED:
    if (doc) {
        xmlFreeDoc(doc);
    }
    return -1;
}


MI_S32 ST_XmlUpdateDevVolumeCfg(const MI_U8 *pu8DevVolume, MI_S32 s32VolValue, MI_BOOL bMute)
{
    xmlDocPtr doc;
    xmlNodePtr cur;
    char volValue[4];
    char mute[2];

    memset(volValue, 0, sizeof(volValue));
    memset(mute, 0, sizeof(mute));
    sprintf(volValue, "%d", s32VolValue);
    sprintf(mute, "%d", bMute);

    doc = xmlParseFile(DEV_INFO_CFG_FILE);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse xml file:%s\n", DEV_INFO_CFG_FILE);
        goto FAILED;
    }

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr, "Root is empty.\n");
        goto FAILED;
    }

    if ((xmlStrcmp(cur->name, (const xmlChar *)"SmartDevInfo_CFG"))) {
        fprintf(stderr, "The root is not smart Devinfo config.\n");
        goto FAILED;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        //printf("cur node name: %s\n", cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar *)pu8DevVolume)))
        {
            printf("find node %s\n", pu8DevVolume);
            xmlSetProp(cur, (const xmlChar *)"VolumeValue", (const xmlChar *)volValue);
            xmlSetProp(cur, (const xmlChar *)"Mute", (const xmlChar *)mute);

            printf("save value:%d, %s\n", s32VolValue, bMute?"Mute On":"Mute Off");
            break;
        }

        cur = cur->next;
    }
    xmlSaveFile(DEV_INFO_CFG_FILE, doc);
    xmlFreeDoc(doc);
    return 0;
FAILED:
    if (doc) {
        xmlFreeDoc(doc);
    }
    return -1;
}


MI_S32 ST_XmlParsePlayFileCfg(MI_U8 *pu8Path, MI_S32 s32PathLen)
{
    xmlDocPtr doc;
    xmlNodePtr cur;
    xmlChar *pPath = NULL;

    doc = xmlParseFile(DEV_INFO_CFG_FILE);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse xml file:%s\n", DEV_INFO_CFG_FILE);
        printf("Failed to parse xml file:%s\n", DEV_INFO_CFG_FILE);
        goto FAILED;
    }

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr, "Root is empty.\n");
        goto FAILED;
    }

    if ((xmlStrcmp(cur->name, (const xmlChar *)"SmartDevInfo_CFG"))) {
        fprintf(stderr, "The root is not smart Devinfo config.\n");
        printf("The root is not smart Devinfo config.\n");
        goto FAILED;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        //printf("cur node name: %s\n", cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"LastPlayFile")))
        {
            pPath = xmlGetProp(cur, (const xmlChar *)"Path");
            memset(pu8Path, 0, s32PathLen);
            memcpy(pu8Path, pPath, strlen((char*)pPath));
            break;
        }

        cur = cur->next;
    }
    xmlFreeDoc(doc);
    return 0;
FAILED:
    if (doc) {
        xmlFreeDoc(doc);
    }
    return -1;
}


MI_S32 ST_XmlUpdatePlayFileCfg(MI_U8 *pu8SavePath)
{
    xmlDocPtr doc;
    xmlNodePtr cur;

    doc = xmlParseFile(DEV_INFO_CFG_FILE);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse xml file:%s\n", DEV_INFO_CFG_FILE);
        goto FAILED;
    }

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr, "Root is empty.\n");
        goto FAILED;
    }

    if ((xmlStrcmp(cur->name, (const xmlChar *)"SmartDevInfo_CFG"))) {
        fprintf(stderr, "The root is not smart Devinfo config.\n");
        goto FAILED;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        //printf("cur node name: %s\n", cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar *)"LastPlayFile")))
        {
            xmlSetProp(cur, (const xmlChar *)"Path", (const xmlChar *)pu8SavePath);
            break;
        }

        cur = cur->next;
    }
    xmlSaveFile(DEV_INFO_CFG_FILE, doc);
    xmlFreeDoc(doc);
    return 0;
FAILED:
    if (doc) {
        xmlFreeDoc(doc);
    }
    return -1;
}


