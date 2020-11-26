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

#define MAJOR_VERSION	  '1'
#define MINOR_VERSION	  '0'
#define REVERSION_VERSION '1'


#define WIFI_SSID 		"WAYZ"
#define WIFI_PASSWORD	"20192019"

#define HOST	"52.82.26.70"
#define PORT	8080


#if   (SPI_FLASH_SIZE_MAP == 2)
//use for user_ota.c upgrade fuction
#define BIN1 "user1.1024.new.2.bin"
#define BIN2 "user2.1024.new.2.bin"

#elif (SPI_FLASH_SIZE_MAP == 6)
#define BIN1 "user1.4096.new.6.bin"
#define BIN2 "user2.4096.new.6.bin"
#endif


//#define FILE_URL "/user1.1024.new.2.bin"
//#define FILE_URL2 "/user2.1024.new.2.bin"
#define FILE_URL3 "/esp8266version.txt"

//now we dont use this way to upgrade
//#define HTTP_REQUEST "GET "FILE_URL" HTTP/1.0\r\nHost: "HOST"\r\n\r\n"
//#define HTTP_REQUEST2 "GET "FILE_URL2" HTTP/1.0\r\nHost: "HOST"\r\n\r\n"
#define HTTP_VERSION "GET "FILE_URL3" HTTP/1.0\r\nHost: "HOST"\r\n\r\n"


// @0x01000
#define USER1_BIN_FLASH_SECTOR	0x01
// @0x81000 for 4MB 512kb+512kb
//#define USER2_BIN_FLASH_SECTOR	0x81
// @0x101000 for 4MB 1024kb+1024kb
#define USER2_BIN_FLASH_SECTOR	0x101

// 是否使用SSL
#define SSL_CLIENT_ENABLE		0

#if SSL_CLIENT_ENABLE
#define SSL_CLIENT_KEY_ADDR		0x9A
#define SSL_CA_ADDR				0x9B
#endif

#endif

