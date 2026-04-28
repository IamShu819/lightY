#ifndef __UNREPORT_H_
#define __UNREPORT_H_

#include "stm32h7xx_hal.h"
#include "Tool.h"
#include "Queue.h"

#define UNREPORT_BUF_SIZE  512

typedef enum {
    UnReportParseError  = -1,
    UnReportDataError   = -2,
    UnReportTypeError   = -3,
    UnReportTargetError = -4,

    CbReportSetLightGearCode    = 0,
    CbReportSetLightSwitchCode,
    CbReportFixSensorErrorCode,
    CbReportRollingDoorCode,
    CbRelayCode,
    CbLightModeSwitchCode,
    UavOperationCode,
    CbReportSetPusherCode
} UnReportCode_t;

typedef int (*UnReportCb_t)(VariantDef *param, int speakId);

typedef struct {
    const char *Target;
    UnReportCb_t Cb;
    VariantDef Param;
    UnReportCode_t Code;
    uint32_t SpeakId;
    uint8_t DefaultFlag;
} UnReportTarget_t;

int UnReportInit(void);
Queue_t *UnReportGetFIFO(void);
UnReportCode_t UnReportGetTargetEx(VariantDef *variant, const char *txt);
int UnReport(void);

#endif
