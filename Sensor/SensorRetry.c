#include "SensorRetry.h"
#include "Modbus.h"
#include "Debug.h"

static SensorRetryEntry_t RetryTable[SENSOR_RETRY_MAX];

int SensorRetryRegister(uint8_t addr, uint32_t baud, uint16_t reg, SensorStatus_t *status)
{
    for (int i = 0; i < SENSOR_RETRY_MAX; i++)
    {
        if (!RetryTable[i].Active)
        {
            RetryTable[i].ModbusAddr = addr;
            RetryTable[i].ModbusBaudRate = baud;
            RetryTable[i].TestReadReg = reg;
            RetryTable[i].Status = status;
            RetryTable[i].Active = 1;
            return 0;
        }
    }
    return -1;
}

void SensorRetryPoll(void)
{
    for (int i = 0; i < SENSOR_RETRY_MAX; i++)
    {
        if (!RetryTable[i].Active) continue;

        SensorRetryEntry_t *e = &RetryTable[i];
        uint16_t data;
        int ret = ModbusRtu->ReadHoldingRegs03(e->ModbusAddr, e->TestReadReg, 1, &data, e->ModbusBaudRate);

        if (ret > 0)
        {
            pScreen("Addr 0x%02X retry OK", e->ModbusAddr);
            e->Status->FailedCount = 0;
            e->Status->State = SensorNormal;
            e->Active = 0;
        }
        else
        {
            pScreen("Addr 0x%02X retry failed", e->ModbusAddr);
        }
    }
}
