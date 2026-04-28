#include "Weather.h"
#include "Modbus.h"
#include "Debug.h"

static SensorStatus_t g_WeatherStatus = {0, SensorNormal};
static WeatherData_t g_WeatherData;

SensorStatus_t *WeatherGetStatus(void)
{
    return &g_WeatherStatus;
}

WeatherData_t *WeatherGetData(void)
{
    return &g_WeatherData;
}

int WeatherUpdate(void)
{
    WeatherData_t *w = &g_WeatherData;
    uint16_t humi, temp;

    if (g_WeatherStatus.State != SensorNormal) return -1;
    SensorCheckAndRegisterRetry(&g_WeatherStatus, WEATHER_MODBUS_ADDR, WEATHER_MODBUS_BAUD, WEATHER_REG_HUMI);
    if (g_WeatherStatus.State != SensorNormal) return -1;

    if (SensorReadWithRetry(WEATHER_MODBUS_ADDR, WEATHER_REG_HUMI, &humi, WEATHER_MODBUS_BAUD) <= 0)
    {
        g_WeatherStatus.FailedCount++;
        return -1;
    }
    w->Humi = (float)humi / 10.0f;

    if (SensorReadWithRetry(WEATHER_MODBUS_ADDR, WEATHER_REG_TEMP, &temp, WEATHER_MODBUS_BAUD) <= 0)
    {
        g_WeatherStatus.FailedCount++;
        return -1;
    }
    w->Temp = (float)temp / 10.0f;

    if (SensorReadWithRetry(WEATHER_MODBUS_ADDR, WEATHER_REG_PM25, &w->PM25, WEATHER_MODBUS_BAUD) <= 0)
    {
        g_WeatherStatus.FailedCount++;
        return -1;
    }

    if (SensorReadWithRetry(WEATHER_MODBUS_ADDR, WEATHER_REG_PM10, &w->PM10, WEATHER_MODBUS_BAUD) <= 0)
    {
        g_WeatherStatus.FailedCount++;
        return -1;
    }

    uint16_t luxH, luxL;
    if (SensorReadWithRetry(WEATHER_MODBUS_ADDR, WEATHER_REG_LUXH, &luxH, WEATHER_MODBUS_BAUD) <= 0)
    {
        g_WeatherStatus.FailedCount++;
        return -1;
    }
    if (SensorReadWithRetry(WEATHER_MODBUS_ADDR, WEATHER_REG_LUXL, &luxL, WEATHER_MODBUS_BAUD) <= 0)
    {
        g_WeatherStatus.FailedCount++;
        return -1;
    }
    w->Lux = ((uint32_t)luxH << 16) | luxL;

    g_WeatherStatus.FailedCount = 0;
    return 0;
}
