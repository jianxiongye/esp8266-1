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
u32 secB_addr=0x302000;
u32 secC_addr=0x202000;//1024+1024
#define secD_addr 0x301000


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
char fistini=0;
extern uint8 mWifiName[32];
extern uint8 mWifiPass[32];
extern uint8 mServer[32];
extern u16 mPort;
extern uint8 m_sniff_send_loop_min;


 void ICACHE_FLASH_ATTR
keepinfo(){
	u8 macAddr[6] = { 0 };
	spi_flash_read(secA_addr, des_addr,  4);

	uint8* data;
	data=(uint8*)des_addr;
	os_printf("\nkeepinfo-----\nsetting:m_sniff_send_loop_min=%d\nsetting:mWifiName=%s\nsetting:mWifiPass=%s\nsetting:ip=%s:%d\n",\
				m_sniff_send_loop_min,mWifiName,mWifiPass,mServer, mPort);



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
	os_printf("ProduceDate=%02d%02d%02d%02d\n",data[0],data[1],data[2],data[3]);

	spi_flash_read(secC_addr, des_addr,  100);
	//if(data[4]){
		os_memcpy(mWifiName, data,32);
		os_memcpy(mWifiPass, data+32,32);
		os_memcpy(mServer, data+64,32);
		os_memcpy(&mPort, data+96,2);
		m_sniff_send_loop_min=data[98];
		os_printf("copy data from flash\n");
	//}


	os_printf("WifiName=%s\n",mWifiName);//WifiName 32B
	os_printf("WifiPass=%s\n",mWifiPass);//WifiPass 32B
	os_printf("Server=%s\n",mServer);//Server 32B
	os_printf("Port=%d\n",mPort);//mPort 2B
	os_printf("sniff_send_loop_min=%d\n",m_sniff_send_loop_min);//1B
	

init:
	if(fistini==0){
		fistini=1;

		for(i=0; i<SNIFF_SEND_LOOP_MIN*10; i++){
			/*  ProduceDate=20200423
				state: 5 -> 0 (0)
				rm 0
				pm close 7
				Fatal exception 28(LoadProhibitedCause):
				epc1=0x401063e8, epc2=0x00000000, epc3=0x00000000, excvaddr=0x00000004, depc=0x00000000
				试修复:
			*/
				spi_flash_erase_sector((secB_addr/FourKB) +i);
				//spi_flash_erase_sector((u16)(302 +i));
			//os_printf("erase sec=%d ori=%d\n",(u16)(302 +i),(secB_addr/FourKB) +i);
		}
	}

	
	
}
/**/
int firstin=0;
int record=0;
#define num_print 51
#define size_overflow (7000) //7000x56=392000
 void ICACHE_FLASH_ATTR
writeSniffDataToFlash(uint8_t* data,uint8 len){
	
	//#define size_overflow (20)

	
	u32 des_addr1[num_print*56];
	
	
	if(record<size_overflow){
		
		spi_flash_write(secB_addr+((uint32)((os_strlen(data)/4)+1)*4*record), (uint32_t*)data, (uint32)((os_strlen(data)/4)+1)*4);
		
		spi_flash_read(secB_addr+((uint32)((os_strlen(data)/4)+1)*4*record), des_addr,         (uint32)((os_strlen(data)/4)+1)*4);
				os_printf("recordis \n");
		os_printf("%d raw  is %s", record, data);
		os_printf("%d data is %s", record, des_addr);
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
	/*移动到http_client.c中
	static int ci=0;
	if(ci<=record/18){
		spi_flash_read(secB_addr+14*ci*4*18, des_addr,   56*18);
		os_printf("ci=%d spi_flash_read:%s\r\n",ci,des_addr);
		http_client_connect();
		ci++;
	}else{
		//switch to sniffer mode,400KB flash clean
		for(i=0; i<SNIFF_SEND_LOOP_MIN*10; i++){
			spi_flash_erase_sector((secB_addr/FourKB) +i);
		}
		
		
		firstin=0;
		record=0;
		ci=0;
		task_post('a');
	}*/
	http_client_connect();

}
