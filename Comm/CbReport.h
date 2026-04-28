#ifndef __CBREPORT_H_
#define __CBREPORT_H_

#include "stm32h7xx_hal.h"
#include "Tool.h"

int CbReportSetLightGear(VariantDef *msg, int speakId);
int CbReportSetPusher(VariantDef *msg, int speakId);
int CbReportSetPlatform(VariantDef *msg, int speakId);
int CbReportLightSwitch(VariantDef *msg, int speakId);
int CbReportFixSensorError(VariantDef *msg, int speakId);
int CbReportRollingDoor(VariantDef *msg, int speakId);
int CbRelay(VariantDef *msg, int speakId);
int CbLightModeSwitch(VariantDef *msg, int speakId);
int CbUavOperation(VariantDef *msg, int speakId);

#endif
