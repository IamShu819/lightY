#include "App.h"

#define PUSHER_IN1_PORT     GPIOD
#define PUSHER_IN1_PIN      GPIO_PIN_14
#define PUSHER_IN2_PORT     GPIOD
#define PUSHER_IN2_PIN      GPIO_PIN_15

#define PLATFORM_IN1_PORT   GPIOD
#define PLATFORM_IN1_PIN    GPIO_PIN_11
#define PLATFORM_IN2_PORT   GPIOD
#define PLATFORM_IN2_PIN    GPIO_PIN_10

/* ---------- Scheduler task IDs ---------- */
static SchedId_t g_WeatherTaskId;
static SchedId_t g_WindTaskId;
static SchedId_t g_DistanceTaskId;
static SchedId_t g_LightAutoTaskId;
static SchedId_t g_SensorRetryTaskId;

/* ---------- Task callbacks ---------- */
static void WeatherTaskEntry(void *arg)
{
    if (WeatherUpdate() == 0)
    {
        WeatherData_t *w = WeatherGetData();
        printf("[Weather] Temp:%.1fC Humi:%.1f%% PM2.5:%u PM10:%u Lux:%lu\r\n",
               w->Temp, w->Humi, w->PM25, w->PM10, w->Lux);
    }
}

static void WindTaskEntry(void *arg)
{
    if (WindUpdate() == 0)
    {
        WindData_t *w = WindGetData();
        printf("[Wind] Speed:%.2f m/s Dir:%.2f deg\r\n", w->Speed, w->Dir);
    }
}

static void DistanceTaskEntry(void *arg)
{
    DistanceUpdate();
}

static void LightAutoTaskEntry(void *arg)
{
    LightAutoModeHandle();
}

static void SensorRetryTaskEntry(void *arg)
{
    SensorRetryPoll();
}

/* ---------- SOS button state machine ---------- */
typedef enum {
    SosIdle,
    SosDebouncing,
    SosLocked
} SosState_t;

static SosState_t g_SosState = SosIdle;
static uint32_t g_SosTick = 0;

static void SosSendEvent(void)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "JSON:{\"type\":\"sos\"}\n");
    extern UART_HandleTypeDef huart2;
    pSerial(&huart2, buf);
    printf("SOS sent\r\n");
}

static void SysTask(void)
{
    uint8_t level = (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_1) == GPIO_PIN_SET) ? 1 : 0;

    switch (g_SosState)
    {
        case SosIdle:
            if (level)
            {
                g_SosTick = HAL_GetTick();
                g_SosState = SosDebouncing;
            }
            break;

        case SosDebouncing:
            if (!level)
            {
                g_SosState = SosIdle;
            }
            else if (HAL_GetTick() - g_SosTick > 30)
            {
                SosSendEvent();
                g_SosState = SosLocked;
            }
            break;

        case SosLocked:
            if (!level)
                g_SosState = SosIdle;
            break;
    }
}

/* ---------- Init / Main ---------- */
void Init(void)
{
    DebugInit();
    SchedulerInit();
    ModbusRtu->MasterInit();

    /* Sensors */
    WeatherGetStatus();
    WindGetStatus();
    DistanceInit();

    /* I2C sensors — disabled due to pin conflict with Platform motor (PD10/PD11) */
    /* Ina219Init(); */

    /* UnReport (JSON commands on USART2) */
    UnReportInit();

    /* UAV */
    UavInit();

    /* Light */
    LightInit();
    LightEnable();
    LightModeManual();
    LightSetBright(0);

    /* Scheduler tasks */
    g_WeatherTaskId = SchedulerCreate(900, 900, WeatherTaskEntry, NULL);
    SchedulerSetState(g_WeatherTaskId, SCHED_ENABLE);

    g_WindTaskId = SchedulerCreate(1200, 1200, WindTaskEntry, NULL);
    SchedulerSetState(g_WindTaskId, SCHED_ENABLE);

    g_DistanceTaskId = SchedulerCreate(100, 100, DistanceTaskEntry, NULL);
    SchedulerSetState(g_DistanceTaskId, SCHED_ENABLE);

    g_LightAutoTaskId = SchedulerCreate(500, 500, LightAutoTaskEntry, NULL);
    SchedulerSetState(g_LightAutoTaskId, SCHED_ENABLE);

    g_SensorRetryTaskId = SchedulerCreate(3000, 3000, SensorRetryTaskEntry, NULL);
    SchedulerSetState(g_SensorRetryTaskId, SCHED_ENABLE);

    /* Report timer (TIM15) */
    ReportInit();

    /* Pusher / Platform GPIO initial state */
    HAL_GPIO_WritePin(PUSHER_IN1_PORT, PUSHER_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PUSHER_IN2_PORT, PUSHER_IN2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PLATFORM_IN1_PORT, PLATFORM_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PLATFORM_IN2_PORT, PLATFORM_IN2_PIN, GPIO_PIN_RESET);

    printf("Init complete\r\n");
}

void Main(void)
{
    while (1)
    {
        SysTask();
        SchedulerPoll();

        if (g_ReportFlag)
        {
            g_ReportFlag = 0;
            ReportSensor();
        }

        UnReport();
    }
}

/* ---------- Pusher / Platform ---------- */
void SetPusherState(PusherState_t state)
{
    switch (state)
    {
        case PUSHER_EXTEND:
            HAL_GPIO_WritePin(PUSHER_IN1_PORT, PUSHER_IN1_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(PUSHER_IN2_PORT, PUSHER_IN2_PIN, GPIO_PIN_RESET);
            printf("Pusher EXTENDING\r\n");
            break;
        case PUSHER_RETRACT:
            HAL_GPIO_WritePin(PUSHER_IN1_PORT, PUSHER_IN1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(PUSHER_IN2_PORT, PUSHER_IN2_PIN, GPIO_PIN_SET);
            printf("Pusher RETRACTING\r\n");
            break;
        case PUSHER_STOP:
        default:
            HAL_GPIO_WritePin(PUSHER_IN1_PORT, PUSHER_IN1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(PUSHER_IN2_PORT, PUSHER_IN2_PIN, GPIO_PIN_RESET);
            printf("Pusher STOPPED\r\n");
            break;
    }
}

void SetPlatformState(PlatformState_t state)
{
    switch (state)
    {
        case PLATFORM_UP:
            HAL_GPIO_WritePin(PLATFORM_IN1_PORT, PLATFORM_IN1_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(PLATFORM_IN2_PORT, PLATFORM_IN2_PIN, GPIO_PIN_RESET);
            printf("Platform RISING\r\n");
            break;
        case PLATFORM_DOWN:
            HAL_GPIO_WritePin(PLATFORM_IN1_PORT, PLATFORM_IN1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(PLATFORM_IN2_PORT, PLATFORM_IN2_PIN, GPIO_PIN_SET);
            printf("Platform DOWN\r\n");
            break;
        case PLATFORM_STOP:
        default:
            HAL_GPIO_WritePin(PLATFORM_IN1_PORT, PLATFORM_IN1_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(PLATFORM_IN2_PORT, PLATFORM_IN2_PIN, GPIO_PIN_RESET);
            printf("Platform STOPPED\r\n");
            break;
    }
}
