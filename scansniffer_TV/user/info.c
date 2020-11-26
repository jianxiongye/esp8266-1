/*
 * sniffer.c
 */


#include "info.h"

#include "driver/uart.h"
#include "user_config.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"

//uint32 spi_flash_get_id(void);
//SpiFlashOpResult spi_flash_erase_sector(uint16 sec);
//SpiFlashOpResult spi_flash_write(uint32 des_addr, uint32 *src_addr, uint32 size);
//SpiFlashOpResult spi_flash_read(uint32 src_addr, uint32 *des_addr, uint32 size);
#define FourKB 0x1000

u32 des_addr[FourKB/4];
u32 secA_addr=0x201000;//1024+1024
u32 secB_addr=0x202000;
u32 i;
u32 static_addr[20]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};

u32 size=FourKB;
 void ICACHE_FLASH_ATTR
keepinfo1(){

	int rc = spi_flash_read(secA_addr, des_addr,  size);
	os_printf("keepinfo secA_addr=\n");
	for(i = 0; i<FourKB/4; i++){
		os_printf("0x%x ",des_addr[i]);
	}
	
	//os_printf("keepinfo rc=%d des_addr=0x%x\n",rc,des_addr);

	//
	

	spi_flash_erase_sector(0x201000/FourKB);
	for(i = 0; i<FourKB/4; i++){
		spi_flash_write(secA_addr+=4, &i, 4);
	}
}

#define FwVersion "1.0"

 void ICACHE_FLASH_ATTR
keepinfo(){
	u8 macAddr[6] = { 0 };
	spi_flash_read(secA_addr, des_addr,  4);
	char* date;
	date=(char*)des_addr;

	os_printf("----------info\n");
	os_printf("FwVersion=%s\n",FwVersion);
	os_printf("ChipId=%d\n",system_get_chip_id());
	if (wifi_get_macaddr(STATION_IF, macAddr)) {
		os_printf("Mac="MACSTR"\n", MAC2STR(macAddr));
	} else {
		os_printf("GET FAILED\n");
	}
	os_printf("Name=FindU\n");
	os_printf("Manufacturer=01\n");
	os_printf("ProduceDate=%02d%02d%02d%02d\n",date[0],date[1],date[2],date[3]);
	


}

