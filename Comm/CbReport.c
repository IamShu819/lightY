#include <string.h>
#include <stdio.h>
#include "CbReport.h"
#include "Debug.h"
#include "Light.h"
#include "Unreport.h"
#include "Weather.h"
#include "Wind.h"
#include "Distance.h"
#include "SensorCommon.h"
#include "App.h"
#include "MtrDrv.h"
#include "Uav.h"

extern UART_HandleTypeDef huart3;

int CbReportSetLightGear(VariantDef *msg, int speakId)
{
    if (MsgIsUint(msg) || MsgIsInt(msg))
    {
        if (MsgGetUint(msg) > 0)
            LightEnable();
        else
            LightDisable();

        LightSetBright(MsgGetUint(msg));
        printf("gear %u\r\n", MsgGetUint(msg));
        pScreen("Gear-->%d", msg->Data.i);
    }
    else
    {
        return -1;
    }
    return 0;
}

int CbReportSetPusher(VariantDef *msg, int speakId)
{
    if (MsgIsString(msg))
    {
        const char *cmd = MsgGetString(msg);
        if (!strcmp(cmd, "extend"))
            SetPusherState(PUSHER_EXTEND);
        else if (!strcmp(cmd, "retract"))
            SetPusherState(PUSHER_RETRACT);
        else if (!strcmp(cmd, "stop"))
            SetPusherState(PUSHER_STOP);
        else
        {
            MsgReleaseString(msg);
            return -1;
        }
        MsgReleaseString(msg);
        return 0;
    }
    return -1;
}

int CbReportSetPlatform(VariantDef *msg, int speakId)
{
    if (MsgIsString(msg))
    {
        const char *cmd = MsgGetString(msg);
        if (!strcmp(cmd, "up"))
            SetPlatformState(PLATFORM_UP);
        else if (!strcmp(cmd, "down"))
            SetPlatformState(PLATFORM_DOWN);
        else if (!strcmp(cmd, "stop"))
            SetPlatformState(PLATFORM_STOP);
        else
        {
            MsgReleaseString(msg);
            return -1;
        }
        MsgReleaseString(msg);
        return 0;
    }
    return -1;
}

int CbReportLightSwitch(VariantDef *msg, int speakId)
{
    if (MsgIsUint(msg))
    {
        if (msg->Data.ui)
        {
            LightEnable();
            pScreen("Light Enable");
        }
        else
        {
            LightDisable();
            pScreen("Light Disable");
        }
    }
    else
    {
        return -1;
    }
    return 0;
}

int CbReportFixSensorError(VariantDef *msg, int speakId)
{
    int sensorState = 0x7FFFFFFF;

    if (!MsgIsUint(msg))
        return -1;

    if (WeatherGetStatus()->State == SensorError)
        sensorState &= ~(0x01 << WEATHER_MODBUS_ADDR);

    if (WindGetStatus()->State == SensorError)
        sensorState &= ~(0x01 << WIND_MODBUS_ADDR);

    for (int i = 0; i < DisCount; i++)
    {
        DisDev_t *dev = DistanceGetPoint((DisChannel_t)i);
        if (dev && dev->Status.State == SensorError)
            sensorState &= ~(0x01 << dev->ModbusAddr);
    }

    if (!msg->Data.ui && !speakId)
        pScreen("Sensor State:0x%02X", sensorState);

    return sensorState;
}

int CbReportRollingDoor(VariantDef *msg, int speakId)
{
    if (MsgIsUint(msg))
    {
        if (msg->Data.ui == 1)
        {
            MtrControl(1);
            printf("Command RollingDoorOpen\r\n");
        }
        else if (!msg->Data.ui)
        {
            MtrControl(0);
            printf("Command RollingDoorClose\r\n");
        }
        else
        {
            MtrControl(msg->Data.ui);
            printf("Command RollingAngle:%d\r\n", msg->Data.ui);
        }
    }
    else if (MsgIsString(msg))
    {
        if (!strcmp(MsgGetString(msg), "clear"))
        {
            MtrTempClear();
            printf("Command clear temp\r\n");
        }
        MsgReleaseString(msg);
    }
    else
    {
        return -1;
    }
    return 0;
}

void USART3_IRQHandler(void)
{
    __HAL_UART_CLEAR_OREFLAG(&huart3);
    if (__HAL_UART_GET_IT(&huart3, UART_IT_RXNE))
    {
        __HAL_UART_CLEAR_IT(&huart3, UART_IT_RXNE);
        MtrSetState((MtrState_t)(USART3->RDR & 0xFF));
    }
}

int CbRelay(VariantDef *msg, int speakId)
{
    if (MsgIsString(msg))
    {
        const char *cmd = MsgGetString(msg);
        if (!strcmp(cmd, "WcOpen"))          WC_ENABLE;
        else if (!strcmp(cmd, "WcClose"))    WC_DISABLE;
        else if (!strcmp(cmd, "LedOpen"))    LED_ENABLE;
        else if (!strcmp(cmd, "LedClose"))   LED_DISABLE;
        MsgReleaseString(msg);
    }
    else
    {
        return -1;
    }
    return 0;
}

int CbLightModeSwitch(VariantDef *msg, int speakId)
{
    if (MsgIsUint(msg) || MsgIsInt(msg))
    {
        if (!MsgGetUint(msg))
        {
            LightModeAuto();
            printf("Auto mode\r\n");
        }
        else
        {
            LightModeManual();
            printf("Manual mode\r\n");
        }
    }
    else
    {
        return -1;
    }
    return 0;
}

int CbUavOperation(VariantDef *msg, int speakId)
{
    if (MsgIsString(msg))
    {
        const char *cmd = MsgGetString(msg);
        if      (!strcmp(cmd, "CabinDoorOpen"))        { UavCabinDoorOpen(); printf("Cabin door open\r\n"); }
        else if (!strcmp(cmd, "CabinDoorClose"))       { UavCabinDoorClose(); printf("Cabin door close\r\n"); }
        else if (!strcmp(cmd, "LocalOpen"))            { UavLocalOpen(); printf("Fixture open\r\n"); }
        else if (!strcmp(cmd, "LocalClose"))           { UavLocalClose(); printf("Fixture close\r\n"); }
        else if (!strcmp(cmd, "WirelessChargingOpen")) { UavWcOpen(); printf("WC open\r\n"); }
        else if (!strcmp(cmd, "WirelessChargingClose")){ UavWcClose(); printf("WC close\r\n"); }
        else if (!strcmp(cmd, "UavUp"))                { UavUp(); printf("Uav up\r\n"); }
        else if (!strcmp(cmd, "UavDown"))              { UavDown(); printf("Uav down\r\n"); }
        else if (!strcmp(cmd, "UavSleep"))             { UavSleep(); printf("Uav sleep\r\n"); }
        else if (!strcmp(cmd, "UavUnlock"))            { UavUnlock(); printf("Uav unlock\r\n"); }
        MsgReleaseString(msg);
    }
    else
    {
        return -1;
    }
    return 0;
}
