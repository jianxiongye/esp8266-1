
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "user_uart.h"
#include "user_nvs.h"
#include "user_http.h"
#include "hytera_protol.h"
#include "user_wifi.h"

static const char *TAG = "user_uart";

static QueueHandle_t uart0_queue;

void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    char *dtmp = (char *) malloc(RD_BUF_SIZE);
    char buf[64] = {0};
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
                    #if 1
                    if (0 == strncmp(dtmp, "set", 3))
                    {
                        switch (dtmp[3])
                        {
                            case 'N': // wifi 名字
                                vTaskSuspend(tCJSONTask);
                                memcpy(buf, dtmp + 5, event.size - 5);
                                ESP_LOGI(TAG, "wifi name: %s", buf);
                                NVS_Write(WIFI_NAME_KEY, buf, sizeof (buf));
                                memset(buf, 0, sizeof (buf));
                                break;
                            case 'P': // wifi 密码
                                memcpy(buf, dtmp + 5, event.size - 5);
                                ESP_LOGI(TAG, "wifi passwd: %s", buf);
                           
                                // 写入内存中，供下次操作wifi时使用
                                NVS_Write(WIFI_PASSWD_KEY, buf, sizeof (buf));
                                memset(buf, 0, sizeof (buf));
                                vTaskResume(tCJSONTask);
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        tHyteraProRep *replayData = (tHyteraProRep *)dtmp;

                        if (HYTERA_HEADER_ID == replayData->hy_header.hy_headerId)
                        {
                            switch (replayData->hy_header.hy_opCode)
                            {
                                case HYTERA_ACCEPT: 
                                    xEventGroupSetBits(wifi_event_group, HYTERA_ACCEPT_BIT);
                                    break;
                                case HYTERA_REJECT: 
                                    xEventGroupSetBits(wifi_event_group, HYTERA_REJECT_BIT);
                                    break;
                                case HYTERA_DATA_ACK: 
                                    ESP_LOGI(TAG, "data ack info: 0x%04x, 0x%02d \r\n", replayData->cmd, replayData->result);
                                    if (REPLY_CMD == replayData->cmd && SUCCESS == replayData->result)
                                    {
                                        xEventGroupSetBits(wifi_event_group, HYTERA_DATA_OK_BIT);
                                    }
                                    else
                                    {
                                        xEventGroupSetBits(wifi_event_group, HYTERA_DATA_FAIL_BIT);
                                    }
                                    
                                    break;
                                case HYTERA_CLOSE_ACK: 
                                    xEventGroupSetBits(wifi_event_group, HYTERA_CLOSE_ACK_BIT);
                                    break;
                                default:
                                    break;
                            }
                            #if 1
                            printf("-------uart_event_task start---------\r\n");
                            for (int i = 0; i < event.size; i ++)
                            {
                                printf("%02X ", dtmp[i]);
                            }
                            printf("\r\n");
                            printf("-------uart_event_task end---------\r\n");
                            #endif
                        }
#if 0
                        if (SUCCESS == dtmp[2])
                        {
                            ESP_LOGI(TAG, "Longitude and latitude set successfully");
                        }
                        else 
                        {
                            ESP_LOGI(TAG, "Failed to set latitude and longitude");
                        }      
#endif
                    }
                    #endif
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

void uart_send_data(const char *data, int len)
{
    uart_write_bytes(EX_UART_NUM, data, len);
}

void create_Uart_Task()
{
    connect_hytera();
    xTaskCreate(uart_event_task, "uart_event_task", 1024, NULL, UART_PRIORITY, NULL);

}



