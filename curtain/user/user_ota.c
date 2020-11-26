#include "user_ota.h"

/**
 * server_ip: 服务器地址
 * port:服务器端口
 * path:文件路径
 */
void ICACHE_FLASH_ATTR ota_start_Upgrade(const char *server_ip, uint16_t port,const char *path) {
    const char* file;
    //获取系统的目前加载的是哪个bin文件
    uint8_t userBin = system_upgrade_userbin_check();
    switch (userBin) {
    //如果检查当前的是处于user1的加载文件，那么拉取的就是user2.bin
    case UPGRADE_FW_BIN1:
        file = BIN2;
        break;
        //如果检查当前的是处于user2的加载文件，那么拉取的就是user1.bin
    case UPGRADE_FW_BIN2:
        file = BIN1;
        break;
        //如果检查都不是，可能此刻不是OTA的bin固件
    default:
        os_printf("Fail read system_upgrade_userbin_check! \n\n");
        return;
    }
    struct upgrade_server_info* update =(struct upgrade_server_info *) os_zalloc(sizeof(struct upgrade_server_info));
    update->pespconn = (struct espconn *) os_zalloc(sizeof(struct espconn));
    //设置服务器地址
    u32 u32_ip = ipaddr_addr(server_ip);
    os_memcpy(update->ip, &u32_ip, 4);
    //设置服务器端口
    update->port = port;
    //设置OTA回调函数
    update->check_cb = ota_finished_callback;
    //设置定时回调时间
    update->check_times = 120000;
    //从 4M *1024 =4096申请内存
    update->url = (uint8 *)os_zalloc(2048);
    //打印下請求地址
    os_printf("Http Server Address:%d.%d.%d.%d ,port: %d,filePath: %s,fileName: %s \n",
            IP2STR(update->ip), update->port, path, file);
    //拼接完整的 URL去请求服务器
    os_sprintf((char*) update->url, "GET /%s%s HTTP/1.1\r\n"
            "Host: "IPSTR":%d\r\n"
    "Connection: keep-alive\r\n"
    "\r\n", path, file, IP2STR(update->ip), update->port);
    if (system_upgrade_start(update) == false) {
        os_printf(" Could not start upgrade or already started\n");
        //释放资源
        /*
        os_free(update->pespconn);
        os_free(update->url);
        os_free(update);*/
    } else {
        os_printf(" Upgrading...\n");
    }
}

void ICACHE_FLASH_ATTR ota_finished_callback(struct upgrade_server_info* arg){
    struct upgrade_server_info *update = arg;
    if (update->upgrade_flag == true){
        os_printf("OTA  Success ! rebooting!\n");
        system_upgrade_reboot();
    }else{
        os_printf("OTA failed!\n");
    }


}

