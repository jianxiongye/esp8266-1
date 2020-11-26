/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "wifi.h"
#include "user_nvs.h"
#include "esp_spi_flash.h"

#include "lwip/apps/sntp.h"
#include "user_ota.h"

static const char *TAG = "udp_client";

#define  SEND_BUF_LEN       4096
#define  SEND_FLAG          1
static char sendBuf[SEND_BUF_LEN] = {0};
static int sock = -1;
static struct sockaddr_in destAddr = {0};

#define  START_STNP_EN      1

#if START_STNP_EN

#define  TIME_LEN   20
char bufTime[TIME_LEN] = {0};

#endif

void send_udp_data(void)
{
    int i = 0, j = 0;
    
    int cycles = 0;
    int div = tInfoMisc.record / SEND_BODY_COUNT;
    int mod = tInfoMisc.record % SEND_BODY_COUNT;

    memset(sendBuf, 0, sizeof (sendBuf));
#if 0
    for (i = 0; i < tInfoMisc.record; i ++)
    {
        ESP_LOGI(TAG, "%s", nvsData + i * SNIFFER_DATA_LEN);
    }
#endif    
    ESP_LOGI(TAG, "----------------------------------");

#if START_STNP_EN
    strncpy(sendBuf, bufTime, TIME_LEN);
#endif

    for (i = 0; i <= div; i ++)
    {
        if (i != div)
        {
            cycles = SEND_BODY_COUNT;
        }
        else if (i == div && 0 != mod)
        {
            cycles = mod;
        }
        else if (i == div && 0 == mod)
        {
            cycles = 0;
            break;
        }

        //memset(sendBuf, 0, sizeof (sendBuf));

        for (j = 0; j < cycles; j ++)
        {
            //strncpy(sendBuf + j * SNIFFER_DATA_LEN, nvsData + i * SEND_BODY_COUNT * SNIFFER_DATA_LEN  + j * SNIFFER_DATA_LEN, SNIFFER_DATA_LEN);
            if (ENBLE == tInfoMisc.fifoFlag)
            {
                spi_flash_read(secB_addr + ((uint32_t)sizeof(des_addr) * (i * SEND_BODY_COUNT + j)), des_addr, (uint32_t)sizeof(des_addr));
            }
            else
            {
                memcpy(des_addr, nvsData + (i * SEND_BODY_COUNT + j) * SNIFFER_DATA_LEN, SNIFFER_DATA_LEN);
            }
#if START_STNP_EN
            strncpy(sendBuf + j * SNIFFER_DATA_LEN + TIME_LEN, des_addr, SNIFFER_DATA_LEN);
#else
            strncpy(sendBuf + j * SNIFFER_DATA_LEN, des_addr, SNIFFER_DATA_LEN);
#endif
            
            //vTaskDelay(1 / portTICK_PERIOD_MS);
        }
        
        //memset(fifo, 0, sizeof (fifo));
        //NVS_Read(create_key(i), sendBuf, cycles * SNIFFER_DATA_LEN);
#if 0
        ESP_LOGI(TAG, "----------%d------", i);
        for (j = 0; j < cycles; j ++)
            ESP_LOGI(TAG, "%s", sendBuf + j * SNIFFER_DATA_LEN);
#else
        //ESP_LOGI(TAG, "%s", sendBuf);
#endif
#if SEND_FLAG
        //int err = send(sock, sendBuf, cycles * SNIFFER_DATA_LEN, 0);
#if START_STNP_EN
        int err = sendto(sock, sendBuf, cycles * SNIFFER_DATA_LEN + TIME_LEN, 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
#else
        int err = sendto(sock, sendBuf, cycles * SNIFFER_DATA_LEN, 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
#endif
        if (err < 0) {
            ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "send %d data success by udp\r\n", i);
#endif
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    tInfoMisc.record = 0;
    memset(nvsData, 0, sizeof (nvsData));
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;

    while (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d)", retry);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
        retry ++;
        if (retry == 200)
        {
            esp_restart();
        }
    }
}

int record_ap_number()
{
    char fifo[70] = {0};
    int i = 0;
    int count = 0;
    memset(sendBuf, 0, sizeof (sendBuf));
    sprintf(fifo, "%02X:%02X:%02X:%02X:%02X:%02X\n", MAC2STR(staMacAddr));
    //ESP_LOGI(TAG, "count: %d, fifo len: %d, %s\n", count, strlen(fifo), fifo);
    memcpy(sendBuf, fifo, strlen(fifo));
    memset(fifo, 0, sizeof (fifo));
    for (i = 0; i < aucApInfo.count; i++) {
		sprintf(fifo, "%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%s\n", MAC2STR(aucApInfo.tinfoAp[i].mac), 
				aucApInfo.tinfoAp[i].rssi, aucApInfo.tinfoAp[i].channel, aucApInfo.tinfoAp[i].ssid);
        //ESP_LOGI(TAG, "count: %d, fifo len: %d, %s\n", count, strlen(fifo), fifo);
        memcpy(sendBuf + MAC_LEN + count, fifo, strlen(fifo));
        count += strlen(fifo);
	}
    ESP_LOGI(TAG, "count: %d \n", count);
    int err = sendto(sock, sendBuf, count + MAC_LEN, 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
    if (err < 0) {
        ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
    }
    vTaskDelay(150 / portTICK_PERIOD_MS);
    return count;
}

static void udp_client_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    char ip[32] = {0};
    uint16_t port = 0;
    EventBits_t uxBits = 0;
    const TickType_t xTicksToWait = 30000 / portTICK_PERIOD_MS;
    time_t now;
    struct tm timeinfo;
    while (1) {

        xEventGroupWaitBits(wifi_event_group, SEND_BIT, false, true, portMAX_DELAY);

        uxBits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, xTicksToWait);
        //ESP_LOGI(TAG, "uxBits: %d\r\n", uxBits);
        if (WIFI_CONNECTED_BIT != (uxBits & WIFI_CONNECTED_BIT))
        {
            ESP_LOGI(TAG, "connected to AP failure, restart system.");
            esp_restart();
        }
        ESP_LOGI(TAG, "Connected to AP");

#if START_STNP_EN
        time(&now);
        localtime_r(&now, &timeinfo);
#if PRINT_TIME_EN
        printf("%04d-%02d-%02d %02d:%02d:%02d get_time start\n", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
		            timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
#endif
        // Is time set? If not, tm_year will be (1970 - 1900).
        if (timeinfo.tm_year < (2016 - 1900)) {
            ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
            initialize_sntp();
        }

        setenv("TZ", "CST-8", 1);
        tzset();
        time(&now);
        localtime_r(&now, &timeinfo);
        sprintf(bufTime, "%04d-%02d-%02d %02d:%02d:%02d\n", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
		            timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
#if PRINT_TIME_EN
        printf("%04d-%02d-%02d %02d:%02d:%02d get_time start\n", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
		            timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
#endif
#endif
#if 1
        printf("%04d-%02d-%02d %02d:%02d:%02d update_data start\n", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
		            timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
#endif
        0 == strlen((char *)tInfoMisc.tuartWifiInfo.mServer) ? sprintf(ip, "%s", HOST_IP_ADDR) : sprintf(ip, "%s", tInfoMisc.tuartWifiInfo.mServer);
        port = tInfoMisc.tuartWifiInfo.mPort == 0 ? PORT : tInfoMisc.tuartWifiInfo.mPort;
        ESP_LOGI(TAG, "ip: %s, port: %d", ip, port);

        memset(&destAddr, 0, sizeof (destAddr));
        destAddr.sin_addr.s_addr = inet_addr(ip);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(port);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

        sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");
        vTaskDelay(10 / portTICK_PERIOD_MS);
        send_udp_data();
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if (ENBLE == tInfoMisc.scanFlag)
        {
            record_ap_number();
            printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4\r\n");
            tInfoMisc.scanFlag = DISABLE;
        }
        //if (sock != -1) 
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            vTaskDelay(100 / portTICK_PERIOD_MS);
            shutdown(sock, 0);
            close(sock);
        }
#if PRINT_TIME_EN
                printf("%04d-%02d-%02d %02d:%02d:%02d update_data end\n", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
		            timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
#endif

#if 1
        if (timeinfo.tm_hour >= OTA_UPDATA_MIX && timeinfo.tm_hour <= OTA_UPDATA_MAX)
        {
            ota_resume();
            xEventGroupWaitBits(wifi_event_group, OTA_BIT, false, true, portMAX_DELAY);
            xEventGroupClearBits(wifi_event_group, OTA_BIT);
        }
#endif

        vTaskResume(snifferHandle);
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        xEventGroupClearBits(wifi_event_group, SEND_BIT);
    }
    vTaskDelete(NULL);
}

void init_get_time(void)
{
    EventBits_t uxBits = 0;
    const TickType_t xTicksToWait = 30000 / portTICK_PERIOD_MS;
    wifi_init_sta();
    uxBits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, xTicksToWait);
    
    if (WIFI_CONNECTED_BIT != (uxBits & WIFI_CONNECTED_BIT))
    {
        ESP_LOGI(TAG, "connected to AP failure, restart system.");
        esp_wifi_disconnect();
        //esp_restart();
        return ;
    }

    initialize_sntp();
    setenv("TZ", "CST-8", 1);
    tzset();
    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
}

void create_udp_task()
{
    create_ota_updata();
    xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 13, NULL);
}
