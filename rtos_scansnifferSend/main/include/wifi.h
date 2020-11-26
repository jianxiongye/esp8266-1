

#ifndef __USER_WIFI_
#define __USER_WIFI_

#include "sniffer_main.h"


#define  WIFI_AP_MODE        (0) //TRUE:AP FALSE:STA
#define  WIFI_SSID           CONFIG_ESP_WIFI_SSID
#define  WIFI_PASS           CONFIG_ESP_WIFI_PASSWORD
#define  MAX_STA_CONN        CONFIG_MAX_STA_CONN
#define  AP_MAX_NUMBER       100

#if 0
#define  HOST_IP_ADDR        CONFIG_UPDATA_SERVER_IP//"52.82.44.25" 
#define  PORT                CONFIG_UPDATA_SERVER_PORT//26200
#else
#define  HOST_IP_ADDR        "192.168.1.10" 
#define  PORT                8266
#endif

#define  OTA_UPDATA_MIX      18
#define  OTA_UPDATA_MAX      23



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
void tcp_client_task(void *pvParameters);
void send_head_data(void);
void send_body_data(void);
void create_tcp_task();
void create_udp_task();
void init_get_time(void);
extern tApInfo aucApInfo;


#endif
