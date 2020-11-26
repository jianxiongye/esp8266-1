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

#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_err.h"
#include "rom/ets_sys.h"

const char *payload = "Message from ESP32 ";
#define PORT 3334
#define HOST_IP_ADDR "122.51.2.195"



xTaskHandle sniffhandle;
xTaskHandle uarthandle;
xTaskHandle lorahandle;
static void initialise_wifi(void);
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
int SEND_BIT = BIT10;
uint8_t recordLoraSendCount = 0;

//static char printbuf[100];
#define HOP_JUMP_ENABLE			1
#define CHANNEL_HOP_INTERVAL    250
int flag=0;


#if HOP_JUMP_ENABLE
static os_timer_t channelHop_timer;
uint16_t m_min=0;
uint16_t m_count=0;
//发送时间，默认值为10min
uint16_t m_sniff_send_loop_min = 1;
int f_sendbuf=0;
int ci=0;
int record=0;
int f_sentdone=0;
int f_joined=0;

#define GPIO_OUTPUT_IO_0	14
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_OUTPUT_IO_0) // LORA模块上SETA
#define GPIO_OUTPUT_IO_1	12
#define GPIO_OUTPUT_PIN_SETB  (1ULL<<GPIO_OUTPUT_IO_1) // LORA模块上SETB


void diable_channel_hop(){
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
	os_printf("stateMachine[d]:switch to send data mode\n");
	f_sendbuf = 1;//send data
	os_timer_disarm(&channelHop_timer);
	xEventGroupSetBits(wifi_event_group, SEND_BIT);
	m_count=0;
	m_min=0;

	ci=0;
}


static void channelHop(void *arg)
{
    // 1 - 13 channel hopping
    // uint8 new_channel = wifi_get_channel() % 12 + 1;
    static uint8_t new_channel = 1;
	new_channel ++;
	if(new_channel >13)
	{
		new_channel = 1;
	}

    // os_printf("** hop to %d **\t    Client MAC\t\t    AP MAC\r\n", new_channel);
    esp_wifi_set_channel(new_channel, 0);
	m_count ++;
	if(m_count % 240 == 0){
		m_min ++;
		os_printf("time is up %dmin\n", m_min);
	}
	
	//if(m_min>=SNIFF_SEND_LOOP_MIN){m_sniff_send_loop_min
	if(m_min >= m_sniff_send_loop_min){
		os_printf("time is up %dmin send data now\n", m_sniff_send_loop_min);
		m_min = 0;
		m_count = 0;
		diable_channel_hop();
	}
	
}
#endif

#define FourKB 0x1000

uint32_t des_addr[FourKB/4];

uint32_t secA_addr=0x201000;//1024+1024
uint32_t secB_addr=0x202000;
uint32_t i; 
char databuf[FourKB][7];



#define SNIFF_SEND_LOOP_MIN 10

//static int firstin=0;

#define num_print 51
#define size_overflow (2000) //too few .must increase
char mstore[1024*10]={0};


void openLightSleepMode(uint32_t sleep_ms)
{
	//esp_err_t error;
	printf("start wifi light sleep. \r\n");
	esp_wifi_stop();
	esp_sleep_enable_timer_wakeup(sleep_ms * 1000);
	esp_light_sleep_start();
	printf("end wifi light sleep. \r\n");
}

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


void writeSniffDataToFlash(char* data){
	int i = 0;
	char aucMacBroadCast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // 广播的mac地址
	char aucMacMultiCast[6] = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x00}; // 多播的mac地址
	if(record < size_overflow){// 
		printf("data="MACSTR"\n", PRINT(data, 0));
		printf("sig=%d\n", (signed char)data[6]);

		// 过滤数据 广播、多播mac地址
		if (0 == strncmp(aucMacBroadCast, data, 6) ||  
			0 == strncmp(aucMacMultiCast, data, 3))
		{
			return ;
		}
		// 过滤相同数据
		for (i = 0; i < ci; i ++)
		{
			if (0 == strncmp(databuf[i], data, 6))
			{
//				printf("-------- data are equal -------------\r\n");
				return ;
			}
		}

		memcpy(databuf[ci], data, 7);
		ci ++;
		record ++;
	}else{ // 超过2000条数据也将停止sniffer
		diable_channel_hop();
	}
}



void lora_client_task(void *pvParameters)
{
	int i = 0;
	int k = 0;
	int j = 0;
	int c_list, c_buf = 0;//存下数据c_list表示列表循环，c_buf表示最终留下的非重复列表
	int timeout = 0;
	int cycles = 0;
    while (1) {
		os_printf("slora_client_task\n");
		xEventGroupWaitBits(wifi_event_group, SEND_BIT, false, true, portMAX_DELAY);
		if(f_sendbuf){ // sniffer 停止后，数据上传到LORA模块中
			ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
			os_printf("send data\n");

			for(k = 0; k < record; k ++){
				printf("=old=databuf="MACSTR"\n",PRINT(databuf[k], 0));
			}

			for(c_list = 0; c_list < record; c_list ++){
				printf("c_list     %d %d\n", c_list, c_buf);
				for(k = 0; k < c_buf; k ++){
					if(0 == memcmp(&databuf[c_list], &databuf[k], 6)){//不需要比较信号强度
						printf("same break %d %d\n", c_list, c_buf);	
						break;
					}
				}
				if(k == c_buf){//全比较完成，都不同就需要拷贝
					if(c_buf != c_list)
						memcpy(&databuf[c_buf], &databuf[c_list], 7);//需要拷贝信号强度
					c_buf ++;
					printf("different  %d %d\n", c_list, c_buf);
				}
			}

			for(k = 0 ; k < c_buf; k++){
				printf("=filter=databuf="MACSTR"|%d\n", PRINT(databuf[k], 0), (signed char)databuf[k][6]);
			}

			printf("---c_buf : %d------send data: %d-------\r\n", c_buf, c_buf / 7 + (c_buf % 7 == 0 ? 0 : 1));

			vTaskDelay(300/portTICK_PERIOD_MS );
			os_printf("switch to 9600\n");
			
			uart_config_t uart_config = {
				.baud_rate = 9600,
				.data_bits = UART_DATA_8_BITS,
				.parity = UART_PARITY_DISABLE,
				.stop_bits = UART_STOP_BITS_1,
				.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
			};
			uart_param_config(EX_UART_NUM, &uart_config);
			os_printf("switch from 115200\n");
			vTaskDelay( 500/portTICK_PERIOD_MS );

			gpio_set_level(GPIO_OUTPUT_IO_0, 0);
			os_printf("AT+BAUD=8\n");
			vTaskDelay(500/portTICK_PERIOD_MS );
			uart_config.baud_rate = 115200;
			uart_param_config(EX_UART_NUM, &uart_config);
			os_printf("AT+EXITTRANS\n");
			vTaskDelay(500/portTICK_PERIOD_MS );
			

			//加入网络
			os_printf("AT+JOIN=1\n");
			/*while(f_joined==0){
				vTaskDelay(2000/portTICK_PERIOD_MS );
				os_printf("AT+JOIN=1\n");
			}*/
			vTaskDelay( 10000/portTICK_PERIOD_MS );

			//发送数据
			for(j = 0; j <= c_buf / 7; j ++){
				f_sentdone = 0;
				
				if (j != (c_buf / 7))
				{
					cycles = 49;
				}
				else if (j == (c_buf / 7) && 0 != (c_buf % 7))
				{
					cycles = (c_buf % 7 * 7);
				}
				else if (j == (c_buf / 7) && 0 == (c_buf % 7))
				{
					cycles = 0;
					break;
				}
				os_printf("AT+SEND=0,4,"); // AT+SEND=P1,P2,P3  P1:是否应答，1应答，0不应答；P2:重发次数，若有应答才生效，无应答不生效；P3：数据
				for(i = 0; i < cycles; i ++)
				{
					os_printf("%02X",databuf[j * 7][i]);
				}

				os_printf("\n");
#if 1
				timeout = 0;
				//等待发送完毕
				while(f_sentdone == 0 && timeout < 60){// timeout < 60也就是30s超时
					vTaskDelay( 500/portTICK_PERIOD_MS );
					timeout ++;
				}
#endif
			}

			gpio_set_level(GPIO_OUTPUT_IO_0, 1);
			
			os_printf("upload complete!delay 30s to re-sniff...\n");

			f_sendbuf = 0;
			record = 0;
			//vTaskDelay(30000/portTICK_PERIOD_MS );

			#if  SLEEPMODLE			
			recordLoraSendCount ++;
			
			if (SENDLORACOUNT == recordLoraSendCount)
			{
				recordLoraSendCount = 0;
				openLightSleepMode(SLEEPTIME); // ms 
				vTaskDelay(2000 / portTICK_RATE_MS);
				esp_wifi_start();
				esp_wifi_set_ps(DEFAULT_PS_MODE);
				esp_restart(); // 重置esp8266可解决light sleep 后wifi扫描无法运行

				vTaskDelay(200 / portTICK_RATE_MS);
			}
			#endif 
			vTaskResume(sniffhandle);
		}
		xEventGroupClearBits(wifi_event_group, SEND_BIT);
		//vTaskDelay(2000/portTICK_PERIOD_MS );

	}
	vTaskDelete(NULL);
}


static void sniffer_cb(void* buf, wifi_promiscuous_pkt_type_t type)
{
    wifi_pkt_rx_ctrl_t* rx_ctrl = (wifi_pkt_rx_ctrl_t*)buf;
    uint8_t* frame = (uint8_t*)(rx_ctrl + 1);
    uint8_t i = 0; 

	struct framecontrol_t *framecontrol;

	framecontrol = (struct framecontrol_t*)(buf + 12);
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
	if(0!=frame[4]%4)//如果不能被4整除除，那就不是我们需要的mac
		return;
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
	framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi, framecontrol->datadir);
	memcpy(mstore, &frame[4], 6);
	mstore[6] = rx_ctrl->rssi;
	writeSniffDataToFlash((char*)mstore);
	return;
/*		
	print23:
		os_printf("-------------------------------|%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%d|%d|0|%d|0\n",\
		PRINT(frame,10),PRINT(frame,16),\
		framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi,framecontrol->datadir);
		return;
*/

print32:
	if(0!=frame[16]%4)//如果不能被4整除除，那就不是我们需要的mac
		return;

	// 缓存上次的MAC，避免重复打印
	if(0==memcmp(temp_mac1, &frame[16], 6)){
		return;
	}
	for (i = 0; i < 6; i ++){
		temp_mac1[i] = frame[i+16];
	}
	os_printf("%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%02d|%d|0|%d|0\n",\
	PRINT(staMacAddr,0),PRINT(frame,16),PRINT(frame,10),\
	framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi, framecontrol->datadir);
	memcpy(mstore, &frame[16], 6);
	mstore[6] = rx_ctrl->rssi;
	writeSniffDataToFlash((char*)mstore);
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
	os_printf("%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02X:%02X:%02X:%02X:%02X:%02X|%02d|%02d|%02d|%d|0|%d|0\n",\
	PRINT(staMacAddr,0),PRINT(frame,10),PRINT(frame,4),\
	framecontrol->frametype, framecontrol->subtype, rx_ctrl->channel, rx_ctrl->rssi, framecontrol->datadir);
	memcpy(mstore, &frame[10], 6);
	mstore[6] = rx_ctrl->rssi;
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


	#if HOP_JUMP_ENABLE // 1min 切换一个通道
		os_timer_disarm(&channelHop_timer);
		os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop, NULL);
		os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL, 1);
		esp_wifi_get_mac(ESP_IF_WIFI_STA, staMacAddr);
	#endif
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

	esp_wifi_set_ps(DEFAULT_PS_MODE);
}

void heap_task(void *pvParameters)
{
	while(1){
		vTaskDelay(2000/portTICK_PERIOD_MS );
		
		//ESP_LOGI(TAG, "Free heap size: %d\n", esp_get_free_heap_size());

	}
}

static void prinfChipVersion()
{
	esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ", chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	printf("software version: 20%02d-%02d-%02d \r\n", VER_H, VER_M, VER_L); 

	printf("\n\n-------------------------------- Get Systrm Info------------------------------------------\n");
    //获取IDF版本
    printf("     SDK version:%s\n", esp_get_idf_version());
    //获取芯片可用内存
    printf("     esp_get_free_heap_size : %d  \n", esp_get_free_heap_size());
    //获取从未使用过的最小内存
    printf("     esp_get_minimum_free_heap_size : %d  \n", esp_get_minimum_free_heap_size());
    //获取芯片的内存分布，返回值具体见结构体 flash_size_map
    printf("     system_get_flash_size_map(): %d \n", system_get_flash_size_map());
    uint8_t mac[6];
    //获取mac地址（station模式）
    esp_wifi_get_mac(WIFI_MODE_STA, mac);
    printf(" Station esp_wifi_get_mac(): %02x:%02x:%02x:%02x:%02x:%02x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    //获取mac地址（ap模式）
    esp_wifi_get_mac(WIFI_MODE_AP, mac);
    printf(" AP esp_wifi_get_mac(): %02x:%02x:%02x:%02x:%02x:%02x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("--------------------------------------------------------------------------\n\n");
}

void app_main()
{
	uint8_t gpio_sta = 0; 
	uart_disable_swap();
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

	prinfChipVersion();

    // Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, &uarthandle);
    initialise_wifi();
    xTaskCreate(sniffer_task, "sniffer_task", 2048, NULL, 10, &sniffhandle);
	xTaskCreate(lora_client_task, "lora_client", 1024, NULL, 5, &lorahandle);
//	xTaskCreate(heap_task, "heap_task", 1024, NULL, 6, NULL);
	
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
	gpio_set_level(GPIO_OUTPUT_IO_0, 1);

	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SETB;
	gpio_config(&io_conf);
	gpio_sta = gpio_get_level(GPIO_OUTPUT_IO_1);
	printf("SETB gpio status : %d \r\n", gpio_sta);


}

void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t *dtmp = (uint8_t *) malloc(RD_BUF_SIZE);

	//uint16_t len = 0;
	//uint16_t mlen = 0;

	//char* mlen1, mlen2 = 0;
	//int i = 0;

    for (;;) {
        // Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);

            switch (event.type) {
                // Event of UART receving data
                // We'd better handler data event fast, there would be much more data events than
                // other types of events. If we take too much time on data event, the queue might be full.
                case UART_DATA:
					
                    //ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);

					if (0 == memcmp(dtmp, "OK+RECV", 7)) {
						printf("OK+RECV!\n");
						f_sentdone=1;
					}else if (0 == memcmp(dtmp, "OK+JOINED", 9)) {
						printf("OK+JOINED!\n");
						f_joined = 1;
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
						xEventGroupWaitBits(wifi_event_group, START_BIT, false, true, portMAX_DELAY);
					}else if (0 == memcmp(dtmp, "scanwif", 7)) {

					}else if (0 == memcmp(dtmp, "start&", 6)) {
						uart_write_bytes(EX_UART_NUM, (const char *) dtmp, event.size);
						//if(eTaskGetState(sniffer_task) != eSuspended)
    					
						//if(eTaskGetState(sniffer_task) != eRunning)
						//vTaskResume(sniffhandle);
						esp_restart();
					}

					//================================
                    //ESP_LOGI(TAG, "[DATA EVT]:");
                    //uart_write_bytes(EX_UART_NUM, (const char *) dtmp, event.size);
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


