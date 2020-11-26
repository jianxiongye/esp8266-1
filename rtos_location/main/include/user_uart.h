

#ifndef  __USER_UART_
#define  __USER_UART_

#include "esp_system.h"
#include "user_main.h"


#define EX_UART_NUM UART_NUM_0

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

#pragma pack(push)
#pragma pack(1)


#pragma pack(pop)


void uart_init(uint32_t baud);
void create_Uart_Task();
void uart_send_data(const char *data, int len);


#endif