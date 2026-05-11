#include "App.h"
#include "EyeTest.h"
#include "LShell.h"

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
        SendDataToLubanCat();
    }
}

static void WindTaskEntry(void *arg)
{
    if (WindUpdate() == 0)
    {
        WindData_t *w = WindGetData();
        printf("[Wind] Speed:%.2f m/s Dir:%.2f deg\r\n", w->Speed, w->Dir);
        SendDataToLubanCat();
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

/* ---------- SOS button state machine ----------
 * PD1 (SOS) shares port D with Pusher (PD14/PD15) and Platform (PD10/PD11).
 * Motor GPIO toggles cause voltage glitches on PD1.
 * Counter-based majority filter (3/5) rejects single-sample noise.
 */
static void SosSendEvent(void)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "JSON:{\"type\":\"sos\"}\n");
    extern UART_HandleTypeDef huart2;
    pSerial(&huart2, buf);
    printf("SOS sent\r\n");
}

void SysTask(void *arg)
{
    static uint8_t sosLocked = 0;
    static uint8_t sosCounter = 0;
    static uint32_t sosPressTime = 0;

    uint8_t level = (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_1) == GPIO_PIN_SET) ? 1 : 0;

    /* Majority vote filter: increment on high, decrement on low */
    if (level)
    {
        if (sosCounter < 5)
            sosCounter++;
    }
    else
    {
        if (sosCounter > 0)
            sosCounter--;
    }

    /* Trigger only when 3+ of last samples agree on high */
    if (sosCounter >= 3)
    {
        if (!sosLocked)
        {
            if (sosPressTime == 0)
            {
                sosPressTime = HAL_GetTick();
            }
            else if (HAL_GetTick() - sosPressTime > 30)
            {
                SosSendEvent();
                sosLocked = 1;
                sosPressTime = 0;
            }
        }
    }
    else if (sosCounter == 0)
    {
        sosLocked = 0;
        sosPressTime = 0;
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

    /* I2C sensors (PB0=SDA, PC5=SCL) */
    Ina219Init();

    /* UnReport (JSON commands on USART2) */
    UnReportInit();

    /* UAV */
    UavInit();

    /* Shell commands */
    LShellInit();

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

    /* USART6 eye diagram test (15 Mbps DMA) */
    EyeTestStart();

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
        SysTask(NULL);
        SchedulerPoll();

        if (g_ReportFlag)
        {
            g_ReportFlag = 0;
            ReportSensor();
        }

        if (g_UnReportFlag)
        {
            g_UnReportFlag = 0;
            UnReport();
        }
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
