#ifndef __APP_H_
#define __APP_H_

#include "stm32h7xx_hal.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "Debug.h"
#include "Scheduler.h"
#include "Modbus.h"
#include "Report.h"
#include "Weather.h"
#include "Wind.h"
#include "Distance.h"
#include "SensorRetry.h"
#include "SensorCommon.h"
#include "Light.h"
#include "MtrDrv.h"
#include "Rgb.h"
#include "I2c.h"
#include "Ina219.h"
#include "Bh1750.h"
#include "Queue.h"
#include "Tool.h"
#include "Unreport.h"
#include "Uav.h"
#include "nr_micro_shell.h"
#include "GpioMacros.h"

void Init(void);
void Main(void);

/* Pusher / Platform */
typedef enum {
    PUSHER_STOP,
    PUSHER_EXTEND,
    PUSHER_RETRACT
} PusherState_t;

typedef enum {
    PLATFORM_STOP,
    PLATFORM_UP,
    PLATFORM_DOWN
} PlatformState_t;

void SetPusherState(PusherState_t state);
void SetPlatformState(PlatformState_t state);

#endif
