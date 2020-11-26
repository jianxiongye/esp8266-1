/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include <netdb.h>
#include <sys/socket.h>

#include "cJSON.h"
#include "esp_http_client.h"

#include "user_main.h"
#include "user_wifi.h"
#include "user_uart.h"
#include "user_http.h"

EventGroupHandle_t wifi_event_group;

static const char *TAG = "user_main";

char *cJsonBuffer = NULL;
TaskHandle_t tCJSONTask;


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

    //while (1)
    {
        ESP_LOGI(TAG, "start scan wifi.");
        wifi_scan();
        ESP_LOGI(TAG, "scan wifi finish.");
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

        cJsonBuffer = cJSON_Print(root);
        printf("len: %d, result: %s\r\n", strlen(cJsonBuffer), cJsonBuffer);

        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
        //https_post_task(NULL);
        https_request_by_POST(HTTPS_URL_LOCATION);
        vTaskSuspend(NULL);
    }

}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );

    wifi_event_group = xEventGroupCreate();

    uart_init(76800);
    wifi_init();

    //cJsonTask(NULL);
    xTaskCreate(cJsonTask, "cJSON_task", 32768, NULL, 5, &tCJSONTask);
    //create_Http_Task();
    create_Uart_Task();
}
