#ifndef __UAV_H_
#define __UAV_H_

#include "stm32h7xx_hal.h"
#include "Debug.h"

#define UART_FIFO_SIZE  256

#define LORA_HEAD       0xAA

#define BMP388_DATA_REPORT              0x0001
#define JY901S_TIME_REPORT              0x0002
#define JY901S_ACC_REPORT               0x0003
#define JY901S_ANG_REPORT               0x0004
#define JY901S_ANGLE_REPORT             0x0005
#define JY901S_AIR_PRE_HIGHT_REPORT     0x0006
#define JY901S_FNUM_REPORT              0x0007

#define RCV_32BIT_INT_INDEX  sizeof(uint32_t)
#define RCV_16BIT_INT_INDEX  sizeof(uint16_t)

#define LORA_RCV_32BIT(p)  (((uint32_t)(p)[0] << 24) | ((uint32_t)(p)[1] << 16) | ((uint32_t)(p)[2] << 8) | (uint32_t)(p)[3])
#define LORA_RCV_16BIT(p)  (((uint16_t)(p)[0] << 8)  | (uint16_t)(p)[1])

typedef struct {
    uint8_t Year, Month, Day, Hour, Minute, Second;
} Jy90Time_t;

typedef struct {
    float Ax, Ay, Az;
} Jy90AccSpeed_t;

typedef struct {
    float Wx, Wy, Wz;
} Jy90AngSpeed_t;

typedef struct {
    float Roll, Pitch, Yaw;
} Jy90Angle_t;

typedef struct {
    float q0, q1, q2, q3;
} Jy90FourNum_t;

typedef struct {
    float Press;
    float High;
    float Temp;
} Bmp388Data_t;

typedef struct {
    uint8_t Head;
    uint8_t Id;
    uint8_t PassBackFlag;
    uint16_t Cmd;
    uint8_t Len;
    uint8_t *PayLoad;
    uint8_t Sum;
} LoraFrame_t;

typedef enum {
    CtlOpen  = 0x01,
    CtlClose = 0x02,
    CtlStop  = 0x04
} CtlState_t;

extern const volatile Jy90Time_t      *Jy90TimePtr;
extern const volatile Jy90AccSpeed_t  *Jy90AccSpeedPtr;
extern const volatile Jy90AngSpeed_t  *Jy90AngSpeedPtr;
extern const volatile Jy90Angle_t     *Jy90AnglePtr;
extern const volatile Jy90FourNum_t   *Jy90FourNumPtr;
extern const volatile Bmp388Data_t    *Bmp388DataPtr;

extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart6;

void UavInit(void);

static inline void UavCabinDoorOpen(void)  { pSerial(&huart6, "CabinDoorOpen"); }
static inline void UavCabinDoorClose(void) { pSerial(&huart6, "CabinDoorClose"); }
static inline void UavLocalOpen(void)      { pSerial(&huart6, "LocalOpen"); }
static inline void UavLocalClose(void)     { pSerial(&huart6, "LocalClose"); }
static inline void UavWcOpen(void)         { pSerial(&huart6, "UavWcOpen"); }
static inline void UavWcClose(void)        { pSerial(&huart6, "UavWcClose"); }

static inline void UavUp(void)
{
    const uint8_t frame[] = { 0xAA, 0x00, 0x01, 0x00, 0x08, 0x04, 0x66, 0x6c, 0x79, 0x00, 0x02 };
    HAL_UART_Transmit(&huart5, (uint8_t *)frame, sizeof(frame), 100);
}

static inline void UavDown(void)
{
    const uint8_t frame[] = { 0xAA, 0x00, 0x01, 0x00, 0x09, 0x05, 0x64, 0x6f, 0x77, 0x6e, 0x00, 0x71 };
    HAL_UART_Transmit(&huart5, (uint8_t *)frame, sizeof(frame), 100);
}

static inline void UavUnlock(void)
{
    const uint8_t frame[] = { 0xAA, 0x00, 0x01, 0x00, 0x0A, 0x07, 0x75, 0x6e, 0x6c, 0x6f, 0x63, 0x6b, 0x00, 0x48 };
    HAL_UART_Transmit(&huart5, (uint8_t *)frame, sizeof(frame), 100);
}

static inline void UavSleep(void)
{
    const uint8_t frame[] = { 0xAA, 0x00, 0x01, 0x00, 0x0B, 0x06, 0x73, 0x6c, 0x65, 0x65, 0x70, 0x00, 0xD5 };
    HAL_UART_Transmit(&huart5, (uint8_t *)frame, sizeof(frame), 100);
}

#endif
