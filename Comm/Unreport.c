#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "stm32h7xx_hal.h"
#include "Unreport.h"
#include "CbReport.h"
#include "Debug.h"
#include "Queue.h"
#include "cJSON.h"

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart4;

#define UNREPORT_UART   huart2

static UnReportTarget_t g_TargetList[] =
{
    { "SetLightGear",    CbReportSetLightGear,    {0}, CbReportSetLightGearCode,    0, 1 },
    { "SetPusher",       CbReportSetPusher,       {0}, CbReportSetPusherCode,       0, 1 },
    { "SetPlatform",     CbReportSetPlatform,     {0}, CbReportSetPlatformCode,     0, 1 },
    { "SetLightSwitch",  CbReportLightSwitch,     {0}, CbReportSetLightSwitchCode,  0, 1 },
    { "FixSensorError",  CbReportFixSensorError,  {0}, CbReportFixSensorErrorCode, 0, 1 },
    { "RollingDoor",     CbReportRollingDoor,     {0}, CbReportRollingDoorCode,     0, 1 },
    { "Relay",           CbRelay,                 {0}, CbRelayCode,                 0, 1 },
    { "LightModeSwitch", CbLightModeSwitch,       {0}, CbLightModeSwitchCode,       0, 1 },
    { "UavOperation",    CbUavOperation,          {0}, UavOperationCode,            0, 1 },
};

#define TARGET_LIST_SIZE  (sizeof(g_TargetList) / sizeof(g_TargetList[0]))

static Queue_t *g_RcvFIFO;
static char g_RcvJson[UNREPORT_BUF_SIZE + 5];
static size_t g_RcvCount = 0;

int UnReportInit(void)
{
    __HAL_UART_CLEAR_OREFLAG(&UNREPORT_UART);
    __HAL_UART_CLEAR_IT(&UNREPORT_UART, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&UNREPORT_UART, UART_IT_RXNE);
    __HAL_UART_CLEAR_IDLEFLAG(&UNREPORT_UART);
    __HAL_UART_ENABLE_IT(&UNREPORT_UART, UART_IT_IDLE);
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    __HAL_UART_CLEAR_OREFLAG(&huart3);
    __HAL_UART_CLEAR_IT(&huart3, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);

    g_RcvFIFO = QueueCreate(UNREPORT_BUF_SIZE);
    if (!g_RcvFIFO)
    {
        ERROR_LOG("UnReport RcvFIFO Create Failed");
        return -1;
    }
    printf("UnReport Init ok\r\n");
    return 0;
}

Queue_t *UnReportGetFIFO(void)
{
    return g_RcvFIFO;
}

static bool UnReportTypeValid(cJSON *json)
{
    return cJSON_IsString(json);
}

static bool UnReportSpeakIdValid(cJSON *json)
{
    return (cJSON_IsString(json) || cJSON_IsNumber(json));
}

UnReportCode_t UnReportGetTargetEx(VariantDef *variant, const char *txt)
{
    VariantDef internal;
    memset(&internal, 0, sizeof(VariantDef));

    if (!txt)
    {
        ERROR_LOG("TXT Error");
        return UnReportParseError;
    }

    cJSON *msg = cJSON_Parse(txt);
    if (!msg)
    {
        ERROR_LOG("%s Parse Failed", txt);
        return UnReportParseError;
    }

    cJSON *target   = cJSON_GetObjectItem(msg, "Target");
    cJSON *obj      = cJSON_GetObjectItem(msg, "Data");
    cJSON *type     = cJSON_GetObjectItem(msg, "Type");
    cJSON *speakid  = cJSON_GetObjectItem(msg, "SpeakId");

    if (!obj)
    {
        ERROR_LOG("%s Get data Failed", txt);
        cJSON_Delete(msg);
        return UnReportDataError;
    }
    if (!UnReportTypeValid(type))
    {
        ERROR_LOG("%s Get Type Failed", txt);
        cJSON_Delete(msg);
        return UnReportTypeError;
    }
    if (!UnReportTypeValid(target))
    {
        ERROR_LOG("%s Get target Failed", txt);
        cJSON_Delete(msg);
        return UnReportTargetError;
    }
    if (!UnReportSpeakIdValid(speakid))
    {
        ERROR_LOG("%s Get SpeakId Failed", txt);
        cJSON_Delete(msg);
        return UnReportTargetError;
    }

    if (!strcmp(type->valuestring, "string") && cJSON_IsString(obj))
        VariantToString(&internal, obj->valuestring);
    else if (!strcmp(type->valuestring, "float") && cJSON_IsNumber(obj))
        VariantToFloat(&internal, (float)obj->valuedouble);
    else if (!strcmp(type->valuestring, "int") && cJSON_IsNumber(obj))
        VariantToInt(&internal, (int)obj->valueint);
    else if (!strcmp(type->valuestring, "uint") && cJSON_IsNumber(obj))
        VariantToUint(&internal, (uint32_t)obj->valueint);
    else
    {
        ERROR_LOG("error type:%s", type->valuestring);
        cJSON_Delete(msg);
        return UnReportTypeError;
    }

    if (variant)
        *variant = internal;

    for (size_t i = 0; i < TARGET_LIST_SIZE; i++)
    {
        if (g_TargetList[i].Target && !strcmp(target->valuestring, g_TargetList[i].Target))
        {
            int ret = -1;
            g_TargetList[i].SpeakId = (uint32_t)speakid->valueint;
            g_TargetList[i].Param = internal;

            if (g_TargetList[i].Cb)
            {
                ret = g_TargetList[i].Cb(&g_TargetList[i].Param, (int)g_TargetList[i].SpeakId);
                if (g_TargetList[i].SpeakId || g_TargetList[i].DefaultFlag)
                {
                    pSerial(&UNREPORT_UART, "{ \"Command\":{\"SpeakId\": %d,\"Execute_Status\":%d}}",
                            g_TargetList[i].SpeakId, ret);
                }
            }

            cJSON_Delete(msg);
            return g_TargetList[i].Code;
        }
    }

    cJSON_Delete(msg);
    return UnReportTargetError;
}

int UnReport(void)
{
    if (QueueIsEmpty(g_RcvFIFO))
        return -1;

    g_RcvCount = 0;
    memset(g_RcvJson, 0, sizeof(g_RcvJson));

    uint8_t data;
    uint8_t okFlag = 0;

    while (!QueueIsEmpty(g_RcvFIFO))
    {
        QueueDequeue(g_RcvFIFO, &data);
        if (data != '{') continue;

        g_RcvJson[g_RcvCount++] = data;
        while (!QueueIsEmpty(g_RcvFIFO) && data != '}')
        {
            QueueDequeue(g_RcvFIFO, &data);
            g_RcvJson[g_RcvCount++] = data;
            if (g_RcvCount > UNREPORT_BUF_SIZE) return -1;
        }
        g_RcvJson[g_RcvCount++] = '\0';
        okFlag = 1;
        break;
    }

    if (okFlag)
    {
        UnReportGetTargetEx(NULL, g_RcvJson);
        return 0;
    }

    return -1;
}

volatile uint8_t g_UnReportFlag = 0;

void USART2_IRQHandler(void)
{
    __HAL_UART_CLEAR_OREFLAG(&UNREPORT_UART);
    if (__HAL_UART_GET_IT(&UNREPORT_UART, UART_IT_RXNE))
    {
        __HAL_UART_CLEAR_IT(&UNREPORT_UART, UART_IT_RXNE);
        uint8_t data;
        HAL_UART_Receive(&UNREPORT_UART, &data, 1, 0xFFFF);
        if (g_RcvFIFO)
            QueueEnqueue(g_RcvFIFO, data);
    }

    if (__HAL_UART_GET_FLAG(&UNREPORT_UART, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&UNREPORT_UART);
        g_UnReportFlag = 1;
    }
}

void UART4_IRQHandler(void)
{
    __HAL_UART_CLEAR_OREFLAG(&huart4);
    if (__HAL_UART_GET_IT(&huart4, UART_IT_RXNE))
    {
        __HAL_UART_CLEAR_IT(&huart4, UART_IT_RXNE);
        uint8_t data;
        HAL_UART_Receive(&huart4, &data, 1, 0xFFFF);
        if (g_RcvFIFO)
            QueueEnqueue(g_RcvFIFO, data);
    }

    if (__HAL_UART_GET_FLAG(&huart4, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart4);
        g_UnReportFlag = 1;
    }
}
