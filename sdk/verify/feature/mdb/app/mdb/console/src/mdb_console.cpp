/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include <string>
#include <vector>

#include "mdb_console.h"

#define ECHOFLAGS (ECHO | ECHOE | ECHOK | ECHONL)

typedef enum
{
    EN_CONSOLE_WELCOME,
    EN_CONSOLE_MAIN,
    EN_CONSOLE_SUB,
    EN_CONSOLE_NULL
}EN_CONSOLE_STATUS;
typedef enum
{
    EN_KEY_CHAR,
    EN_KEY_ENTER,
    EN_KEY_BACKSPACE,
    EN_KEY_UP,
    EN_KEY_DOWN,
    EN_KEY_LEFT,
    EN_KEY_RIGHT,
    EN_KEY_DEL,
    EN_KEY_TAB,
    EN_KEY_NOT_SUPPORT
}EN_KEY;
#define MOVELEFT(y) do{ \
   if ((y) != 0)    \
   {    \
       printf("\033[%dD", (y)); \
   }    \
}while (0);
#define MOVERIGHT(y) do{ \
   if ((y) != 0)    \
   {    \
       printf("\033[%dC", (y)); \
   }    \
}while (0);
#define CLEARLINE printf("\033[K")
#define PRINT_GAP(x, y) do{    \
    unsigned short int i = 0;   \
    for (; i < x; i++)  \
    {   \
        printf(y);    \
    }   \
}while(0);
struct shm_st
{
    char buf[TRANS_BUFFER];
    int bWrite;
};
static int gsocketFd = 0;
static std::vector<std::string> strCmdStrings;
static std::vector<std::string> strTabStrings;
static unsigned int intCmdIdx = 0;
static char pSubCmdName[20];
static struct termios gstOrgOpts;
static int init_console(const char *pInitStr)
{
    if (pInitStr == NULL)
    {
        return -1;
    }
    printf("[%s]:", pInitStr);
    fflush(stdin);

    return 0;
}
static int move_string_by_one_step(char *pCharFront, char *pCharBack, unsigned short bLeft, char cGap) //Move string and fill the gap;
{
    if (pCharFront == NULL || pCharBack == NULL)
    {
        return -1;
    }
    if (bLeft)
    {
        for (--pCharFront; pCharFront != pCharBack; pCharFront++)
        {
            *pCharFront ^= *(pCharFront + 1);
            *(pCharFront + 1) ^= *pCharFront;
            *pCharFront ^= *(pCharFront + 1);
        }
        *pCharBack = cGap;
    }
    else
    {
        for (++pCharBack; pCharBack != pCharFront; pCharBack--)
        {
            *pCharBack ^= *(pCharBack - 1);
            *(pCharBack - 1) ^= *pCharBack;
            *pCharBack ^= *(pCharBack - 1);
        }
        *pCharFront = cGap;
    }
    return 0;
}
static EN_KEY get_key(char *pChar)
{
    char pGetStr[4] = {0};
    EN_KEY enKey = EN_KEY_NOT_SUPPORT;
    unsigned int intOffset = 0;

    memset(pGetStr, 0, sizeof(pGetStr));
    while(1)
    {
        if (read(0, pGetStr + intOffset, 1) == 1)
        {
            if (pGetStr[0] == 27)
            {
                if (pGetStr[1] == 91 && pGetStr[2] == 65) //up
                {
                    enKey = EN_KEY_UP;
                    break;
                }
                else if (pGetStr[1] == 91 && pGetStr[2] == 66) //down
                {
                    enKey = EN_KEY_DOWN;
                    break;
                }
                else if (pGetStr[1] == 91 && pGetStr[2] == 68) //left
                {
                    enKey = EN_KEY_LEFT;
                    break;
                }
                else if (pGetStr[1] == 91 && pGetStr[2] == 67) //right
                {
                    enKey = EN_KEY_RIGHT;
                    break;
                }
                else if (pGetStr[1] == 91 && pGetStr[2] == 51 && pGetStr[3] == 126) //del
                {
                    enKey = EN_KEY_DEL;
                    break;
                } //I need fill all the keyboard keycode so that key will not lost.
                intOffset++;
                if (intOffset >= sizeof(pGetStr))
                {
                    intOffset = 0;
                    memset(pGetStr, 0, sizeof(pGetStr));
                }
                else
                {
                    continue; //Continue get key.
                }
            }
            else if (pGetStr[0] >= 32 && pGetStr[0] <= 126)
            {
                enKey = EN_KEY_CHAR;
                if (pChar)
                {
                    *pChar = pGetStr[0];
                }
                break;
            }
            else if (pGetStr[0] == '\n')
            {
                enKey = EN_KEY_ENTER;
                break;
            }
            else if (pGetStr[0] == 8)
            {
                enKey = EN_KEY_BACKSPACE;
                break;
            }
            else if (pGetStr[0] == 9)
            {
                enKey = EN_KEY_TAB;
                break;
            }
        }
    }

    return enKey;
}
static int socket_init(const char *pAddr)
{
    struct sockaddr_in stAddress;
    int socketFd;
    int len;
    int intResult;

    if (pAddr == NULL)
    {
        return -1;
    }
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("socket");
        exit (EXIT_FAILURE);
    }

    stAddress.sin_family = AF_INET;
    stAddress.sin_port = htons(8000);
    if(inet_pton(AF_INET, pAddr, &stAddress.sin_addr) <= 0)
    {
        printf("inet_pton error for %s\n", pAddr);
        return 0;
    }  
    len = sizeof (stAddress);

    intResult = connect(socketFd, (struct sockaddr *)&stAddress, len);
    if (intResult == -1)
    {
        printf ("ensure the server is up\n");
        perror ("connect");
        exit (EXIT_FAILURE);
    }

    return socketFd;
}
static void socket_deinit(int socketFd)
{
    if (-1 != socketFd)
        close (socketFd);
}
static void socket_send_data(int socketFd, char *pTransData, int intTransDataSize)
{
    if (pTransData == NULL || intTransDataSize == 0)
    {
        return;
    }
    if (send(socketFd, pTransData, strlen(pTransData) + 1, 0) == -1)
    {
        perror ("send");
        exit (EXIT_FAILURE);
    }
    memset(pTransData, 0, intTransDataSize);
    if (recv(socketFd, pTransData, intTransDataSize, 0) == -1)
    {
        perror ("read");
        exit (EXIT_FAILURE);
    }
}
static void * shm_send_data(key_t key, char *pTransData, int intTransDataSize)
{
    struct shm_st * p_shm_config = NULL;
    int shmid = 0;
    void *shm = NULL;

    if (intTransDataSize == 0 || pTransData == NULL)
    {
        return NULL;
    }
    shmid = shmget(key, sizeof(struct shm_st), 0666|IPC_CREAT);
    if(shmid == -1)
    {
        fprintf(stderr, "shmget failed\n");
        return NULL;
    }
    shm = shmat(shmid, (void*)0, 0);
    if(shm == (void*)-1 || shm == NULL)
    {
        fprintf(stderr, "shmat failed\n");
        return NULL;
    }
    p_shm_config = (struct shm_st *)shm;
    if (p_shm_config != NULL)
    {
        memcpy(p_shm_config->buf, pTransData, sizeof(p_shm_config->buf));
        p_shm_config->bWrite = 1;
        while (p_shm_config->bWrite == 1)
        {
            usleep(1000);
        }
        memset(pTransData, 0, intTransDataSize);
        memcpy(pTransData, p_shm_config->buf, sizeof(p_shm_config->buf));
        if(shmdt(shm) == -1)
        {
            fprintf(stderr, "shmdt failed\n");
            return NULL;
        }
    }

    return NULL;
}
static void find_tab_strings_idx(const char* pExistStr, unsigned int size, std::vector<unsigned int> &showIdx)
{
    unsigned int idx = 0;
    std::vector<std::string>::iterator it = strTabStrings.begin();

    for (; it != strTabStrings.end(); ++it)
    {
        if (pExistStr == NULL || size == 0)
        {
            showIdx.push_back(idx);
        }
        else if (0 == strncmp(pExistStr, it->c_str(), size))
        {
            showIdx.push_back(idx);
        }
        idx++;
    }
    return;
}
static void find_match_strings(std::vector<std::string> &strStrings, std::string &strMatch)
{
    std::vector<std::string>::iterator it = strStrings.begin();
    unsigned short i = 0, bAbort = 0, minStrSize = it->length();
    const char *pStr = NULL;
    char cChar = 0;

    if (strStrings.size() == 1)
    {
        strMatch = strStrings[0];
        return;
    }
    for (; it != strStrings.end(); ++it)
    {
        if (minStrSize > it->length())
        {
            minStrSize = it->length();
        }
    }
    for (i = 0; i < minStrSize; i++)
    {
        it = strStrings.begin();
        pStr = it->c_str();
        cChar =  pStr[i];
        ++it;
        for (; it != strStrings.end(); ++it)
        {
            pStr = it->c_str();
            if (cChar != pStr[i])
            {
                bAbort = 1;
                break;
            }
            cChar =  pStr[i];
        }
        if (bAbort == 1)
        {
            break;
        }
    }
    if (i != 0)
    {
        strMatch.assign(pStr, i);
    }
}
static std::vector<unsigned int> auto_adjust_tab(const char* pExistStr, unsigned int size, std::string &strAdjust)
{
    unsigned int intMatchSize = 0;
    std::vector<unsigned int> intIdx;
    std::vector<unsigned int>::iterator it = intIdx.begin();

    find_tab_strings_idx(pExistStr, size, intIdx);
    intMatchSize = intIdx.size();
    if (intMatchSize == 1)
    {
        if (size < strTabStrings[intIdx[0]].length())
        {
            strAdjust.assign(strTabStrings[intIdx[0]].c_str() + size);
        }
    }
    else if (intMatchSize > 1)
    {
        for (it = intIdx.begin(); it != intIdx.end(); ++it)
        {
            if (strTabStrings[*it].length() <= size)
            {
                break;
            }
        }
        if (it == intIdx.end()) //All find strings size is larger than pExistStr.
        {
            std::vector<std::string> strTmpStrings;
            std::string strTmp;

            it = intIdx.begin();
            for (; it != intIdx.end(); ++it)
            {
                strTmp.assign(strTabStrings[*it].c_str() + size);
                strTmpStrings.push_back(strTmp);
            }
            find_match_strings(strTmpStrings, strAdjust) ;
        }
    }

    return intIdx;
}
static int show_tab_strings(std::vector<unsigned int> showIdx)
{
    if (showIdx.empty())
    {
        return -1;
    }

    std::vector<unsigned int>::iterator it = showIdx.begin();

    printf("\n");
    for (;it != showIdx.end(); ++it)
    {
        printf("%s", strTabStrings[*it].c_str());
        PRINT_GAP(3, " ");
    }
    printf("\n");
    return -1;
}
static FILE *script_fp = NULL;
static int get_cmd(char *pGetBuf, int intOffset)
{
    char cGetChar = 0;
    int i = intOffset;
    int intBufferSize = intOffset;
    struct termios new_opts;
    int res=0;
    std::string str;
    EN_KEY enKey = EN_KEY_NOT_SUPPORT;
    unsigned short bExitGetCmd = 0;
    unsigned short uTabCount = 0;
    char *script_str = NULL;
    char *script_str_end = NULL;

    if (pGetBuf == NULL)
    {
        return 0;
    }
    if (script_fp != NULL)
    {
        script_str = &pGetBuf[intOffset];
        if (script_str == fgets(script_str, TRANS_BUFFER, script_fp))
        {
            if (*script_str == '\n' || *script_str == '\r' || *script_str == '#')
            {
                return 0;
            }
            script_str_end = strstr(script_str, ";");
            if (script_str_end == NULL)
            {
                printf("\033[1;31m");
                printf("Script error not parse \';\'\n");
                printf("\033[0m");
                fclose(script_fp);
                script_fp = NULL;

                return 0;
            }
            *script_str_end = 0;

            printf("\033[1;36m");
            printf("[LOAD SCRIPT]%s\n", script_str);
            printf("\033[0m");

            return script_str_end - script_str;
        }
        else
        {
            printf("close file!\n");
            fclose(script_fp);
            script_fp = NULL;

            return 0;
        }
    }
    memcpy(&new_opts, &gstOrgOpts, sizeof(new_opts));
    new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
    res = tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
    if (res == -1)
    {
        assert(0);
    }
    while (bExitGetCmd == 0)
    {
        cGetChar = 0;
        enKey = get_key(&cGetChar);
        //printf("key %d\n", enKey);
        //continue;
        if (enKey == EN_KEY_TAB)
        {
            if (uTabCount < 2)
            {
                uTabCount++;
            }
        }
        else
        {
            uTabCount = 0;
        }
        switch (enKey)
        {
            case EN_KEY_TAB:
            {
                if (strTabStrings.empty())
                {
                    break;
                }
                std::vector<unsigned int> showIdx;
                std::string strAdjust;

                /*
                   sample1_cmd sample2_cmd samplen_cmd

                   rev = auto_adjust_tab(x, y, str)

                   rev :0,str: empty :
                        Cmd : empty, Input: nothing ->tab
                   rev :1,str: empty :
                        Cmd : sample1_cmd, Input: sample1_cmd ->tab
                   rev :n,str: empty :
                        Cmd : sample1_cmd/sample2_cmd/...samplen_cmd, Input: nothing ->tab
                   rev :1,str: not empty :
                        Cmd : sample1_cmd/sample2_cmd/...samplen_cmd, Input: sample1 ->tab
                        Cmd : sample1_cmd, Input: sample1 ->tab
                   rev :n,str: not empty :
                        Cmd : sample1_cmd/sample2_cmd/...samplen_cmd, Input: sample ->tab
                */
                showIdx = auto_adjust_tab(&pGetBuf[intOffset], i - intOffset, strAdjust);
                if (uTabCount == 1)//first tab
                {
                    std::string tmpStr;
                    if (showIdx.size() > 1)
                    {
                        if (intBufferSize + strAdjust.length() < TRANS_BUFFER && strAdjust.empty() == 0) //protect buffer overwrite.
                        {
                            if (i < intBufferSize)
                            {
                                tmpStr.assign(&pGetBuf[i]);
                            }
                            strcpy(&pGetBuf[i], strAdjust.c_str());//Auto adjust
                            printf("%s", strAdjust.c_str());
                            i += strAdjust.length();
                            intBufferSize += strAdjust.length();
                            if (tmpStr.empty() == 0)
                            {
                                strcpy(&pGetBuf[i], tmpStr.c_str());
                                printf("%s", tmpStr.c_str());
                                MOVELEFT((int)tmpStr.length());
                            }
                            uTabCount = 0;
                        }
                    }
                    else if (showIdx.size() == 1) //if find only one
                    {
                        if (intBufferSize + 1 + strAdjust.length() < TRANS_BUFFER) //protect buffer overwrite.
                        {
                            if (i < intBufferSize)
                            {
                                tmpStr.assign(&pGetBuf[i]);
                            }

                            if (strAdjust.empty() == 0)
                            {
                                strcpy(&pGetBuf[i], strAdjust.c_str());//Auto adjust
                                printf("%s", strAdjust.c_str());
                                i += strAdjust.length();
                                intBufferSize += strAdjust.length();
                            }
                            move_string_by_one_step(&pGetBuf[i], &pGetBuf[intBufferSize - 1], 0, ' ');
                            intBufferSize++;
                            i++;                       // insert space.
                            printf(" ");

                            if (tmpStr.empty() == 0)
                            {
                                strcpy(&pGetBuf[i], tmpStr.c_str());
                                printf("%s", tmpStr.c_str());
                                MOVELEFT((int)tmpStr.length());
                            }
                            uTabCount = 0;
                        }
                    }
                }
                else if (uTabCount == 2)
                {
                    if (showIdx.empty() == 0)
                    {
                        show_tab_strings(showIdx);//show all cmd.
                        init_console(pSubCmdName);
                        printf("%s", &pGetBuf[intOffset]);
                        MOVELEFT(intBufferSize - i);
                    }
                }
            }
            break;
            case EN_KEY_ENTER:
            {
                if (intBufferSize > intOffset) //get strings
                {
                    str.assign(&pGetBuf[intOffset], intBufferSize - intOffset);
                    strCmdStrings.push_back(str);
                    //printf("\nintCmdIdx %d i %d intBufferSize %d", intCmdIdx, i, intBufferSize);
                    intCmdIdx = strCmdStrings.size();
                }
                printf("\n");
                bExitGetCmd = 1;
            }
            break;
            case EN_KEY_BACKSPACE:
            {
                if (i > intOffset)
                {
                    move_string_by_one_step(&pGetBuf[i], &pGetBuf[intBufferSize - 1], 1, 0);
                    i--;
                    intBufferSize--;
                    MOVELEFT(1);
                    CLEARLINE;
                    for (int j = i; j < intBufferSize; j++)
                    {
                        printf("%c", pGetBuf[j]);
                    }
                    MOVELEFT(intBufferSize - i);
                    intCmdIdx = strCmdStrings.size();
                }
            }
            break;
            case EN_KEY_CHAR:
            {
                if (intBufferSize < TRANS_BUFFER - 1) //skip pGetBuf[TRANS_BUFFER - 1] = 0
                {
                    move_string_by_one_step(&pGetBuf[i], &pGetBuf[intBufferSize - 1], 0, cGetChar);
                    intBufferSize++;
                    i++;
                    for (int j = i - 1; j < intBufferSize; j++)
                    {
                        printf("%c", pGetBuf[j]);
                    }
                    MOVELEFT(intBufferSize - i);
                    intCmdIdx = strCmdStrings.size();
                }
            }
            break;
            case EN_KEY_UP:
            {
                if (strCmdStrings.size() && intCmdIdx > 0)
                {
                    memset(&pGetBuf[intOffset], 0, sizeof(char) * TRANS_BUFFER - intOffset);
                    MOVELEFT(i - intOffset);
                    CLEARLINE;
                    strcpy(&pGetBuf[intOffset], strCmdStrings[intCmdIdx - 1].c_str());
                    i = intBufferSize = strCmdStrings[intCmdIdx - 1].length() + intOffset;
                    printf("%s", &pGetBuf[intOffset]);
                    intCmdIdx--;
                }
            }
            break;
            case EN_KEY_DOWN:
            {
                if (strCmdStrings.size() && intCmdIdx < strCmdStrings.size() -  1)
                {
                    memset(&pGetBuf[intOffset], 0, sizeof(char) * TRANS_BUFFER - intOffset);
                    MOVELEFT(i - intOffset);
                    CLEARLINE;
                    strcpy(&pGetBuf[intOffset], strCmdStrings[intCmdIdx + 1].c_str());
                    i = intBufferSize = strCmdStrings[intCmdIdx + 1].length() + intOffset;
                    printf("%s", &pGetBuf[intOffset]);
                    intCmdIdx++;
                }
            }
            break;
            case EN_KEY_LEFT:
            {
                if (i > intOffset)
                {
                    MOVELEFT(1);
                    i--;
                }
            }
            break;
            case EN_KEY_RIGHT:
            {
                if (i < intBufferSize)
                {
                    MOVERIGHT(1);
                    i++;
                }
            }
            break;
            case EN_KEY_DEL:
            {
                if (intBufferSize > i)
                {
                    move_string_by_one_step(&pGetBuf[i + 1], &pGetBuf[intBufferSize - 1], 1, 0);
                    intBufferSize--;
                    pGetBuf[intBufferSize] = 0;
                    CLEARLINE;
                    for (int j = i; j < intBufferSize; j++)
                    {
                        printf("%c", pGetBuf[j]);
                    }
                    MOVELEFT(intBufferSize - i);
                    intCmdIdx = strCmdStrings.size() - 1;
                }
            }
            break;
            default:
                break;
        }
    }
    res = tcsetattr(STDIN_FILENO, TCSANOW, &gstOrgOpts);
    if (res == -1)
    {
        assert(0);
    }
    return i;
}

static int setup_init_cmd(char *pDetBuf, char *pMainCmd, char *pSubCmd)
{
    if (pMainCmd == NULL || pDetBuf == NULL)
    {
        return -1;
    }

    if (pSubCmd != NULL)
    {
        sprintf(pDetBuf, "%s %s ", pMainCmd, pSubCmd);
    }
    else
    {
        sprintf(pDetBuf, "%s ", pMainCmd);
    }

    return 0;
}

static int send_cmd(char *pTransData, int intTransDataSize)
{
    if (pTransData == NULL || intTransDataSize == 0)
    {
        return 0;
    }

    if (-1 == gsocketFd)
        shm_send_data(88860611, pTransData, intTransDataSize);
    else
        socket_send_data(gsocketFd, pTransData, intTransDataSize);

    return strlen(pTransData);
}
static int check_cmd_if_exist(char *pDstChar, int offset)
{
    char *pStr = NULL;
    int intRet = 0;

    if (pDstChar == NULL)
    {
        return -1;
    }

    pStr = pDstChar + offset;
    pStr = strstr(pStr, " ");
    if (pStr != NULL)
    {
        while (*(pStr) == ' ')
        {
            pStr++;
        }
        if (*(pStr) != 0)
        {
            intRet = 1;
        }
    }

    return intRet;
}
static int use_script(char *pDstChar, int intOffset)
{
    char *pStr = NULL;
    char *pStrEnd = NULL;
    char *pStrEnd2 = NULL;

    int intRet = 0;
    char strFile[50];

    if (pDstChar == NULL)
    {
        return -1;
    }
    pStr = strstr(pDstChar + intOffset, " ");
    if (pStr)
    {
        pStr++;
        while (*(pStr) == ' ')
        {
            pStr++;
        }
        if (*(pStr) 
!= 0)
        {
            if (*(pStr) == 's' && *(pStr + 1) == ' ')
            {
                printf("func %s line %d\n", __FUNCTION__, __LINE__);
                pStr++;
                while (*(pStr) == ' ')
                {
                    pStr++;
                }
                if (*pStr == 0)
                {
                    return -1;
                }
                pStrEnd = pStr;
                while(*pStrEnd != ' ' && *pStrEnd != 0)
                {
                    pStrEnd++;
                }
                if (*pStrEnd == ' ')
                {
                    pStrEnd2 = pStrEnd;
                    while (*(pStrEnd2) == ' ')
                    {
                        pStrEnd2++;
                    }
                    if (*pStrEnd2 != 0)
                    {
                        printf("func %s line %d\n", __FUNCTION__, __LINE__);

                        return -1;
                    }
                }
                strncpy(strFile, pStr, pStrEnd - pStr);
                strFile[pStrEnd - pStr] = 0;
                printf("open file %s\n", strFile);
                script_fp = fopen(strFile, "r");
                if (script_fp == NULL)
                {
                    printf("Open file error!\n");
                    perror("fopen");
                    return -1;
                }
            }
        }
    }

    return intRet;
}
static int check_cmd_if_only_one(char *pDstChar, int intOffset)
{
    char *pStr = NULL;
    int intRet = 0;

    if (pDstChar == NULL)
    {
        return -1;
    }
    pStr = strstr(pDstChar + intOffset, " ");
    if (pStr)
    {
        pStr++;
        while (*(pStr) == ' ')
        {
            pStr++;
        }
        if (*(pStr) != 0)
        {
            while (*(pStr) != ' ' && *(pStr) != 0)
            {
                pStr++;
            }
            if (*(pStr) == 0)
            {
                intRet = 1;
            }
            else if (*(pStr) == ' ')
            {
                while (*(pStr) == ' ')
                {
                    pStr++;
                }
                if (*(pStr) == 0)
                {
                    intRet = 1;
                }
            }

        }
    }

    return intRet;
}
static int check_cmd_if_exist_str(char *pRetBuf, char *pDstChar, int infOffset)
{
    char *pStr = NULL;

    if (pRetBuf == NULL || pDstChar == NULL)
    {
        return -1;
    }
    pStr = strstr(pRetBuf + infOffset, " ");
    if (pStr)
    {
        pStr++;
        while (*(pStr) == ' ')
        {
            pStr++;
        }
        if (strstr(pStr, pDstChar) != NULL)
        {
            pStr += strlen(pDstChar);
            while (*pStr == ' ')
            {
                pStr++;
            }
            if (*pStr == 0)
            {
                return 1;
            }
        }
    }
    return 0;

}
static int parse_tab_strings(const char *pStr)
{
    if (pStr == NULL)
    {
        return -1;
    }

    const char *posFront = NULL;
    const char *posBack = NULL;
    std::string tmpString;

    strTabStrings.clear();
    posFront = pStr;
    posBack = strstr(posFront, "/");

    while (posBack != NULL && posFront != posBack)
    {
        tmpString.assign(posFront, (posBack - posFront));
        strTabStrings.push_back(tmpString);
        posFront = posBack + 1;
        posBack = strstr(posFront, "/");
    }
    return 0;
}
static void do_signal(int)
{
    if (tcsetattr(STDIN_FILENO, TCSANOW, &gstOrgOpts) == -1)
    {
        perror("tcsetattr");
    }
    exit(0);
}
static void do_exit(void)
{
    if (tcsetattr(STDIN_FILENO, TCSANOW, &gstOrgOpts) == -1)
    {
        perror("tcsetattr");
    }
}
int mdb_console(int bExitService, char *pAddr)
{
    EN_CONSOLE_STATUS enConsoleStatus = EN_CONSOLE_WELCOME;
    int bExit = 0;
    char cTransBuffer[TRANS_BUFFER];
    int intRetSize;
    int intOffset = 0;

    if (tcgetattr(STDIN_FILENO, &gstOrgOpts) == -1)
    {
        perror("tcgetattr");
        return -1;
    }
    if (atexit(do_exit) != 0)
    {
        perror("atexit");
        return -1;
    }
    if (signal(SIGINT, do_signal) != 0)
    {
        perror("signal");
        return -1;
    }
    if (signal(SIGTSTP, do_signal) != 0)
    {
        perror("signal");
        return -1;
    }
    gsocketFd = socket_init(pAddr);

    setvbuf(stdout, NULL, _IONBF, 0);
    while (bExit == 0)
    {
        switch (enConsoleStatus)
        {
            case EN_CONSOLE_WELCOME:
            {
                memset(pSubCmdName, 0, sizeof(pSubCmdName));
                memset(cTransBuffer, 0, sizeof(cTransBuffer));
                strcpy(cTransBuffer, "mdb w");
                intRetSize = send_cmd(cTransBuffer, sizeof(cTransBuffer));
                if (intRetSize != 0)
                {
                    printf("%s", cTransBuffer);
                }
                while (intRetSize == TRANS_BUFFER - 1)
                {
                    memset(cTransBuffer, 0, sizeof(cTransBuffer));
                    strcpy(cTransBuffer, "mdb c");
                    intRetSize = send_cmd(cTransBuffer, sizeof(cTransBuffer));
                    if (intRetSize != 0)
                    {
                        printf("%s", cTransBuffer);
                    }
                }
                enConsoleStatus = EN_CONSOLE_MAIN;
            }
            break;
            case EN_CONSOLE_MAIN:
            {
                memset(pSubCmdName, 0, sizeof(pSubCmdName));
                memset(cTransBuffer, 0, sizeof(cTransBuffer));
                init_console("mdb");
                setup_init_cmd(cTransBuffer, (char *)"mdb", NULL);
                intOffset = strlen(cTransBuffer);
                get_cmd(cTransBuffer, intOffset);
                if (check_cmd_if_only_one(cTransBuffer, intOffset - 1) != 1)
                {
                    use_script(cTransBuffer, intOffset - 1);
                }
                else
                {
                    if (check_cmd_if_exist_str(cTransBuffer, (char *)"q", intOffset - 1) == 1)
                    {
                        bExit = 1;
                        if (bExitService) //exit service
                            send_cmd(cTransBuffer, sizeof(cTransBuffer));
                        break;
                    }
                    if (check_cmd_if_exist_str(cTransBuffer, (char *)"w", intOffset - 1))
                    {
                        break;
                    }
                    if (check_cmd_if_exist_str(cTransBuffer, (char *)"c", intOffset - 1))
                    {
                        break;
                    }
                    //Step 1: Send cmd to select module.
                    intRetSize = send_cmd(cTransBuffer, sizeof(cTransBuffer));
                    if (intRetSize == 0)
                    {
                        break;
                    }
                    printf("%s", cTransBuffer);
                    while (intRetSize == TRANS_BUFFER - 1)
                    {
                        memset(cTransBuffer, 0, sizeof(cTransBuffer));
                        strcpy(cTransBuffer, "mdb c");
                        intRetSize = send_cmd(cTransBuffer, sizeof(cTransBuffer));
                        if (intRetSize != 0)
                        {
                            printf("%s", cTransBuffer);
                        }
                    }
                    //Step 2: Send cmd to get module name.
                    memset(cTransBuffer, 0, sizeof(cTransBuffer));
                    strcpy(cTransBuffer, "mdb n");
                    intRetSize = send_cmd(cTransBuffer, sizeof(cTransBuffer));
                    if (intRetSize != 0)
                    {
                        strncpy(pSubCmdName, cTransBuffer, intRetSize);
                        enConsoleStatus = EN_CONSOLE_SUB;
                    }

                    //Step 3: Send cmd to get module tab list.
                    std::string strString;

                    memset(cTransBuffer, 0, sizeof(cTransBuffer));
                    strcpy(cTransBuffer, "mdb t");
                    intRetSize = send_cmd(cTransBuffer, sizeof(cTransBuffer));
                    if (intRetSize != 0)
                    {
                        strString += cTransBuffer;
                    }
                    while (intRetSize == TRANS_BUFFER - 1)
                    {
                        memset(cTransBuffer, 0, sizeof(cTransBuffer));
                        strcpy(cTransBuffer, "mdb c");
                        intRetSize = send_cmd(cTransBuffer, sizeof(cTransBuffer));
                        if (intRetSize != 0)
                        {
                            strString += cTransBuffer;
                        }
                    }
                    parse_tab_strings(strString.c_str());
                }
            }
            break;
            case EN_CONSOLE_SUB:
            {
                memset(cTransBuffer, 0, sizeof(cTransBuffer));
                init_console(pSubCmdName);
                setup_init_cmd(cTransBuffer, (char *)"mdb", NULL);
                intOffset = strlen(cTransBuffer);
                get_cmd(cTransBuffer, intOffset);
                if (check_cmd_if_exist(cTransBuffer, intOffset - 1) == 1)
                {
                    if (check_cmd_if_exist_str(cTransBuffer, (char *)"q", intOffset - 1) == 1)
                    {
                        enConsoleStatus = EN_CONSOLE_WELCOME;
                        send_cmd(cTransBuffer, sizeof(cTransBuffer));
                        break;
                    }
                    if (check_cmd_if_exist_str(cTransBuffer, (char *)"c", intOffset - 1))
                    {
                        break;
                    }
                    if (check_cmd_if_exist_str(cTransBuffer, (char *)"w", intOffset - 1))
                    {
                        break;
                    }
                    if (check_cmd_if_exist_str(cTransBuffer, (char *)"n", intOffset - 1))
                    {
                        break;
                    }
                    if (check_cmd_if_exist_str(cTransBuffer, (char *)"t", intOffset - 1))
                    {
                        break;
                    }
                    intRetSize = send_cmd(cTransBuffer, sizeof(cTransBuffer));
                    if (intRetSize != 0)
                    {
                        printf("%s", cTransBuffer);
                    }
                    while (intRetSize == TRANS_BUFFER - 1)
                    {
                        memset(cTransBuffer, 0, sizeof(cTransBuffer));
                        strcpy(cTransBuffer, "mdb c");
                        intRetSize = send_cmd(cTransBuffer, sizeof(cTransBuffer));
                        if (intRetSize != 0)
                        {
                            printf("%s", cTransBuffer);
                        }
                    }
                }
            }
            break;
            default:
                bExit = 1;
                break;
        }
    }
    socket_deinit(gsocketFd);

    return 0;
}
