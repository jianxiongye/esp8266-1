

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "user_uart.h"
#include "hytera_protol.h"


tSendUartData tUartSendData = {0};
static tHyteraProReq tHyteraProData = {0};

uint16_t checksum(uint16_t * buffer, int size)
{
    uint16_t temp = 0;
    uint16_t answer = 0;
    unsigned long cksum = 0;

    while (size > 1)
    {
        answer = *buffer;
        temp = ~answer;
        cksum += temp;
        buffer++;
        size = size - sizeof(uint16_t);
    }

    if (size)
    {
        answer = *buffer;
        temp = ~answer;
        cksum += temp;
    }

    while(cksum >> 16)
    {
        cksum = (cksum >> 16) + (cksum & 0xffff);
    }

    answer = (uint16_t)cksum;

    return answer;
}

void connect_hytera(void)
{
    uint8_t len = sizeof (tHyteraProUnit);
    memset(&tHyteraProData, 0, sizeof(tHyteraProData));
    tHyteraProData.hy_header.hy_headerId = HYTERA_HEADER_ID;
    tHyteraProData.hy_header.hy_version = HYTERA_VERSION;
    tHyteraProData.hy_header.hy_block = HYTERA_BLOCK;
    tHyteraProData.hy_header.hy_opCode = HYTERA_CONNECT;
    tHyteraProData.hy_header.hy_srcId = HYTERA_SCRID;
    tHyteraProData.hy_header.hy_desId = HYTERA_DESID;
    tHyteraProData.hy_header.hy_pn = 0x00;
    tHyteraProData.hy_header.hy_length = BigtoLittle16(len);
    tHyteraProData.hy_header.hy_checkSum = checksum((uint16_t *)&tHyteraProData, len);

#if OPEN_PRINTF
    printf("-------connect_hytera start---------\r\n");
    char *buf = (char *)&tHyteraProData;
    for (int i = 0; i < len; i ++)
    {
        printf("%02X ", buf[i]);
    }
    printf("\r\n");
    printf("-------connect_hytera end---------\r\n");
#endif
    uart_send_data((const char *) &tHyteraProData, len);
    uart_send_data("\r\n", 2);
}

void send_data_hytera(char *data, uint8_t length)
{
    uint8_t len = sizeof (tHyteraProData);
    memset(&tHyteraProData, 0, sizeof(tHyteraProData));
    tHyteraProData.hy_header.hy_headerId = HYTERA_HEADER_ID;
    tHyteraProData.hy_header.hy_version = HYTERA_VERSION;
    tHyteraProData.hy_header.hy_block = HYTERA_BLOCK;
    tHyteraProData.hy_header.hy_opCode = HYTERA_DATA;
    tHyteraProData.hy_header.hy_srcId = HYTERA_SCRID;
    tHyteraProData.hy_header.hy_desId = HYTERA_DESID;
    tHyteraProData.hy_header.hy_pn = 0x00;
    tHyteraProData.hy_header.hy_length = BigtoLittle16(len);
    //tHyteraProData.hy_header.hy_checkSum = BigtoLittle16(checksum((uint16_t *)&tHyteraProData, len));
    memcpy(&tHyteraProData.hy_payload, data, length);
    tHyteraProData.hy_header.hy_checkSum = checksum((uint16_t *)&tHyteraProData, len);

#if OPEN_PRINTF
    printf("-------send_data_hytera start---------\r\n");
    char *buf = (char *)&tHyteraProData;
    for (int i = 0; i < len; i ++)
    {
        printf("%02X ", buf[i]);
    }
    printf("\r\n");
    printf("-------send_data_hytera end---------\r\n");
#endif
    uart_send_data((const char *) &tHyteraProData, len);
    uart_send_data("\n", 1);
}

void close_hytera(void)
{
    uint8_t len = sizeof (tHyteraProUnit);
    memset(&tHyteraProData, 0, sizeof(tHyteraProData));
    tHyteraProData.hy_header.hy_headerId = HYTERA_HEADER_ID;
    tHyteraProData.hy_header.hy_version = HYTERA_VERSION;
    tHyteraProData.hy_header.hy_block = HYTERA_BLOCK;
    tHyteraProData.hy_header.hy_opCode = HYTERA_CLOSE;
    tHyteraProData.hy_header.hy_srcId = HYTERA_SCRID;
    tHyteraProData.hy_header.hy_desId = HYTERA_DESID;
    tHyteraProData.hy_header.hy_pn = 0x00;
    tHyteraProData.hy_header.hy_length = BigtoLittle16(len);
    tHyteraProData.hy_header.hy_checkSum = checksum((uint16_t *)&tHyteraProData, len);

#if OPEN_PRINTF
    printf("-------close_hytera start---------\r\n");
    char *buf = (char *)&tHyteraProData;
    for (int i = 0; i < len; i ++)
    {
        printf("%02X ", buf[i]);
    }
    printf("\r\n");
    printf("-------close_hytera end---------\r\n");
#endif
    uart_send_data((const char *) &tHyteraProData, len);
    uart_send_data("\n", 1);
}
