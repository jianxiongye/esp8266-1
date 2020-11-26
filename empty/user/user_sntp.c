/*
 * user_sntp.c
 *
 *  Created on: 2017��7��7��
 *      Author: Administrator
 */

#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"

#define SNTP_READ_INTERVAL	3000
static os_timer_t user_sntp_timer;


static void ICACHE_FLASH_ATTR
sntp_timer_cb(void *arg) {
	uint32_t time = sntp_get_current_timestamp();
	os_printf("[sntp_timer_cb] time:%d\t - %s", time, sntp_get_real_time(time));
	if(time){
		os_printf("[sntp_timer_cb] we got time:%d\t - %s", time, sntp_get_real_time(time));
		os_timer_disarm(&user_sntp_timer);
		wifiscan();
	}
}

void ICACHE_FLASH_ATTR
user_sntp_init(void) {


	// set sntp server
	//sntp_setservername(0, "0.cn.pool.ntp.org");
	//sntp_setservername(1, "1.cn.pool.ntp.org");
	//sntp_setservername(2, "2.cn.pool.ntp.org");
	sntp_setservername(0, "ntp1.aliyun.com");
	sntp_setservername(1, "ntp2.aliyun.com");

	// sntp init
	sntp_init();
	
	uint32_t time = sntp_get_current_timestamp();
	os_printf("time:%d\t - %s", time, sntp_get_real_time(time));

	// timer init
	os_timer_disarm(&user_sntp_timer);
	os_timer_setfn(&user_sntp_timer, sntp_timer_cb, NULL);
	os_timer_arm(&user_sntp_timer, SNTP_READ_INTERVAL, 1);
}
