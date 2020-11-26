#ifndef   __USER_MAIN_H_
#define   __USER_MAIN_H_
#include "esp_system.h"


#define   ENBLE              (1)
#define   DISABLE            (0)

#define  PRINT( buf, i )     buf[i + 0], buf[i + 1], buf[i + 2], buf[i + 3], buf[i + 4], buf[i + 5]
#define  MACPRINT            "%02X:%02X:%02X:%02X:%02X:%02X"
#define  MAC_LEN             (17)
/*------------------事件通知 start----------------------*/
#define  START_BIT           BIT0
#define  WIFI_CONNECTED_BIT  BIT1
#define  HYTERA_ACCEPT_BIT   BIT2
#define  HYTERA_REJECT_BIT   BIT3
#define  HYTERA_DATA_OK_BIT   BIT4
#define  HYTERA_DATA_FAIL_BIT   BIT5
#define  HYTERA_CLOSE_ACK_BIT  BIT6
#define  OTA_BIT             BIT7
/*------------------事件通知 end----------------------*/

/*------------------优先级定义 start----------------------*/
#define  UART_PRIORITY       5
#define  CJSON_PRIORITY      6
#define  HTTP_PRIORITY       7
/*------------------优先级定义 end----------------------*/

/*------------------堆栈大小 start----------------------*/
#define  UART_STACK         2048
#define  CJSON_STACK        8192
#define  HTTP_STACK         8192
/*------------------堆栈大小 end----------------------*/

#define  OPEN_PRINTF        0

#pragma  pack(push)
#pragma  pack(1)



#pragma  pack(pop)


//extern char *cJsonBuffer;
//extern TaskHandle_t tCJSONTask;

#endif




