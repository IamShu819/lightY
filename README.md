# lightY — 智慧灯杆控制系统

基于 STM32H750VBT6 (Cortex-M7, 480MHz) 的环境监测与智能照明控制系统。

## 硬件资源

| 外设 | 用途 |
|------|------|
| USART1 | Debug Shell (NR_SHELL) |
| USART2 | 上位机通信 (JSON 报文) |
| USART3 | 卷帘门状态反馈 |
| UART4 | HMI 屏幕 (pScreen) |
| UART5 | UAV Lora 通信 |
| UART6 | UAV 舱门/本地控制 |
| I2C (软件) | INA219 电流检测, BH1750 光照 |
| TIM1 | PWM 灯光调光 |
| TIM15 | 定时上报 (100ms 周期) |
| RS485 (Modbus RTU) | 气象站(0x01), 风速仪(0x02), 测距×4 |

## 目录结构

```
D:\lightY\
├── Core\           STM32CubeMX 生成文件 (main.c, stm32h7xx_it.c ...)
├── Drivers\        HAL 库 / CMSIS
├── App\            应用层 (App, Scheduler)
├── Hardware\       硬件驱动 (Modbus, Light, Debug, I2c, Msbase, Rgb, MtrDrv)
├── Sensor\         传感器 (Weather, Wind, Distance, Ina219, Bh1750, SensorCommon, SensorRetry)
├── Comm\           通信协议 (Report, Unreport, CbReport)
├── UAV\            无人机接口 (Uav)
├── Utils\          工具库 (Queue, CharStack, Tool, cJSON)
├── NR_SHELL\       第三方命令行 Shell
└── MDK-ARM\        Keil 工程文件
```

## 快速开始

- **STM32CubeIDE**: 导入 `D:\lightY` 目录，直接 Build
- **Keil MDK**: 打开 `MDK-ARM\lightY.uvprojx`，直接编译

---

# API 参考手册

## App — 应用层

### App.h

主应用入口和板级宏定义。

```c
void Init(void);   // 初始化所有外设、传感器、调度器任务
void Main(void);   // 主循环 (永不返回)
```

**GPIO 快捷宏:**

| 宏 | 说明 |
|----|------|
| `WC_ENABLE` / `WC_DISABLE` | 厕灯继电器 |
| `LED_ENABLE` / `LED_DISABLE` | 照明继电器 |
| `KEY_PC13` / `KEY_PC14` | 按键读取 (低电平有效) |

**推杆 / 升降平台:**

```c
typedef enum { PUSHER_STOP, PUSHER_EXTEND, PUSHER_RETRACT } PusherState_t;
typedef enum { PLATFORM_STOP, PLATFORM_UP, PLATFORM_DOWN } PlatformState_t;

void SetPusherState(PusherState_t state);
void SetPlatformState(PlatformState_t state);
```

**调度器任务列表 (Init 中创建):**

| 任务 | 周期 | 功能 |
|------|------|------|
| WeatherTask | 900ms | 读取气象站数据 |
| WindTask | 1200ms | 读取风速风向 |
| DistanceTask | 100ms | 读取 4 路测距 |
| LightAutoTask | 500ms | 自动调光逻辑 |
| PowerTask | 2000ms | 读取 INA219 功率 |
| SensorRetryTask | 3000ms | 传感器掉线重连 |

---

## Scheduler — 协作式任务调度器

静态数组实现，O(1) 查找，无 malloc。最多 32 个任务。

```c
void SchedulerInit(void);

// 创建任务，返回任务 ID (>=0)，失败返回 -1
// period: 执行周期(ms), delay: 首次延迟(ms)
SchedId_t SchedulerCreate(uint32_t period, uint32_t delay,
                          void (*entry)(void *arg), void *arg);

int SchedulerDelete(SchedId_t id);
int SchedulerSetState(SchedId_t id, SchedState_t state);  // SCHED_ENABLE / SCHED_DISABLE
int SchedulerSetPeriod(SchedId_t id, uint32_t period);
void SchedulerPoll(void);  // 在 main loop 中调用
```

**示例:**

```c
SchedId_t id = SchedulerCreate(1000, 0, MyTaskEntry, NULL);
SchedulerSetState(id, SCHED_ENABLE);
```

---

## Hardware — 硬件驱动

### Modbus — Modbus RTU 主站

非阻塞状态机实现，波特率自动缓存。

```c
typedef struct {
    uint16_t (*Crc16)(const uint8_t *buf, uint16_t len);
    int (*ReadHoldingRegs03)(uint8_t slaveAddr, uint16_t regAddr,
                             uint16_t numRegs, uint16_t *respBuf, uint32_t baudRate);
    int (*WriteSingleRegister06)(uint8_t slaveAddr, uint16_t regAddr,
                                 uint16_t value, uint32_t baudRate);
    int (*WriteMultipleRegisters16)(uint8_t slaveAddr, uint16_t regAddr,
                                    uint16_t numRegs, uint16_t *values, uint32_t baudRate);
    void (*MasterInit)(void);
} ModbusRtu_t;

extern ModbusRtu_t *ModbusRtu;
```

**使用:**

```c
ModbusRtu->MasterInit();
uint16_t buf[2];
int ret = ModbusRtu->ReadHoldingRegs03(0x01, 500, 2, buf, 4800);
// ret == 0 成功, ret == -1 超时/错误
```

**辅助函数:**

```c
HAL_StatusTypeDef ModbusSendFrame(uint8_t *frame, uint16_t len);
ModbusBusStatus_t ModbusGetBusStatus(void);  // ModbusIdle / ModbusBusy
```

### Light — PWM 灯光控制

整数运算，无浮点。

```c
void LightInit(void);
void LightEnable(void);
void LightDisable(void);
void LightSetBright(int target);     // 0~1000
int  LightGetBright(void);
void LightSetCompare(uint16_t compare);
void LightModeAuto(void);
void LightModeManual(void);
int  LightGetMode(void);             // LIGHT_MODE_AUTO / LIGHT_MODE_MANUAL
uint8_t LightGetState(void);

// 自动模式处理 (由调度器每 500ms 调用)
void LightAutoModeHandle(void);
```

**自动调光逻辑:**
1. 光照 (Lux) 低于 `LIGHT_OPEN_LUX`(250) 时开灯
2. 光照高于 `LIGHT_CLOSE_LUX`(280) 时关灯
3. 根据距离传感器判断是否有人靠近，动态调整亮度
4. EMA 滤波消除传感器抖动

### Debug — 调试输出

```c
void DebugInit(void);

// 格式化发送 (静态 buffer，无 malloc)
int pSerial(UART_HandleTypeDef *huart, const char *format, ...);

// HMI 屏幕快捷宏
pScreen("value=%d", val);   // → USART4
pReport("temp=%.1f", temp); // → USART4

// 日志宏
LOG_BASE(fmt, ...);
ERROR_LOG(fmt, ...);
DEBUG_LOG(fmt, ...);
```

`printf` 重定向到 USART1 (Debug 串口)。

### I2c — 软件 I2C

DWT->CYCCNT 周期精确延时，不占用定时器。

```c
extern I2cBasicTools_t *I2cBus;  // 底层: Start/Stop/SndByte/RcvByte
extern I2cStdTools_t   *I2cStd;  // 高层: WriteData/ReadData

void I2cDelay_Us(uint16_t us);
```

**I2cStd 用法:**

```c
uint8_t data[2];
I2cStd->WriteData(0x46, 0x10, data, 2);      // 写 2 字节
I2cStd->ReadData(0x46, 0x10, data, 2);       // 读 2 字节
```

### Msbase — 微秒/毫秒延时

```c
void Delay_Us(uint16_t us);   // DWT 周期精确延时
void Delay_Ms(uint32_t ms);   // 基于 Delay_Us
```

### Rgb — RGB LED 控制

```c
typedef enum {
    RgbBlack, RgbRed, RgbGreen, RgbBlue,
    RgbYellow, RgbPurple, RgbCyan, RgbWhite
} RgbColor_t;

void RgbControl(RgbColor_t color);
RgbColor_t RgbGetColor(void);
```

### MtrDrv — 电机驱动 (卷帘门)

```c
typedef enum {
    MtrStateOpen      = 0x01,
    MtrStateOpening   = 0x02,
    MtrStateClose     = 0x04,
    MtrStateClosing   = 0x08,
    MtrLocalCompleted = (MtrStateOpen | MtrStateClose)  // 0x05
} MtrState_t;

void MtrControl(int opn);           // 1=开, 0=关
void MtrTempClear(void);
void MtrSetState(MtrState_t state);
MtrState_t MtrGetState(void);
```

---

## Sensor — 传感器

### SensorCommon — 传感器公共组件

错误计数、重试注册、EMA 滤波。

```c
#define SENSOR_FAILED_THRESHOLD  4

typedef enum { SensorError = 0, SensorNormal = 1 } SensorState_t;
typedef struct {
    uint8_t FailedCount;
    SensorState_t State;
} SensorStatus_t;

uint16_t SensorBaudRateToReg(uint32_t baud);
int SensorReadWithRetry(uint8_t addr, uint16_t reg, uint16_t *data, uint32_t baud);
void SensorCheckAndRegisterRetry(SensorStatus_t *status, uint8_t addr,
                                 uint32_t baud, uint16_t reg);
// EMA 滤波: shift 控制平滑度 (值越大越平滑)
uint16_t SensorFilterEma(uint16_t newValue, uint16_t *filteredValue, uint8_t shift);
```

### Weather — 气象站 (Modbus 0x01, 4800bps)

```c
typedef struct {
    float    Humi;   // 湿度 %
    float    Temp;   // 温度 °C
    uint16_t PM25;   // PM2.5 μg/m³
    uint16_t PM10;   // PM10  μg/m³
    uint32_t Lux;    // 光照度 lux
} WeatherData_t;

int WeatherUpdate(void);              // 0=成功
WeatherData_t *WeatherGetData(void);
SensorStatus_t *WeatherGetStatus(void);
```

### Wind — 风速风向仪 (Modbus 0x02, 9600bps)

```c
typedef struct {
    float Speed;  // 风速 m/s
    float Dir;    // 风向 °
} WindData_t;

int WindUpdate(void);
WindData_t *WindGetData(void);
SensorStatus_t *WindGetStatus(void);
```

### Distance — 测距传感器 ×4 (Modbus, 各自独立地址)

```c
typedef enum { DisFront = 0, DisRear, DisLeft, DisRight, DisCount } DisChannel_t;

typedef struct {
    uint8_t         ModbusAddr;
    uint16_t        Data;
    uint32_t        BaudRate;
    SensorStatus_t  Status;
} DisDev_t;

void DistanceInit(void);
void DistanceUpdate(void);
DisDev_t *DistanceGetPoint(DisChannel_t ch);
```

### Ina219 — 电流/功率检测 (I2C)

两个 INA219: 灯光电源 (`0x40`) 和 车载电源 (`0x45`)。

```c
typedef struct {
    float Voltage;  // V
    float Elec;     // A
    float Pow;      // W
} Ina219Data_t;

extern Ina219Data_t g_CarPower;
extern Ina219Data_t g_LightPower;

int Ina219Init(void);
int Ina219GetLightData(float *vol, float *elec, float *pow);
int Ina219GetCarData(float *vol, float *elec, float *pow);
```

### Bh1750 — 光照度传感器 (I2C, 地址 0x46)

```c
int Bh1750Start(void);    // 启动测量
float Bh1750Read(void);   // 读取 lux 值
```

### SensorRetry — 传感器掉线重连

静态数组，最多 8 个注册项。由调度器每 3000ms 轮询。

```c
int SensorRetryRegister(uint8_t addr, uint32_t baud, uint16_t reg, SensorStatus_t *status);
void SensorRetryPoll(void);
```

---

## Comm — 通信协议

### Report — 定时上报 (USART2)

TIM15 ISR 仅设置 `g_ReportFlag = 1`，main loop 中执行实际拼包发送。零 malloc，使用 snprintf 拼 JSON。

```c
extern volatile uint8_t g_ReportFlag;

int ReportInit(void);       // 启动 TIM15
void ReportSensor(void);    // 拼包并发送所有传感器数据
```

**上报格式 (USART2, 115200bps):**

```json
{"type":"sensor","temp":25.3,"humi":60.2,"pm25":35,"pm10":42,
 "lux":1200,"wind_speed":3.5,"wind_dir":180.0,
 "dis_front":1200,"dis_rear":1500,"dis_left":800,"dis_right":900,
 "light_voltage":12.1,"light_current":2.3,"light_power":27.8,
 "car_voltage":24.5,"car_current":1.2,"car_power":29.4}
```

### Unreport — 指令解析 (USART2)

接收 JSON 指令，cJSON 解析，回调分发。

```c
int UnReportInit(void);
int UnReport(void);                  // 主循环调用
Queue_t *UnReportGetFIFO(void);
UnReportCode_t UnReportGetTargetEx(VariantDef *variant, const char *txt);
```

**支持的指令码:**

| Code | 枚举名 | 功能 |
|------|--------|------|
| 0 | `CbReportSetLightGearCode` | 设置灯光亮度 |
| 1 | `CbReportSetLightSwitchCode` | 灯光开关 |
| 2 | `CbReportFixSensorErrorCode` | 传感器错误修复 |
| 3 | `CbReportRollingDoorCode` | 卷帘门控制 |
| 4 | `CbRelayCode` | 继电器控制 |
| 5 | `CbLightModeSwitchCode` | 灯光模式切换 |
| 6 | `UavOperationCode` | UAV 操作 |
| 7 | `CbReportSetPusherCode` | 推杆控制 |

### CbReport — 指令回调

```c
int CbReportSetLightGear(VariantDef *msg, int speakId);
int CbReportSetPusher(VariantDef *msg, int speakId);
int CbReportSetPlatform(VariantDef *msg, int speakId);
int CbReportLightSwitch(VariantDef *msg, int speakId);
int CbReportFixSensorError(VariantDef *msg, int speakId);
int CbReportRollingDoor(VariantDef *msg, int speakId);
int CbRelay(VariantDef *msg, int speakId);
int CbLightModeSwitch(VariantDef *msg, int speakId);
int CbUavOperation(VariantDef *msg, int speakId);
```

---

## UAV — 无人机接口

Lora 帧通信 (UART5) + 舱门控制 (UART6)。

```c
void UavInit(void);
```

**下行指令 (UART5, Lora 帧):**

| 函数 | 功能 |
|------|------|
| `UavUp()` | 起飞 |
| `UavDown()` | 降落 |
| `UavUnlock()` | 解锁 |
| `UavSleep()` | 休眠 |

**舱门控制 (UART6, 文本协议):**

| 函数 | 功能 |
|------|------|
| `UavCabinDoorOpen()` | 开舱门 |
| `UavCabinDoorClose()` | 关舱门 |
| `UavLocalOpen()` | 本地开 |
| `UavLocalClose()` | 本地关 |
| `UavWcOpen()` | 厕所开 |
| `UavWcClose()` | 厕所关 |

**传感器数据 (UART5 中断接收):**

```c
extern const volatile Jy90Time_t      *Jy90TimePtr;      // 时间
extern const volatile Jy90AccSpeed_t  *Jy90AccSpeedPtr;  // 加速度
extern const volatile Jy90AngSpeed_t  *Jy90AngSpeedPtr;  // 角速度
extern const volatile Jy90Angle_t     *Jy90AnglePtr;     // 欧拉角
extern const volatile Jy90FourNum_t   *Jy90FourNumPtr;   // 四元数
extern const volatile Bmp388Data_t    *Bmp388DataPtr;    // 气压高度温度
```

---

## Utils — 工具库

### Queue — 环形队列

动态分配，线程不安全 (仅在 main loop 使用)。

```c
Queue_t *QueueCreate(uint32_t size);
int QueueDelete(Queue_t *queue);
int QueueEnqueue(Queue_t *queue, uint8_t data);
int QueueDequeue(Queue_t *queue, uint8_t *data);
int QueueEnString(Queue_t *queue, const char *str);  // 入队整个字符串
void QueueClear(Queue_t *queue);
int QueueIsEmpty(Queue_t *queue);
int QueueIsFull(Queue_t *queue);
uint32_t QueueCount(Queue_t *queue);
```

### CharStack — 字符栈 / 动态字符串

```c
CharStack_t *CharStackCreate(size_t size, const char *name);
int CharStackPush(CharStack_t *stack, uint8_t data);
int CharStackPull(CharStack_t *stack, uint8_t *data);
int CharStackDelete(CharStack_t *stack);

// String 操作 (基于 CharStack)
String_t StringCreate(unsigned slen, const char *name);
char *StringGetString(String_t str);
int StringGetLen(String_t str);
int StringAddChar(String_t str, const char ch);
int StringAddString(String_t str, const char *ch, size_t nbest);
int StringDelChar(String_t str, char *ch);
int StringDelFrontChar(String_t str);
int StringDelEndNChar(String_t str, unsigned len, char *del_data);
int StringInsertString(String_t str, size_t index, const char *pstr);
int StringDelIndexChar(String_t str, size_t index, char *data);
void StringAddStringFormat(String_t str, const char *format, ...);
void StringClean(String_t str);
void StringFree(String_t str);
int StringFilterKeepNumber(String_t str);
int StringFilterKeepAlpha(String_t str);
int StringFilterKeepList(String_t str, const char *keep_list, size_t list_len);
int StringFilterDelList(String_t str, const char *del_list, size_t list_len);
```

### Tool — 通用类型 (Variant)

类型安全的联合体，用于指令参数传递。

```c
typedef enum {
    TYPE_NONE = -1, TYPE_UNINIT = 0,
    TYPE_INT = 1, TYPE_UINT = 2, TYPE_FLOAT = 3, TYPE_STRING = 4
} VariantTypeDef;

typedef struct {
    VariantTypeDef Type;
    union { int i; unsigned int ui; float f; char *s; } Data;
} VariantDef;

VariantStatusTypeDef VariantCreateString(VariantDef *variant, const char *str);
VariantStatusTypeDef VariantToInt(VariantDef *variant, int value);
VariantStatusTypeDef VariantToUint(VariantDef *variant, unsigned int value);
VariantStatusTypeDef VariantToFloat(VariantDef *variant, float value);
VariantStatusTypeDef VariantToString(VariantDef *variant, const char *pValue);
VariantStatusTypeDef VariantGetTypeData(VariantDef *variant, void *pValue);

bool MsgIsUint(VariantDef *msg);
bool MsgIsInt(VariantDef *msg);
bool MsgIsFloat(VariantDef *msg);
bool MsgIsString(VariantDef *msg);
const char *MsgGetString(VariantDef *msg);
int MsgGetInt(VariantDef *msg);
int MsgGetUint(VariantDef *msg);
int MsgGetFloat(VariantDef *msg);
int MsgReleaseString(VariantDef *msg);
```

---

## 命名规范

| 类型 | 规范 | 示例 |
|------|------|------|
| 公共函数 | PascalCase | `ModbusReadHoldingRegs` |
| 静态函数 | PascalCase + 模块前缀 | `ModbusSetBaudRate` |
| 宏 | UPPER_CASE | `MODBUS_MAX_BUF` |
| 类型 | PascalCase + `_t` | `WeatherData_t` |
| 全局变量 | `g_` + PascalCase | `g_WeatherData` |
| 文件名 | PascalCase.c | `Modbus.c`, `Scheduler.c` |

---

## 主循环流程

```
main() → HAL_Init → SystemClock_Config → MX_GPIO_Init → ...
       → Init()
         → DebugInit()          // USART1 shell, printf 重定向
         → SchedulerInit()      // 静态数组清零
         → ModbusRtu->MasterInit()
         → 传感器初始化 (Weather, Wind, Distance, Ina219)
         → UnReportInit()       // USART2 JSON 指令接收
         → UavInit()            // UART5/6
         → LightInit() + LightEnable() + LightModeManual()
         → 创建 7 个调度器任务
         → ReportInit()         // TIM15 定时上报
       → Main()
         → while(1):
             SysTask()           // SOS 按键状态机
             SchedulerPoll()     // 执行到期任务
             if g_ReportFlag: ReportSensor()
             UnReport()          // 处理入站指令
```
