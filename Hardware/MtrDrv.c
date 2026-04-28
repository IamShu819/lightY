#include "MtrDrv.h"
#include "Debug.h"

static MtrState_t MtrState;

extern UART_HandleTypeDef huart3;

void MtrControl(int opn)
{
    if (opn == 1)
        pSerial(&huart3, "RollingDoorOpen");
    else if (opn == 0)
        pSerial(&huart3, "RollingDoorClose");
    else
        pSerial(&huart3, "RollingAngle:%d", opn);
}

void MtrTempClear(void)
{
    pSerial(&huart3, "RollingTempClear");
}

void MtrSetState(MtrState_t state)
{
    MtrState = state;
}

MtrState_t MtrGetState(void)
{
    return MtrState;
}
