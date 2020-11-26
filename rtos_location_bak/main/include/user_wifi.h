

#ifndef __USER_WIFI_
#define __USER_WIFI_

#include "esp_system.h"
#include "user_main.h"

#define  WIFI_AP_MODE        (0) //TRUE:AP FALSE:STA
#define  WIFI_SSID           CONFIG_WIFI_SSID
#define  WIFI_PASS           CONFIG_WIFI_PASSWORD
#define  MAX_STA_CONN        4
#define  AP_MAX_NUMBER       100

#if 1
#define  HOST_IP_ADDR        "114.67.117.254" 
#define  PORT                12018
#else
#define  HOST_IP_ADDR        "192.168.1.23" 
#define  PORT                8266
#endif

#define  FREQ_START          2412 // M
#define  SEGMEMTATION        5    // M

#pragma  pack(push)
#pragma  pack(1)

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
    int32_t length;
}tApInfo;

#pragma  pack(pop)

void wifi_init();
void wifi_init_softap(uint8_t *pSsid, uint8_t *pPass);
void wifi_init_sta();
void wifi_scan();
uint16_t chnTofreq(uint8_t channel);

extern tApInfo aucApInfo;


#endif
