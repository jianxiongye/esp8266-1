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
#define FourKB 0x1000
extern u32 des_addr[FourKB/4];
extern int record;
extern u32 secB_addr;
extern int firstin;
static int ci=0;
extern int wifiscanLen;
#define secD_addr 0x301000


/*
 * function: tcp_client_send_cb
 */
static void ICACHE_FLASH_ATTR
tcp_client_send_cb(void *arg) {
	struct espconn *pesp_conn = arg;

	int i=0;
	int ret=0;
	os_printf("tcp_client_send_cb\n");
	//record=36; for only test
	if(ci<record/18){
		os_printf("record=%d\n",record);
		des_addr[0]=0;
	
		ret=spi_flash_read(secB_addr+14*ci*4*18, des_addr,   56*18);
			os_printf("ret=%d\n",ret);
		os_printf("ci=%d spi_flash_read:%s\r\n",ci,des_addr);

		espconn_send(pesp_conn, (u8*)des_addr, 56*18);
		os_printf("tcpsenddata:%s\r\n",des_addr);
		ci++;
	}else{
			if((record%18)!=0 && (ci==record/18)){
				//最后一条如果还有数据只需要传余数，没有的话需要断联
				
				os_printf("last not enough 18 data send!\n");
				ret=spi_flash_read(secB_addr+14*ci*4*18, des_addr,   56*18);
				os_printf("ret=%d\n",ret);
				os_printf("ci=%d spi_flash_read:%s\r\n",ci,des_addr);
				espconn_send(pesp_conn, (u8*)des_addr, 56*(record%18));
				os_printf("tcpsenddata:%s\r\n",des_addr);
				ci++;
				return;
			}

			
		os_printf("upload complete!we should disconnet connection!\n");
		//switch to sniffer mode,400KB flash clean
		for(i=0; i<SNIFF_SEND_LOOP_MIN*10; i++){
			spi_flash_erase_sector((secB_addr/FourKB) +i);
		}
		
		espconn_disconnect(g_ptcp_conn);


		return;
	}
	

	
}

/*
 * function:tcp_client_discon_cb
 */
static void ICACHE_FLASH_ATTR
tcp_client_discon_cb(void *arg) {
	os_printf("tcp_client_discon_cb\n");
	firstin=0;
	record=0;
	ci=0;
	//task_post('s');
	task_post('a');//switch to sniff
}

/*
 * function: tcp_client_recv
 */
static void ICACHE_FLASH_ATTR
tcp_client_recv(void *arg, char *pdata, unsigned short len) {
	os_printf("tcp_client_recv\n");
	//某种情况下，看来是在发送内容为空的时候会产生tcp recev，需要重新断连
	espconn_disconnect(g_ptcp_conn);

	os_printf("lenth:%d\r\n", len);
	os_printf("data:%s\r\n", pdata);

	// TODO:
#if 0
	os_memset(tcp_rev_buff, 0, TCP_BUF_LEN);
	os_memcpy(tcp_rev_buff, pdata, len);
	rev_data_len = len;
#endif

}
static os_timer_t retry_timer;


static void ICACHE_FLASH_ATTR
retry_connect(){
	http_client_connect();
	os_timer_disarm(&retry_timer);
}

/*
 * function: tcp_client_recon_cb
 */
static void ICACHE_FLASH_ATTR
tcp_client_recon_cb(void *arg, sint8 err) {
//	struct espconn *pesp_conn = arg;

	os_printf("tcp_client_recon_cb err=%d please check wayz SERVER!\r\n",err);
	os_printf("I will retry after 3s until connection is ok!");
	os_timer_disarm(&retry_timer);
	os_timer_setfn(&retry_timer, (os_timer_func_t *) retry_connect, NULL);
	os_timer_arm(&retry_timer, 3000, 1);

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
	
	

	os_printf("tcp_client_connect_cb\r\n");

#if SSL_CLIENT_ENABLE
	espconn_secure_send(pesp_conn, hello, os_strlen(hello));
#else
	//espconn_send(pesp_conn, hello, os_strlen(hello));
	//espconn_send(pesp_conn, (u8*)des_addr, os_strlen((u8*)des_addr));
	u8 macAddr[6] = { 0 };
	uint8_t mstoremac[32]={0};
	uint8_t mstore_head[4096]={0x30};
	int i=0;
	wifi_get_macaddr(STATION_IF, macAddr);
	os_sprintf(mstoremac,MACSTR,MAC2STR(macAddr));

	//+18是mac地址+回车
	uint32_t num=record*56+18+wifiscanLen+1;
	char v_mod=0;
	char count_head=1;
	
	do{ 
		v_mod = num%128;
		num = num/128;

		if(num>0)
			v_mod=v_mod|0x80;
		mstore_head[count_head]=v_mod;
		count_head++;
	}while(num>0);
	//mstoremac[17]='\n';回车放在扫描wifi里面了
	os_memcpy(mstore_head+count_head,mstoremac,17);
	//espconn_send(pesp_conn, "", 56*18);
	for(i=0;i<20;i++)
	{
		os_printf("0x%02x",mstore_head[i]);
	}
	u32 wifiscandata[0x1000];
	os_memset(wifiscandata,0,4096);
	os_printf("wifiscanLen=%d\n",wifiscanLen);
	spi_flash_read(secD_addr, wifiscandata, wifiscanLen);
	os_printf("read buf=%s\n",wifiscandata);
	char* data=(char *)wifiscandata;
	
	os_printf("buf=0x%02X\n",data[wifiscanLen-1]);
	os_printf("buf=0x%02X\n",data[wifiscanLen]);
	os_printf("buf=0x%02X\n",data[wifiscanLen+1]);
	data[wifiscanLen]='\n';
	data[1+wifiscanLen]='\0';
	
	os_memcpy(mstore_head+17+count_head, (uint8_t *)data, wifiscanLen+1);
	//发送头部20字节 0x30 + 长度 + 17Bmac + 回车
	espconn_send(pesp_conn, mstore_head, 18+count_head+wifiscanLen+1);


#endif
}

/*
 * function: http_client_init
 * parameter: u8* ip - server ip
 *            u16 port - server port
 */
void ICACHE_FLASH_ATTR
http_client_init(u8* ip, u16 port) {
	static esp_tcp esptcp;
	esp_tcp *pesptcp = &esptcp;

	u32 u32_ip = ipaddr_addr(ip);

	os_printf("http_client_init\n");

//	g_tcp_client_conn.type = ESPCONN_TCP;
//	g_tcp_client_conn.state = ESPCONN_NONE;
//	g_tcp_client_conn.proto.tcp = pesptcp;

	g_ptcp_conn->type = ESPCONN_TCP;
	g_ptcp_conn->state = ESPCONN_NONE;
	g_ptcp_conn->proto.tcp = pesptcp;

	os_memcpy(pesptcp->remote_ip, &u32_ip, 4); //set server ip
	pesptcp->remote_port = port;
	pesptcp->local_port = espconn_port();

//	os_memcpy(g_ptcp_conn->proto.tcp->remote_ip, &u32_ip, 4); //set server ip
//	g_ptcp_conn->proto.tcp->remote_port = port;			//set server port
//	g_ptcp_conn->proto.tcp->local_port = espconn_port();

	espconn_regist_connectcb(g_ptcp_conn, tcp_client_connect_cb);
	espconn_regist_reconcb(g_ptcp_conn, tcp_client_recon_cb);
	espconn_regist_disconcb(g_ptcp_conn, tcp_client_discon_cb);
}

void ICACHE_FLASH_ATTR
http_client_connect(void) {
	s8 rc = 0;
	espconn_disconnect(g_ptcp_conn);
#if SSL_CLIENT_ENABLE
	//espconn_secure_ca_enable(0x01, SSL_CA_ADDR);
	espconn_secure_cert_req_enable(0x01, SSL_CLIENT_KEY_ADDR);
	rc = espconn_secure_connect(g_ptcp_conn);
#else
	rc = espconn_connect(g_ptcp_conn);
#endif
	os_printf("local "IPSTR":%d\r\n", IP2STR(g_ptcp_conn->proto.tcp->local_ip),
			g_ptcp_conn->proto.tcp->local_port);
	os_printf("connect to "IPSTR":%d rc=%d\r\n",
			IP2STR(g_ptcp_conn->proto.tcp->remote_ip),
			g_ptcp_conn->proto.tcp->remote_port, rc);
}

