#include "Report.h"
#include "Debug.h"
#include "Weather.h"
#include "Wind.h"
#include "Distance.h"
#include "Ina219.h"
#include "Light.h"
#include "MtrDrv.h"
#include "Uav.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern TIM_HandleTypeDef htim15;
extern UART_HandleTypeDef huart2;

volatile uint8_t g_ReportFlag = 0;

int ReportInit(void)
{
    __HAL_TIM_CLEAR_FLAG(&htim15, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&htim15, TIM_FLAG_UPDATE);
    HAL_NVIC_SetPriority(TIM15_IRQn, 2, 2);
    HAL_NVIC_EnableIRQ(TIM15_IRQn);
    HAL_TIM_Base_Start(&htim15);
    printf("Report Init ok\r\n");
    return 0;
}

void TIM15_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&htim15, TIM_FLAG_UPDATE))
    {
        __HAL_TIM_CLEAR_FLAG(&htim15, TIM_FLAG_UPDATE);
        g_ReportFlag = 1;
    }
}

void ReportSensor(void)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    /* ---------- Data.Modbus ---------- */
    cJSON *dataNode = cJSON_CreateObject();
    cJSON *modbusNode = cJSON_CreateObject();

    /* Weather */
    cJSON *weatherJson = cJSON_CreateObject();
    WeatherData_t *w = WeatherGetData();
    cJSON_AddNumberToObject(weatherJson, "Humidity", w->Humi);
    cJSON_AddNumberToObject(weatherJson, "Temperature", w->Temp);
    cJSON_AddNumberToObject(weatherJson, "PM10", w->PM10);
    cJSON_AddNumberToObject(weatherJson, "PM2.5", w->PM25);
    cJSON_AddNumberToObject(weatherJson, "Illuminance", w->Lux);
    cJSON_AddNumberToObject(weatherJson, "Failed_Number", WeatherGetStatus()->FailedCount);
    cJSON_AddStringToObject(weatherJson, "State",
        (WeatherGetStatus()->State == SensorNormal) ? "Normal" : "Error");
    cJSON_AddItemToObject(modbusNode, "Weather", weatherJson);

    /* Wind */
    cJSON *windJson = cJSON_CreateObject();
    WindData_t *wind = WindGetData();
    cJSON_AddNumberToObject(windJson, "Wind_Speed", wind->Speed);
    cJSON_AddNumberToObject(windJson, "Wind_Direction", wind->Dir);
    cJSON_AddNumberToObject(windJson, "Failed_Number", WindGetStatus()->FailedCount);
    cJSON_AddStringToObject(windJson, "State",
        (WindGetStatus()->State == SensorNormal) ? "Normal" : "Error");
    cJSON_AddItemToObject(modbusNode, "Wind_Speed", windJson);

    /* Distance (4 channels) */
    static const char *disNames[] = {"Distance_Front", "Distance_Rear",
                                     "Distance_Left",  "Distance_Right"};
    for (int i = 0; i < DisCount; i++)
    {
        DisDev_t *dev = DistanceGetPoint((DisChannel_t)i);
        if (!dev) continue;
        cJSON *disJson = cJSON_CreateObject();
        cJSON_AddNumberToObject(disJson, "Distance", dev->Data);
        cJSON_AddNumberToObject(disJson, "Failed_Number", dev->Status.FailedCount);
        cJSON_AddStringToObject(disJson, "State",
            (dev->Status.State == SensorNormal) ? "Normal" : "Error");
        cJSON_AddItemToObject(modbusNode, disNames[i], disJson);
    }

    cJSON_AddItemToObject(dataNode, "Modbus", modbusNode);
    cJSON_AddItemToObject(root, "Data", dataNode);

    /* ---------- Sys ---------- */
    cJSON *sysNode = cJSON_CreateObject();
    cJSON_AddNumberToObject(sysNode, "Time_Stamp", HAL_GetTick());
    cJSON_AddItemToObject(root, "Sys", sysNode);

    /* ---------- Sensor ---------- */
    cJSON *sensorNode = cJSON_CreateObject();
    cJSON_AddNumberToObject(sensorNode, "Car_Voltage", g_CarPower.Voltage);
    cJSON_AddNumberToObject(sensorNode, "Car_Electricity", g_CarPower.Elec);
    cJSON_AddNumberToObject(sensorNode, "Car_Power", g_CarPower.Pow);
    cJSON_AddNumberToObject(sensorNode, "Light_Voltage", g_LightPower.Voltage);
    cJSON_AddNumberToObject(sensorNode, "Light_Electricity", g_LightPower.Elec);
    cJSON_AddNumberToObject(sensorNode, "Light_Power", g_LightPower.Pow);
    cJSON_AddNumberToObject(sensorNode, "Light_Mode", LightGetMode());
    cJSON_AddNumberToObject(sensorNode, "Light_State", LightGetState());
    cJSON_AddNumberToObject(sensorNode, "Light_Gear", LightGetBright());

    /* Rolling door state */
    switch (MtrGetState())
    {
        case MtrStateOpen:    cJSON_AddStringToObject(sensorNode, "Rolling_State", "OPENED");  break;
        case MtrStateOpening: cJSON_AddStringToObject(sensorNode, "Rolling_State", "OPENING"); break;
        case MtrStateClose:   cJSON_AddStringToObject(sensorNode, "Rolling_State", "CLOSED");  break;
        case MtrStateClosing: cJSON_AddStringToObject(sensorNode, "Rolling_State", "CLOSING"); break;
        default: break;
    }
    cJSON_AddItemToObject(root, "Sensor", sensorNode);

    /* ---------- Uav ---------- */
    cJSON *uavNode = cJSON_CreateObject();
    cJSON_AddNumberToObject(uavNode, "Uav_Accelerated_Speed_X", Jy90AccSpeedPtr->Ax);
    cJSON_AddNumberToObject(uavNode, "Uav_Accelerated_Speed_Y", Jy90AccSpeedPtr->Ay);
    cJSON_AddNumberToObject(uavNode, "Uav_Accelerated_Speed_Z", Jy90AccSpeedPtr->Az);
    cJSON_AddNumberToObject(uavNode, "Uav_Angular_Velocity_X", Jy90AngSpeedPtr->Wx);
    cJSON_AddNumberToObject(uavNode, "Uav_Angular_Velocity_Y", Jy90AngSpeedPtr->Wy);
    cJSON_AddNumberToObject(uavNode, "Uav_Angular_Velocity_Z", Jy90AngSpeedPtr->Wz);
    cJSON_AddNumberToObject(uavNode, "Uav_Angle_Roll", Jy90AnglePtr->Roll);
    cJSON_AddNumberToObject(uavNode, "Uav_Angle_Pitch", Jy90AnglePtr->Pitch);
    cJSON_AddNumberToObject(uavNode, "Uav_Angle_Yaw", Jy90AnglePtr->Yaw);
    cJSON_AddNumberToObject(uavNode, "Uav_Quaternion_Q0", Jy90FourNumPtr->q0);
    cJSON_AddNumberToObject(uavNode, "Uav_Quaternion_Q1", Jy90FourNumPtr->q1);
    cJSON_AddNumberToObject(uavNode, "Uav_Quaternion_Q2", Jy90FourNumPtr->q2);
    cJSON_AddNumberToObject(uavNode, "Uav_Quaternion_Q3", Jy90FourNumPtr->q3);
    cJSON_AddNumberToObject(uavNode, "Uav_Air_Pressure", Bmp388DataPtr->Press);
    cJSON_AddNumberToObject(uavNode, "Uav_Elevation", Bmp388DataPtr->High);
    cJSON_AddItemToObject(root, "Uav", uavNode);

    /* ---------- Print & Send ---------- */
    char *reportTxt = cJSON_Print(root);
    if (reportTxt)
    {
        pSerial(&huart2, "%s", reportTxt);
        free(reportTxt);
    }

    cJSON_Delete(root);
}

void SendDataToLubanCat(void)
{
    WeatherData_t *weather = WeatherGetData();
    WindData_t *wind = WindGetData();
    char jsonBuf[512];
    char finalBuf[550];

    snprintf(jsonBuf, sizeof(jsonBuf),
        "{\"Enviroment_Temperation\":%.1f,"
        "\"Enviroment_Humidity\":%.1f,"
        "\"Enviroment_Light\":%lu,"
        "\"Enviroment_Pm10\":%u,"
        "\"Enviroment_Pm25\":%u,"
        "\"Wind_Speed\":%.2f,"
        "\"Wind_Direction\":%.2f,"
        "\"deviceStatus\":\"online\","
        "\"deviceName\":\"Light\","
        "\"deviceType\":\"SensorH7\"}\n",
        weather->Temp, weather->Humi, weather->Lux,
        weather->PM10, weather->PM25,
        wind->Speed, wind->Dir);

    snprintf(finalBuf, sizeof(finalBuf), "JSON:%s", jsonBuf);
    pSerial(&huart2, finalBuf);
}
