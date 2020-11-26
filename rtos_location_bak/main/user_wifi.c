#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/ets_sys.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "user_wifi.h"


static const char *TAG = "user_wifi";

tApInfo aucApInfo = {0};
uint8_t staModleStartFlg = 0;
static wifi_scan_config_t scanConf  = {
	.ssid = NULL,
	.bssid = NULL,
	.channel = 0,
	.show_hidden = true
};

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    system_event_info_t *info = &event->event_info;
    
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        xEventGroupSetBits(wifi_event_group, START_BIT);
        if (ENBLE == staModleStartFlg)
        {
            ESP_ERROR_CHECK( esp_wifi_connect() );
        }
		ESP_LOGI(TAG, "system event sta start! \r\n");
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        staModleStartFlg = DISABLE;
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
        if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
            /*Switch to 802.11 bgn mode */
            esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
        }
        if (ENBLE == staModleStartFlg)
        {
            ESP_ERROR_CHECK( esp_wifi_connect() );
        }

        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
		ESP_LOGI(TAG, "system event sta disconnected! \r\n");
        break;
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init(void)
{
    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_init_softap(uint8_t *pSsid, uint8_t *pPass)
{
    char ssid[32] = {0};
    char passwd[64] = {0};
    wifi_config_t wifi_config = {
        .ap = {
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    NULL == pSsid ? sprintf(ssid, "%s", WIFI_SSID) : sprintf(ssid, "%s", pSsid);
    NULL == pPass ? sprintf(passwd, "%s", WIFI_PASS) : sprintf(passwd, "%s", pPass);

    ESP_ERROR_CHECK(esp_wifi_stop());

    memcpy(wifi_config.ap.ssid, ssid, 32);
    memcpy(wifi_config.ap.password, passwd, 64);
    wifi_config.ap.ssid_len = strlen(ssid);

    ESP_ERROR_CHECK(esp_wifi_stop());

    if (strlen(passwd) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s", ssid, passwd);
}

void wifi_init_sta()
{
    ESP_LOGI(TAG, "wifi_init_sta start.");
    char ssid[32] = {0};
    char passwd[64] = {0};
    wifi_config_t wifi_config = {0};

    //0 == strlen((char *)tInfoMisc.tuartWifiInfo.mWifiName) ? sprintf(ssid, "%s", WIFI_SSID) : sprintf(ssid, "%s", tInfoMisc.tuartWifiInfo.mWifiName);
    //0 == strlen((char *)tInfoMisc.tuartWifiInfo.mWifiPass) ? sprintf(passwd, "%s", WIFI_PASS) : sprintf(passwd, "%s", tInfoMisc.tuartWifiInfo.mWifiPass);
    staModleStartFlg = ENBLE;
    ESP_ERROR_CHECK(esp_wifi_stop());

    sprintf(ssid, "%s", WIFI_SSID);
    sprintf(passwd, "%s", WIFI_PASS);
    memcpy(wifi_config.sta.ssid, ssid, 32);
    memcpy(wifi_config.sta.password, passwd, 64);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s", ssid, passwd);
}

void wifi_scan()
{
	uint16_t apCount;
	int i = 0;
    char fifo[70] = {0};
    int count = 0;
	
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
		printf("%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%s\n", MAC2STR(aucApInfo.tinfoAp[i].mac), 
				aucApInfo.tinfoAp[i].rssi, aucApInfo.tinfoAp[i].channel, aucApInfo.tinfoAp[i].ssid);
        sprintf(fifo, "%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%s\n", MAC2STR(aucApInfo.tinfoAp[i].mac), 
				aucApInfo.tinfoAp[i].rssi, aucApInfo.tinfoAp[i].channel, aucApInfo.tinfoAp[i].ssid);
        count += strlen(fifo);
	}
    aucApInfo.length = count;
	free(list);
    //NVS_Write(AP_INFO_KEY, &aucApInfo, sizeof(aucApInfo));
}

uint16_t chnTofreq(uint8_t channel)
{
    return (FREQ_START + (channel - 1) * SEGMEMTATION);
}


