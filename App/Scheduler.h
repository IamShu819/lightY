#ifndef __SCHEDULER_H_
#define __SCHEDULER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"

#define SCHEDULER_MAX_TASKS     32
#define SCHEDULER_TICK          HAL_GetTick()

typedef int SchedId_t;

typedef enum {
    SCHED_DISABLE = 0,
    SCHED_ENABLE  = 1
} SchedState_t;

typedef struct {
    uint32_t Period;
    uint32_t Deadline;
    void (*Entry)(void *arg);
    void *Arg;
    uint8_t Active;
    uint8_t Run;
} SchedTask_t;

void SchedulerInit(void);
SchedId_t SchedulerCreate(uint32_t period, uint32_t delay, void (*entry)(void *arg), void *arg);
int SchedulerDelete(SchedId_t id);
int SchedulerSetState(SchedId_t id, SchedState_t state);
int SchedulerSetPeriod(SchedId_t id, uint32_t period);
int SchedulerSetEntry(SchedId_t id, void (*entry)(void *arg));
int SchedulerSetArg(SchedId_t id, void *arg);
int SchedulerExecute(SchedId_t id);
void SchedulerPoll(void);

__attribute__((weak)) void SysTask(void *arg);

#ifdef __cplusplus
}
#endif

#endif
