cd sniffer_demo  
./go.sh

////////  
cd ota  
./1Mflash.sh  
download  
baud 76800  

or  
./4MBflash1024KB.sh  
download  
baud 76800


add  
1.doublehttpclient for test project  
2.light for relay light project  
3.scansdk for wayz scan wifi and locate project  
4.scansniffer for wayz locate and sniffer project  
5.scansniffer_TV for android TV used firmware ,findu v1.0   
6.curtain	for IOT curtain made    
7.scansniffer_sendall	send all sniff data 
8.scansnifferStoreSend  single wifi sniff 10mins send to server nonos version   

9.rtos_snifferbackup/ rtos version sniffer project. RTOSv3.3, findu v2.0   
10.rtos_sniffer/  rtos version single wifi project.   
11.rtos_bin/   rtos fw download. copy from rtos_snifferbackup/build/   

12.stm32wifiscanLocate/ for stm32 use AT cmd to control esp8266 module scanwifi and locate  
13.rtos_sniffer_lora/   for sniff and store 6Bytes MAC + 1Byte Sig then use M200 easylink RF send  

