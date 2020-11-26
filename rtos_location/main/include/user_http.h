
#ifndef  __USER_HTTP_
#define  __USER_HTTP_

#include "esp_system.h"
#include "user_main.h"

#if 0
#define WEB_SERVER          "example.com"
#define WEB_PORT            80
#define WEB_URL             "http://example.com/"

#define WEB_POST_URL        "https://api.newayz.com/location/hub/v1/track_points?access_key=Etp3Ypv5j34wq8jy0TZN2bZSzjs"
#endif

#define  HTTP_OK            200
#define  BUFFER_SIZE        (1024 * 4)
#define  RETRY_COUNT        10

void create_Http_Task();
//void https_post_task(void *pvParameters);
TaskHandle_t tCJSONTask;

#endif