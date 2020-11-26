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
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "nvs_flash.h"

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

#include "user_http.h"
#include "user_wifi.h"
#include <time.h>
#include "user_uart.h"
#include "driver/uart.h"
#include "hytera_protol.h"
#include "user_ota.h"

char *cJsonBuffer = NULL;
TaskHandle_t tCJSONTask;
/* Constants that aren't configurable in menuconfig */
#define WEB_URL "https://api.newayz.com/location/hub/v1/track_points?access_key=Etp3Ypv5j34wq8jy0TZN2bZSzjs"

char cJsonParseBuffer[BUFFER_SIZE] = {0};

static const char *TAG = "user_http";
void https_async_Task(void *pvParameters);

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                // printf("%.*s", evt->data_len, (char*)evt->data);
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

static void https_async()
{
    esp_http_client_config_t config = {
        .url = WEB_URL, //"https://postman-echo.com/post",
        .event_handler = _http_event_handler,
        .is_async = true,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;
    ESP_LOGI(TAG, "https_async start");
#if 0
    const char *post_data = "Using a Palantír requires a person with great strength of will and wisdom. The Palantíri were meant to "
                            "be used by the Dúnedain to communicate throughout the Realms in Exile. During the War of the Ring, "
                            "the Palantíri were used by many individuals. Sauron used the Ithil-stone to take advantage of the users "
                            "of the other two stones, the Orthanc-stone and Anor-stone, but was also susceptible to deception himself.";
#endif
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, cJsonBuffer, strlen(cJsonBuffer));
    while (1) {
        err = esp_http_client_perform(client);
        if (err != ESP_ERR_HTTP_EAGAIN) {
            break;
        }
        vTaskDelay( 10 / portTICK_PERIOD_MS );
        ESP_LOGI(TAG, "1111 sleep");
    }
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        int len  = esp_http_client_get_content_length(client);
        #if 1
        char *buf = (char *)malloc(len);
        esp_http_client_read(client, buf, len);
        printf("-----\r\n %s \r\n", buf);
        free(buf);
        #endif
    } else {
        ESP_LOGI(TAG, "Error perform http request %d", err);
    }
    esp_http_client_cleanup(client);
}



double get_point_loction(const char *headbuf, const char *key)
{
    char *ret = strstr(headbuf, key);
    if (NULL == ret)
    {
        return 0;
    }
    char *ret1 = strstr(ret, ":");
    char *ret2 = strstr(ret, ",");
    char valueBuf[20] = {0};
    memcpy(valueBuf, ret1 + 1, ret2 - ret1 - 1);
    double fValue = atof(valueBuf);

    return fValue;
}

void get_time_stamp(const char *headbuf)
{
    char *pos = strstr(headbuf, "position");
    if (NULL == pos)
    {
        return ;
    }

    char *ret = strstr(pos, "timestamp");
    if (NULL == ret)
    {
        return ;
    }
    char *ret1 = strstr(ret, ":");
    char *ret2 = strstr(ret, ",");
    char valueBuf[20] = {0};
    memcpy(valueBuf, ret1 + 1, ret2 - ret1 - 1 - 3);
    uint32_t value = atoi(valueBuf);
    struct tm *p = NULL;
    p = localtime((time_t *)&value);
    #if OPEN_PRINTF
    char s[100] = {0};
    strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", p);
    printf("%s: %s", valueBuf, s);
    printf("%04d-%02d-%02d  %02d:%02d:%02d\r\n", p->tm_year + 1900, p->tm_mon + 1,
		  p->tm_mday, p->tm_hour + 8, p->tm_min, p->tm_sec);
    #endif
    memset(valueBuf, 0, sizeof (valueBuf));
    sprintf(valueBuf, "%04d", p->tm_year + 1900);
    
    sprintf((char *)tUartSendData.time, "%02d%02d%02d", p->tm_hour + 8, p->tm_min, p->tm_sec);
    sprintf((char *)tUartSendData.data, "%c%c%02d%02d", valueBuf[2], valueBuf[3], p->tm_mon + 1, p->tm_mday);
}

void http_post()
{
    esp_err_t err = 0;
    EventBits_t uxBits = 0;
    TickType_t xTicksToWait = 10000 / portTICK_PERIOD_MS;
    int count = 0;
    //cJSON *root = NULL;
    //printf("     esp_get_free_heap_size : %d  \n", esp_get_free_heap_size());
    #if 1
    esp_http_client_config_t config = {
        .url = WEB_URL, //"http://httpbin.org/get",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    #endif

    // POST
    //const char *post_data = "field1=value1&field2=value2";
    //esp_http_client_set_url(client, WEB_URL); //"http://httpbin.org/post");
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, cJsonBuffer, strlen(cJsonBuffer));

    //memset(cJsonParseBuffer, 0, sizeof (cJsonParseBuffer));
    free(cJsonBuffer);
    cJsonBuffer = NULL;

    err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        int status = esp_http_client_get_status_code(client);

        if (HTTP_OK == status)
        {
            int len  = esp_http_client_get_content_length(client);
            esp_http_client_read(client, cJsonParseBuffer, len);
            //printf("%s", cJsonParseBuffer);
            tUartSendData.cmd = BigtoLittle16(SET_CMD);
            tUartSendData.operat = OP_SET;
            tUartSendData.result = BigtoLittle16(R_SUCCESS);
            get_time_stamp(cJsonParseBuffer);
            double longitude = get_point_loction(cJsonParseBuffer, "longitude");
            double latitude = get_point_loction(cJsonParseBuffer, "latitude");

            tUartSendData.NSFlag = latitude > 0 ? NORTH : SOUTH;
            tUartSendData.EWFlag = longitude > 0 ? EAST : WEST;
            
            snprintf((char *)tUartSendData.longitude, sizeof (tUartSendData.longitude), "%.6f", fabs(longitude));
            snprintf((char *)tUartSendData.latitude, sizeof (tUartSendData.latitude), "%.6f", fabs(latitude));

            #if OPEN_PRINTF
            printf("\r\n$$$$$$$$$$$$$$$$$$   longitude: %f, latitude: %f \r\n", longitude, latitude);

            printf("-------tUartSendData start---------\r\n");
            char *buf = (char *)&tUartSendData;
            for (int i = 0; i < sizeof (tUartSendData); i ++)
            {
                printf("%02X ", buf[i]);
            }
            printf("\r\n");
            printf("-------tUartSendData end---------\r\n");
            #endif

retryConnect:
            connect_hytera();
            uxBits = xEventGroupWaitBits(wifi_event_group, HYTERA_ACCEPT_BIT | HYTERA_REJECT_BIT, false, true, xTicksToWait);
            //ESP_LOGI(TAG, "uxBits: %d\r\n", uxBits);
            //xEventGroupClearBits(wifi_event_group, HYTERA_ACCEPT_BIT | HYTERA_REJECT_BIT);
            if (HYTERA_ACCEPT_BIT == (uxBits & HYTERA_ACCEPT_BIT))
            {
                count = 0;
retrySendData:
                send_data_hytera((char *) &tUartSendData, sizeof (tUartSendData));
                uxBits = xEventGroupWaitBits(wifi_event_group, HYTERA_DATA_OK_BIT | HYTERA_DATA_FAIL_BIT, false, true, xTicksToWait);
                xEventGroupClearBits(wifi_event_group, HYTERA_DATA_OK_BIT | HYTERA_DATA_FAIL_BIT);

                if (HYTERA_DATA_OK_BIT == (uxBits & HYTERA_DATA_OK_BIT))
                {
                    #if 0
                    close_hytera();
                    xEventGroupWaitBits(wifi_event_group, HYTERA_CLOSE_ACK_BIT, false, true, xTicksToWait);
                    xEventGroupClearBits(wifi_event_group, HYTERA_CLOSE_ACK_BIT);
                    #endif
                }
                else
                {
                    count ++;
                    if (count <= RETRY_COUNT)
                    {
                        //printf("Try to send data for the %d time. \r\n", count);
                        goto retrySendData;
                    }
                    else
                    {
                        printf("&&& Retrieve send AP information location. \r\n");
                        goto exitFail;
                    }
                }
            }
            else
            {
                count ++;
                if (count <= RETRY_COUNT)
                {
                    //printf("Try to reconnect for the %d time. \r\n", count);
                    goto retryConnect;
                }
                else
                {
                    printf("\r\n&&& Retrieve connect AP information location. \r\n");
                    goto exitFail;
                }
            }

            //printf("user_http----------\r\n");
            #if 0
            root = cJSON_Parse(cJsonParseBuffer);
            printf("==================\r\n");
            if(NULL == root)
            {
                printf("@@@@@@@@@@@@Error before: [%s]\n", cJSON_GetErrorPtr());
                goto exitFail;
            }
            cJSON *arrayItem = cJSON_GetObjectItem(root, "location");
            
            cJSON *arrayItemP = cJSON_GetObjectItem(arrayItem, "position");
            cJSON *arrayItemPoint = cJSON_GetObjectItem(arrayItemP, "point");

            double a = cJSON_GetObjectItem(arrayItemPoint, "longitude")->valuedouble;
            double b = cJSON_GetObjectItem(arrayItemPoint, "latitude")->valuedouble;
            printf("longitude: %f, latitude %f\r\n", a, b);
            #endif
        }
    } else {
        ESP_LOGI(TAG, "HTTP POST request failed: %d", err);
    }
exitFail:
    //memset(cJsonParseBuffer, 0, sizeof (cJsonParseBuffer));
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    //free(cJsonBuffer);
}

void https_async_Task(void *pvParameters)
{
    while (1)
    {
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
        //https_async();
        printf("     esp_get_free_heap_size : %d  \n", esp_get_free_heap_size());
        //http_post();

        printf("https_async_Task finsh\r\n");
        
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d...", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        vTaskResume(tCJSONTask);
    }
}

#if 0
#define EXAMPLE_SERVER_URL "api.newayz.com"
static int socket_id = -1;
static char text[BUFFSIZE + 1] = { 0 };
static struct addrinfo *resolve_host_name(const char *host, size_t hostlen)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
 
    char *use_host = strndup(host, hostlen);
    if (!use_host)
    {
        return NULL;
    }
 
    ESP_LOGD(TAG, "host:%s: strlen %lu", use_host, (unsigned long)hostlen);
    struct addrinfo *res;
    if (getaddrinfo(use_host, NULL, &hints, &res))
    {
        ESP_LOGE(TAG, "couldn't get hostname for :%s:", use_host);
        free(use_host);
        return NULL;
    }
    free(use_host);
    return res;
}

static bool connect_to_http_server()
{
    char ip[128];
    struct addrinfo *cur;
    struct addrinfo *res = resolve_host_name(EXAMPLE_SERVER_URL, strlen(EXAMPLE_SERVER_URL));
    if (!res)
    {
        ESP_LOGI(TAG, "DNS resolve host name failed!");
        return false;
    }
 
    for (cur = res; cur != NULL; cur = cur->ai_next)
    {
        inet_ntop(AF_INET, &(((struct sockaddr_in *)cur->ai_addr)->sin_addr), ip, sizeof(ip));
        ESP_LOGI(TAG, "DNS IP:[%s]", ip);
    }

    ESP_LOGI(TAG, "Server IP: %s Server Port:%s", ip, EXAMPLE_SERVER_PORT);

    int  http_connect_flag = -1;
    struct sockaddr_in sock_info;

    socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_id == -1) {
        ESP_LOGE(TAG, "Create socket failed!");
        return false;
    }

    int reuse = 1;
    if (setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
         return false;
    }
    // set connect info
    memset(&sock_info, 0, sizeof(struct sockaddr_in));
    sock_info.sin_family = AF_INET;
    sock_info.sin_addr.s_addr = inet_addr(ip);
    sock_info.sin_port = htons(atoi(EXAMPLE_SERVER_PORT));

    // connect to http server
    http_connect_flag = connect(socket_id, (struct sockaddr *)&sock_info, sizeof(sock_info));
    if (http_connect_flag == -1) {
        ESP_LOGE(TAG, "Connect to server failed! errno=%d", errno);
        close(socket_id);
        return false;
    } else {
        ESP_LOGI(TAG, "Connected to server");
        return true;
    }
    return false;
}

void test_post()
{
    /*connect to http server*/
    if (connect_to_http_server()) {
        ESP_LOGI(TAG, "Connected to http server");
    } else {
        ESP_LOGE(TAG, "Connect to http server failed!");
    }

    /*send GET request to http server*/
    const char *GET_FORMAT =
        "POST /location/hub/v1/track_points?access_key=Etp3Ypv5j34wq8jy0TZN2bZSzjs HTTP/1.0\r\n"
        "Host: api.newayz.com\r\n"
        "Content-Type: application/json\r\n"
        "User-Agent: esp-idf/1.0 esp32\r\n\r\n"
        "%s\r\n";

    char *http_request = NULL;
    int get_len = asprintf(&http_request, GET_FORMAT, cJsonBuffer);
    if (get_len < 0) {
        ESP_LOGE(TAG, "Failed to allocate memory for GET request buffer");
    }
    printf("%s", http_request);
    int res = send(socket_id, http_request, get_len, 0);
    free(http_request);

    if (res < 0) {
        ESP_LOGE(TAG, "Send GET request to server failed");
    } else {
        ESP_LOGI(TAG, "Send GET request to server succeeded");
    }

    while (1)
    {
        memset(text, 0, TEXT_BUFFSIZE);
        int buff_len = recv(socket_id, text, TEXT_BUFFSIZE, 0);
        if (buff_len < 0) { /*receive error*/
            ESP_LOGE(TAG, "Error: receive data error! errno=%d", errno);
            break;
        } else if (buff_len > 0) { /*deal with response body*/
            printf("%s \r\n", text);
        }
    }

}


#endif

static void cJsonTask(void *pvParameters)
{
 /* declare a few. */
    cJSON *root = NULL;
    cJSON *fmt = NULL;
    cJSON *img = NULL;
    cJSON *thm = NULL;
    cJSON *fld = NULL;
    int i = 0;
    char macBuf[MAC_LEN] = {0};
#if 0
    esp_http_client_config_t config = {
        .url = WEB_URL, //"http://httpbin.org/get",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
#endif
    while (1)
    {
        #if OPEN_PRINTF
        ESP_LOGI(TAG, "start scan wifi.");
        #endif
        wifi_scan();
        #if OPEN_PRINTF
        ESP_LOGI(TAG, "scan wifi finish.");
        #endif
        wifi_init_sta();
        root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "timestamp", 1593715944000);
        cJSON_AddItemToObject(root, "id", cJSON_CreateString("38efe26e-bcd8-11ea-bb12-acbc327d0c21"));
        cJSON_AddItemToObject(root, "asset", fmt = cJSON_CreateObject());
        cJSON_AddItemToObject(fmt, "id", cJSON_CreateString("123456789000008"));
        cJSON_AddItemToObject(root, "location", img = cJSON_CreateObject());
        cJSON_AddItemToObject(img, "wifis", thm = cJSON_CreateArray());

        for (i = 0; i < aucApInfo.count; i++)
        {
            sprintf(macBuf, ""MACPRINT, PRINT(aucApInfo.tinfoAp[i].mac, 0));
            cJSON_AddItemToArray(thm, fld = cJSON_CreateObject());
            cJSON_AddStringToObject(fld, "macAddress", macBuf);
            cJSON_AddStringToObject(fld, "ssid", (char *)aucApInfo.tinfoAp[i].ssid);
            cJSON_AddNumberToObject(fld, "frequency", chnTofreq(aucApInfo.tinfoAp[i].channel));
            cJSON_AddNumberToObject(fld, "signalStrength", abs(aucApInfo.tinfoAp[i].rssi));
        }
        vTaskDelay( 100 / portTICK_PERIOD_MS );
        cJsonBuffer = cJSON_Print(root);
        #if OPEN_PRINTF
        printf("len: %d, result: %s\r\n", strlen(cJsonBuffer), cJsonBuffer);
        #endif
#if 0
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
        https_async();
#endif
        printf("\r\n \r\n");
        cJSON_Delete(root);
        //printf("cJsonTask finsh\r\n");
        #if 1
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
        //https_async();
        http_post();
        //test_post();
        //create_ota_updata();
        //ota_resume();
        //xEventGroupWaitBits(wifi_event_group, OTA_BIT, false, true, portMAX_DELAY);
        #if 0
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d...", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        #endif
        #else
        vTaskSuspend(NULL);
        #endif
    }
}

void create_Http_Task(void)
{
    //xTaskCreate(&https_get_task, "https_get_task", 8192, NULL, 7, NULL);
    xTaskCreate(&cJsonTask, "cJsonTask", CJSON_STACK, NULL, CJSON_PRIORITY, &tCJSONTask);
    //xTaskCreate(&https_async_Task, "https_async_Task", HTTP_STACK, NULL, HTTP_PRIORITY, NULL);
    create_ota_updata();
}
