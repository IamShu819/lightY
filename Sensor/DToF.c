#ifdef DToF_ENABLE

#include "DToF.h"
#include "Debug.h"
#include "Msbase.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/*
 * UART assignment — change these to match your hardware wiring.
 * USART2/3/4/5 are used by default. Adjust if dToF uses different UARTs.
 */
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;

static Queue_t *DToF_Queue[DToFChCount];
static volatile uint8_t DToF_RcvData;
static uint8_t DToF_InitFlag = 0;
static DToFRcvStreamFrame_t DToF_Frame[DToFChCount];

static const int DToF_CmdList[] = {
    DToF_CMD_START_STREAM,
    DToF_CMD_STOP_STREAM,
    DToF_CMD_VERSION,
    DToF_CMD_SET_BAUD_RATE,
    DToF_CMD_GET_BAUD_RATE,
    DToF_CMD_SET_I2C_ADDR,
    DToF_CMD_GET_I2C_ADDR,
    DToF_CMD_SET_FREQ,
    DToF_CMD_GET_FREQ
};

#define DToF_CH_SEND(__CH__, __DATA__, __LEN__) do { \
    switch (__CH__) { \
        case DToFCh0: HAL_UART_Transmit(&huart2, __DATA__, __LEN__, HAL_MAX_DELAY); break; \
        case DToFCh1: HAL_UART_Transmit(&huart3, __DATA__, __LEN__, HAL_MAX_DELAY); break; \
        case DToFCh2: HAL_UART_Transmit(&huart4, __DATA__, __LEN__, HAL_MAX_DELAY); break; \
        case DToFCh3: HAL_UART_Transmit(&huart5, __DATA__, __LEN__, HAL_MAX_DELAY); break; \
    } \
} while(0)

static UART_HandleTypeDef *DToF_GetUart(DToFChannel_t ch)
{
    switch (ch) {
        case DToFCh0: return &huart2;
        case DToFCh1: return &huart3;
        case DToFCh2: return &huart4;
        case DToFCh3: return &huart5;
        default: return NULL;
    }
}

uint16_t DToF_Crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

static int DToF_IsCmd(uint8_t cmd)
{
    for (uint8_t i = 0; i < sizeof(DToF_CmdList) / sizeof(DToF_CmdList[0]); i++) {
        if (cmd == DToF_CmdList[i])
            return 0;
    }
    return -1;
}

static int DToF_StreamCtl(DToFChannel_t ch, DToFCtl_t ctl)
{
    uint8_t cmd[9] = {0xA5, 0x03, 0x20,
                      (ctl == DToFStreamStart) ? 0x01 : 0x02,
                      0x00, 0x00, 0x00};
    uint16_t crc = DToF_Crc16(cmd, 7);
    cmd[7] = (uint8_t)(crc >> 8);
    cmd[8] = (uint8_t)crc;

    DToF_CH_SEND(ch, cmd, sizeof(cmd));
    UART_HandleTypeDef *huart = DToF_GetUart(ch);
    if (huart)
        while (__HAL_UART_GET_FLAG(huart, UART_FLAG_TC) != SET);
    return 0;
}

static int DToF_AnalysisStream(Queue_t *q, DToFRcvStreamFrame_t *frame)
{
#define TAKE_TWO_BYTE(__TARGET__) do { \
    QueueDequeue(q, &Data); __TARGET__ = ((uint16_t)Data << 8); \
    QueueDequeue(q, &Data); __TARGET__ |= (uint16_t)Data; \
} while(0)

    uint8_t Data;
    if (!q || !frame) return -1;
    DToFRcvStreamFrame_t backup = *frame;

    while (!QueueIsEmpty(q)) {
        QueueDequeue(q, &Data);
        if (Data != DToF_PACKET_HEAD) continue;

        QueueDequeue(q, &Data);
        if (Data != DToF_DEVICE_NUM) continue;

        QueueDequeue(q, &Data);
        if (Data != DToF_DEVICE_TYPE) continue;

        QueueDequeue(q, &Data);
        if (DToF_IsCmd(Data) < 0) continue;
        uint8_t Cmd = Data;

        QueueDequeue(q, &Data);
        if (Data != 0) continue;

        uint16_t DataLen = 0;
        TAKE_TWO_BYTE(DataLen);

        TAKE_TWO_BYTE(frame->MinorTargetDis);
        TAKE_TWO_BYTE(frame->MinorTargetCorrect);
        TAKE_TWO_BYTE(frame->MinorTargetStrength);

        /* MainTargetDis: LSB first */
        QueueDequeue(q, &Data);
        frame->MainTargetDis = (uint16_t)Data;
        QueueDequeue(q, &Data);
        frame->MainTargetDis |= ((uint16_t)Data << 8);

        TAKE_TWO_BYTE(frame->MainTargetCorrect);
        TAKE_TWO_BYTE(frame->MainTargetStrength);
        TAKE_TWO_BYTE(frame->SunBase);

        uint8_t unfold[] = {
            DToF_PACKET_HEAD, DToF_DEVICE_NUM, DToF_DEVICE_TYPE, Cmd, 0x00,
            DataLen >> 8, DataLen,
            frame->MinorTargetDis >> 8, frame->MinorTargetDis,
            frame->MinorTargetCorrect >> 8, frame->MinorTargetCorrect,
            frame->MinorTargetStrength >> 8, frame->MinorTargetStrength,
            frame->MainTargetDis, frame->MainTargetDis >> 8,
            frame->MainTargetCorrect >> 8, frame->MainTargetCorrect,
            frame->MainTargetStrength >> 8, frame->MainTargetStrength,
            frame->SunBase >> 8, frame->SunBase
        };

        uint16_t Crc, nCrc;
        TAKE_TWO_BYTE(Crc);
        nCrc = DToF_Crc16(unfold, sizeof(unfold));

        if (nCrc != Crc) {
            *frame = backup;
        } else {
            QueueClear(q);
            return 0;
        }
    }
    return 0;
}

Queue_t *DToFGetQueue(DToFChannel_t ch)
{
    if (ch < DToFChCount) return DToF_Queue[ch];
    return NULL;
}

DToFRcvStreamFrame_t *DToFGetData(DToFChannel_t ch)
{
    if (ch < DToFChCount) return &DToF_Frame[ch];
    return NULL;
}

int DToFInit(void)
{
    for (int i = 0; i < DToFChCount; i++) {
        DToF_Queue[i] = QueueCreate(64);
        if (!DToF_Queue[i]) return -1;
    }

    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart4, UART_IT_RXNE);
    __HAL_UART_ENABLE_IT(&huart5, UART_IT_RXNE);

    HAL_NVIC_SetPriority(USART2_IRQn, 2, 2);
    HAL_NVIC_SetPriority(USART3_IRQn, 2, 2);
    HAL_NVIC_SetPriority(UART4_IRQn, 2, 2);
    HAL_NVIC_SetPriority(UART5_IRQn, 2, 2);

    HAL_NVIC_EnableIRQ(USART2_IRQn);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    HAL_NVIC_EnableIRQ(UART4_IRQn);
    HAL_NVIC_EnableIRQ(UART5_IRQn);

    DToF_InitFlag = 1;
    printf("DToF Init ok\r\n");
    return 0;
}

void DToFUpdate(void)
{
    uint32_t timeout;
    for (int ch = 0; ch < DToFChCount; ch++) {
        timeout = 0;
        DToF_StreamCtl((DToFChannel_t)ch, DToFStreamStart);
        while (QueueIsEmpty(DToF_Queue[ch]) && DToF_Queue[ch]->occupy < 20) {
            timeout++;
            delay_ms(1);
            if (timeout >= 200) {
                ERROR_LOG("DToF Ch%d start stream timeout", ch);
                break;
            }
        }
        if (timeout < 200)
            DToF_AnalysisStream(DToF_Queue[ch], &DToF_Frame[ch]);
    }
}

/*
 * ISR handlers — these will CONFLICT with Unreport.c (USART2/UART4)
 * and Uav.c (UART5) if dToF shares the same UARTs.
 *
 * When dToF is assigned dedicated UARTs, uncomment the corresponding
 * handlers and update DToF_GetUart() and DToF_CH_SEND() accordingly.
 */
/*
void USART2_IRQHandler(void)
{
    if (__HAL_UART_GET_IT(&huart2, UART_IT_RXNE)) {
        __HAL_UART_CLEAR_OREFLAG(&huart2);
        HAL_UART_Receive(&huart2, (uint8_t *)&DToF_RcvData, 1, 0xFFFF);
        QueueEnqueue(DToF_Queue[DToFCh0], DToF_RcvData);
        __HAL_UART_CLEAR_IT(&huart2, UART_IT_RXNE);
    }
}
void USART3_IRQHandler(void)
{
    if (__HAL_UART_GET_IT(&huart3, UART_IT_RXNE)) {
        __HAL_UART_CLEAR_OREFLAG(&huart3);
        HAL_UART_Receive(&huart3, (uint8_t *)&DToF_RcvData, 1, 0xFFFF);
        QueueEnqueue(DToF_Queue[DToFCh1], DToF_RcvData);
        __HAL_UART_CLEAR_IT(&huart3, UART_IT_RXNE);
    }
}
void UART4_IRQHandler(void)
{
    if (__HAL_UART_GET_IT(&huart4, UART_IT_RXNE)) {
        __HAL_UART_CLEAR_OREFLAG(&huart4);
        HAL_UART_Receive(&huart4, (uint8_t *)&DToF_RcvData, 1, 0xFFFF);
        QueueEnqueue(DToF_Queue[DToFCh2], DToF_RcvData);
        __HAL_UART_CLEAR_IT(&huart4, UART_IT_RXNE);
    }
}
void UART5_IRQHandler(void)
{
    if (__HAL_UART_GET_IT(&huart5, UART_IT_RXNE)) {
        __HAL_UART_CLEAR_OREFLAG(&huart5);
        HAL_UART_Receive(&huart5, (uint8_t *)&DToF_RcvData, 1, 0xFFFF);
        QueueEnqueue(DToF_Queue[DToFCh3], DToF_RcvData);
        __HAL_UART_CLEAR_IT(&huart5, UART_IT_RXNE);
    }
}
*/

#endif /* DToF_ENABLE */
