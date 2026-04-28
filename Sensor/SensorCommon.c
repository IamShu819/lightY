#include "SensorCommon.h"
#include "SensorRetry.h"
#include "Modbus.h"
#include "Debug.h"

uint16_t SensorBaudRateToReg(uint32_t baud)
{
    switch (baud)
    {
        case 2400:   return 0;
        case 4800:   return 1;
        case 9600:   return 2;
        case 19200:  return 3;
        case 38400:  return 4;
        case 57600:  return 5;
        case 115200: return 6;
        default:     return 0xFFFF;
    }
}

int SensorReadWithRetry(uint8_t addr, uint16_t reg, uint16_t *data, uint32_t baud)
{
    return ModbusRtu->ReadHoldingRegs03(addr, reg, 1, data, baud);
}

void SensorCheckAndRegisterRetry(SensorStatus_t *status, uint8_t addr, uint32_t baud, uint16_t reg)
{
    if (status->FailedCount >= SENSOR_FAILED_THRESHOLD)
    {
        status->State = SensorError;
        status->FailedCount = 0;
        SensorRetryRegister(addr, baud, reg, status);
    }
}

uint16_t SensorFilterEma(uint16_t newValue, uint16_t *filteredValue, uint8_t shift)
{
    if (shift == 0 || *filteredValue == 0)
    {
        *filteredValue = newValue;
        return newValue;
    }
    *filteredValue = (*filteredValue * ((1 << shift) - 1) + newValue) >> shift;
    return *filteredValue;
}
