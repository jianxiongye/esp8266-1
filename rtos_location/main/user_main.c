/* HTTPS GET Example using plain mbedTLS sockets
 *
 * Contacts the howsmyssl.com API via TLS v1.2 and reads a JSON
 * response.
 *
 * Adapted from the ssl_client1 example in mbedtls.
 *
 * Original Copyright (C) 2006-2016, ARM Limited, All Rights Reserved, Apache 2.0 License.
 * Additions Copyright (C) Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD, Apache 2.0 License.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "esp_spi_flash.h"
#include <netdb.h>
#include <sys/socket.h>

#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "cJSON.h"
#include "esp_http_client.h"

#include "user_main.h"
#include "user_uart.h"
#include "user_wifi.h"
#include "user_http.h"

/* FreeRTOS event group to signal when we are connected & ready to make a request */


static const char *TAG = "user_main";
uint8_t staMacAddr[6];
uint8_t apMacAddr[6];

esp_err_t get_chip_id(uint32_t* chip_id){
    esp_err_t status = ESP_OK;
    *chip_id = (REG_READ(0x3FF00050) & 0xFF000000) |
                         (REG_READ(0x3ff0005C) & 0xFFFFFF);
    return status;
}

static void prinfChipVersion()
{
	uint32_t id ;
    get_chip_id(&id);

    ESP_LOGI(TAG, "start system run");
	esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ", chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	printf("\n\n------------------ Get Systrm Info------------------\n");
	printf("File: %s, \nLine: %d, Date: %s, Time: %s, Timestamp: %s\n", __FILE__, __LINE__, __DATE__, __TIME__, __TIMESTAMP__);    
    //获取IDF版本
    printf("     SDK version:%s, chip id: %u\n", esp_get_idf_version(), id);
    //获取芯片可用内存
    printf("     esp_get_free_heap_size : %d  \n", esp_get_free_heap_size());
    //获取从未使用过的最小内存
    printf("     esp_get_minimum_free_heap_size : %d  \n", esp_get_minimum_free_heap_size());
    //获取芯片的内存分布，返回值具体见结构体 flash_size_map
    printf("     system_get_flash_size_map(): %d \n", system_get_flash_size_map());
    //获取mac地址（station模式）
    esp_wifi_get_mac(ESP_IF_WIFI_STA, staMacAddr);
    printf(" Station esp_wifi_get_mac(): %02x:%02x:%02x:%02x:%02x:%02x \n", 
            staMacAddr[0], staMacAddr[1], staMacAddr[2], staMacAddr[3], staMacAddr[4], staMacAddr[5]);
    //获取mac地址（ap模式）
    esp_wifi_get_mac(ESP_IF_WIFI_AP, apMacAddr);
    printf(" AP esp_wifi_get_mac(): %02x:%02x:%02x:%02x:%02x:%02x \n", 
            apMacAddr[0], apMacAddr[1], apMacAddr[2], apMacAddr[3], apMacAddr[4], apMacAddr[5]);
    printf("----------------------------------------------------------\n\n");
}


void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );

    uart_init(115200);
    prinfChipVersion();
    wifi_init();

    create_Http_Task();
    create_Uart_Task();
}
