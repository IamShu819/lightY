#include "LShell.h"
#include "nr_micro_shell.h"
#include "Debug.h"
#include "Light.h"
#include "Weather.h"
#include "Wind.h"
#include "Distance.h"
#include "Ina219.h"
#include "Bh1750.h"
#include "MtrDrv.h"
#include "Uav.h"
#include "SensorCommon.h"
#include "GpioMacros.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define SHELL_ARG(__ARGV__, __INDEX__) ((char *)((__ARGV__) + (unsigned char)(__ARGV__)[__INDEX__]))

/* ===== Helper: show sensor status ===== */
static void ShellShowSensorStatus(const char *name, SensorStatus_t *s)
{
    printf("%s: state=%s, failed=%d\r\n",
           name,
           (s->State == SensorNormal) ? "Normal" : "Error",
           s->FailedCount);
}

/* ===== Helper: show distance data for a channel ===== */
static void ShellShowDisData(DisChannel_t ch)
{
    static const char *chName[] = {"Front", "Rear", "Left", "Right"};
    DisDev_t *d = DistanceGetPoint(ch);
    if (d) {
        printf("%s: addr=%d, data=%u, status=%s, failed=%d\r\n",
               chName[ch], d->ModbusAddr, d->Data,
               (d->Status.State == SensorNormal) ? "Normal" : "Error",
               d->Status.FailedCount);
    }
}

/* ===== 1. light ===== */
static void shell_light(char argc, char *argv)
{
    if (argc < 2) {
        printf("Usage: light -d | -set <gear> | -pulse <val> | -auto | -manual\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    if (strcmp(opt, "-d") == 0) {
        printf("bright=%d, mode=%s\r\n", LightGetBright(),
               (LightGetMode() == LIGHT_MODE_AUTO) ? "Auto" : "Manual");
    } else if (strcmp(opt, "-set") == 0 && argc >= 3) {
        int gear = atoi(SHELL_ARG(argv, 2));
        LightSetBright(gear);
        printf("bright set to %d\r\n", gear);
    } else if (strcmp(opt, "-pulse") == 0 && argc >= 3) {
        uint16_t pulse = (uint16_t)atoi(SHELL_ARG(argv, 2));
        LightSetCompare(pulse);
        printf("pulse set to %u\r\n", pulse);
    } else if (strcmp(opt, "-auto") == 0) {
        LightModeAuto();
        printf("mode set to Auto\r\n");
    } else if (strcmp(opt, "-manual") == 0) {
        LightModeManual();
        printf("mode set to Manual\r\n");
    } else {
        printf("Unknown option: %s\r\n", opt);
    }
}

/* ===== 2. weather ===== */
static void shell_weather(char argc, char *argv)
{
    WeatherData_t *w = WeatherGetData();
    SensorStatus_t *s = WeatherGetStatus();

    if (argc < 2) {
        printf("Usage: weather -d | -humi | -temp | -lux | -pm2.5 | -pm10 | -s | -info | -h\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    if (strcmp(opt, "-d") == 0) {
        printf("Humi=%.1f, Temp=%.1f, Lux=%u, PM2.5=%u, PM10=%u\r\n",
               w->Humi, w->Temp, (unsigned)w->Lux, w->PM25, w->PM10);
    } else if (strcmp(opt, "-humi") == 0) {
        printf("Humi=%.1f\r\n", w->Humi);
    } else if (strcmp(opt, "-temp") == 0) {
        printf("Temp=%.1f\r\n", w->Temp);
    } else if (strcmp(opt, "-lux") == 0) {
        printf("Lux=%u\r\n", (unsigned)w->Lux);
    } else if (strcmp(opt, "-pm2.5") == 0) {
        printf("PM2.5=%u\r\n", w->PM25);
    } else if (strcmp(opt, "-pm10") == 0) {
        printf("PM10=%u\r\n", w->PM10);
    } else if (strcmp(opt, "-s") == 0) {
        ShellShowSensorStatus("Weather", s);
    } else if (strcmp(opt, "-info") == 0) {
        printf("Humi=%.1f, Temp=%.1f, Lux=%u, PM2.5=%u, PM10=%u\r\n",
               w->Humi, w->Temp, (unsigned)w->Lux, w->PM25, w->PM10);
        ShellShowSensorStatus("Weather", s);
    } else if (strcmp(opt, "-h") == 0) {
        printf("Usage: weather -d | -humi | -temp | -lux | -pm2.5 | -pm10 | -s | -info | -h\r\n");
    } else {
        printf("Unknown option: %s\r\n", opt);
    }
}

/* ===== 3. wind ===== */
static void shell_wind(char argc, char *argv)
{
    WindData_t *w = WindGetData();
    SensorStatus_t *s = WindGetStatus();

    if (argc < 2) {
        printf("Usage: wind -d | -speed | -dir | -s | -info | -h\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    if (strcmp(opt, "-d") == 0) {
        printf("Speed=%.2f, Dir=%.2f\r\n", w->Speed, w->Dir);
    } else if (strcmp(opt, "-speed") == 0) {
        printf("Speed=%.2f\r\n", w->Speed);
    } else if (strcmp(opt, "-dir") == 0) {
        printf("Dir=%.2f\r\n", w->Dir);
    } else if (strcmp(opt, "-s") == 0) {
        ShellShowSensorStatus("Wind", s);
    } else if (strcmp(opt, "-info") == 0) {
        printf("Speed=%.2f, Dir=%.2f\r\n", w->Speed, w->Dir);
        ShellShowSensorStatus("Wind", s);
    } else if (strcmp(opt, "-h") == 0) {
        printf("Usage: wind -d | -speed | -dir | -s | -info | -h\r\n");
    } else {
        printf("Unknown option: %s\r\n", opt);
    }
}

/* ===== 4. dis ===== */
static void shell_dis(char argc, char *argv)
{
    if (argc < 2) {
        printf("Usage: dis -left | -right | -front | -rear | -all [-d | -s | -info]\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    DisChannel_t ch = DisCount;
    bool all = false;

    if (strcmp(opt, "-front") == 0) ch = DisFront;
    else if (strcmp(opt, "-rear") == 0) ch = DisRear;
    else if (strcmp(opt, "-left") == 0) ch = DisLeft;
    else if (strcmp(opt, "-right") == 0) ch = DisRight;
    else if (strcmp(opt, "-all") == 0) all = true;
    else {
        printf("Unknown option: %s\r\n", opt);
        return;
    }

    if (all) {
        for (int i = 0; i < DisCount; i++)
            ShellShowDisData((DisChannel_t)i);
    } else {
        ShellShowDisData(ch);
    }
}

/* ===== 5. power ===== */
static void shell_power(char argc, char *argv)
{
    float v, e, p;

    if (argc < 2) {
        printf("Usage: power car | light | all\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    if (strcmp(opt, "car") == 0) {
        Ina219GetCarData(&v, &e, &p);
        printf("Car: V=%.2fV, I=%.2fA, P=%.2fW\r\n", v, e, p);
    } else if (strcmp(opt, "light") == 0) {
        Ina219GetLightData(&v, &e, &p);
        printf("Light: V=%.2fV, I=%.2fA, P=%.2fW\r\n", v, e, p);
    } else if (strcmp(opt, "all") == 0) {
        Ina219GetCarData(&v, &e, &p);
        printf("Car: V=%.2fV, I=%.2fA, P=%.2fW\r\n", v, e, p);
        Ina219GetLightData(&v, &e, &p);
        printf("Light: V=%.2fV, I=%.2fA, P=%.2fW\r\n", v, e, p);
    } else {
        printf("Unknown option: %s\r\n", opt);
    }
}

/* ===== 6. rollingdoor ===== */
static void shell_rollingdoor(char argc, char *argv)
{
    if (argc < 2) {
        printf("Usage: rollingdoor <number> | clear | -h\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    if (strcmp(opt, "-h") == 0) {
        printf("Usage: rollingdoor <number> | clear | -h\r\n");
    } else if (strcmp(opt, "clear") == 0) {
        MtrTempClear();
        printf("Motor temp cleared\r\n");
    } else {
        int num = atoi(opt);
        MtrControl(num);
        printf("Motor control: %d\r\n", num);
    }
}

/* ===== 7. relay ===== */
static void shell_relay(char argc, char *argv)
{
    if (argc < 2) {
        printf("Usage: relay -s | -h | wc <0|1> | led <0|1>\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    if (strcmp(opt, "-h") == 0) {
        printf("Usage: relay -s | -h | wc <0|1> | led <0|1>\r\n");
    } else if (strcmp(opt, "-s") == 0) {
        printf("WC=%d, LED=%d\r\n", WC_STATE, LED_STATE);
    } else if (strcmp(opt, "wc") == 0 && argc >= 3) {
        int val = atoi(SHELL_ARG(argv, 2));
        if (val) WC_ENABLE; else WC_DISABLE;
        printf("WC set to %d\r\n", val);
    } else if (strcmp(opt, "led") == 0 && argc >= 3) {
        int val = atoi(SHELL_ARG(argv, 2));
        if (val) LED_ENABLE; else LED_DISABLE;
        printf("LED set to %d\r\n", val);
    } else {
        printf("Unknown option: %s\r\n", opt);
    }
}

/* ===== 8. uav ===== */
static void shell_uav(char argc, char *argv)
{
    if (argc < 2) {
        printf("Usage: uav cabin-open | cabin-close | local-open | local-close | wc-open | wc-close | up | down\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    if (strcmp(opt, "cabin-open") == 0) {
        UavCabinDoorOpen();
        printf("Cabin door open\r\n");
    } else if (strcmp(opt, "cabin-close") == 0) {
        UavCabinDoorClose();
        printf("Cabin door close\r\n");
    } else if (strcmp(opt, "local-open") == 0) {
        UavLocalOpen();
        printf("Local open\r\n");
    } else if (strcmp(opt, "local-close") == 0) {
        UavLocalClose();
        printf("Local close\r\n");
    } else if (strcmp(opt, "wc-open") == 0) {
        UavWcOpen();
        printf("WC open\r\n");
    } else if (strcmp(opt, "wc-close") == 0) {
        UavWcClose();
        printf("WC close\r\n");
    } else if (strcmp(opt, "up") == 0) {
        UavUp();
        printf("UAV up\r\n");
    } else if (strcmp(opt, "down") == 0) {
        UavDown();
        printf("UAV down\r\n");
    } else {
        printf("Unknown option: %s\r\n", opt);
    }
}

/* ===== 9. uav-data ===== */
static void shell_uav_data(char argc, char *argv)
{
    if (argc < 2) {
        printf("Usage: uav-data -all | -acc | -ang | -angle | -quat | -bmp | -time | -h\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    if (strcmp(opt, "-h") == 0) {
        printf("Usage: uav-data -all | -acc | -ang | -angle | -quat | -bmp | -time | -h\r\n");
    } else if (strcmp(opt, "-all") == 0) {
        if (Jy90AccSpeedPtr)
            printf("Acc: %.3f %.3f %.3f\r\n", Jy90AccSpeedPtr->Ax, Jy90AccSpeedPtr->Ay, Jy90AccSpeedPtr->Az);
        if (Jy90AngSpeedPtr)
            printf("AngSpd: %.3f %.3f %.3f\r\n", Jy90AngSpeedPtr->Wx, Jy90AngSpeedPtr->Wy, Jy90AngSpeedPtr->Wz);
        if (Jy90AnglePtr)
            printf("Angle: %.2f %.2f %.2f\r\n", Jy90AnglePtr->Roll, Jy90AnglePtr->Pitch, Jy90AnglePtr->Yaw);
        if (Jy90FourNumPtr)
            printf("Quat: %.4f %.4f %.4f %.4f\r\n", Jy90FourNumPtr->q0, Jy90FourNumPtr->q1, Jy90FourNumPtr->q2, Jy90FourNumPtr->q3);
        if (Bmp388DataPtr)
            printf("BMP: P=%.2fPa, H=%.2fm, T=%.2fC\r\n", Bmp388DataPtr->Press, Bmp388DataPtr->High, Bmp388DataPtr->Temp);
        if (Jy90TimePtr)
            printf("Time: %02d-%02d-%02d %02d:%02d:%02d\r\n",
                   Jy90TimePtr->Year, Jy90TimePtr->Month, Jy90TimePtr->Day,
                   Jy90TimePtr->Hour, Jy90TimePtr->Minute, Jy90TimePtr->Second);
    } else if (strcmp(opt, "-acc") == 0) {
        if (Jy90AccSpeedPtr)
            printf("Acc: %.3f %.3f %.3f\r\n", Jy90AccSpeedPtr->Ax, Jy90AccSpeedPtr->Ay, Jy90AccSpeedPtr->Az);
    } else if (strcmp(opt, "-ang") == 0) {
        if (Jy90AngSpeedPtr)
            printf("AngSpd: %.3f %.3f %.3f\r\n", Jy90AngSpeedPtr->Wx, Jy90AngSpeedPtr->Wy, Jy90AngSpeedPtr->Wz);
    } else if (strcmp(opt, "-angle") == 0) {
        if (Jy90AnglePtr)
            printf("Angle: %.2f %.2f %.2f\r\n", Jy90AnglePtr->Roll, Jy90AnglePtr->Pitch, Jy90AnglePtr->Yaw);
    } else if (strcmp(opt, "-quat") == 0) {
        if (Jy90FourNumPtr)
            printf("Quat: %.4f %.4f %.4f %.4f\r\n", Jy90FourNumPtr->q0, Jy90FourNumPtr->q1, Jy90FourNumPtr->q2, Jy90FourNumPtr->q3);
    } else if (strcmp(opt, "-bmp") == 0) {
        if (Bmp388DataPtr)
            printf("BMP: P=%.2fPa, H=%.2fm, T=%.2fC\r\n", Bmp388DataPtr->Press, Bmp388DataPtr->High, Bmp388DataPtr->Temp);
    } else if (strcmp(opt, "-time") == 0) {
        if (Jy90TimePtr)
            printf("Time: %02d-%02d-%02d %02d:%02d:%02d\r\n",
                   Jy90TimePtr->Year, Jy90TimePtr->Month, Jy90TimePtr->Day,
                   Jy90TimePtr->Hour, Jy90TimePtr->Minute, Jy90TimePtr->Second);
    } else {
        printf("Unknown option: %s\r\n", opt);
    }
}

/* ===== 10. bh1750 ===== */
static void shell_bh1750(char argc, char *argv)
{
    if (argc < 2) {
        printf("Usage: bh1750 -d | -h\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    if (strcmp(opt, "-d") == 0) {
        float lux = Bh1750Read();
        printf("BH1750 Lux=%.1f\r\n", lux);
    } else if (strcmp(opt, "-h") == 0) {
        printf("Usage: bh1750 -d | -h\r\n");
    } else {
        printf("Unknown option: %s\r\n", opt);
    }
}

/* ===== 11. distance ===== */
static void shell_distance(char argc, char *argv)
{
    if (argc < 2) {
        printf("Usage: distance -d | -h\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    if (strcmp(opt, "-d") == 0) {
        for (int i = 0; i < DisCount; i++)
            ShellShowDisData((DisChannel_t)i);
    } else if (strcmp(opt, "-h") == 0) {
        printf("Usage: distance -d | -h\r\n");
    } else {
        printf("Unknown option: %s\r\n", opt);
    }
}

/* ===== 12. distanceUpdate ===== */
static void shell_distance_update(char argc, char *argv)
{
    if (argc < 2) {
        printf("Usage: distanceUpdate -up\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    if (strcmp(opt, "-up") == 0) {
        DistanceUpdate();
        printf("Distance updated\r\n");
    } else {
        printf("Unknown option: %s\r\n", opt);
    }
}

/* ===== 13. rolldoor ===== */
static void shell_rolldoor(char argc, char *argv)
{
    if (argc < 2) {
        printf("Usage: rolldoor -open | -close | <number>\r\n");
        return;
    }
    char *opt = SHELL_ARG(argv, 1);

    if (strcmp(opt, "-open") == 0) {
        MtrControl(MtrStateOpen);
        printf("Roll door open\r\n");
    } else if (strcmp(opt, "-close") == 0) {
        MtrControl(MtrStateClose);
        printf("Roll door close\r\n");
    } else {
        int num = atoi(opt);
        MtrControl(num);
        printf("Roll door control: %d\r\n", num);
    }
}

/* ===== Command table ===== */
/* Using NR_SHELL_CMD_EXPORT to place commands in the .rodata.nr_shell_cmd section.
   The section layout enforced by nr_micro_shell.c:
   [NR_SHELL_CMD_EXPORT_START] [cmd1] [cmd2] ... [cmd13] [NR_SHELL_CMD_EXPORT_END] */

NR_SHELL_CMD_EXPORT(light, shell_light);
NR_SHELL_CMD_EXPORT(weather, shell_weather);
NR_SHELL_CMD_EXPORT(wind, shell_wind);
NR_SHELL_CMD_EXPORT(dis, shell_dis);
NR_SHELL_CMD_EXPORT(power, shell_power);
NR_SHELL_CMD_EXPORT(rollingdoor, shell_rollingdoor);
NR_SHELL_CMD_EXPORT(relay, shell_relay);
NR_SHELL_CMD_EXPORT(uav, shell_uav);
NR_SHELL_CMD_EXPORT(uav_data, shell_uav_data);
NR_SHELL_CMD_EXPORT(bh1750, shell_bh1750);
NR_SHELL_CMD_EXPORT(distance, shell_distance);
NR_SHELL_CMD_EXPORT(distanceUpdate, shell_distance_update);
NR_SHELL_CMD_EXPORT(rolldoor, shell_rolldoor);

void LShellInit(void)
{
    shell_init();
}
