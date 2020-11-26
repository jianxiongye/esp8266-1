/* sniffer example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_libc.h"
#include "driver/uart.h"
#include "rom/ets_sys.h"
#include "esp_spi_flash.h"

#include "sniffer_main.h"
#include "uart.h"
#include "wifi.h"
#include "user_nvs.h"

#define TAG "sniffer"


EventGroupHandle_t wifi_event_group;

static os_timer_t channelHop_timer;
uint8_t temp_mac1[6] = {0xc4, 0x6a, 0xb7, 0x9f, 0xcc, 0x34};
uint8_t staMacAddr[6];
uint8_t apMacAddr[6];
tMiscInfo tInfoMisc = {0};

//char nvsData[OVERFLOW * SNIFFER_DATA_LEN] = {0};
//char readnvs[SEND_BODY_COUNT * SNIFFER_DATA_LEN] = {0};
char des_addr[(SNIFFER_DATA_LEN / 4 + 1) * 4] = {0};

xTaskHandle snifferHandle;

const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
	switch(type) {
	case WIFI_PKT_MGMT: return "MGMT";
	case WIFI_PKT_DATA: return "DATA";
	default:	
	case WIFI_PKT_MISC: return "MISC";
	}
}

char *create_key(int num)
{
    static char key[KEY_LEN] = {0};
    sprintf(key, "sniffer%d", num);
    return key;
}

void writeSniffDataToFlash(char* data)
{
#if 1
    int i = 0;
    char aucMacBroadCast[] = "FF:FF:FF:FF:FF:FF"; // 广播的mac地址
	char aucMacMultiCast[] = "01:00:5E"; // 多播的mac地址

    if (tInfoMisc.record < OVERFLOW)
    {
        for (i = 0; i < tInfoMisc.record; i ++)
        {
		#if 0
			if (0 == strncmp(nvsData + i * SNIFFER_DATA_LEN, data, 35))
			{
				printf("-------- data are equal -------------\r\n");
				return ;
			}
		#endif
			spi_flash_read(secB_addr + (sizeof (des_addr) * i), des_addr, sizeof (des_addr));
			if (0 == strncmp(des_addr, data, 35))
			{
				printf("-------- data are equal -------------\r\n");
				return ;
			}

			if (0 == strncmp(aucMacBroadCast, data, strlen(aucMacBroadCast)) ||  
				0 == strncmp(aucMacMultiCast, data, 8) ||
				0 == strncmp(aucMacBroadCast, data + MAC_LEN, strlen(aucMacBroadCast)) ||  
				0 == strncmp(aucMacMultiCast, data + MAC_LEN, 8))
			{
				printf("-------- data are equal broadcast -------------\r\n");
				return ;
			}
        }

		spi_flash_write(secB_addr + (sizeof (des_addr) * tInfoMisc.record), data, \
							(uint32_t)((strlen(data) / 4) + 1) * 4);
		//spi_flash_read(secB_addr + (sizeof (des_addr) * tInfoMisc.record), des_addr, sizeof (des_addr));
		//ESP_LOGI(TAG, "%s", des_addr);
		//spi_flash_read(secB_addr + ((uint32_t)((os_strlen(data)/4)+1)*4*record), des_addr,         (uint32_t)((os_strlen(data)/4)+1)*4);

		//strcpy(nvsData + tInfoMisc.record * SNIFFER_DATA_LEN, data);

        tInfoMisc.record ++;
    }
    else
    {
        diable_channel_hop();
    }
#endif
}

static void sniffer_cb(void* buf, wifi_promiscuous_pkt_type_t type)
{
    wifi_pkt_rx_ctrl_t* rx_ctrl = (wifi_pkt_rx_ctrl_t*)buf;
    uint8_t* frame = (uint8_t*)(rx_ctrl + 1);
    uint8_t i = 0; 
    char fifo[SNIFFER_DATA_LEN] = {0};
	struct framecontrol_t *framecontrol;

	framecontrol = (struct framecontrol_t*)(buf + 12);

    switch (type) {
        case WIFI_PKT_MGMT:
			if(framecontrol->subtype == SUBTYPE_PRBREQ \
				|| framecontrol->subtype == SUBTYPE_ASSREQ \
				|| framecontrol->subtype == SUBTYPE_RESREQ)//00|0d  00|00
				//|| framecontrol->subtype == SUBTYPE_ACTION 00|13 判断不了发送和接受端
				goto printDS;
			else if(framecontrol->subtype == SUBTYPE_RESREP \
				  ||framecontrol->subtype == SUBTYPE_ASSREP \
				  ||framecontrol->subtype == SUBTYPE_PRBREP)//00|01
				goto printSD;
			//认证的时候，ap也会发送数据，需要通过BSSID判断发送方
			else if(framecontrol->subtype == SUBTYPE_AUTH ){ 
				if(memcmp(&frame[4], &frame[16], 6)){
					goto printSD;
				}else
				{
					goto printDS;
				}	
			}
			return;

        case WIFI_PKT_CTRL:
            return;

        case WIFI_PKT_DATA:
            	//ap发出的帧,可以用来当作探针数据
				//datadir为0时为IBSS，设备都是对等的，无需此模式;为3时是WDS中继，也不需要的
			switch(framecontrol->datadir){
				case 0:
					break;
				case DATA_DIR_TOAP:
					goto printDS;
					break;
				case DATA_DIR_FROMAP:
					if(!memcmp(&frame[10], &frame[16], 6))
					{
						goto printSD;
					}	
					else
					{
						goto print32;
					}	
					break;
				default:
					break;
			}
            break;

        case WIFI_PKT_MISC:
            return;
            break;

        default :
			return;
    }
	return;
				
printSD:
	//打印本机stamac
	// 缓存上次的MAC，避免重复打印
	if(0 != frame[4] % 4)//如果不能被4整除除，那就不是我们需要的mac
		return;
	// 如果MAC地址和上一次一样就返回
	if(0==memcmp(temp_mac1, &frame[4], 6)){
		return;
	}
	for (i=0; i<6; i++){
		temp_mac1[i] = frame[i+4];
	}
	//check 真实mac地址
	
	sprintf(fifo, ""MACPRINT"|"MACPRINT"|%02d|%02d|%02d|%d|0|%d|%d\n", PRINT(frame,4), PRINT(frame,10),\
            framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi, framecontrol->datadir, tInfoMisc.minute);
	//memcpy(mstore, &frame[4], 6);
    printf("printSD: %s", fifo);
	writeSniffDataToFlash(fifo);
	return;

print32:
	if(0 != frame[16] % 4)//如果不能被4整除除，那就不是我们需要的mac
		return;

	// 缓存上次的MAC，避免重复打印
	if(0==memcmp(temp_mac1, &frame[16], 6)){
		return;
	}
	for (i = 0; i < 6; i ++){
		temp_mac1[i] = frame[i+16];
	}
	sprintf(fifo, ""MACPRINT"|"MACPRINT"|%02d|%02d|%02d|%d|0|%d|%d\n", PRINT(frame,16), PRINT(frame,10),\
            framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi, framecontrol->datadir, tInfoMisc.minute);
	//memcpy(mstore, &frame[16], 6);
    printf("print32: %s", fifo);
	writeSniffDataToFlash(fifo);
	return ;

printDS:
	if(0 != frame[10] % 4)//如果不能被4整除除，那就不是我们需要的mac
		return;
	if(0 == memcmp(temp_mac1, &frame[10], 6)){
		return;
	}
	for (i = 0; i < 6; i ++){
		temp_mac1[i] = frame[i+10];
	}
	sprintf(fifo, ""MACPRINT"|"MACPRINT"|%02d|%02d|%02d|%d|0|%d|%d\n", PRINT(frame,10), PRINT(frame,4),\
            framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi, framecontrol->datadir, tInfoMisc.minute);
	//memcpy(mstore, &frame[10], 6);
    printf("printDS: %s", fifo);
	writeSniffDataToFlash(fifo);
	return;
}

void diable_channel_hop()
{
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
	os_printf("stateMachine[d]:switch to send data mode\n");
	os_timer_disarm(&channelHop_timer);
	printf("diable_channel_hop: %d \n", tInfoMisc.record);
	xEventGroupSetBits(wifi_event_group, SEND_BIT);
}


void channelHopFunc(void *arg)
{
    static uint8_t new_channel = CONFIG_CHANNEL;
    uint8_t sendDataTime = 0;
	new_channel ++;
	if(new_channel > MAX_CHN)
	{
		new_channel = CONFIG_CHANNEL;
	}

    esp_wifi_set_channel(new_channel, 0);
	tInfoMisc.count ++;
	if(tInfoMisc.count % 240 == 0){
		tInfoMisc.minute ++;
		if (tInfoMisc.minute < SNIFFER_MIN_TIME)
		{
			printf("time is up %dmin\n", tInfoMisc.minute);
		}
	}

    sendDataTime = tInfoMisc.tuartWifiInfo.mSniffSendMin >= 1 ? tInfoMisc.tuartWifiInfo.mSniffSendMin : SNIFFER_MIN_TIME;
	if(tInfoMisc.minute >= sendDataTime){
		printf("time is up %dmin send data now\n", SNIFFER_MIN_TIME);
		tInfoMisc.minute = 0;
		tInfoMisc.count = 0;
		diable_channel_hop();
	}
}

static void sniffer_task(void* pvParameters)
{
    wifi_promiscuous_filter_t sniffer_filter = {0};
	while(1){
		printf("----start------sniffer\n");
        wifi_scan();
		flash_erase();
	#if CONFIG_FILTER_MASK_MGMT
		sniffer_filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT;
	#endif

	#if CONFIG_FILTER_MASK_CTRL
		sniffer_filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL;
	#endif

	#if CONFIG_FILTER_MASK_DATA
		sniffer_filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA;
	#endif

	#if CONFIG_FILTER_MASK_DATA_FRAME_PAYLOAD
		/*Enable to receive the correct data frame payload*/
		extern esp_err_t esp_wifi_set_recv_data_frame_payload(bool enable_recv);
		ESP_ERROR_CHECK(esp_wifi_set_recv_data_frame_payload(true));
	#endif

	#if CONFIG_FILTER_MASK_MISC
		sniffer_filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_MISC;
	#endif

		if (sniffer_filter.filter_mask == 0) {
			ESP_LOGI(TAG, "Please add one filter at least!");
			vTaskDelete(NULL);
		}

		xEventGroupWaitBits(wifi_event_group, START_BIT,
							false, true, portMAX_DELAY);
		ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_CHANNEL, 0));
		ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(sniffer_cb));
		ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&sniffer_filter));

		ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

	#if HOP_JUMP_ENABLE // 1min 切换一个通道
		os_timer_disarm(&channelHop_timer);
		os_timer_setfn(&channelHop_timer, (os_timer_func_t *)channelHopFunc, NULL);
		os_timer_arm(&channelHop_timer, TIMEOUT, REPEATFLG);
	#endif
        printf("sniffer_task resume!\n");
		vTaskSuspend(NULL);
	}

	while(1)
    {
		vTaskDelay(1000/portTICK_PERIOD_MS );
		printf("working...\n");
	}
	vTaskDelete(NULL);
}

esp_err_t get_chip_id(uint32_t* chip_id){
    esp_err_t status = ESP_OK;
    *chip_id = (REG_READ(0x3FF00050) & 0xFF000000) |
                         (REG_READ(0x3ff0005C) & 0xFFFFFF);
    return status;
}

static void prinfChipVersion()
{
	uint32_t id ;
    get_chip_id(&id);

	esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ", chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	printf("software version: 20%02d-%02d-%02d \r\n", VER_H, VER_M, VER_L); 

	printf("\n\n------------------ Get Systrm Info------------------\n");
	printf("File: %s, \nLine: %d, Date: %s, Time: %s, Timestamp: %s\n", __FILE__, __LINE__, __DATE__, __TIME__, __TIMESTAMP__);    
    //获取IDF版本
    printf("     SDK version:%s, chip id: %u\n", esp_get_idf_version(), id);
    //获取芯片可用内存
    printf("     esp_get_free_heap_size : %d  \n", esp_get_free_heap_size());
    //获取从未使用过的最小内存
    printf("     esp_get_minimum_free_heap_size : %d  \n", esp_get_minimum_free_heap_size());
    //获取芯片的内存分布，返回值具体见结构体 flash_size_map
    printf("     system_get_flash_size_map(): %d \n", system_get_flash_size_map());
    //获取mac地址（station模式）
    esp_wifi_get_mac(WIFI_MODE_STA, staMacAddr);
    printf(" Station esp_wifi_get_mac(): %02x:%02x:%02x:%02x:%02x:%02x \n", 
            staMacAddr[0], staMacAddr[1], staMacAddr[2], staMacAddr[3], staMacAddr[4], staMacAddr[5]);
    //获取mac地址（ap模式）
    esp_wifi_get_mac(WIFI_MODE_AP, apMacAddr);
    printf(" AP esp_wifi_get_mac(): %02x:%02x:%02x:%02x:%02x:%02x \n", 
            apMacAddr[0], apMacAddr[1], apMacAddr[2], apMacAddr[3], apMacAddr[4], apMacAddr[5]);
    printf("----------------------------------------------------------\n\n");
}

void read_nvs_wifi_info()
{
	tInfoWifi temp = {0};
	NVS_Read(WIFI_INFO_KEY, &temp, sizeof (tInfoMisc.tuartWifiInfo));
	ESP_LOGI(WIFI_INFO_KEY, "count : %d", sizeof (temp));
	ESP_LOGI(WIFI_INFO_KEY, "wifi name: %s", temp.mWifiName);
	ESP_LOGI(WIFI_INFO_KEY, "wifi passwd: %s", temp.mWifiPass);
	ESP_LOGI(WIFI_INFO_KEY, "server: %s, %d", temp.mServer, temp.mPort);
	ESP_LOGI(WIFI_INFO_KEY, "send data time: %d min", temp.mSniffSendMin);
	memcpy(&tInfoMisc.tuartWifiInfo, &temp, sizeof (temp));
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_event_group = xEventGroupCreate();
    uart_init(115200);
    prinfChipVersion();
    wifi_init();
	read_nvs_wifi_info();
    //NVS_erase_all();
	create_tcp_task();
    xTaskCreate(sniffer_task, "sniffer_task", 2048, NULL, 10, &snifferHandle);
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
}
