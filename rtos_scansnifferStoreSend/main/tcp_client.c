/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdio.h>
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
#include "esp_spi_flash.h"

#include "wifi.h"
#include "user_nvs.h"


static const char *TAG = "tcp_client";

#define  SEND_BUF_LEN       4096
#define  SEND_FLAG          1
char sendBuf[SEND_BUF_LEN] = {0};
int sock = -1;

int record_ap_number(int32_t status)
{
    char fifo[70] = {0};
    int i = 0;
    int count = 0;
    for (i = 0; i < aucApInfo.count; i++) {
		sprintf(fifo, "%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%s\n", MAC2STR(aucApInfo.tinfoAp[i].mac), 
				aucApInfo.tinfoAp[i].rssi, aucApInfo.tinfoAp[i].channel, aucApInfo.tinfoAp[i].ssid);
        //ESP_LOGI(TAG, "count: %d, fifo len: %d, %s\n", count, strlen(fifo), fifo);
        memcpy(sendBuf + status + count, fifo, strlen(fifo));
        count += strlen(fifo);
	}
    ESP_LOGI(TAG, "count: %d \n", count);
    return count;
}

void send_head_data(void)
{
    int dataTotalLen = tInfoMisc.record * SNIFFER_DATA_LEN + MAC_LEN + 1;
    int apWifiLen = 0;
    char v_mod = 0;
	int count_head = 1;
    int num = 0;
    char macstr[MAC_LEN] = {0};
    sendBuf[0] = SEND_HEAD;

    ESP_LOGI(TAG, "dataTotalLen : %d \n", dataTotalLen);
    apWifiLen = aucApInfo.length;//record_ap_number();
    dataTotalLen += apWifiLen;
    num = dataTotalLen;
	ESP_LOGI(TAG, "dataTotalLen : %d \n", dataTotalLen);
	do{ 
		v_mod = num % 128;
		num = num / 128;

		if(num > 0)
			v_mod = v_mod | 0x80;
		sendBuf[count_head] = v_mod;
		count_head ++;
	}while( num > 0 );

    sprintf(macstr, "%02X:%02X:%02X:%02X:%02X:%02X\n", PRINT( staMacAddr, 0 ));
    memcpy(sendBuf + count_head, macstr, MAC_LEN);
    record_ap_number(count_head + MAC_LEN);
#if 0
    ESP_LOGI(TAG, "printf sendBuf %d char.\n", count_head + MAC_LEN);
    for (int i = 0; i < (count_head + MAC_LEN); i ++)
    {
        printf("%02X ", sendBuf[i]);
    }
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "%s\n", sendBuf + count_head);
#endif

#if SEND_FLAG
    // 发送头部20字节 0x30 + 长度 + 17Bmac + 回车 + wifi ap数据
    int err = send(sock, sendBuf, count_head + MAC_LEN + apWifiLen, 0);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
    }
#endif
    memset(sendBuf, 0, sizeof (sendBuf));
}

void send_body_data(void)
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

        memset(sendBuf, 0, sizeof (sendBuf));

        for (j = 0; j < cycles; j ++)
        {
            //strncpy(sendBuf + j * SNIFFER_DATA_LEN, nvsData + i * SEND_BODY_COUNT * SNIFFER_DATA_LEN  + j * SNIFFER_DATA_LEN, SNIFFER_DATA_LEN);
            spi_flash_read(secB_addr + ((uint32_t)sizeof(des_addr) * (i * SEND_BODY_COUNT + j)), des_addr, (uint32_t)sizeof(des_addr));
            strncpy(sendBuf + j * SNIFFER_DATA_LEN, des_addr, SNIFFER_DATA_LEN);
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
        
        //memset(fifo, 0, sizeof (fifo));
        //NVS_Read(create_key(i), sendBuf, cycles * SNIFFER_DATA_LEN);
#if 0
        ESP_LOGI(TAG, "----------%d------", i);
        for (j = 0; j < cycles; j ++)
            ESP_LOGI(TAG, "%s", sendBuf + j * SNIFFER_DATA_LEN);
#else
        ESP_LOGI(TAG, "%s", sendBuf);
#endif
#if SEND_FLAG
        int err = send(sock, sendBuf, cycles * SNIFFER_DATA_LEN, 0);
        if (err < 0) {
            ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
            break;
        }
#endif
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    tInfoMisc.record = 0;
    //memset(nvsData, 0, sizeof (nvsData));
}

void tcp_client_task(void *pvParameters)
{
    char addr_str[128] = {0};
    int addr_family = 0;
    int ip_protocol = 0;
    struct sockaddr_in destAddr = {0};
    char ip[32] = {0};
    uint16_t port = 0;
    uint8_t connectFlg = 0;
    ESP_LOGI(TAG, "----------tcp_client_task----------");
    while (1) {
        xEventGroupWaitBits(wifi_event_group, SEND_BIT, false, true, portMAX_DELAY);
        wifi_init_sta();
        ESP_LOGI(TAG, "Waiting for AP connection...");
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "Connected to AP");

        0 == strlen((char *)tInfoMisc.tuartWifiInfo.mServer) ? sprintf(ip, "%s", HOST_IP_ADDR) : sprintf(ip, "%s", tInfoMisc.tuartWifiInfo.mServer);
        port = tInfoMisc.tuartWifiInfo.mPort == 0 ? PORT : tInfoMisc.tuartWifiInfo.mPort;
        ESP_LOGI(TAG, "ip: %s, port: %d", ip, port);
        destAddr.sin_addr.s_addr = inet_addr(ip);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(port);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

        sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = connect(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            connectFlg = DISABLE;
        }
        else
        {
            ESP_LOGI(TAG, "Successfully connected");
            connectFlg = ENBLE;
        }

        if (ENBLE == connectFlg)
        {
            send_head_data();
            vTaskDelay(100 / portTICK_PERIOD_MS);
            send_body_data();
        }

        //if (sock != -1) 
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
        vTaskResume(snifferHandle);
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        xEventGroupClearBits(wifi_event_group, SEND_BIT);
    }
    vTaskDelete(NULL);
}

void create_tcp_task()
{

    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}

