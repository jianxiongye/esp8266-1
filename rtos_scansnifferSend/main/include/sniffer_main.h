
#ifndef __SNIFFER_MAIN_
#define __SNIFFER_MAIN_

#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define  VER_H               20
#define  VER_M               9
#define  VER_L               30

#define  HOP_JUMP_ENABLE     (1)
#define  TIMEOUT             (50)
#define  REPEATFLG           (1)  // 是否重复
#define  SNIFFER_MAX_TIME    (10 * 60) //sniffer 时间，当到达后将关闭混杂模式即关闭sniffer 单位为秒
#define  SNIFFER_MIN_TIME    (1) //sniffer 时间，当到达后将关闭混杂模式即关闭sniffer 单位为秒
#define  TIMEOUT_COUNT		 (20) // 1s 有 20个 50ms；1分钟有 20 * 60 = 1200 个 50ms
#define  SCANTIME			 (1)//(3600) // 单位s，1h = 60 * 60

#define  PRINT( buf, i )     buf[i + 0], buf[i + 1], buf[i + 2], buf[i + 3], buf[i + 4], buf[i + 5]
#define  MACPRINT            "%02X:%02X:%02X:%02X:%02X:%02X"
#define  WIFI_CONNECTED_BIT  BIT0  // wifi连接事件
#define  START_BIT           BIT1  // wifi 开始时通知sniffer任务
#define  SEND_BIT			 BIT2
#define  OTA_BIT			 BIT3

#define  FRAME_MANAGE        0
#define  FRAME_CONTROL       1
#define  FRAME_DATA          2
#define  FRAME_RESERVED      3

#define  SUBTYPE_ASSREQ      0   // 连接请求
#define  SUBTYPE_ASSREP      1   // 连接响应
#define  SUBTYPE_RESREQ      2   // 重连接请求
#define  SUBTYPE_RESREP      3   // 重连接响应
#define  SUBTYPE_PRBREQ      4   // 探测请求
#define  SUBTYPE_PRBREP      5   // 探测响应
#define  SUBTYPE_TIMEAD      6
#define  SUBTYPE_RESERV      7
#define  SUBTYPE_BEACON      8
#define  SUBTYPE_ATIM        9
#define  SUBTYPE_DISASS      10
#define  SUBTYPE_AUTH        11  // 身份认证
#define  SUBTYPE_DEAUTH      12
#define  SUBTYPE_ACTION      13
#define  SUBTYPE_ANOACK      14
#define  SUBTYPE_RESER2      15

#define  SUBTYPE_DATA        0
#define  SUBTYPE_NULL_DATA   4
#define  SUBTYPE_QOS_DATA    8
#define  SUBTYPE_QOS_NULL_DATA 12 //0xc

#define  DATA_DIR_TOAP       1
#define  DATA_DIR_FROMAP     2

#define  SEND_HEAD           0x30 // 发送的头部
#define  OVERFLOW            7000 // sniffer 探测的最大条数
#define  MAC_LEN             18   // 字符串mac地址的长度+'\n'：FF:FF:FF:FF:FF:FF
#define  SNIFFER_DATA_LEN    73   // 组织sniffer 探测数据的长度
#define  SEND_BODY_COUNT     18    // 发送数据，每18条发送一次
#define  FIFO_COUNT			 200  // 缓存200条数据


#define  ENBLE               (1)
#define  DISABLE             (0)
#define  MAX_CHN             (13)

#define  FLASH_START_ADDR    (0x330000) // 存储sniffer扫描数据

#define  PRINT_TIME_EN		 (0)
#define  PRINT_LOG_EN		 (1)

#if PRINT_LOG_EN
#define  qDebug(...)  printf(__VA_ARGS__)
#else
#define  qDebug(...)  
#endif


#pragma pack(push)
#pragma pack(1)

struct framecontrol_t{
	unsigned:2;//frame protocol
	unsigned frametype:2;//frame type
	unsigned subtype:4;//sub type
	unsigned datadir:2;
	unsigned:6;//other
};

typedef struct {
	unsigned frame_ctrl:16;
	unsigned duration_id:16;
	uint8_t addr1[6]; /* receiver address */
	uint8_t addr2[6]; /* sender address */
	uint8_t addr3[6]; /* filtering address */
	unsigned sequence_ctrl:16;
	uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
	wifi_ieee80211_mac_hdr_t hdr;
	uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

typedef struct __wifi_info_
{
    uint8_t mWifiName[32];
    uint8_t mWifiPass[32];
    uint8_t mServer[32];
    uint16_t mPort;
    uint32_t mSniffSendMin;
}tInfoWifi;

typedef struct _misc_
{
    uint32_t count; // 定时器中计数
    uint8_t minute; // 分钟数
    tInfoWifi tuartWifiInfo;
    uint32_t record; // sniffer计数
	uint8_t fifoFlag; // 默认数据存入缓存
	uint8_t scanFlag; // 默认数据存入缓存
}tMiscInfo;




#pragma pack(pop)

void diable_channel_hop();
char *create_key(int num);
extern EventGroupHandle_t wifi_event_group;
extern tMiscInfo tInfoMisc;
extern xTaskHandle snifferHandle;
extern uint8_t staMacAddr[6];
extern char nvsData[FIFO_COUNT * SNIFFER_DATA_LEN];
extern char des_addr[(SNIFFER_DATA_LEN / 4 + 1) * 4];

#endif

