#include "wayz.h"
#define DEBUG_ZAKI
int32 wificount = 0;
#define DEFAULT_TIME 1567991349999
//#define KEY_SUNMI "BTHX7yrJ5PgrbY4EiymN9YbqDk"
iot_wifis_t wifi_x[30];//sizeof(iot_wifis_t) = 88
iot_wifis_t *wifi_m=wifi_x;//sizeof(iot_wifis_t) = 88
char post_content[210];  //144+55=200
//cip发送的长度
char cipsend[]="AT+CIPSEND=12345689";
char senddata[100*40];






#define _LINE_LENGTH_DEVID 20
#define _LINE_LENGTH_MAC   20
#define _LINE_LENGTH_UUID  38
#define _LINE_LENGTH_MODEL 20
#define _LINE_LENGTH_SOFTV 100

//usmart支持部分
//将收到的AT指令应答数据返回给电脑串口
//mode:0,不清零USART3_RX_STA;
//     1,清零USART3_RX_STA;
void atk_8266_bridge(u8 mode)
{	
	while(1){

		POINT_COLOR = BLACK;
		delay_ms(10);

		if(USART3_RX_STA&0X8000)		//接收到一次数据了
		{ 
			USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;//添加结束符
			printf("%s",USART3_RX_BUF);	//发送到串口
			LCD_ShowString(10,80,300,16,16,USART3_RX_BUF);
			if(mode)USART3_RX_STA=0;
			//清0
			memset((void*)USART3_RX_BUF,'\0',USART3_MAX_RECV_LEN);
		}
		if(USART_RX_STA&0X8000){
			USART_RX_BUF[USART_RX_STA&0X7FFF]=0;//添加结束符
			LCD_ShowString(10,100,300,16,16,USART_RX_BUF);
			u3_printf("%s\r\n",USART_RX_BUF);	//发送命令
			if(mode)USART_RX_STA=0;
			memset((void*)USART_RX_BUF,'\0',USART_REC_LEN);
		}
	}
}



//ATK-ESP8266发送命令后,检测接收到的应答
//str:期待的应答结果
//返回值:0,没有得到期待的应答结果
//    其他,期待应答结果的位置(str的位置)
u8* atk_8266_check_cmd(u8 *str)
{
	
	char *strx=0;
	if(USART3_RX_STA&0X8000)		//接收到一次数据了
	{ 
		USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;//添加结束符
		strx=strstr((const char*)USART3_RX_BUF,(const char*)str);
		if(strstr((const char*)USART3_RX_BUF,"ERROR")){
			loge("send_cmd-----error detected!");
			atk_8266_bridge(1);
		}
	} 
	return (u8 *)strx;
}
//向ATK-ESP8266发送命令
//cmd:发送的命令字符串
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)
//       1,发送失败
u8 atk_8266_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART3_RX_STA=0;
	u3_printf("%s\r\n",cmd);	//发送命令
	logd("send_cmd-|cmd=%s",cmd);
	if(ack&&waittime)		//需要等待应答
	{
		while(--waittime)	//等待倒计时
		{
			
			//搜索wifi指令应当适当多加点延时
			if(!strcmp((const char*)cmd, "AT+CWLAP")){
					delay_ms(200);
			}

			delay_ms(10);
			if(USART3_RX_STA&0X8000)//接收到期待的应答结果
			{	logd("send_cmd---lenth=%d %s\r\n",strlen((const char*)USART3_RX_BUF),USART3_RX_BUF);
				if(atk_8266_check_cmd(ack))
				{
					logi("send_cmd----ack:%s\r\n",(u8*)ack);
					logd("send_cmd-----USART3_RX_BUF=%s",(char*)USART3_RX_BUF);
					break;//得到有效数据 
				}
					USART3_RX_STA=0;
			} 
		}
		if(waittime==0){
			res=1;
			loge("send_cmd------CMD TIME OUT! cmd=%s",cmd);
		}
	}
	return res;
} 
//向ATK-ESP8266发送指定数据
//data:发送的数据(不需要添加回车了)
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//返回值:0,发送成功(得到了期待的应答结果)luojian
u8 atk_8266_send_data(u8 *data,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART3_RX_STA=0;
	u3_printf("%s",data);	//发送命令
	if(ack&&waittime)		//需要等待应答
	{
		while(--waittime)	//等待倒计时
		{
			delay_ms(10);
			if(USART3_RX_STA&0X8000)//接收到期待的应答结果
			{
				logi((char*)USART3_RX_BUF);
				if(atk_8266_check_cmd(ack))break;//得到有效数据 
				USART3_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
}

//ATK-ESP8266退出透传模式
//返回值:0,退出成功;
//       1,退出失败
u8 atk_8266_quit_trans(void)
{
	while((USART3->SR&0X40)==0);	//等待发送空
	USART3->DR='+';      
	delay_ms(15);					//大于串口组帧时间(10ms)
	while((USART3->SR&0X40)==0);	//等待发送空
	USART3->DR='+';      
	delay_ms(15);					//大于串口组帧时间(10ms)
	while((USART3->SR&0X40)==0);	//等待发送空
	USART3->DR='+';      
	delay_ms(500);					//等待500ms
	return atk_8266_send_cmd("AT","OK",20);//退出透传判断.
}

int HAL_Snprintf(char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int     rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}


////wifi个数获取函数
//int getWifiCount(char* data)
//{
//	char *strTmp = data;
//	int count=0;
//	while(strTmp!=NULL){
//		strTmp = strstr(strTmp,"+CWLAP");
//		if(strTmp != NULL){
//			strTmp += 6;
//			count++;
//		}
//	}
//	return count;
//}

//str查找个数函数
//限制：最多能承载32位无符号型的个数，原因你懂的
u32 strCount(char* data, const char* str){
	char *strTmp = data;
	u32 count=0;
	u32 len = strlen(str);
	while(strTmp!=NULL){
		strTmp = strstr(strTmp,str);
		if(strTmp != NULL){
			strTmp += len;
			count++;
		}
	}
	return count;
}

//wifi个数获取函数
u32 getWifiCount(char* data)
{
	return strCount(data, "+CWLAP");
}

/**************FUCTION AtoI()******************/
/**********************************************
 Author: zakiWang
 fuction: Less than 19byte string input convert to long long interger.
 		  Can automatically identify comma and space key and then jump.
 params:*input: input string
 return:return a long long(64bit) data back
 constrain:input data must less than 19byte!
 Example:	
 	printf("%lld\r\t",AtoI("1234"));
	printf("%lld\r\t",AtoI("+1234"));
	printf("%lld\r\t",AtoI("-123,4,"));
	printf("%lld\r\t",AtoI("-2,036,854,775,808"));
	printf("%lld\r\t",AtoI("-36, 854,77 5,808"));

************************************************/

int64 AtoI(const char* input)
{
	//表示值介于 -2^63 ( -9,223,372,036,854,775,808) 到2^63-1(+9,223,372,036,854,775,807 )
	int64 ans=0;
	const char *temp = input;
	char minus = 0;//符号标志
	char i=0;
	char count = 19;//数据个数 
	if(*input == '-'){
		minus = 1;
		temp++;
	}else if(*input == '+'){
		temp++;
	}
	for(i=0; i<count; i++){
		if(*temp == ','|| *temp == ' '){
			;
		}else if(*temp == NULL){
			if(minus == 1)
				return (-ans);
			else
				return ans;
		}else{
			ans = ans* 10 + (*temp - '0');
		}
		temp++;
	}
	return ans;
}

void printOutInfo(iot_wifis_t *wifi){
	POINT_COLOR = GREEN;
//	#define RGBLCD4_3_X 272
//#define RGBLCD4_3_Y 480
	LCD_DrawLine(100, 0, 100, 5);
	LCD_DrawLine(200, 0, 200, 5);
	LCD_DrawLine(300, 0, 300, 5);
	LCD_DrawLine(0, 100, 5, 100);
	LCD_DrawLine(0, 200, 5, 200);
	LCD_DrawLine(0, 300, 5, 300);
	LCD_DrawLine(0, 400, 5, 400);
	LCD_DrawLine(0, 500, 5, 500);

	LCD_DrawLine(136, 0, 136, 480);
	LCD_DrawLine(0, 240, 272, 240);
POINT_COLOR = BLUE;
	for(int i=0; i<wificount; i++){
		LCD_ShowString(5,80+i*20,300,16,16,(u8*)wifi[i].macAddress);
		LCD_ShowNum(150,80+i*20,wifi[i].signalStrength,2,16);
		LCD_ShowNum(170,80+i*20,wifi[i].channel,2,16);
		LCD_ShowString(190,80+i*20,300,16,16,(u8*)wifi[i].ssid);
		
	}

}

char *getUUID(){
	static	 char line[_LINE_LENGTH_UUID]="123123123";
	return line;
}
char *getMODEL(){
	static	 char line[_LINE_LENGTH_MODEL];
	return line;
}
char *getDEVID(){
	static	 char line[_LINE_LENGTH_DEVID];
	return line;
}
char *getSOFTV(){
	static	 char line[_LINE_LENGTH_SOFTV];
	return line;
}
char *getMAC(){
	static	 char line[_LINE_LENGTH_MAC];
	char *p = (char*)USART3_RX_BUF;
	int i=0;
	
	atk_8266_send_cmd("AT+CIPSTAMAC?", "", 20);
	//剪切mac：+CIPSTAMAC:"4c:11:ae:0d:92:0b"
	while(*p++ != '"');//p直接移位到mac起始位
	for(i=0; *p != '"'; i++){
		line[i] = *p++; 
	}
	line[i]=NULL;

	logd("line=%s\r\n",line);
	logd("line=%s",line);
	loge("line=%s",line);
	return line;
}

//测试52.82.35.145：8084端口的http协议通路
void postTest(){
	atk_8266_send_cmd("AT+CIPMUX?","",20);			//查询是否为多链接模式，还是单链接模式
	
	atk_8266_send_cmd("AT+CIPSTATUS","STATUS",20);	//查询链接状态
	atk_8266_send_cmd("AT+CIPCLOSE","CLOSE",200);	//关闭TCP链接

	
	atk_8266_send_cmd(RUI2,"OK",200);				//发送TCP链接指令
	atk_8266_send_cmd("AT+CIPSEND=?","OK",20);		//是否可以发送数据

	sprintf(cipsend,"AT+CIPSEND=%d",strlen(GETRUI2));//将长度值放入

	while(atk_8266_send_cmd((u8*)cipsend,">",20));		//发送长度协商
	atk_8266_send_cmd(GETRUI2,"IPD",200);			//发送数据并查看结果是否正常返回
	logd("send completed!");
	delay_ms(1000);
	
	logi("---------------startPOST_WAYZ_HTTP--------------");
	delay_ms(1000);
	atk_8266_send_cmd("AT+CIPSTATUS","STATUS",20);
	atk_8266_send_cmd(WAYZ_T,"OK",200);
	atk_8266_send_cmd("AT+CIPSEND=?","OK",20);
	sprintf(cipsend,"AT+CIPSEND=%d",strlen(POST_WAYZ_HTTP));

	while(atk_8266_send_cmd((u8*)cipsend,">",20));
	while(atk_8266_send_cmd(POST_WAYZ_HTTP,"IPD",1000));
	logd("send completed!");
	delay_ms(1000);
	logi("---------------endPOST_WAYZ_HTTP--------------");

//while(1);
}

//测试api.newayz.com的https协议通路
void postTestHttps(){
	logi("---------------startPOST_WAYZ_HTTPS--------------");
	delay_ms(1000);

	//建立连接
	if(atk_8266_send_cmd("AT+CIPSSLSIZE?","4096",20))   //如果返回结果是4096就无需设置
		atk_8266_send_cmd("AT+CIPSSLSIZE=4096","OK",20);
	atk_8266_send_cmd("AT+CIPSTATUS","STATUS",20);
	atk_8266_send_cmd(SSL_WAYZ,"OK",200);

	//发送请求，返回结果
	atk_8266_send_cmd("AT+CIPSEND=?","OK",20);
	sprintf(cipsend,"AT+CIPSEND=%d",strlen(POST_WAYZ_HTTPS));
	while(atk_8266_send_cmd((u8*)cipsend,">",20));
	while(atk_8266_send_cmd(POST_WAYZ_HTTPS,"IPD",1000));
	logd("send completed!");
	delay_ms(1000);
	logi("---------------endPOST_WAYZ_HTTPS--------------");
}

void initESP8266(){
	atk_8266_quit_trans();

	delay_ms(1000);
	atk_8266_send_cmd("AT","OK",200);
	atk_8266_send_cmd("AT+CWMODE=1","OK",50);		//设置WIFI STA模式
	atk_8266_send_cmd("AT+CWAUTOCONN=0","",200);    //关闭上电自动连接AP
	
	/*while(atk_8266_send_cmd("AT+PING=\"www.baidu.com\"","OK",20)){
		//atk_8266_send_cmd("AT+CWJAP=\"home\",\"qwer801c\"","CWJAP",200))
		//atk_8266_send_cmd("AT+CWJAP=\"WAYZ\",\"20192019\"","CWJAP",200))
		logi("Connecting wifi...");
		delay_ms(500);
	}		//是否有ap链接*/

	
	while(atk_8266_send_cmd("ATE0","OK",20));		//关闭回显
	delay_ms(1000);
}


void connectWifi(){
	int timeout=10;
	if(!atk_8266_send_cmd("AT+CIPSTATUS","STATUS:3",20))//如果之前有连接，需要先关闭连接
		atk_8266_send_cmd("AT+CIPCLOSE","OK",20);    
	delay_ms(500);
	LCD_ShowString(10,80,300,16,16,"Scan completed!Connecting wifi...");
	jap:	atk_8266_send_cmd("AT+CWJAP_CUR=\"WAYZ\",\"20192019\"","CWJAP",0);
	timeout =20;

		//连接wifi
	while(atk_8266_send_cmd("AT+CIPSTATUS","STATUS:2",20) ){
		LCD_ShowString(252,50,300,16,16," ");
		LCD_ShowString(250,30,300,16,16," ");
		LCD_ShowString(250,10,300,16,16," ");
		delay_ms(200);
if(!timeout--){

	goto jap;
}

		logi("Connecting wifi...");
		LCD_ShowString(252,50,300,16,16,".");

		delay_ms(200);
		LCD_ShowString(250,30,300,16,16,"o");
		delay_ms(200);
		LCD_ShowString(250,10,300,16,16,"O");
		delay_ms(200);
		
		
	}
	LCD_ShowString(10,80,300,16,16,"Wifi Connected!!! Locate now!");
}
cJSON *gnss2Json(iot_gnss_t *gnss) {

    if (gnss) {

        cJSON *gnssJson = cJSON_CreateObject();
        cJSON_AddNumberToObject(gnssJson, "timestamp", gnss->timestamp);
        if (gnss->valid == 0) {
            cJSON_AddBoolToObject(gnssJson, "valid", cJSON_True);
        } else {
            cJSON_AddBoolToObject(gnssJson, "valid", cJSON_False);
        }
        if (gnss->velocity > 0) {
            cJSON_AddNumberToObject(gnssJson, "velocity", gnss->velocity);
        }
 /*   if (gnss->satellite_count > 0) {
            cJSON_AddNumberToObject(gnssJson, "satelliteCount", gnss->satellite_count);
        }*/
        if (gnss->accuracy > 0) {
            cJSON_AddNumberToObject(gnssJson, "accuracy", gnss->accuracy);
        }
        if (gnss->heading > 0) {
            cJSON_AddNumberToObject(gnssJson, "heading", gnss->heading);
        }
        cJSON *point = cJSON_CreateObject();

        cJSON_AddNumberToObject(point, "longitude", gnss->point.longitude);
        cJSON_AddNumberToObject(point, "latitude", gnss->point.latitude);
        cJSON_AddNumberToObject(point, "altitude", gnss->point.altitude);
        cJSON_AddItemToObject(gnssJson, "point", point);
        return gnssJson;
    }
    return NULL;
}

cJSON *wifi2Json(iot_wifis_t *wifi) {
    if (wifi) {
		logi("wifi2Json() wificount=%d\n",wifi[0].wificount);
		
	cJSON *wifiArray = cJSON_CreateArray();
		for(int32 i=0; i<wifi[0].wificount; i++){
	        
			cJSON *wifiJson = cJSON_CreateObject();
			cJSON_AddItemToArray(wifiArray,wifiJson);
// 	        cJSON_AddNumberToObject(wifiJson, "timestamp", wifi[i].timestamp);
			
	        if (wifi[i].signalStrength != 0) {
	            cJSON_AddNumberToObject(wifiJson, "signalStrength", wifi[i].signalStrength);
	        } else {
	            loge("signalStrength is zero\n");
	        }

			if(wifi[i].macAddress != NULL)
			{
				cJSON_AddStringToObject(wifiJson, "macAddress", wifi[i].macAddress);
			}

			if(wifi[i].ssid != NULL)
			{
				cJSON_AddStringToObject(wifiJson, "ssid", wifi[i].ssid);
			}
			if (wifi[i].frequency != 0) {
	            cJSON_AddNumberToObject(wifiJson, "frequency", wifi[i].frequency);
	        }
			if (wifi[i].channel != 0) {
	            cJSON_AddNumberToObject(wifiJson, "channel", wifi[i].channel);
	        }
			//printf("wifi[i].connected = %d\n",wifi[i].connected);
			/**/
	        if (wifi[i].connected == 0) {
				//这里有个bug，cjson的bug，只有cjson_invaild才是falseS
	            cJSON_AddBoolToObject(wifiJson, "connected", cJSON_Invalid);
	        } else {
	            cJSON_AddBoolToObject(wifiJson, "connected", cJSON_True);
	        }

	        
		}
		return wifiArray;
    }
    return NULL;
}

cJSON *location2Json(iot_location_t *location) {
    cJSON *locationJson = cJSON_CreateObject();
	logi("location2Jsonakisssssssssssssssssss\n");
    cJSON_AddItemToObject(locationJson, "gnss", gnss2Json(location->gnss));
	cJSON_AddItemToObject(locationJson, "wifis", wifi2Json(location->wifis));
	logi("2location2Jsonsakisssssssssssssssssss\n");
    return locationJson;
}

cJSON *os2Json(iot_os_t *os) {
    if (os != NULL) {
        cJSON *osJson = cJSON_CreateObject();
        cJSON_AddStringToObject(osJson, "type", os->type);
        cJSON_AddStringToObject(osJson, "version", os->version);
        return osJson;
    }
    return NULL;
}

cJSON *asset2Json(iot_asset_t *asset) {
    if (asset) {
        cJSON *assetJson = cJSON_CreateObject();
        cJSON_AddStringToObject(assetJson, "id", asset->id);
        cJSON_AddStringToObject(assetJson, "manufacturer", asset->manufacturer);
        cJSON_AddStringToObject(assetJson, "model", asset->model);
        cJSON_AddStringToObject(assetJson, "imei", asset->imei);
        cJSON_AddStringToObject(assetJson, "macAddress", asset->macAddress);
		cJSON_AddStringToObject(assetJson, "serialNumber", asset->serialNumber);
        cJSON_AddItemToObject(assetJson, "os", os2Json(asset->os));
        return assetJson;
    }
    return NULL;
}


cJSON *track2CJson(iot_track_point_t *track) {

    if (track) {
		logi("track2CJsonzakisssssssssssssssssss\n");
        cJSON *trackJson = cJSON_CreateObject();
	    cJSON_AddNumberToObject(trackJson, "timestamp", track->timestamp);
        if (track->id) {
                cJSON_AddStringToObject(trackJson, "id", track->id);
				logi("track2CJsonzakisssssssssssssssssss %d\n",__LINE__);
        }
		if (track->asset) {
            cJSON_AddItemToObject(trackJson, "asset", asset2Json(track->asset));
			logi("track2CJsonzakisssssssssssssssssss %d\n",__LINE__);
        }
		logi("track2CJsonzakisssssssssssssssssss %d\n",__LINE__);
        if (track->location) {
            cJSON_AddItemToObject(trackJson, "location", location2Json(track->location));
        }

        return trackJson;
    }
    return NULL;
}


char *track2JsonString(iot_track_point_t *track) {
    cJSON *trackCJson = track2CJson(track);
	logi("zakisssssssssssssssssss\n");
	logi("%s\n",cJSON_Print(trackCJson));
    char *string = cJSON_PrintUnformatted(trackCJson);
	logi("%s\n",string);
    cJSON_Delete(trackCJson);
    return string;
};

int wifiscan(int reseverd){

	printf("hello wayz!\n");
	//LCD_Clear(WHITE);		   	//清屏
	POINT_COLOR=RED;			//设置字体为红色	   	   	  
	//LCD_ShowString(60,50,200,16,16,"ALIENTEK STM32");
	LCD_ShowString(10,30,300,16,16,"BUF LEN:");
	LCD_ShowString(10,50,300,16,16,"wificount:");
	LCD_ShowNum(120,30,9999,4,16);
	LCD_ShowNum(120,50,9999,4,16);
	
	//初始化
	while(atk_8266_send_cmd("AT","OK",50))
	{
		LCD_ShowString(10,80,300,16,16,"Connecting ESP8266>--");
		delay_ms(500);
		LCD_ShowString(10,80,300,16,16,"Connecting ESP8266->-");
		delay_ms(500);
		LCD_ShowString(10,80,300,16,16,"Connecting ESP8266-->");
		delay_ms(500);
	}
	LCD_ShowString(10,80,300,16,16,"Connected! Initing...");
	initESP8266();
	LCD_ShowString(10,80,300,16,16,"Init completed!Start scanning...");
	//测试网络post
	//postTestHttps();
	//atk_8266_bridge(1);
	
	atk_8266_send_cmd("AT+CWLAP","",200);		//搜索wifi信号
	printf("zaki: %s\r\n",USART3_RX_BUF);		//打印wifi信号
	
	wificount = getWifiCount((char*)USART3_RX_BUF);

	LCD_ShowNum(120,30,strlen((const char*)USART3_RX_BUF),4,16);
	LCD_ShowNum(120,50,wificount,4,16);
	
	
if(wificount>28)
	wificount=28;
	//分析扫描到的wifi数据，并在液晶屏上打印
	getInfoFromRaw(wifi_m);
		wifi_m[0].wificount = wificount;
	printOutInfo(wifi_m);

connectWifi();

	iot_asset_t asset_m;
	iot_os_t        os_m;
	iot_track_point_t rq;
	iot_location_t    lo;
		
	rq.id        = getUUID();
	rq.timestamp = DEFAULT_TIME;
	rq.asset     = &asset_m;
	rq.location  = &lo;
		//lo.gnss = NULL;
		lo.wifis= wifi_m;

		printf("zaki wifimssid=%s\r\n",wifi_m[0].ssid);


		//rq->location->gnss=gnss_m;
		
		asset_m.id          = getMAC();
		asset_m.manufacturer= "ST";
		asset_m.macAddress  = asset_m.id;
		asset_m.model       = getMODEL();
		asset_m.serialNumber= getDEVID();
		asset_m.uniqueId    = "UNKNOWN";
		asset_m.os          = &os_m;
					os_m.type   = "None";
					os_m.version= getSOFTV();
	//POST wifi信息
	//printf("mac :%s",rq.asset->id);
	logd("mac :%s",rq.asset->macAddress);
	logd("rq.id :%s",rq.id);
	logd("rq.ostype :%s",rq.asset->os->type);
	logd("iot_wifis_t = %d",sizeof(iot_wifis_t));

	/*****************信息集中投递**********************/
#define HTTPCLIENT_CHUNK_SIZE     4096          /* read payload */
#define HTTPCLIENT_RAED_HEAD_SIZE 128            /* read header */
#define HTTPCLIENT_SEND_BUF_SIZE  1000//4096          /* send */

#define HTTPCLIENT_MAX_HOST_LEN   128
#define HTTPCLIENT_MAX_URL_LEN    1024
	//char post_buff_len[HTTPCLIENT_SEND_BUF_SIZE]={NULL};
	//char post_content[500]={NULL};

#define POST_CONTENT_TYPE "application/json"//wayz现在只用这一个type

	char *trackJson = track2JsonString(&rq);
	

logi("zaki = %s",trackJson);

	//建立SSL连接
	if(atk_8266_send_cmd("AT+CIPSSLSIZE?","4096",20))
		atk_8266_send_cmd("AT+CIPSSLSIZE=4096","OK",20);//增大size
	atk_8266_send_cmd("AT+CIPSTATUS","STATUS",20);
	while(atk_8266_send_cmd(SSL_WAYZ,"CONNECT",200));
	//while(1);
	//打印出真实的content字符串
	int32 post_content_len=strlen(trackJson);
	//HAL_Snprintf(post_content,sizeof(post_content),POST_CONTENT,post_content_len,POST_CONTENT_TYPE);
	while(atk_8266_send_cmd("AT+CIPSEND=?","OK",20));		//判断是否可以发送数据
	//sprintf(cipsend,"AT+CIPSEND=%d\0",strlen(POST_WAYZ));	//将长度值放入
	//while(atk_8266_send_cmd((u8*)cipsend,">",20));		//发送协商长度
	//atk_8266_send_cmd(POST_WAYZ,NULL,200);				//发送数据并查看结果是否正常返回

//	HAL_Snprintf(post_content,sizeof(post_content),POST_CONTENT,213,POST_CONTENT_TYPE);
	//HAL_Snprintf(post_content,sizeof(post_content),POST_CONTENT,213,POST_CONTENT_TYPE);


	
	//HAL_Snprintf(senddata,sizeof(senddata),POST_WAYZ_SSL,POST_SEND);
	HAL_Snprintf(senddata,sizeof(senddata),POST_WAYZ_SSL,post_content_len,trackJson);
	logi("post_content_len=%d",post_content_len);
	logi("!!!!!!!!!senddata len=%d str=%s",strlen(senddata),senddata);
	

	//如果大于2000的话，需要分包发送
	if(strlen(senddata)>2000)
	{
		sprintf(cipsend,"AT+CIPSEND=%d\0",strlen(senddata)-2000);//将长度值放入
		//先发2000
		while(atk_8266_send_cmd("AT+CIPSEND=2000\0",">",20));			//发送长度协商
		atk_8266_send_cmd((u8*)senddata,"Recv",400);			//发送数据并查看结果是否正常返回
		//再发剩余
		while(atk_8266_send_cmd((u8*)cipsend,">",20));		//发送长度协商
		atk_8266_send_cmd((u8*)(senddata+2000),"IPD",400);				//发送数据并查看结果是否正常返回
	}else if(strlen(senddata)>4000){
		loge("senddata big than 4000");
		while(1);
	}else{
		//小于2000字节直接发送
		sprintf(cipsend,"AT+CIPSEND=%d\0",strlen(senddata));//将长度值放入
		while(atk_8266_send_cmd((u8*)cipsend,">",20));			//发送长度协商
		atk_8266_send_cmd((u8*)senddata,"IPD",200);			//发送数据并查看结果是否正常返回
	}
	



	//trackJson

	//sprintf(cipsend,"AT+CIPSEND=%d\0",213);				//将长度值放入
	//while(atk_8266_send_cmd((u8*)cipsend,">",20));		//发送长度协商
	//atk_8266_send_cmd(POST_SEND,"IPD",100);				//发送数据并查看结果是否正常返回
	delay_ms(3000);
	logd("send completed!-----------end");
//	logi((char*)USART3_RX_BUF);
;
	POINT_COLOR=RED;
	LCD_ShowString(0, 400, 400, 16, 16, "Locate sucess:                              ");
		LCD_ShowString(0, 420, 300, 16, 16, strstr(USART3_RX_BUF,"long"));
		LCD_ShowString(0, 440, 300, 16, 16, strstr(USART3_RX_BUF,"lati"));
		LCD_ShowString(0, 460, 300, 16, 16, strstr(USART3_RX_BUF,"alti"));

	
	

	//进入串口转发功能，usart和usart3的互相转发，方便调试
	atk_8266_bridge(1);

	return 0;
}

void getInfoFromRaw(iot_wifis_t *wifi_m){
	char *p    = (char*)USART3_RX_BUF;
	char *start= NULL;
	char *end  = NULL;
	char *next = (char*)USART3_RX_BUF;
	char ssid[50];
	int i=0;
	int count = 0;
	int commaCount = 0;

	//RXBUFFER大概为+CWLAP:(4,"WAYZ",-68,"a8:0c:ca:11:76:86",1,32767,0,5,3,6,0)
	//分别为（加密方式，SSID，信号强度，MAC，信道，。。。）
	//	for(count=0; count<wificount; count++){
	for(count=0; count<wificount; count++){
		logd("count[%d] Rawinfo=%.80s\r\n",count,next);
		commaCount = 0;
		start = next;
	/*******获取SSID**********/
		//找到开始引号符
		while(*start!='\"'){
			start++;
		}
		
		//找到尾部回车符
		while(*end!='\n'){
			end = p++;
		}
		//存下尾部指针，作为下次使用
		next = end + 1;
		//从尾部发🚗，找到第九个逗号，为ssid结束符
		while(  (commaCount < 9) ){
			if(*end == ',')
				commaCount++;
			end--;
		}
		//start - end就是带引号的真实SSID
		for(i=0; start!=end; i++){
			ssid[i] = *(++start);
		}
		//最后一个字符置NULL
		ssid[--i] = NULL;
		logd("zaki ssid printf:%s\r\n",ssid);
		

			strcpy(wifi_m[count].ssid, ssid);	
			logd("zaki ssid printf:%s\r\n",wifi_m[count].ssid);

	/*******获取wifi强度**********/	
		//这时候start的指针刚读取完成SSID，在ssid的后部引号位，所以start+=2可以到达信号强度位
		start += 2;
		char strength[4];
		for(i=0; i<4; i++){
			
			if(*start == ','){
				strength[i] = NULL;
				break;
			}else{
				strength[i]=*start++;
			}
		}
		logd("stength = %s\r\n",strength);
		delay_ms(10);
		wifi_m[count].signalStrength = 0-(int32)AtoI(strength); //SDK默认发送的是正值
		logd("wifi_m.atoi(strength)= %d\r\n",(int32)AtoI(strength));
		logd("wifi_m.signalStrength= %d\r\n",wifi_m[count].signalStrength);
		//logd("wifi_m.signalStrength= %d\r\n",-123);
		
	/*******获取MAC地址**********/
		//这时候的start指针刚读取完wifi强度，在wifi强度后部引号位，所以start+2可以到达MAC地址位
		start += 2;
		for(i=0; i<17; i++){
			wifi_m[count].macAddress[i] = *start++;
		}
		wifi_m[count].macAddress[17]=NULL;//需要置NULL
		logd("wifi_m.macAddress= %s\r\n",wifi_m[count].macAddress);

	/*******获取信道信息**********/
		//这时候的start指针刚读取完mac地址，在mac后，所以start+2可以到达信道
		//为了后续兼容5G的wifi，这里数据长度定为4
		start += 2;
		char channel[4]={NULL,NULL,NULL,NULL};
		for(i=0; i<4; i++){
			if(*start == ','){
				channel[i] = NULL;
				break;
			}else{
				channel[i] = *start++;
			}
		}
		wifi_m[count].channel = (int32)AtoI(channel);
		logd("wifi_m[count].channel=%d\r\n",wifi_m[count].channel);


	}

}
