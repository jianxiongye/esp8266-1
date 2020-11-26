# stm32+ESP8266 wifi scan
## FUNCTION:
- 1.use ESP8266 scanning wifis around
- 2.format data to json view
- 3.snprinf data contain HTTPS head
- 4.use AT cmd send HTTPS to wayz api and get location
## ATTENTION:

```
#define USART3_MAX_RECV_LEN		2000					//最大接收缓存字节数800 zaki修改为2000
#define USART3_MAX_SEND_LEN		4000					//最大发送缓存字节数800 zaki修改为4000
char senddata[100*40];
iot_wifis_t wifi_x[30];//sizeof(iot_wifis_t) = 88
```
Now we can only support below 30wifis data transfer!It should be modify in time.


