/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
功能说明:
空工程



*/

#include "driver/uart.h"
#include "ets_sys.h"
#include "os_type.h"

#include "osapi.h"
#include "user_interface.h"
#include "http_client.h"
#include "user_config.h"
#include "sniffer.h"

#include "driver/uart.h"                                                                                                                                                                                                                     
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "gpio.h"
#include "mem.h"
#include "user_interface.h"
#include "sniffer.h"
#include "info.h"

#include "hw_timer.h"



//**********************************************************************************/
// SDK v3.0
//zaki task
#define SIG_RX 0
#define TEST_QUEUE_LEN 4
os_event_t *testQueue;
void task_post(char par);



#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
#error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 2)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0xfb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0xfc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0xfd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0x7c000
#elif (SPI_FLASH_SIZE_MAP == 3)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0x7c000
#elif (SPI_FLASH_SIZE_MAP == 4)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0x7c000
#elif (SPI_FLASH_SIZE_MAP == 5)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0xfc000
#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR           0xfc000
#else
#error "The flash map is not supported"
#endif

#define SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM                SYSTEM_PARTITION_CUSTOMER_BEGIN

uint32 priv_param_start_sec;

static const partition_item_t at_partition_table[] = {
    { SYSTEM_PARTITION_BOOTLOADER, 						0x0, 												0x1000},
    { SYSTEM_PARTITION_OTA_1,   						0x1000, 											SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_OTA_2,   						SYSTEM_PARTITION_OTA_2_ADDR, 						SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_RF_CAL,  						SYSTEM_PARTITION_RF_CAL_ADDR, 						0x1000},
    { SYSTEM_PARTITION_PHY_DATA, 						SYSTEM_PARTITION_PHY_DATA_ADDR, 					0x1000},
    { SYSTEM_PARTITION_SYSTEM_PARAMETER, 				SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 			0x3000},
    { SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM,             SYSTEM_PARTITION_CUSTOMER_PRIV_PARAM_ADDR,          0x1000},
};

void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, sizeof(at_partition_table)/sizeof(at_partition_table[0]),SPI_FLASH_SIZE_MAP)) {
		os_printf("system_partition_table_regist fail\r\n");
		while(1);
	}
}

//**********************************************************************************/

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void) {
	enum flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;

	switch (size_map) {
	case FLASH_SIZE_4M_MAP_256_256:
		rf_cal_sec = 128 - 5;
		break;

	case FLASH_SIZE_8M_MAP_512_512:
		rf_cal_sec = 256 - 5;
		break;

	case FLASH_SIZE_16M_MAP_512_512:
	case FLASH_SIZE_16M_MAP_1024_1024:
		rf_cal_sec = 512 - 5;
		break;

	case FLASH_SIZE_32M_MAP_512_512:
	case FLASH_SIZE_32M_MAP_1024_1024:
		rf_cal_sec = 1024 - 5;
		break;

	default:
		rf_cal_sec = 0;
		break;
	}

	return rf_cal_sec;
}

void ICACHE_FLASH_ATTR
user_rf_pre_init(void) {
}

void ICACHE_FLASH_ATTR
print_chip_info(void) {
	u8 macAddr[6] = { 0 };
	os_printf("\n*********************************\r\n");
	os_printf("SDK version:%s\r\n", system_get_sdk_version());
	os_printf("chip ID:%d\r\n", system_get_chip_id());
	os_printf("CPU freq:%d\r\n", system_get_cpu_freq());
	os_printf("free heap size:%d\r\n", system_get_free_heap_size());
	if (wifi_get_macaddr(STATION_IF, macAddr)) {
		os_printf("MAC:"MACSTR"\r\n", MAC2STR(macAddr));
	} else {
		os_printf("Get MAC fail!\r\n");
	}
	os_printf("meminfo:\r\n");
	system_print_meminfo();
	os_printf("*********************************\r\n");
}

//stateMachine
void ICACHE_FLASH_ATTR
test_task (os_event_t *e) {
	switch (e->sig) {
		case SIG_RX:
			if((char)e->par == 'a'){
				os_printf("stateMachine[a]:start to sniff\n");
				//os_printf("sig_rx ----------%c\n", (char)e->par);
				keepinfo();
				sniffer_init();
				sniffer_init_in_system_init_done();
	         	//os_printf("\r\n initial completed!start catching...\r\n\"mac\",\"rssi\",\"channel\",\r\n");
         	}else if((char)e->par == 'b'){
         		//start
         		os_printf("stateMachine[b]:restart\n");
				wifiscan();
			}else if((char)e->par == 'c'){
				//end
				wifiscan();
			}else if((char)e->par == 'd'){
				//uploaddata
				os_printf("stateMachine[d]:switch to send data mode\n");
				diable_channel_hop();
				wifi_promiscuous_enable(0);
				user_wifi_init();
				
			}else if((char)e->par == 's'){
				os_printf("stateMachine[s]:send data now\n");
				send_all_data();
				
			}else if((char)e->par == 't'){
				//tcp disconnected
				os_printf("stateMachine[t]:switch to sniff mode\n");
				send_all_data();
			}

         	
			break;
		default:
			break;
	}
}
void ICACHE_FLASH_ATTR
task_init(void) {
	testQueue=(os_event_t *)os_malloc(sizeof(os_event_t)*TEST_QUEUE_LEN);
	system_os_task(test_task,USER_TASK_PRIO_1,testQueue,TEST_QUEUE_LEN);
}
void task_post(char par) {
	system_os_post(USER_TASK_PRIO_1, SIG_RX, par);
}
void ICACHE_FLASH_ATTR hwtimer_init(u64 time);


void ICACHE_FLASH_ATTR hw_test_timer_cb(){
	static u32 count=0;
	count++;
	if(count>=20){
		os_printf("time is up 20s\n");
		hwtimer_init(500000);
	}
	os_printf("time is %d\n",count);
}

void ICACHE_FLASH_ATTR hwtimer_init(u64 time)
{
	hw_timer_init(FRC1_SOURCE,1);
	hw_timer_set_func(hw_test_timer_cb);
	hw_timer_arm(time);
}



void ICACHE_FLASH_ATTR
init_done_cb_init(void) {
	print_chip_info();
	wifi_set_opmode(STATION_MODE);
	hwtimer_init(1000000);

#if 1
	//user_wifi_init();
#else
	// parameter: STATION_MODE/SOFTAP_MODE/STATIONAP_MODE
	wifi_set_opmode(STATION_MODE);
	user_set_station_config();
#endif
	wifi_station_disconnect();
	//wifi_station_connect();
	//user_sntp_init();
	task_init();
	os_printf("init over");
	//task_post('d');


	

	//http_client_init(HOST, PORT);
	//http_get_version_init(HOST_WAYZ, PORT);
	
	//http_location_init(HOST_WAYZ_DNS, PORT);
}



/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void) {
	//uart_init(BIT_RATE_74880, BIT_RATE_74880);
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_printf("SDK version:%s\n", system_get_sdk_version());
	system_init_done_cb(init_done_cb_init);
}

