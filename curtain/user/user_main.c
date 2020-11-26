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

#include "driver/uart.h"
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "http_client.h"
#include "user_config.h"
#include "user_ota.h"
#include "user_curtain.h"


//**********************************************************************************/
// SDK v3.0



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
u16 ota_flag;

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
	os_printf("user bin:%d\r\n", system_upgrade_userbin_check());
	os_printf("SDK version:%s\r\n", system_get_sdk_version());
	os_printf("Build Datetime:%s %s\r\n", __DATE__, __TIME__);
	os_printf("Chip ID:%d\r\n", system_get_chip_id());
	os_printf("CPU freq:%d\r\n", system_get_cpu_freq());
	os_printf("Free heap size:%d\r\n", system_get_free_heap_size());
	if (wifi_get_macaddr(STATION_IF, macAddr)) {
		os_printf("MAC:"MACSTR"\r\n", MAC2STR(macAddr));
	} else {
		os_printf("Get MAC fail!\r\n");
	}

	os_printf("meminfo:\r\n");
	system_print_meminfo();
	os_printf("*********************************\r\n");
}
static os_timer_t channelHop_timer;

void ICACHE_FLASH_ATTR
diarmTimer(void){
	os_timer_disarm(&channelHop_timer);

}


void ICACHE_FLASH_ATTR
channelHop(void *arg){
	//http_get_lightstate_send();
	//os_printf("channelHop sending\n");
	static uint64 i=1000;
	static int firstin =0;
	firstin++;
		char power_open2[4]  = {0xA0,0x02,0x01,0xA3};
	if(firstin<100){
		//open motor driver/uart
		//uart0_tx_buffer(power_open2, 4);
		return;
	}else if(firstin<200 && firstin>100){
		uart0_tx_buffer(power_open2, 4);
	}
	else{
		if(i==0){
			//top
			os_printf("over\n");
			os_timer_disarm(&channelHop_timer);
			user_pwm_close();
			//恢复现场，继续查询
			task_postb();i=1000;
			
			char power_close2[4] = {0xA0,0x02,0x00,0xA2};
			uart0_tx_buffer(power_close2, 4);
			uart0_tx_buffer(power_close2, 4);
			uart0_tx_buffer(power_close2, 4);
			uart0_tx_buffer(power_close2, 4);
			uart0_tx_buffer(power_close2, 4);
			firstin=0;
		}else{
			user_pwm_open();
			///gpio_output_set( BIT0, 0,BIT0, 0);
			i--;

			//GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 1);
			//os_delay_us(100);
			//GPIO_OUTPUT_SET(GPIO_ID_PIN(2), 0);
			//os_delay_us(100);
		}
	}


}

void ICACHE_FLASH_ATTR
channelHop1(void *arg){
	os_printf("channelHop1\n");
	http_get_lightstate_connect();

}
#define CHANNEL_HOP_INTERVAL1 1000

#define CHANNEL_HOP_INTERVAL 10

void ICACHE_FLASH_ATTR
test_task (os_event_t *e) {
	switch (e->sig) {
		case SIG_RX:
			os_printf("sig_rx ----------%c\n", (char)e->par);
			if((char)e->par == 'a'){
				os_delay_us(60000);
	
				http_client_disconnect();
				ota_start_Upgrade(HOST, PORT, "");
			}else if((char)e->par == 'b'){
	
				http_get_lightstate_connect();
				os_timer_disarm(&channelHop_timer);
			    os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop1, NULL);
			    os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL1, 1);
				
			}else{
	
				os_timer_disarm(&channelHop_timer);
			    os_timer_setfn(&channelHop_timer, (os_timer_func_t *) channelHop, NULL);
			    os_timer_arm(&channelHop_timer, CHANNEL_HOP_INTERVAL, 1);
				//start gpio2 enable
				


		
				
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
void task_post(void) {
system_os_post(USER_TASK_PRIO_1, SIG_RX, 'a');
}

void task_postb(void) {
system_os_post(USER_TASK_PRIO_1, SIG_RX, 'b');
}


void task_postc(void) {
system_os_post(USER_TASK_PRIO_1, SIG_RX, 'c');
 //user_light_init();

}





void ICACHE_FLASH_ATTR
init_done_cb_init(void) {
	print_chip_info();

#if 1
	user_wifi_init();
#else
	// parameter: STATION_MODE/SOFTAP_MODE/STATIONAP_MODE
	wifi_set_opmode(STATION_MODE);
	user_set_station_config();
#endif
	wifi_station_disconnect();
	wifi_station_connect();

	http_get_version_init(HOST, PORT);
	http_get_lightstate_init(HOST, PORT);
/*	while(1){
		os_delay_us(60000);
			if(ota_flag == 1){
				os_delay_us(60000);
				http_client_disconnect();

				ota_start_Upgrade(HOST, PORT, "");
			}
	}*/
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2);

task_init();
//gpio2 pwm enable
user_light_init();

//system_os_post(USER_TASK_PRIO_1, SIG_RX, 'c');

//task_post();

}



/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void) {
//	uart_init(76800, 76800);
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_printf("SDK version:%s\n", system_get_sdk_version());
//	os_printf("user bin:%d\r\n", system_upgrade_userbin_check());
	system_init_done_cb(init_done_cb_init);
}

