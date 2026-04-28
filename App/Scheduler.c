#include "Scheduler.h"
#include <string.h>
#include <stdio.h>

static SchedTask_t Tasks[SCHEDULER_MAX_TASKS];

void SchedulerInit(void)
{
    memset(Tasks, 0, sizeof(Tasks));
    printf("Scheduler Init ok\r\n");
}

SchedId_t SchedulerCreate(uint32_t period, uint32_t delay, void (*entry)(void *arg), void *arg)
{
    for (uint8_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        if (!Tasks[i].Active)
        {
            Tasks[i].Period  = period;
            Tasks[i].Deadline = SCHEDULER_TICK + delay;
            Tasks[i].Entry   = entry;
            Tasks[i].Arg     = arg;
            Tasks[i].Active  = 1;
            Tasks[i].Run     = SCHED_DISABLE;
            return i;
        }
    }
    return -1;
}

int SchedulerDelete(SchedId_t id)
{
    if (id < 0 || id >= SCHEDULER_MAX_TASKS || !Tasks[id].Active)
        return -1;
    Tasks[id].Active = 0;
    Tasks[id].Run = SCHED_DISABLE;
    return 0;
}

int SchedulerSetState(SchedId_t id, SchedState_t state)
{
    if (id < 0 || id >= SCHEDULER_MAX_TASKS || !Tasks[id].Active)
        return -1;
    Tasks[id].Run = state;
    return 0;
}

int SchedulerSetPeriod(SchedId_t id, uint32_t period)
{
    if (id < 0 || id >= SCHEDULER_MAX_TASKS || !Tasks[id].Active)
        return -1;
    Tasks[id].Period = period;
    return 0;
}

void SchedulerPoll(void)
{
    uint32_t now = SCHEDULER_TICK;
    for (uint8_t i = 0; i < SCHEDULER_MAX_TASKS; i++)
    {
        if (!Tasks[i].Active) continue;
        if (Tasks[i].Run != SCHED_ENABLE) continue;
        if (now >= Tasks[i].Deadline)
        {
            Tasks[i].Entry(Tasks[i].Arg);
            Tasks[i].Deadline = now + Tasks[i].Period;
        }
    }
}
