
#ifndef __USER_UART_
#define __USER_UART_

#include "esp_system.h"
#include "sniffer_main.h"
#include "user_nvs.h"

#define EX_UART_NUM UART_NUM_0

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)


void uart_event_task(void *pvParameters);
void uart_init(uint32_t baud);

#endif
