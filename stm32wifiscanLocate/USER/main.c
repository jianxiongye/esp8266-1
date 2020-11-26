#include "sys.h"  
#include "usart.h"  
#include "delay.h" 
#include "usart3.h"
#include "led.h"  
#include "usart.h" 
#include "lcd.h" 
#include "ltdc.h"
#include "wayz.h"
#include "sdram.h"
//LED端口定义
#define LED0 PBout(1)	// DS0
#define LED1 PBout(0)	// DS1	
#define KEY_zaki PFin(9)
//编码UTF-8
//说明zaki实现按键PF9引脚接地会导致
//利用stm32的uart3去发送AT指令给ESP8266，扫描周围wifi并串口打印

int main(void) {   
	u8 t=0;

	Stm32_Clock_Init(360,25,2,8);
	delay_init(180);
	uart_init(90,115200);	//初始化uart1,串口调试，使用PA9和PA10
	usart3_init(45,115200); //初始化uart3，发送AT指令给8266，使用PB10和PB11
	SDRAM_Init();			//初始化SDRAM,这里会初始化LCD的功能引脚！所以必须执行，或者后续再修改
	LCD_Init();

		LCD_Clear(WHITE);		   	//清屏
 		POINT_COLOR=RED;			//设置字体为红色	   	   	  
	LCD_ShowString(60,10,200,16,16,"<==WAYZ LOCATOR==>");

	//while(1);
	printf("zaki (GPIOx->MODER-0x40020000)/0x0400)= %d\n",(((uint32_t)GPIOC-0x40020000)/0x0400));
	GPIO_Set(GPIOB,PIN0|PIN1,GPIO_MODE_OUT,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PU); //PB0,PB1设?
	GPIO_Set(GPIOF,PIN9,GPIO_MODE_IN,GPIO_OTYPE_PP,GPIO_SPEED_100M,GPIO_PUPD_PU);
	while(1)  {
		wifiscan(0);
printf("zaki here transfer data:%d\r\n",t);  
delay_ms(500);   t++;  


	LED0=~LED0;//LED0关闭
	LED1=~LED1;//LED1关闭
		
		if(KEY_zaki == 1)
		{
			printf("zaki keypressed:%d\r\n",t); 
			KEY_zaki = 0;
		}
}
}

