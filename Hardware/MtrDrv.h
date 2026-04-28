#ifndef __MTR_DRV_H_
#define __MTR_DRV_H_

#include "stm32h7xx_hal.h"

typedef enum {
    MtrStateOpen      = (0x01 << 0),
    MtrStateOpening   = (0x01 << 1),
    MtrStateClose     = (0x01 << 2),
    MtrStateClosing   = (0x01 << 3),
    MtrLocalCompleted = (MtrStateOpen | MtrStateClose)
} MtrState_t;

void MtrControl(int opn);
void MtrTempClear(void);
void MtrSetState(MtrState_t state);
MtrState_t MtrGetState(void);

#endif
