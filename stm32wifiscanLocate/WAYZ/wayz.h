#ifndef __WAYZ_H__
#define __WAYZ_H__
#include <stdio.h>
#include <lcd.h>
#include "usart3.h"
#include "string.h"
#include "delay.h"
#include <stdlib.h>
#include "track_point.h"
#include <zlog.h>
#include "cJSON.h"

int wifiscan(int reserved);
#ifndef int64
typedef int64_t int64;
#endif

#ifndef int32
typedef int32_t int32;
#endif

//#define BAIDU "AT+CIPSTART=\"TCP\",\"220.181.38.148\",443"
#define BAIDU2 "AT+CIPSTART=\"SSL\",\"220.181.38.148\",443"
#define WAYZ_T "AT+CIPSTART=\"TCP\",\"52.82.35.145\",8084"
#define GUYU   "AT+CIPSTART=\"TCP\",\"115.29.240.46\",80"
#define WANGR  "AT+CIPSTART=\"TCP\",\"112.74.89.58\",80"

#define SOURCEF "AT+CIPSTART=\"TCP\",\"216.105.38.10\",80"
#define RUI2    "AT+CIPSTART=\"TCP\",\"39.98.236.221\",80"  //myscgj.top
#define SSL_WAYZ "AT+CIPSTART=\"SSL\",\"api.newayz.com\",443" //api.newayz.com
//#define DEFAULT_URL "https://api.newayz.com/location/hub/v1/tracks?access_key=%s&field_mask=tags,location{address,place,position}"






#define GET "GET /index.html\r\n"
#define GETRUI2 "GET /\r\n" //ok


#define GET2 "GET / HTTP/1.1\r\n"


#define POST_WAYZ_HTTP "POST /location/hub/v1/tracks?access_key=Gionq0cq0yDy66hVJOuIOSichuqKQqcV&field_mask=location{address,place,position} HTTP/1.1\r\n\
Host: 52.82.35.145:8084\r\n\
Accept: */*\r\n\
Content-Length: 213\r\n\
Content-Type: application/json\r\n\
\r\n\
{\"id\":\"123123123\",\"asset\":{\"id\":\"123123123\"},\"location\":{\"gnss\":{\"timestamp\":1567764408000,\"valid\":true,\"accuracy\":10,\"point\":{\"longitude\":121.60103,\"latitude\":31.18147,\"altitude\":20}}},\"timestamp\":1567764408000}\r\n"
//ok

#define POST_WAYZ_HTTPS "POST /location/hub/v1/tracks?access_key=Gionq0cq0yDy66hVJOuIOSichuqKQqcV&field_mask=location{address,place,position} HTTP/1.1\r\n\
Host: api.newayz.com\r\n\
Accept: */*\r\n\
Content-Length: 213\r\n\
Content-Type: application/json\r\n\
\r\n\
{\"id\":\"123123123\",\"asset\":{\"id\":\"123123123\"},\"location\":{\"gnss\":{\"timestamp\":1567764408000,\"valid\":true,\"accuracy\":10,\"point\":{\"longitude\":121.60103,\"latitude\":31.18147,\"altitude\":20}}},\"timestamp\":1567764408000}\r\n"
//ok


#define POST_WAYZ_SSL "POST /location/hub/v1/tracks?access_key=Gionq0cq0yDy66hVJOuIOSichuqKQqcV&field_mask=location{address,place,position} HTTP/1.1\r\n\
Host: api.newayz.com\r\n\
Accept: */*\r\n\
Content-Length: %d\r\n\
Content-Type: application/json\r\n\
\r\n\
%s"
//ok


//52.82.35.145:8084
#define POST_CONTENT "POST /location/hub/v1/tracks?access_key=Gionq0cq0yDy66hVJOuIOSichuqKQqcV&field_mask=location{address,place,position} HTTP/1.1\r\n\
Host: api.newayz.com\r\n\
Accept: */*\r\n\
Content-Length: %d\r\nContent-Type: %s\r\n\r\n"
#define POST_SEND "{\"id\":\"123123123\",\"asset\":{\"id\":\"123123123\"},\"location\":{\"gnss\":{\"timestamp\":1567764408000,\"valid\":true,\"accuracy\":10,\"point\":{\"longitude\":121.60103,\"latitude\":31.18147,\"altitude\":20}}},\"timestamp\":1567764408000}"
//ok


void getInfoFromRaw(iot_wifis_t *wifi_m);

#endif
