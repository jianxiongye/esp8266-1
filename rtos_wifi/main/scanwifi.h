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
#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"
#endif
 void 
writeSniffDataToFlash(char* data );

#include "spi_flash.h"


#define FRAME_MANAGE   0
#define FRAME_CONTROL  1
#define FRAME_DATA     2
#define FRAME_RESERVED 3

#define SUBTYPE_ASSREQ 0
#define SUBTYPE_ASSREP 1
#define SUBTYPE_RESREQ 2
#define SUBTYPE_RESREP 3
#define SUBTYPE_PRBREQ 4
#define SUBTYPE_PRBREP 5
#define SUBTYPE_TIMEAD 6
#define SUBTYPE_RESERV 7
#define SUBTYPE_BEACON 8
#define SUBTYPE_ATIM   9
#define SUBTYPE_DISASS 10
#define SUBTYPE_AUTH   11
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
#endif
