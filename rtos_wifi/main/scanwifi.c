#if 1
#include "scanwifi.h"


static wifi_scan_config_t scanConf  = {
	.ssid = NULL,
	.bssid = NULL,
	.channel = 0,
	.show_hidden = true
};


void
wifiscan(){
	
	//bzero(buf2,BUF_LEN);
	//bzero(buf,BUF_LEN);
	uint16_t apCount;

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


	// A8:0C:CA:01:76:86|-56|1|WAYZ Guest
	int i;
	for (i=0; i<apCount; i++) {
		
		printf(""MACSTR"|%d|%d|%s\n",MAC2STR(list[i].bssid), list[i].rssi,list[i].primary, list[i].ssid);
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


void keepinfo(){

	spi_flash_read(secA_addr, des_addr,  4);
	char* date;
	date=(char*)des_addr;
	esp_wifi_get_mac(ESP_IF_WIFI_STA, staMacAddr);

	printf("----------info\n");
	printf("FwVersion=%s\n",FwVersion);
	//printf("ChipId=%d\n",system_get_chip_id());
	printf("ChipId=%d\n",100);

	printf("Mac="MACSTR"\n", MAC2STR(staMacAddr));
	printf("Name=FindU\n");
	printf("Manufacturer=01\n");
	printf("ProduceDate=%02d%02d%02d%02d\n",date[0],date[1],date[2],date[3]);
	


}


#endif

