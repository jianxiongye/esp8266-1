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
	

init:
	spi_flash_erase_sector(secB_addr/FourKB);
}
/**/
static int firstin=0;
int record=0;
#define num_print 51
#define size_overflow (50)
 void ICACHE_FLASH_ATTR
writeSniffDataToFlash(uint8_t* data,uint8 len){
	
	//#define size_overflow (20)

	
	u32 des_addr1[num_print*56];
	
	
	if(record<size_overflow){
		spi_flash_write(secB_addr+((uint32)((os_strlen(data)/4)+1)*4*record), (uint32_t*)data, (uint32)((os_strlen(data)/4)+1)*4);
		record++;

	}else{
		
		if(firstin==0){
			firstin=1;
			spi_flash_write(secB_addr+((uint32)((os_strlen(data)/4)+1)*4*record), (uint32_t*)data, (uint32)((os_strlen(data)/4)+1)*4);
			os_printf("data overflow------------------------------------------\n");
			spi_flash_read(secB_addr, des_addr,   (uint32)((os_strlen(data)/4)+1)*4*num_print);
			/*for(i = 0; i<FourKB/4; i++){
				os_printf(" %08X ", des_addr[i]);
			}*/
			for(i = 0; i<num_print; i++){
				os_printf("data is %s", des_addr+i*((os_strlen(data)/4)+1));

			}
			task_post('d');

		}
		
	}
}

void ICACHE_FLASH_ATTR
send_all_data(){
	//reload data and send to client
	static int ci=0;
	if(ci<num_print){
		spi_flash_read(secB_addr+14*ci*4, des_addr,   14*4);
		os_printf("ci=%d spi_flash_read:%s\r\n",ci,des_addr);
		http_client_connect();
		ci++;
	}else{
		//switch to sniffer mode
		spi_flash_erase_sector(secB_addr/FourKB);
		task_post('a');
		firstin=0;
		record=0;
		ci=0;
	}

}