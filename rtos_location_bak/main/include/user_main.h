#ifndef   __USER_MAIN_
#define   __USER_MAIN_
#include "esp_system.h"
#include "freertos/event_groups.h"

#define   ENBLE              (1)
#define   DISABLE            (0)

#define  PRINT( buf, i )     buf[i + 0], buf[i + 1], buf[i + 2], buf[i + 3], buf[i + 4], buf[i + 5]
#define  MACPRINT            "%02X:%02X:%02X:%02X:%02X:%02X"
#define  MAC_LEN             (17)
#define  START_BIT           BIT0
#define  WIFI_CONNECTED_BIT  BIT1

#define HTTPS_URL_LOCATION      "https://api.newayz.com/location/hub/v1/track_points?access_key=Etp3Ypv5j34wq8jy0TZN2bZSzjs"

#pragma  pack(push)
#pragma  pack(1)



#pragma  pack(pop)

extern EventGroupHandle_t wifi_event_group;
extern char *cJsonBuffer;
extern TaskHandle_t tCJSONTask;

#endif




