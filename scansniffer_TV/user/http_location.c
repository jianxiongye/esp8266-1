/*
 * http_client.c
 */

#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"
#include "http_client.h"
#include "user_config.h"

#define TCP_BUF_LEN		2000
static u8 tcp_rev_buff[TCP_BUF_LEN];
static u16 rev_data_len;

static struct espconn g_tcp_client_conn;
static struct espconn *g_ptcp_conn = &g_tcp_client_conn;


#define BUF_LEN 1024*4
char buf[BUF_LEN];//4kb
char buf2[BUF_LEN];//4kb
char buf3[BUF_LEN];

/*
 * function: tcp_client_send_cb
 */
static void ICACHE_FLASH_ATTR
tcp_client_send_cb(void *arg) {
	os_printf("tcp_client_send_cb\n");
}

/*
 * function:tcp_client_discon_cb
 */
static void ICACHE_FLASH_ATTR
tcp_client_discon_cb(void *arg) {
	os_printf("tcp_client_discon_cb\n");
}

/*
 * function: tcp_client_recv
 */
static void ICACHE_FLASH_ATTR
tcp_client_recv(void *arg, char *pdata, unsigned short len) {
	os_printf("tcp_client_recv\n");

	os_printf("lenth:%d\r\n", len);
	os_printf("data:%s\r\n", pdata);
	task_post('a');

	// TODO:
#if 0
	os_memset(tcp_rev_buff, 0, TCP_BUF_LEN);
	os_memcpy(tcp_rev_buff, pdata, len);
	rev_data_len = len;
#endif

}

/*
 * function: tcp_client_recon_cb
 */
static void ICACHE_FLASH_ATTR
tcp_client_recon_cb(void *arg) {
	struct espconn *pesp_conn = arg;

	os_printf("tcp_client_recon_cb\r\n");
}

/*
 * function: tcp_client_connect_cb
 */
static void ICACHE_FLASH_ATTR
tcp_client_connect_cb(void *arg) {
	struct espconn *pesp_conn = arg;

	espconn_regist_disconcb(pesp_conn, tcp_client_discon_cb);
	espconn_regist_recvcb(pesp_conn, tcp_client_recv);
	espconn_regist_sentcb(pesp_conn, tcp_client_send_cb);
	//espconn_regist_reconcb(pesp_conn, tcp_client_recon_cb);

	os_printf("tcp_client_connect_cb\r\n");

#if SSL_CLIENT_WAYZ
	//espconn_secure_send(pesp_conn, HTTP_REQUEST, os_strlen(HTTP_REQUEST));
	
	espconn_secure_send(pesp_conn, buf, os_strlen(buf));
#else
	//espconn_send(pesp_conn, hello, os_strlen(hello));
	espconn_send(pesp_conn, HTTP_REQUEST, os_strlen(HTTP_REQUEST));
#endif
}
uint32 wayzipaddr32=0;

LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_dns_found(const char *name, ip_addr_t *ipaddr, void *arg) {
	struct espconn *pespconn = (struct espconn *)arg;
	if (ipaddr != NULL){
		os_printf("user_esp_platform_dns_found %d.%d.%d.%d\n",
		*((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
		*((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));
		wayzipaddr32 = ipaddr->addr;
		http_location_init(443);
	}else{
		os_printf("dns error!\r\n");
	}
}
void ICACHE_FLASH_ATTR
dns_test(void) {
	espconn_gethostbyname(g_ptcp_conn,HOST_WAYZ_DNS, &ipaddress,
	user_esp_platform_dns_found);
	os_printf("start dns\r\n");
}


/*
 * function: http_client_init
 * parameter: u8* ip - server ip
 *            u16 port - server port
 */
void ICACHE_FLASH_ATTR
http_location_init(u16 port) {
	static esp_tcp esptcp;
	esp_tcp *pesptcp = &esptcp;


	os_printf("http_client_init\n");

//	g_tcp_client_conn.type = ESPCONN_TCP;
//	g_tcp_client_conn.state = ESPCONN_NONE;
//	g_tcp_client_conn.proto.tcp = pesptcp;

	g_ptcp_conn->type = ESPCONN_TCP;
	g_ptcp_conn->state = ESPCONN_NONE;
	g_ptcp_conn->proto.tcp = pesptcp;

	os_memcpy(pesptcp->remote_ip, &wayzipaddr32, 4); //set server ip
	pesptcp->remote_port = port;
	pesptcp->local_port = espconn_port();

//	os_memcpy(g_ptcp_conn->proto.tcp->remote_ip, &u32_ip, 4); //set server ip
//	g_ptcp_conn->proto.tcp->remote_port = port;			//set server port
//	g_ptcp_conn->proto.tcp->local_port = espconn_port();

	espconn_regist_connectcb(g_ptcp_conn, tcp_client_connect_cb);
	http_location_connect();


}

void ICACHE_FLASH_ATTR
http_location_connect(void) {
	s8 rc = 0;
	espconn_disconnect(g_ptcp_conn);
	
#if SSL_CLIENT_WAYZ
	//espconn_secure_ca_enable(0x01, SSL_CA_ADDR);
	//espconn_secure_cert_req_enable(0x01, SSL_CLIENT_KEY_ADDR);
	//连接失败的情况下一定要尝试增大buffer
	espconn_secure_set_size(0x01, 4096);
	os_printf("espconn_secure_get_size =%d\r\n", espconn_secure_get_size(0x01));
	rc = espconn_secure_connect(g_ptcp_conn);//ESPCONN_ARG
#else
	rc = espconn_connect(g_ptcp_conn);
#endif
	os_printf("local "IPSTR":%d\r\n", IP2STR(g_ptcp_conn->proto.tcp->local_ip),
			g_ptcp_conn->proto.tcp->local_port);
	os_printf("connect to "IPSTR":%d rc=%d\r\n",
			IP2STR(g_ptcp_conn->proto.tcp->remote_ip),
			g_ptcp_conn->proto.tcp->remote_port, rc);
	
}





static void ICACHE_FLASH_ATTR
scan_done(void *arg, STATUS status) {
	if (status == OK) {
		
		char *pbuf=buf;
		int i=0;
		struct bss_info *bss_link = (struct bss_info *)arg;
		int16_t wificount=0;
		uint32 time = sntp_get_current_timestamp();
		u8 macAddr[6] = { 0 };
		uint32 chip_id =system_get_chip_id();
		//os_printf("zaki test time=%d   u=%u   llu=%llu   lu=%lu  ld=%ld   %lld\r\n",time ,time ,time ,time ,time ,time );
		//os_printf("zaki test chip_id=%d   u=%u   llu=%llu   lu=%lu  ld=%ld   %lld\r\n",chip_id ,chip_id ,chip_id ,chip_id ,chip_id ,chip_id  );

			//os_printf(POST_WAYZ_INFO, time, chip_id, MAC2STR(macAddr), MAC2STR(macAddr));

		if (wifi_get_macaddr(STATION_IF, macAddr)) {
			os_printf("MAC:"MACSTR"\r\n", MAC2STR(macAddr));
		} else {
			os_printf("Get MAC fail!\r\n");
		}
		os_printf("----------scandone\n");
		//为了在中间添加逗号
		/*if(bss_link){
			wificount++;
			//os_printf("\r\nfirst bssid[6] = "MACSTR" | ssid:",  MAC2STR(bss_link->bssid));
			for(i=0; i<bss_link->ssid_len; i++){
				os_printf("%c",bss_link->ssid[i]);
			}

			os_sprintf(pbuf, POST_WAYZ_WIFI, bss_link->rssi, MAC2STR(bss_link->bssid), bss_link->ssid, bss_link->channel);
			bss_link= bss_link->next.stqe_next;
			//os_printf("addr=%d pbuf=%s\r\n addr= %d buf=%s\r\n",pbuf,pbuf,buf,buf);
			os_printf("addr=%d pbuf=%s\r\n",pbuf,pbuf);
		}*/
		
		while(bss_link)
		{	wificount++;
			while(*pbuf)pbuf++;
			//os_printf("\r\nbssid[6] = "MACSTR" | ssid:",  MAC2STR(bss_link->bssid));
			//for(i=0; i<bss_link->ssid_len; i++){
			//	os_printf("%c",bss_link->ssid[i]);
			//}
			//头部添加逗号并移动指针
			os_sprintf(pbuf, "%c", ',');
			pbuf++;
			os_sprintf(pbuf, SCAN_WAYZ_WIFI, MAC2STR(bss_link->bssid), bss_link->rssi, bss_link->channel,  bss_link->ssid);
			bss_link= bss_link->next.stqe_next;
			os_printf("%s\r\n",pbuf);
		}
		
		//os_printf("wificount=%d pbuf=%s\r\n buf=%s\r\n",wificount, pbuf,buf);

		if(wificount ==0){
			os_printf("no wifi found!!! cannot locate !\r\n");
		}

		//start sniffer
		task_post('a');
		
	}
}



void ICACHE_FLASH_ATTR
wifiscan(){
	
	os_bzero(buf2,BUF_LEN);
	os_bzero(buf,BUF_LEN);
	wifi_promiscuous_enable(0);
	user_wifi_init();
	wifi_station_disconnect();

	system_set_os_print(1);
	wifi_station_scan(NULL, scan_done);
	



	
	//os_sprintf(buf, POST_WAYZ_WIFI, );
	
	
}
