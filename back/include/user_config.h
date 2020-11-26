/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define AP_CACHE           0

#if AP_CACHE
#define AP_CACHE_NUMBER    5
#endif

//#define WIFI_SSID 		"asdf"
//#define WIFI_PASSWORD	"qwertyui"
//#define WIFI_SSID 		"WAYZ Guest"
//#define WIFI_PASSWORD	"zz20192019"
#define WIFI_SSID 		"Wayz Public Device"
#define WIFI_PASSWORD	"wayz2020"



// baidu
//#define HOST	"119.75.217.109"
//#define PORT	80
// local
//#define HOST	"192.168.100.128"

#define HOST	"122.51.2.195"

#define PORT	3334
#define HOST_WAYZ	"52.82.26.70"
#define HOST_WAYZ_DNS	"api.newayz.com"
//161.189.17.53




#define FILE_URL "/"
#define _HTTP_HEADER_HOST	"Host: "HOST"\r\n"
#define HTTP_REQUEST "GET zaki"FILE_URL" HTTP/1.0\r\n"_HTTP_HEADER_HOST"\r\n"
#define HTTP_POST "01:00:5E:7F:FF:FA|70:3A:73:0D:FC:3F|02|00|01|-73|0|2|0\n\
F4:D1:08:9C:4A:82|70:3A:73:0D:FC:3F|02|12|01|-93|0|1|0\n\
01:00:5E:7F:FF:FA|70:3A:73:0D:FC:3F|02|00|01|-76|0|2|0\n\
01:00:5E:00:00:16|70:3A:73:0D:FC:3F|02|00|01|-74|0|2|0\n"

// 是否使用SSL
#define SSL_CLIENT_ENABLE		0
#define SSL_CLIENT_WAYZ			1


#if SSL_CLIENT_WAYZ
#define SSL_CLIENT_KEY_ADDR		0x9A
#define SSL_CA_ADDR				0x9B
#endif

#define POST_WAYZ_HTTP "POST /location/hub/v1/tracks?access_key=Gionq0cq0yDy66hVJOuIOSichuqKQqcV&field_mask=location{address,place,position} HTTP/1.1\r\n\
Host: 52.82.35.145:8084\r\n\
Accept: */*\r\n\
Content-Length: 213\r\n\
Content-Type: application/json\r\n\
\r\n\
{\"id\":\"123123123\",\"asset\":{\"id\":\"123123123\"},\"location\":{\"gnss\":{\"timestamp\":1567764408000,\"valid\":true,\"accuracy\":10,\"point\":{\"longitude\":121.60103,\"latitude\":31.18147,\"altitude\":20}}},\"timestamp\":1567764408000}\r\n"


#define POST_WAYZ_HTTPS "POST /location/hub/v1/tracks?access_key=Gionq0cq0yDy66hVJOuIOSichuqKQqcV&field_mask=location{address,place,position} HTTP/1.1\r\n\
Host: api.newayz.com\r\n\
Accept: */*\r\n\
Content-Length: 213\r\n\
Content-Type: application/json\r\n\
\r\n\
{\"id\":\"123123123\",\"asset\":{\"id\":\"123123123\"},\"location\":{\"gnss\":{\"timestamp\":1567764408000,\"valid\":true,\"accuracy\":10,\"point\":{\"longitude\":121.60103,\"latitude\":31.18147,\"altitude\":20}}},\"timestamp\":1567764408000}\r\n"

#define POST_CONTENT "POST /location/hub/v1/tracks?access_key=Gionq0cq0yDy66hVJOuIOSichuqKQqcV&field_mask=location{address,place,position} HTTP/1.1\r\n\
Host: api.newayz.com\r\n\
Accept: */*\r\n\
Content-Length: %d\r\nContent-Type: %s\r\n\r\n"
#define POST_SEND "{\"id\":\"123123123\",\"asset\":{\"id\":\"123123123\"},\"location\":{\"gnss\":{\"timestamp\":1567764408000,\"valid\":true,\"accuracy\":10,\"point\":{\"longitude\":121.60103,\"latitude\":31.18147,\"altitude\":20}}},\"timestamp\":1567764408000}"

#define POST_WAYZ_SSL "POST /location/hub/v1/tracks?access_key=Gionq0cq0yDy66hVJOuIOSichuqKQqcV&field_mask=location{address,place,position} HTTP/1.1\r\n\
Host: api.newayz.com\r\n\
Accept: */*\r\n\
Content-Length: %d\r\n\
Content-Type: application/json\r\n\
\r\n\
%s"
#define POST_WAYZ_CONTENT "%s%s"
#define POST_WAYZ_INFO "{\"timestamp\":%u000,\"id\":\"%u\",\"asset\":{\"id\":\""MACSTR"\",\"manufacturer\":\"ESP\",\"model\":\"ESP8266\",\"macAddress\":\""MACSTR"\",\"serialNumber\":\"\",\"os\":{\"type\":\"None\",\"version\":\"\"}},%s"
#define POST_WAYZ_LO   "\"location\":{\"wifis\":%s}}"
#define POST_WAYZ_WIFIS "[%s]"
#define POST_WAYZ_WIFI "{\"signalStrength\":%d,\"macAddress\":\""MACSTR"\",\"ssid\":\"%s\",\"channel\":%d}"
#define SCAN_WAYZ_WIFI ""MACSTR"|%d|%d|%s"





#endif

