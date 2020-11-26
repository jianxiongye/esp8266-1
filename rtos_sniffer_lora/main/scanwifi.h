#ifndef __SCANWIFI_H
#define __SCANWIFI_H


#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"

extern void wifiscan(void);
void keepinfo();

#define TAG "sniffer"
void writeSniffDataToFlash(char* data );

#include "spi_flash.h"


#define FRAME_MANAGE   0
#define FRAME_CONTROL  1
#define FRAME_DATA     2
#define FRAME_RESERVED 3

#define SUBTYPE_ASSREQ 0   // 连接请求
#define SUBTYPE_ASSREP 1   // 连接响应
#define SUBTYPE_RESREQ 2   // 重连接请求
#define SUBTYPE_RESREP 3   // 重连接响应
#define SUBTYPE_PRBREQ 4   // 探测请求
#define SUBTYPE_PRBREP 5   // 探测响应
#define SUBTYPE_TIMEAD 6
#define SUBTYPE_RESERV 7
#define SUBTYPE_BEACON 8
#define SUBTYPE_ATIM   9
#define SUBTYPE_DISASS 10
#define SUBTYPE_AUTH   11  // 身份认证
#define SUBTYPE_DEAUTH 12
#define SUBTYPE_ACTION 13
#define SUBTYPE_ANOACK 14
#define SUBTYPE_RESER2 15

#define SUBTYPE_DATA 0
#define SUBTYPE_NULL_DATA 4
#define SUBTYPE_QOS_DATA 8
#define SUBTYPE_QOS_NULL_DATA 12 //0xc

#define DATA_DIR_TOAP   1
#define DATA_DIR_FROMAP 2

#define printmac(buf, i) os_printf("%02X:%02X:%02X:%02X:%02X:%02X", buf[i+0], buf[i+1], buf[i+2], \
                                                   buf[i+3], buf[i+4], buf[i+5])
#define PRINT(buf,i) buf[i+0], buf[i+1], buf[i+2], buf[i+3], buf[i+4], buf[i+5]

#define  AP_MAX_NUMBER  100

#pragma pack(push,1)
typedef unsigned char       uint8_t;
typedef signed char         int8_t;
typedef struct _ap_struct_Info_
{
    uint8_t mac[6];
    int8_t rssi;
    uint8_t channel;
    uint8_t ssid[33];
}tApStructInfo;

typedef struct _apInfo_
{
    tApStructInfo tinfoAp[AP_MAX_NUMBER];
    int8_t count;
}tApInfo;

#pragma pack(pop)


#define  VER_H  20
#define  VER_M  07
#define  VER_L  16

#if 0
#define  SLEEPTIME      (10 * 1000) // 10s //(60 * 60 * 1000) // 1h
#else
#define  SLEEPTIME      (60 * 60 * 1000) // 1h
#endif
#define  SENDLORACOUNT  1
#define  SLEEPMODLE     1

#if 0
#if CONFIG_EXAMPLE_POWER_SAVE_MIN_MODEM
#define DEFAULT_PS_MODE WIFI_PS_MIN_MODEM
#elif CONFIG_EXAMPLE_POWER_SAVE_MAX_MODEM
#define DEFAULT_PS_MODE WIFI_PS_MAX_MODEM
#elif CONFIG_EXAMPLE_POWER_SAVE_NONE
#define DEFAULT_PS_MODE WIFI_PS_NONE
#else
#define DEFAULT_PS_MODE WIFI_PS_NONE
#endif /*CONFIG_POWER_SAVE_MODEM*/
#endif
#define DEFAULT_PS_MODE WIFI_PS_NONE

#endif
