#include <stdio.h>
#include "esp_err.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"


static esp_netif_t *netif_handle = NULL;
#define MAX_GET_AP_RECODERS 20


static  void wifi_scan(void){
    esp_err_t   ret = ESP_OK;
    /* 1.WIFI/LwIP 初始化 */
    ret = esp_netif_init();     /* 创建LwIP核心任务,并初始化LwIP相关工作 */
    ESP_ERROR_CHECK(ret);
    ret = esp_event_loop_create_default();  /* 创建默认的系统事件任务，并初始化应用程序事件的回调 */
    ESP_ERROR_CHECK(ret);
    netif_handle = esp_netif_create_default_wifi_sta();  /* 创建网络接口 */
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&wifi_init_cfg); /* 创建WIFI驱动程序任务,并初始化WIFI驱动程序 */
    ESP_ERROR_CHECK(ret);

    /* 2.WIFI配置 */
    ret = esp_wifi_set_mode(WIFI_MODE_STA); /* 设置wifi模式 */
    ESP_ERROR_CHECK(ret);

    /* 3.WIFI启动 */
    ret = esp_wifi_start();   /* 启动wifi驱动程序 */
    ESP_ERROR_CHECK(ret);

    /* 4.WIFI扫描 */
    wifi_country_t wifi_country_cfg = {
        .cc = "CN", /* 中国 */
        .schan = 1, /* 扫描的起始信道 */
        .nchan = 13,    /* 规定的总信道数,取值最大为13 */
        .policy = WIFI_COUNTRY_POLICY_AUTO,
    };
    ret = esp_wifi_set_country(&wifi_country_cfg);
    ESP_ERROR_CHECK(ret);   
    /* 5.打印扫描到的ap */
    wifi_scan_config_t wifi_scan_cfg = {
        .channel = 0,   /* 全信道扫描 */
        .show_hidden = 0,   /* 扫描时忽略隐藏SSID的AP */
        .scan_type = WIFI_SCAN_TYPE_ACTIVE, /* 主动扫描 */
    };
    ret = esp_wifi_scan_start(&wifi_scan_cfg,true);
    ESP_ERROR_CHECK(ret);
    uint16_t ap_nums = 0;
    ret = esp_wifi_scan_get_ap_num(&ap_nums);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI("wifi-scan","ap_nums:%d",ap_nums);
    wifi_ap_record_t *scan_ap = malloc(sizeof(wifi_ap_record_t)*ap_nums);
    ret = esp_wifi_scan_get_ap_records(&ap_nums,scan_ap);
    ESP_ERROR_CHECK(ret);

    printf("%30s %s %s %s\n","SSID","频道","强度","MAC地址");
    for(int i = 0;i < ap_nums;i++){
        printf("%30s %3d %3d %02X-%02X-%02X-%02X-%02X-%02X\n",scan_ap->ssid,scan_ap->primary,\
        scan_ap->rssi,scan_ap->bssid[0],scan_ap->bssid[1],scan_ap->bssid[2],scan_ap->bssid[3],\
        scan_ap->bssid[4],scan_ap->bssid[5]);
        scan_ap++;
    }    
}


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init()); /* 初始化nvs */
    wifi_scan();
}
