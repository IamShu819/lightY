#include "Uav.h"
#include "Queue.h"
#include "Ina219.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    Queue_t *Fifo;
    int RcvLen;
} UartRcv_t;

static volatile Jy90Time_t      g_Jy90Time;
static volatile Jy90AccSpeed_t  g_Jy90AccSpeed;
static volatile Jy90AngSpeed_t  g_Jy90AngSpeed;
static volatile Jy90Angle_t     g_Jy90Angle;
static volatile Jy90FourNum_t   g_Jy90FourNum;
static volatile Bmp388Data_t    g_Bmp388Data;

const volatile Jy90Time_t      *Jy90TimePtr     = &g_Jy90Time;
const volatile Jy90AccSpeed_t  *Jy90AccSpeedPtr = &g_Jy90AccSpeed;
const volatile Jy90AngSpeed_t  *Jy90AngSpeedPtr = &g_Jy90AngSpeed;
const volatile Jy90Angle_t     *Jy90AnglePtr    = &g_Jy90Angle;
const volatile Jy90FourNum_t   *Jy90FourNumPtr  = &g_Jy90FourNum;
const volatile Bmp388Data_t    *Bmp388DataPtr   = &g_Bmp388Data;

static UartRcv_t g_Uart5;
static UartRcv_t g_Uart6;

void UavInit(void)
{
    g_Uart5.Fifo = QueueCreate(256);
    g_Uart6.Fifo = QueueCreate(256);

    __HAL_UART_CLEAR_OREFLAG(&huart5);
    __HAL_UART_CLEAR_IT(&huart5, UART_IT_RXNE);
    __HAL_UART_CLEAR_IT(&huart5, UART_IT_IDLE);
    __HAL_UART_ENABLE_IT(&huart5, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart5, UART_IT_IDLE);
    HAL_NVIC_SetPriority(UART5_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(UART5_IRQn);

    __HAL_UART_CLEAR_OREFLAG(&huart6);
    __HAL_UART_CLEAR_IT(&huart6, UART_IT_RXNE);
    __HAL_UART_CLEAR_IT(&huart6, UART_IT_IDLE);
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
    HAL_NVIC_SetPriority(USART6_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(USART6_IRQn);

    printf("UAV Init ok\r\n");
}

static bool ComputerSum(const uint8_t *data, uint8_t *sum, uint16_t len)
{
    if (!data || !sum) return false;
    uint8_t s = 0;
    for (uint16_t i = 0; i < len; i++)
        s += data[i];
    *sum = s;
    return true;
}

static void Uart5AnalysisCb(void)
{
    LoraFrame_t frame;
    uint8_t data[g_Uart5.RcvLen];

    for (uint8_t i = 0; i < g_Uart5.RcvLen; i++)
        QueueDequeue(g_Uart5.Fifo, &data[i]);

    frame.Head         = data[0];
    frame.PassBackFlag = data[1];
    frame.Id           = data[2];
    frame.Cmd          = LORA_RCV_16BIT(&data[3]);
    frame.Len          = data[5];
    frame.PayLoad      = &data[6];
    ComputerSum(data, &frame.Sum, g_Uart5.RcvLen - 1);

    if (frame.Sum != data[g_Uart5.RcvLen - 1]) return;
    if (frame.Head != LORA_HEAD) return;

    switch (frame.Cmd)
    {
        case BMP388_DATA_REPORT:
            g_Bmp388Data.Press = (float)(int32_t)LORA_RCV_32BIT(frame.PayLoad) / 100.0f;
            g_Bmp388Data.Temp  = (float)(int32_t)LORA_RCV_32BIT(frame.PayLoad + RCV_32BIT_INT_INDEX) / 100.0f;
            g_Bmp388Data.High  = (float)(int32_t)LORA_RCV_32BIT(frame.PayLoad + 2 * RCV_32BIT_INT_INDEX) / 100.0f;
            break;

        case JY901S_TIME_REPORT:
            g_Jy90Time.Year   = frame.PayLoad[0];
            g_Jy90Time.Month  = frame.PayLoad[1];
            g_Jy90Time.Day    = frame.PayLoad[2];
            g_Jy90Time.Hour   = frame.PayLoad[3];
            g_Jy90Time.Minute = frame.PayLoad[4];
            g_Jy90Time.Second = frame.PayLoad[5];
            break;

        case JY901S_ACC_REPORT:
            sscanf((const char *)frame.PayLoad, "Ax:%f,Ay:%f,Az:%f",
                   &g_Jy90AccSpeed.Ax, &g_Jy90AccSpeed.Ay, &g_Jy90AccSpeed.Az);
            break;

        case JY901S_ANG_REPORT:
            sscanf((const char *)frame.PayLoad, "Wx:%f,Wy:%f,Wz:%f",
                   &g_Jy90AngSpeed.Wx, &g_Jy90AngSpeed.Wy, &g_Jy90AngSpeed.Wz);
            break;

        case JY901S_ANGLE_REPORT:
            sscanf((const char *)frame.PayLoad, "Roll:%f,Pitch:%f,Yaw:%f",
                   &g_Jy90Angle.Roll, &g_Jy90Angle.Pitch, &g_Jy90Angle.Yaw);
            break;

        case JY901S_FNUM_REPORT:
            sscanf((const char *)frame.PayLoad, "Q0:%f,Q1:%f,Q2:%f,Q3:%f",
                   &g_Jy90FourNum.q0, &g_Jy90FourNum.q1,
                   &g_Jy90FourNum.q2, &g_Jy90FourNum.q3);
            break;

        default:
            break;
    }
}

static void Uart6AnalysisCb(void)
{
    uint8_t data[g_Uart6.RcvLen];
    for (uint8_t i = 0; i < g_Uart6.RcvLen; i++)
        QueueDequeue(g_Uart6.Fifo, &data[i]);

    /* UAV side board reports are parsed here if needed */
}

void UART5_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart5, UART_FLAG_RXNE))
    {
        __HAL_UART_CLEAR_FLAG(&huart5, UART_FLAG_RXNE);
        if (g_Uart5.Fifo)
        {
            QueueEnqueue(g_Uart5.Fifo, (uint8_t)(UART5->RDR & 0xFF));
            g_Uart5.RcvLen++;
        }
    }

    if (__HAL_UART_GET_FLAG(&huart5, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart5);
        Uart5AnalysisCb();
        g_Uart5.RcvLen = 0;
    }

    if (__HAL_UART_GET_FLAG(&huart5, UART_FLAG_ORE))
        __HAL_UART_CLEAR_OREFLAG(&huart5);
}

void USART6_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_RXNE))
    {
        __HAL_UART_CLEAR_FLAG(&huart6, UART_FLAG_RXNE);
        if (g_Uart6.Fifo)
        {
            QueueEnqueue(g_Uart6.Fifo, (uint8_t)(USART6->RDR & 0xFF));
            g_Uart6.RcvLen++;
        }
    }

    if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart6);
        Uart6AnalysisCb();
        g_Uart6.RcvLen = 0;
    }

    if (__HAL_UART_GET_FLAG(&huart6, UART_FLAG_ORE))
        __HAL_UART_CLEAR_OREFLAG(&huart6);
}
