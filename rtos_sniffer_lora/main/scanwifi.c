#if 1
#include "scanwifi.h"

tApInfo aucApInfo = {0};

static wifi_scan_config_t scanConf  = {
	.ssid = NULL,
	.bssid = NULL,
	.channel = 0,
	.show_hidden = true
};

// wifiscan模块，但未写入内存中，供其他参数调用
void wifiscan(){
	
	//bzero(buf2,BUF_LEN);
	//bzero(buf,BUF_LEN);
	uint16_t apCount;
	int i = 0;
	
	esp_wifi_disconnect();
	ESP_ERROR_CHECK(esp_wifi_scan_stop());
	//The true parameter cause the function to block until the scan is done.
	ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConf, true));
	apCount = 0;
	esp_wifi_scan_get_ap_num(&apCount);
	//printf("Number of access points found: %d\n", apCount);
	if (apCount == 0) {
		printf("00:00:00:00:00:00|0|0|\n");
		return;
	}
	wifi_ap_record_t *list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, list));

	printf("----------scandone\n");

	memset(&aucApInfo, 0, sizeof (aucApInfo));

	// A8:0C:CA:01:76:86|-56|1|WAYZ Guest

	aucApInfo.count = apCount;
	printf("-----find ap number: %d ---------\r\n", aucApInfo.count);
	for (i = 0; i < apCount; i++) {
		//printf(""MACSTR"|%d|%d|%s\n", MAC2STR(list[i].bssid), list[i].rssi, list[i].primary, list[i].ssid);
		aucApInfo.tinfoAp[i].channel = list[i].primary;
		aucApInfo.tinfoAp[i].rssi = list[i].rssi;
		strncpy((char *)aucApInfo.tinfoAp[i].mac, (char *)list[i].bssid, 6 );
		strcpy( (char *)aucApInfo.tinfoAp[i].ssid, (char *)list[i].ssid );
		printf("%02X:%02X:%02X:%02X:%02X:%02X|%d|%d|%s\n", MAC2STR(aucApInfo.tinfoAp[i].mac), 
				aucApInfo.tinfoAp[i].rssi, aucApInfo.tinfoAp[i].channel, aucApInfo.tinfoAp[i].ssid);
	}
	free(list);

	// scan again
	//vTaskDelay(2000 / portTICK_PERIOD_MS);
	//The true parameter cause the function to block until the scan is done.
	//ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConf, true));
	keepinfo();

	//sprintf(buf, POST_WAYZ_WIFI, );
}

//uint32 spi_flash_get_id(void);
//SpiFlashOpResult spi_flash_erase_sector(uint16 sec);
//SpiFlashOpResult spi_flash_write(uint32 des_addr, uint32 *src_addr, uint32 size);
//SpiFlashOpResult spi_flash_read(uint32 src_addr, uint32 *des_addr, uint32 size);
#define FourKB 0x1000

uint32_t des_addr[FourKB/4];
extern uint32_t secA_addr;//1024+1024
extern uint32_t secB_addr;
uint32_t i; 
uint32_t static_addr[20]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};

uint32_t size=FourKB;
#define FwVersion "2.0"
uint8_t staMacAddr[6];

// 打印相关信息
void keepinfo(){

	spi_flash_read(secA_addr, des_addr,  4);
	char* date;
	date = (char*)des_addr;
	esp_wifi_get_mac(ESP_IF_WIFI_STA, staMacAddr);

	printf("----------info\n");
	printf("FwVersion=%s\n", FwVersion);
	//printf("ChipId=%d\n",system_get_chip_id());
	printf("ChipId=%d\n", 100);

	printf("Mac="MACSTR"\n", MAC2STR(staMacAddr));
	printf("Name=FindU\n");
	printf("Manufacturer=01\n");
	printf("ProduceDate=%02d%02d%02d%02d\n",date[0],date[1],date[2],date[3]);
}


#endif

