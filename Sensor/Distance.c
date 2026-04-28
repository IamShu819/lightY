#include "Distance.h"
#include "Modbus.h"
#include "Debug.h"

static DisDev_t g_DisDev[DisCount];

static const uint8_t DisAddr[DisCount]  = {0x04, 0x05, 0x06, 0x07};
static const uint32_t DisBaud[DisCount] = {38400, 38400, 38400, 38400};

void DistanceInit(void)
{
    for (int i = 0; i < DisCount; i++)
    {
        g_DisDev[i].ModbusAddr = DisAddr[i];
        g_DisDev[i].BaudRate   = DisBaud[i];
        g_DisDev[i].Data       = 0;
        g_DisDev[i].Status.FailedCount = 0;
        g_DisDev[i].Status.State = SensorNormal;
    }
    printf("Distance Init ok\r\n");
}

DisDev_t *DistanceGetPoint(DisChannel_t ch)
{
    if (ch >= DisCount) return NULL;
    return &g_DisDev[ch];
}

void DistanceUpdate(void)
{
    static DisChannel_t current = DisFront;

    DisDev_t *dev = &g_DisDev[current];
    if (dev->Status.State == SensorNormal)
    {
        int ret = ModbusRtu->ReadHoldingRegs03(dev->ModbusAddr, DISTANCE_REG_FILTER_VALUE, 1, &dev->Data, dev->BaudRate);
        if (ret > 0)
        {
            dev->Status.FailedCount = 0;
        }
        else
        {
            dev->Status.FailedCount++;
            SensorCheckAndRegisterRetry(&dev->Status, dev->ModbusAddr, dev->BaudRate, DISTANCE_REG_FILTER_VALUE);
        }
    }
    current = (DisChannel_t)((current + 1) % DisCount);
}
