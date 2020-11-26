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
#define ZDEBUG 0
#include "scanwifi.h"

#include "uart_parse.h"


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>


static const char *payload = "Message from ESP32 ";
#define PORT 3334
#define HOST_IP_ADDR "122.51.2.195"



xTaskHandle sniffhandle;


struct framecontrol_t{
	unsigned:2;//frame protocol
	unsigned frametype:2;//frame type
	unsigned subtype:4;//sub type
	unsigned datadir:2;
	unsigned:6;//other
};



uint8_t temp_mac1[6] = {0xc4, 0x6a, 0xb7, 0x9f, 0xcc, 0x34};
uint8_t staMacAddr[6];

#define TAG "sniffer"

#define MAC_HEADER_LEN 24
#define SNIFFER_DATA_LEN 112
#define MAC_HDR_LEN_MAX 40

static EventGroupHandle_t wifi_event_group;

static const int START_BIT = BIT0;
const int GOTIP_BIT = BIT1;


static char printbuf[100];
#define HOP_JUMP_ENABLE			1
#define CHANNEL_HOP_INTERVAL 250
int flag=0;


#if HOP_JUMP_ENABLE
static os_timer_t channelHop_timer;

static void channelHop(void *arg)
{
    // 1 - 13 channel hopping
   // uint8 new_channel = wifi_get_channel() % 12 + 1;
   static uint8_t new_channel = 1;
	new_channel++;
	if(new_channel >13)
		new_channel = 1;
   // os_printf("** hop to %d **\t    Client MAC\t\t    AP MAC\r\n", new_channel);
    esp_wifi_set_channel(new_channel,0);
	 //ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
	//writeSniffDataToFlash("zaki");
	
}
#endif

#define FourKB 0x1000

uint32_t des_addr[FourKB/4];

uint32_t secA_addr=0x201000;//1024+1024
uint32_t secB_addr=0x202000;
uint32_t i; 


#define SNIFF_SEND_LOOP_MIN 10

static int firstin=0;
int record=0;
#define num_print 51
#define size_overflow (51) //too few .must increase
 char mstore[1024*10]={0};

 static esp_err_t event_handler(void* ctx, system_event_t* event)
 {
	 switch (event->event_id) {
		 case SYSTEM_EVENT_STA_START:
			 xEventGroupSetBits(wifi_event_group, START_BIT);
	 printf("SYSTEM_EVENT_STA_START!---------\n");
			 break;
		 case SYSTEM_EVENT_STA_GOT_IP:
		 	xEventGroupSetBits(wifi_event_group, GOTIP_BIT);
			flag=1;
			 printf("Got ip!---------\n");
			 break;
		 default:
			 break;
	 }
 
	 return ESP_OK;
 }
 
 static void initialise_wifi1(void)
 {
 	 os_timer_disarm(&channelHop_timer);
	 tcpip_adapter_init();
	 wifi_event_group = xEventGroupCreate();
	 //ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
	 
	 wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	 ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	 ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	 wifi_config_t wifi_config = {
		.sta = {
			 .ssid = "WAYZ Guest",
		     .password = "zz20192019"
		 },
		/* .sta = {
			 .ssid = "Wayz Public Device",
		     .password = "wayz2020"
		 },*/ 
	 };
	 ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
	 ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	 ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	 ESP_ERROR_CHECK( esp_wifi_start() );
	 vTaskDelay(3000 / portTICK_PERIOD_MS);
	  
	 esp_wifi_connect();
 }


 void
writeSniffDataToFlash(char* data){
	if(1){//record<size_overflow
		printf("spi_flash_write ans=%d\n",spi_flash_write(secB_addr+((uint32_t)((strlen(data)/4)+1)*4*record), (uint32_t*)data, (uint32_t)((strlen((char*)data)/4)+1)*4));
		
		printf("spi_flash_read  ans=%d\n",spi_flash_read(secB_addr+((uint32_t)((strlen(data)/4)+1)*4*record), des_addr,         (uint32_t)((strlen((char*)data)/4)+1)*4));

		os_printf("recordis \n");
		printf("%d raw  is %s\n", record, (char*)data);
		printf("%d data is %s\n", record, (char*)des_addr);
		record++;

		//vTaskSuspend(sniffhandle);
		ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
			esp_wifi_stop();

			initialise_wifi1();
			xEventGroupWaitBits(wifi_event_group, START_BIT,false, true, portMAX_DELAY);
	}else{

		
		if(firstin==0){
			firstin=1;
			spi_flash_write(secB_addr+((uint32_t)((strlen(data)/4)+1)*4*record), (uint32_t*)data, (uint32_t)((strlen((char*)data)/4)+1)*4);
			printf("data overflow------------------------------------------\n");
			spi_flash_read(secB_addr, des_addr,   (uint32_t)FourKB/4);
			/*for(i = 0; i<FourKB/4; i++){
				printf(" %08X ", des_addr[i]);
			}*/
			for(i = 0; i<num_print; i++){
			os_printf("data is %s", (char*)(des_addr+i*((strlen((char*)data)/4)+1)));

			}

		}

	}
}

void ICACHE_FLASH_ATTR
send_all_data(){
	//reload data and send to client
	static int ci=0;
	if(ci<=record/18){
		spi_flash_read(secB_addr+14*ci*4*18, des_addr,   56*18);
		printf("ci=%d spi_flash_read:%s\r\n",ci,(char *)des_addr);
		//http_client_connect();
		ci++;
	}else{
		//switch to sniffer mode
		for(i=0; i<SNIFF_SEND_LOOP_MIN*10; i++){
			spi_flash_erase_sector((secB_addr/FourKB) +i);
		}
		
		
		firstin=0;
		record=0;
		ci=0;
	}

}


static void tcp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
			
		//xEventGroupWaitBits(wifi_event_group, GOTIP_BIT,false, true, portMAX_DELAY);
		
		

    while (1) {
		while(!flag){
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			printf("tcp_client_task waiting now\n");

		}


        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
		printf("create socket now\n");


        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = connect(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));

		if (err != 0) {
			ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
		}
		ESP_LOGI(TAG, "Successfully connected");

char ccci=0;
		while (1) {
			int err = send(sock, payload, strlen(payload), 0);
			err = send(sock, &ccci, ccci, 0);
			err = send(sock, "\n", 1, 0);
			ccci++;
			if (err < 0) {
				ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
				break;
			}

			int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
			// Error occured during receiving
			if (len < 0) {
				ESP_LOGE(TAG, "recv failed: errno %d", errno);
				break;
			}
			// Data received
			else {
				rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
				ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
				ESP_LOGI(TAG, "%s", rx_buffer);
			}
			esp_wifi_disconnect();

			vTaskDelay(2000 / portTICK_PERIOD_MS);
			
			vTaskResume(sniffhandle);
			flag=0;
			break;
		}

		if (sock != -1) {
			ESP_LOGE(TAG, "Shutting down socket and restarting...");
			shutdown(sock, 0);
			close(sock);
		}
	}
	vTaskDelete(NULL);
}


static void sniffer_cb(void* buf, wifi_promiscuous_pkt_type_t type)
{
    wifi_pkt_rx_ctrl_t* rx_ctrl = (wifi_pkt_rx_ctrl_t*)buf;
    uint8_t* frame = (uint8_t*)(rx_ctrl + 1);
    uint8_t i; 

	struct framecontrol_t *framecontrol;

	framecontrol = (struct framecontrol_t*)(&buf[12]);
	#if ZDEBUG
		if(type==WIFI_PKT_MGMT&&(framecontrol->subtype==8||framecontrol->subtype==12))
			return;

		os_printf("->%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%02d|%d|0|%d|0\n",\
		PRINT(frame,4),PRINT(frame,10),PRINT(frame,16),\
		framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi,framecontrol->datadir);
	#endif

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
					goto printDS;
			}
			return;
			break;

        case WIFI_PKT_CTRL:
            return;
            break;

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
						goto printSD;
					else
						goto print32;
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
	//if(0!=sniffer->buf[10]%4)//如果不能被4整除除，那就不是我们需要的mac
	//	return;
	// 如果MAC地址和上一次一样就返回
	if(0==memcmp(temp_mac1, &frame[4], 6)){
		return;
	}
	for (i=0; i<6; i++){
		temp_mac1[i] = frame[i+4];
	}
	//check 真实mac地址
	

	os_printf("%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%02d|%d|0|%d|0\n",\
	PRINT(staMacAddr,0),PRINT(frame,4),PRINT(frame,10),\
	framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi,framecontrol->datadir);
	
	return;
/*		
	print23:
		os_printf("-------------------------------|%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%d|%d|0|%d|0\n",\
		PRINT(frame,10),PRINT(frame,16),\
		framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi,framecontrol->datadir);
		return;
*/

print32:
	// 缓存上次的MAC，避免重复打印
	if(0==memcmp(temp_mac1, &frame[16], 6)){
		return;
	}
	for (i=0; i<6; i++){
		temp_mac1[i] = frame[i+16];
	}
	os_printf("%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%02d|%d|0|%d|0\n",\
	PRINT(staMacAddr,0),PRINT(frame,16),PRINT(frame,10),\
	framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi,framecontrol->datadir);
	return;
				
printDS:
	if(0==memcmp(temp_mac1, &frame[10], 6)){
		return;
	}
	for (i=0; i<6; i++){
		temp_mac1[i] = frame[i+10];
	}
	os_printf("%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%02d|%d|0|%d|0\n",\
	PRINT(staMacAddr,0),PRINT(frame,10),PRINT(frame,4),\
	framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi,framecontrol->datadir);
	sprintf(mstore,"-%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%02d|%d|0|%d|0\n",\
		PRINT(frame,4),PRINT(frame,10),\
		framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi,framecontrol->datadir);
	os_printf(mstore);
	writeSniffDataToFlash((char*)mstore);
	return;
}

static void sniffer_task(void* pvParameters)
{
    wifi_promiscuous_filter_t sniffer_filter = {0};
while(1){

	printf("rtos v3.3\n");
	wifiscan();
	printf("----------sniffer\n");

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


#if HOP_JUMP_ENABLE
    os_timer_disarm(&channelHop_timer);
    os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop, NULL);
    os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL, 1);
	esp_wifi_get_mac(ESP_IF_WIFI_STA, staMacAddr);
#endif
	spi_flash_erase_sector((secB_addr/FourKB) );
vTaskSuspend(NULL);
printf("sniffer_task resume!\n");
}

	while(1){
		vTaskDelay(1000/portTICK_PERIOD_MS );
		printf("working...\n");
	}
	vTaskDelete(NULL);

	//vTaskSuspend(sniffhandle);


   
}




static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	

    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}
void heap_task(void *pvParameters)
{
while(1){
	vTaskDelay(2000/portTICK_PERIOD_MS );
	
	ESP_LOGI(TAG, "Free heap size: %d\n", esp_get_free_heap_size());

}

}

void app_main()
{uart_disable_swap();
    ESP_ERROR_CHECK(nvs_flash_init());
    // Configure parameters of an UART driver,
	// communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(EX_UART_NUM, &uart_config);

    // Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 100, &uart0_queue, 0);

    // Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
    initialise_wifi();
    xTaskCreate(sniffer_task, "sniffer_task", 2048, NULL, 10, &sniffhandle);
	xTaskCreate(heap_task, "heap_task", 4096, NULL, 5, NULL);
	xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}

void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t *dtmp = (uint8_t *) malloc(RD_BUF_SIZE);

	uint16_t len = 0;
	uint16_t mlen=0;

	char* mlen1,mlen2=0;
	int i=0;

    for (;;) {
        // Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);

            switch (event.type) {
                // Event of UART receving data
                // We'd better handler data event fast, there would be much more data events than
                // other types of events. If we take too much time on data event, the queue might be full.
                case UART_DATA:
					
                    ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);

					if (0 == memcmp(dtmp, "startsn", 7)) {
						vTaskResume(sniffhandle);
						ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
						printf("vTaskResume!\n");

					}else if (0 == memcmp(dtmp, "stopsni", 7)) {
						vTaskSuspend(sniffhandle);
						ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));

						wifi_config_t wifi_config = { 
					        .sta = { 
					            .ssid = "Wayz Public Device",
					            .password = "wayz2020"
					        },
					    };
					    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
					    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
					    ESP_ERROR_CHECK(esp_wifi_start() );
						ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
						xEventGroupWaitBits(wifi_event_group, START_BIT,false, true, portMAX_DELAY);
					}else if (0 == memcmp(dtmp, "scanwif", 7)) {

					}else if (0 == memcmp(dtmp, "start&", 6)) {
						uart_write_bytes(EX_UART_NUM, (const char *) dtmp, event.size);
						//if(eTaskGetState(sniffer_task) != eSuspended)
    						

						//if(eTaskGetState(sniffer_task) != eRunning)
						//vTaskResume(sniffhandle);
						    esp_restart();
					}else if (0 == memcmp(dtmp, "end&", 4)) {
						uart_write_bytes(EX_UART_NUM, (const char *) dtmp, event.size);
						/*
					}else if (0 == memcmp(dtmp, "set", 3)) {
						uart_write_bytes(EX_UART_NUM, (const char *) dtmp, event.size);
						switch(dtmp[3]){
							case 'N':
								//mWifiName=uart_buf+5;
								strcpy(mWifiName,dtmp+5,strlen(dtmp+5));
								os_printf("we receive N=%s\n",mWifiName);
								break;
							case 'P':
								//mWifiPass=uart_buf+5;
								memcpy(mWifiPass,dtmp+5,strlen(dtmp+5));
								os_printf("we receive N=%s P=%s\n",mWifiName,mWifiPass);
								
								////////user_wifi_initNP(mWifiName,mWifiPass);
								break;
							case 'S':
								//mServer=uart_buf+5;
								os_strcpy(mServer,dtmp+5);
								os_printf("we receive S=%s\n",mServer);
								mlen1=strstr(mServer,":");
								mlen=strlen(mlen1);
								
								
								
								///mServer[mlen2]='\0';
								mPort=0;
								for(i=0; i<mlen-1; i++)
								{
									os_printf("%c  %d\n",mlen1[1+i],mlen1[1+i]-'0');
									mPort= (mPort)*(10)+mlen1[1+i]-'0';
									os_printf("mport=%d\n",mPort);
									
								}

								mlen1[0]='\0';
								os_printf("we receive ip=%s port=%d \n",mServer, mPort);
								
								break;
							default:
								os_printf("unregister CMD\n");
								break;
							}
							*/
					}

				//================================
                    ESP_LOGI(TAG, "[DATA EVT]:");
                    uart_write_bytes(EX_UART_NUM, (const char *) dtmp, event.size);
                    break;

                // Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;

                // Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;

                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;

                // Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(TAG, "uart frame error");
                    break;

                // Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }

    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
} 


