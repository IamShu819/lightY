#include "Wind.h"
#include "Modbus.h"
#include "Debug.h"

static SensorStatus_t g_WindStatus = {0, SensorNormal};
static WindData_t g_WindData;

SensorStatus_t *WindGetStatus(void)
{
    return &g_WindStatus;
}

WindData_t *WindGetData(void)
{
    return &g_WindData;
}

int WindUpdate(void)
{
    if (g_WindStatus.State != SensorNormal) return -1;
    SensorCheckAndRegisterRetry(&g_WindStatus, WIND_MODBUS_ADDR, WIND_MODBUS_BAUD, WIND_REG_SPEED);
    if (g_WindStatus.State != SensorNormal) return -1;

    /* Read speed + direction (2 consecutive registers: 0x0000, 0x0001) in one call */
    uint16_t buf[2];
    int ret = ModbusRtu->ReadHoldingRegs03(WIND_MODBUS_ADDR, WIND_REG_SPEED, 2, buf, WIND_MODBUS_BAUD);
    if (ret <= 0)
    {
        g_WindStatus.FailedCount++;
        return -1;
    }

    g_WindData.Speed = (float)buf[0] / 100.0f;
    g_WindData.Dir   = (float)buf[1] / 100.0f;

    g_WindStatus.FailedCount = 0;
    return 0;
}
