#include "Modbus.h"
#include "Msbase.h"
#include "Debug.h"
#include "stdbool.h"

extern UART_HandleTypeDef huart7;

static ModbusBusStatus_t g_BusStatus = ModbusIdle;
static uint8_t RxBuf[MODBUS_MAX_BUF];
static uint16_t RxLen = 0;
static volatile bool FrameReady = false;
static uint32_t CurrentBaud = 0;

static void ModbusMasterInit(void);
static uint16_t ModbusCrc16(const uint8_t *buf, uint16_t len);
static int ModbusReadHoldingRegs03(uint8_t slaveAddr, uint16_t regAddr, uint16_t numRegs, uint16_t *respBuf, uint32_t baudRate);
static int ModbusWriteSingle06(uint8_t slaveAddr, uint16_t regAddr, uint16_t value, uint32_t baudRate);
static int ModbusWriteMultiple16(uint8_t slaveAddr, uint16_t regAddr, uint16_t numRegs, uint16_t *values, uint32_t baudRate);

static ModbusRtu_t ModbusRtuObj = {
    .Crc16                   = ModbusCrc16,
    .ReadHoldingRegs03       = ModbusReadHoldingRegs03,
    .WriteSingleRegister06   = ModbusWriteSingle06,
    .WriteMultipleRegisters16 = ModbusWriteMultiple16,
    .MasterInit              = ModbusMasterInit
};

ModbusRtu_t *ModbusRtu = &ModbusRtuObj;

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    if (huart->Instance == huart7.Instance)
    {
        RxLen = size;
        FrameReady = true;
    }
}

static HAL_StatusTypeDef ModbusSetBaudRate(uint32_t baud)
{
    if (baud == CurrentBaud) return HAL_OK;

    HAL_UART_DeInit(&huart7);
    huart7.Init.BaudRate   = baud;
    huart7.Init.WordLength = UART_WORDLENGTH_8B;
    huart7.Init.StopBits   = UART_STOPBITS_1;
    huart7.Init.Parity     = UART_PARITY_NONE;
    huart7.Init.Mode       = UART_MODE_TX_RX;
    huart7.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    if (HAL_UART_Init(&huart7) != HAL_OK) return HAL_ERROR;
    CurrentBaud = baud;
    FrameReady = false;
    __HAL_UART_CLEAR_IDLEFLAG(&huart7);
    HAL_UARTEx_ReceiveToIdle_IT(&huart7, RxBuf, MODBUS_MAX_BUF);
    return HAL_OK;
}

static void ModbusMasterInit(void)
{
    g_BusStatus = ModbusIdle;
    FrameReady = false;
    HAL_UARTEx_ReceiveToIdle_IT(&huart7, RxBuf, MODBUS_MAX_BUF);
}

static uint16_t ModbusCrc16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++)
    {
        crc ^= buf[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

HAL_StatusTypeDef ModbusSendFrame(uint8_t *frame, uint16_t len)
{
    FrameReady = false;
    __HAL_UART_CLEAR_IDLEFLAG(&huart7);
    HAL_UARTEx_ReceiveToIdle_IT(&huart7, RxBuf, MODBUS_MAX_BUF);
    HAL_StatusTypeDef st = HAL_UART_Transmit(&huart7, frame, len, 1000);
    while (!__HAL_UART_GET_FLAG(&huart7, UART_FLAG_TC));
    return st;
}

static int WaitForFrame(uint8_t *buf, uint16_t *len, uint32_t timeout)
{
    uint32_t start = HAL_GetTick();
    while (!FrameReady)
    {
        if ((HAL_GetTick() - start) >= timeout) return -1;
    }
    *len = RxLen;
    memcpy(buf, RxBuf, RxLen);
    FrameReady = false;
    __HAL_UART_CLEAR_IDLEFLAG(&huart7);
    HAL_UARTEx_ReceiveToIdle_IT(&huart7, RxBuf, MODBUS_MAX_BUF);
    return 0;
}

static int ModbusReadHoldingRegs03(uint8_t slaveAddr, uint16_t regAddr, uint16_t numRegs, uint16_t *respBuf, uint32_t baudRate)
{
    if (ModbusSetBaudRate(baudRate) != HAL_OK) return -100;
    g_BusStatus = ModbusBusy;

    uint8_t tx[8];
    tx[0] = slaveAddr;
    tx[1] = 0x03;
    tx[2] = regAddr >> 8;
    tx[3] = regAddr & 0xFF;
    tx[4] = numRegs >> 8;
    tx[5] = numRegs & 0xFF;
    uint16_t crc = ModbusCrc16(tx, 6);
    tx[6] = crc & 0xFF;
    tx[7] = crc >> 8;

    if (ModbusSendFrame(tx, 8) != HAL_OK) { g_BusStatus = ModbusIdle; return -101; }

    uint8_t rxBuf[MODBUS_MAX_BUF];
    uint16_t rxCount;
    if (WaitForFrame(rxBuf, &rxCount, MODBUS_TIMEOUT_MS) != 0) { g_BusStatus = ModbusIdle; return -1; }
    if (rxCount < 5) { g_BusStatus = ModbusIdle; return -2; }

    uint16_t respCrc = (rxBuf[rxCount - 1] << 8) | rxBuf[rxCount - 2];
    if (ModbusCrc16(rxBuf, rxCount - 2) != respCrc) { g_BusStatus = ModbusIdle; return -3; }
    if (rxBuf[0] != slaveAddr || rxBuf[1] != 0x03) { g_BusStatus = ModbusIdle; return -4; }
    if (rxBuf[2] != numRegs * 2) { g_BusStatus = ModbusIdle; return -5; }

    for (uint16_t i = 0; i < numRegs; i++)
        respBuf[i] = (rxBuf[3 + 2*i] << 8) | rxBuf[4 + 2*i];

    g_BusStatus = ModbusIdle;
    return numRegs;
}

static int ModbusWriteSingle06(uint8_t slaveAddr, uint16_t regAddr, uint16_t value, uint32_t baudRate)
{
    if (ModbusSetBaudRate(baudRate) != HAL_OK) return -100;
    g_BusStatus = ModbusBusy;

    uint8_t tx[8];
    tx[0] = slaveAddr;
    tx[1] = 0x06;
    tx[2] = regAddr >> 8;
    tx[3] = regAddr & 0xFF;
    tx[4] = value >> 8;
    tx[5] = value & 0xFF;
    uint16_t crc = ModbusCrc16(tx, 6);
    tx[6] = crc & 0xFF;
    tx[7] = crc >> 8;

    if (ModbusSendFrame(tx, 8) != HAL_OK) { g_BusStatus = ModbusIdle; return -101; }

    uint8_t rxBuf[8];
    uint16_t rxCount;
    if (WaitForFrame(rxBuf, &rxCount, MODBUS_TIMEOUT_MS) != 0) { g_BusStatus = ModbusIdle; return -1; }
    if (rxCount != 8) { g_BusStatus = ModbusIdle; return -2; }
    if (memcmp(tx, rxBuf, 6) != 0) { g_BusStatus = ModbusIdle; return -3; }
    uint16_t respC = (rxBuf[7] << 8) | rxBuf[6];
    if (ModbusCrc16(rxBuf, 6) != respC) { g_BusStatus = ModbusIdle; return -4; }

    g_BusStatus = ModbusIdle;
    return 0;
}

static int ModbusWriteMultiple16(uint8_t slaveAddr, uint16_t regAddr, uint16_t numRegs, uint16_t *values, uint32_t baudRate)
{
    if (numRegs == 0 || numRegs > 123) return -1;
    if (ModbusSetBaudRate(baudRate) != HAL_OK) return -100;
    g_BusStatus = ModbusBusy;

    uint16_t frameLen = 9 + 2 * numRegs;
    uint8_t tx[260];
    tx[0] = slaveAddr;
    tx[1] = 0x10;
    tx[2] = regAddr >> 8;
    tx[3] = regAddr & 0xFF;
    tx[4] = numRegs >> 8;
    tx[5] = numRegs & 0xFF;
    tx[6] = numRegs * 2;
    for (uint16_t i = 0; i < numRegs; i++)
    {
        tx[7 + 2*i] = values[i] >> 8;
        tx[8 + 2*i] = values[i] & 0xFF;
    }
    uint16_t crc = ModbusCrc16(tx, frameLen - 2);
    tx[frameLen - 2] = crc & 0xFF;
    tx[frameLen - 1] = crc >> 8;

    if (ModbusSendFrame(tx, frameLen) != HAL_OK) { g_BusStatus = ModbusIdle; return -101; }

    uint8_t rxBuf[8];
    uint16_t rxCount;
    if (WaitForFrame(rxBuf, &rxCount, MODBUS_TIMEOUT_MS) != 0) { g_BusStatus = ModbusIdle; return -2; }
    if (rxCount != 8) { g_BusStatus = ModbusIdle; return -3; }
    if (rxBuf[0] != slaveAddr || rxBuf[1] != 0x10) { g_BusStatus = ModbusIdle; return -4; }
    if (rxBuf[2] != tx[2] || rxBuf[3] != tx[3]) { g_BusStatus = ModbusIdle; return -5; }
    if (rxBuf[4] != tx[4] || rxBuf[5] != tx[5]) { g_BusStatus = ModbusIdle; return -6; }
    uint16_t respC2 = (rxBuf[7] << 8) | rxBuf[6];
    if (ModbusCrc16(rxBuf, 6) != respC2) { g_BusStatus = ModbusIdle; return -7; }

    g_BusStatus = ModbusIdle;
    return numRegs;
}

ModbusBusStatus_t ModbusGetBusStatus(void)
{
    return g_BusStatus;
}
