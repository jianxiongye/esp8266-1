
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "uart.h"

static const char *TAG = "user_uart";

static QueueHandle_t uart0_queue;

void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    char *dtmp = (char *) malloc(RD_BUF_SIZE);
    char *ret;
    char cPort[16] = {0};
    for (;;) {
        // Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);

            switch (event.type) {
                // Event of UART receving data
                // We'd better handler data event fast, there would be much more data events than
                // other types of events. If we take too much time on data event, the queue might be full.
                case UART_DATA:
                    ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(EX_UART_NUM, (uint8_t *)dtmp, event.size, portMAX_DELAY);
                    //ESP_LOGI(TAG, "[DATA EVT]:");
                    //uart_write_bytes(EX_UART_NUM, (const char *) dtmp, event.size);
                    if (0 == strncmp(dtmp, "set", 3))
                    {
                        switch (dtmp[3])
                        {
                            case 'N': // wifi 名字
                                memcpy(tInfoMisc.tuartWifiInfo.mWifiName, dtmp + 5, event.size - 5);
                                ESP_LOGI(TAG, "wifi name: %s", tInfoMisc.tuartWifiInfo.mWifiName);
                                ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
                                break;
                            case 'P': // wifi 密码
                                memcpy(tInfoMisc.tuartWifiInfo.mWifiPass, dtmp + 5, event.size - 5);
                                ESP_LOGI(TAG, "wifi passwd: %s", tInfoMisc.tuartWifiInfo.mWifiPass);
                                break;
                            case 'S': // 服务器ip + port
                                ret = strstr(dtmp + 5, ":");
                                memcpy(tInfoMisc.tuartWifiInfo.mServer, dtmp + 5, (ret - dtmp) - 5);
                                strcpy(cPort, dtmp + (ret - dtmp) + 1);
                                tInfoMisc.tuartWifiInfo.mPort = atoi(cPort);
                                ESP_LOGI(TAG, "server: %s, %d", tInfoMisc.tuartWifiInfo.mServer, tInfoMisc.tuartWifiInfo.mPort);
                                break;
                            case 'T': // 设置多长时间上传
                                strcpy(cPort, dtmp + 5);
                                tInfoMisc.tuartWifiInfo.mSniffSendMin = atoi(cPort);
                                ESP_LOGI(TAG, "send data time: %d min", tInfoMisc.tuartWifiInfo.mSniffSendMin);

                                // 写入内存中，供下次操作wifi时使用
                                NVS_Write(WIFI_INFO_KEY, &tInfoMisc.tuartWifiInfo, sizeof (tInfoMisc.tuartWifiInfo));
                                tInfoWifi temp = {0};
                                NVS_Read(WIFI_INFO_KEY, &temp, sizeof (tInfoMisc.tuartWifiInfo));
                                ESP_LOGI(WIFI_INFO_KEY, "count : %d", sizeof (temp));
                                ESP_LOGI(WIFI_INFO_KEY, "wifi name: %s", temp.mWifiName);
                                ESP_LOGI(WIFI_INFO_KEY, "wifi passwd: %s", temp.mWifiPass);
                                ESP_LOGI(WIFI_INFO_KEY, "server: %s, %d", temp.mServer, temp.mPort);
                                ESP_LOGI(WIFI_INFO_KEY, "send data time: %d min", temp.mSniffSendMin);

                                vTaskResume(snifferHandle);
                                break;
                            default:
                                break;
                        }
                    }
                    break;

                // Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;

                // Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;

                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;

                // Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;

                // Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }

    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}


void uart_init(uint32_t baud)
{
    uart_config_t uart_config = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(EX_UART_NUM, &uart_config);

    // Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 100, &uart0_queue, 0);
}

