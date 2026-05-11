#ifndef __DTOF_H_
#define __DTOF_H_

#include "stm32h7xx_hal.h"
#include "Queue.h"
#include <stdlib.h>

/* Uncomment to enable dToF module */
/* #define DToF_ENABLE */

#define DToF_CMD_START_STREAM       0x01
#define DToF_CMD_STOP_STREAM        0x02
#define DToF_CMD_VERSION            0x0A
#define DToF_CMD_SET_BAUD_RATE      0x10
#define DToF_CMD_GET_BAUD_RATE      0x11
#define DToF_CMD_SET_I2C_ADDR       0x12
#define DToF_CMD_GET_I2C_ADDR       0x13
#define DToF_CMD_SET_FREQ           0x1A
#define DToF_CMD_GET_FREQ           0x1B

#define DToF_PACKET_HEAD     0xA5
#define DToF_DEVICE_NUM      0x03
#define DToF_DEVICE_TYPE     0x20

typedef enum {
    DToFStreamStart,
    DToFStreamStop
} DToFCtl_t;

typedef enum {
    DToFCh0 = 0,
    DToFCh1 = 1,
    DToFCh2 = 2,
    DToFCh3 = 3,
    DToFChCount = 4
} DToFChannel_t;

typedef struct {
    uint16_t MinorTargetDis;
    uint16_t MinorTargetCorrect;
    uint16_t MinorTargetStrength;
    uint16_t MainTargetDis;
    uint16_t MainTargetCorrect;
    uint16_t MainTargetStrength;
    uint16_t SunBase;
} DToFRcvStreamFrame_t;

typedef struct {
    uint8_t  PktHead;
    uint8_t  DevNum;
    uint8_t  DevType;
    uint8_t  Cmd;
    uint8_t  Retain;
    uint16_t Len;
    uint8_t *Data;
    uint16_t Crc16;
} DToFFrame_t;

uint16_t DToF_Crc16(const uint8_t *data, size_t len);

#ifdef DToF_ENABLE

int  DToFInit(void);
void DToFUpdate(void);
DToFRcvStreamFrame_t *DToFGetData(DToFChannel_t ch);
Queue_t *DToFGetQueue(DToFChannel_t ch);

#endif /* DToF_ENABLE */

#endif /* __DTOF_H_ */
