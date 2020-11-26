/*
 * user_wifi.c
 */

#include "driver/uart.h"
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "http_client.h"
#include "user_config.h"
struct ip_addr ipaddress;
extern uint8 mServer[32];
extern u16 mPort;

/*
 * function: user_set_station_config
 * parameter: u8* ssid - WiFi SSID
 *            u8 password - WiFi password
 */
void ICACHE_FLASH_ATTR
user_set_station_config(u8* ssid, u8* password) {
	struct station_config stationConf;
	stationConf.bssid_set = 0;		//need	not	check	MAC	address	of	AP
	os_memcpy(&stationConf.ssid, ssid, 32);
	os_memcpy(&stationConf.password, password, 64);
	wifi_station_set_config(&stationConf);
}

/*
 * function: wifi_handle_event_cb
 * parameter: System_Event_t *evt
 */
void ICACHE_FLASH_ATTR
wifi_handle_event_cb(System_Event_t *evt) {
	os_printf("event %x\n", evt->event);

	switch (evt->event) {
	case EVENT_STAMODE_CONNECTED:
		os_printf("connect to ssid %s, channel %d\n",
				evt->event_info.connected.ssid,
				evt->event_info.connected.channel);
		
		break;
	case EVENT_STAMODE_DISCONNECTED:
		os_printf("disconnect from ssid %s, reason %d\n",
				evt->event_info.disconnected.ssid,
				evt->event_info.disconnected.reason);
		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
		os_printf("mode: %d -> %d\n", evt->event_info.auth_change.old_mode,
				evt->event_info.auth_change.new_mode);
		break;
	case EVENT_STAMODE_GOT_IP:
		ipaddress= evt->event_info.got_ip.ip;
		os_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
				IP2STR(&evt->event_info.got_ip.ip),
				IP2STR(&evt->event_info.got_ip.mask),
				IP2STR(&evt->event_info.got_ip.gw));
		os_printf("\n");
		
		//move wifiscan(); to sntp
		//dns_test();

		// TODO:
		http_client_init(mServer, mPort);
		task_post('s');
		break;
	case EVENT_SOFTAPMODE_STACONNECTED:
		os_printf("station: " MACSTR "join, AID = %d\n",
				MAC2STR(evt->event_info.sta_connected.mac),
				evt->event_info.sta_connected.aid);
		break;
	case EVENT_SOFTAPMODE_STADISCONNECTED:
		os_printf("station: " MACSTR "leave, AID = %d\n",
				MAC2STR(evt->event_info.sta_disconnected.mac),
				evt->event_info.sta_disconnected.aid);
		break;
	default:
		break;
	}
}
extern uint8 mWifiName[32];
extern uint8 mWifiPass[32];

/*
 * function: user_wifi_init
 * parameter: void
 * return: void
 */
void ICACHE_FLASH_ATTR
user_wifi_init(void) {
	wifi_set_sleep_type(NONE_SLEEP_T);

	wifi_set_opmode(STATION_MODE);
	user_set_station_config(mWifiName, mWifiPass);
	wifi_set_event_handler_cb(wifi_handle_event_cb);
	wifi_station_disconnect();
	wifi_station_connect();
	os_printf("user_wifi_init");
}

/*
 * function: user_wifi_init
 * parameter: void
 * return: void
 */
void ICACHE_FLASH_ATTR
user_wifi_initNP(char* SSID,char* PASS) {
	//wifi_set_sleep_type(NONE_SLEEP_T);

	//wifi_set_opmode(STATION_MODE);
	user_set_station_config(SSID, PASS);
	//wifi_set_event_handler_cb(wifi_handle_event_cb);
	//wifi_station_disconnect();
	//wifi_station_connect();
	os_printf("user_wifi_initNP");
	
}

