
#ifndef _USER_OTA_H_
#define _USER_OTA_H_
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "upgrade.h"
#include "espconn.h"

#include "mem.h"




extern void  ICACHE_FLASH_ATTR ota_finished_callback();
extern void  ICACHE_FLASH_ATTR ota_start_Upgrade(const char *server_ip, uint16_t port,const char *path);

#endif /* _USER_OTA_H_ */


