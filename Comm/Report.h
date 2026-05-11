#ifndef __REPORT_H_
#define __REPORT_H_

#include "stm32h7xx_hal.h"

extern volatile uint8_t g_ReportFlag;

int ReportInit(void);
void ReportSensor(void);
void SendDataToLubanCat(void);

#endif
