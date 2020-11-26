/*
 * sniffer.h
 */

#ifndef SNIFFER_DEMO_INCLUDE_SNIFFER_H_
#define SNIFFER_DEMO_INCLUDE_SNIFFER_H_

#include "ets_sys.h"
#include "os_type.h"

#define SNIFFER_TEST			1

/* ʹ��DEAUTH */
#define DEAUTH_ENABLE 			0

/* ÿ��x����channel */
#define HOP_JUMP_ENABLE			1
#define CHANNEL_HOP_INTERVAL 1000

extern void ICACHE_FLASH_ATTR sniffer_init(void);
extern void ICACHE_FLASH_ATTR sniffer_init_in_system_init_done(void);

struct RxControl {
    signed rssi:8;				// signal intensity of packet
    unsigned rate:4;
    unsigned is_group:1;
    unsigned:1;
    unsigned sig_mode:2;		// 0:is 11n packet; 1:is not 11n packet;
    unsigned legacy_length:12;	// if not 11n packet, shows length of packet.
    unsigned damatch0:1;
    unsigned damatch1:1;
    unsigned bssidmatch0:1;
    unsigned bssidmatch1:1;
    unsigned MCS:7;			// if is 11n packet, shows the modulation
    						// and code used (range from 0 to 76)
    unsigned CWB:1;			// if is 11n packet, shows if is HT40 packet or not
    unsigned HT_length:16;	// if is 11n packet, shows length of packet.
    unsigned Smoothing:1;
    unsigned Not_Sounding:1;
    unsigned:1;
    unsigned Aggregation:1;
    unsigned STBC:2;
    unsigned FEC_CODING:1;	// if is 11n packet, shows if is LDPC packet or not.
    unsigned SGI:1;
    unsigned rxend_state:8;
    unsigned ampdu_cnt:8;
    unsigned channel:4;		// which channel this packet in.
    unsigned:12;
};

struct LenSeq {
    uint16_t length;
    uint16_t seq;
    uint8_t  address3[6];
};

struct sniffer_buf {
    struct RxControl rx_ctrl;
    uint8_t buf[36];
    uint16_t cnt;
    struct LenSeq lenseq[1];
};

struct sniffer_buf2{
    struct RxControl rx_ctrl;
    uint8_t buf[112];
    uint16_t cnt;
    uint16_t len;
};

struct framecontrol_t{
	unsigned:2;//frame protocol
	unsigned frametype:2;//frame type
	unsigned subtype:4;//sub type
	unsigned datadir:2;
	unsigned:6;//other
};

#define FRAME_MANAGE   0
#define FRAME_CONTROL  1
#define FRAME_DATA     2
#define FRAME_RESERVED 3

#define SUBTYPE_ASSREQ 0
#define SUBTYPE_ASSREP 1
#define SUBTYPE_RESREQ 2
#define SUBTYPE_RESREP 3
#define SUBTYPE_PRBREQ 4
#define SUBTYPE_PRBREP 5
#define SUBTYPE_TIMEAD 6
#define SUBTYPE_RESERV 7
#define SUBTYPE_BEACON 8
#define SUBTYPE_ATIM   9
#define SUBTYPE_DISASS 10
#define SUBTYPE_AUTH   11
#define SUBTYPE_DEAUTH 12
#define SUBTYPE_ACTION 13
#define SUBTYPE_ANOACK 14
#define SUBTYPE_RESER2 15

#define SUBTYPE_DATA 0
#define SUBTYPE_NULL_DATA 4
#define SUBTYPE_QOS_DATA 8
#define SUBTYPE_QOS_NULL_DATA 12 //0xc


#define DATA_DIR_TOAP   1
#define DATA_DIR_FROMAP 2





#define printmac(buf, i) os_printf("%02X:%02X:%02X:%02X:%02X:%02X", buf[i+0], buf[i+1], buf[i+2], \
                                                   buf[i+3], buf[i+4], buf[i+5])

#define PRINT(buf,i) buf[i+0], buf[i+1], buf[i+2], buf[i+3], buf[i+4], buf[i+5]


#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

#endif /* SNIFFER_DEMO_INCLUDE_SNIFFER_H_ */
