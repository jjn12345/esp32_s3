#include <stdio.h>
#include "esp_err.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#define WIFI_STA_START      BIT0
#define WIFI_SCAN_DONE      BIT1
#define MAX_GET_AP_RECODERS 20

static EventGroupHandle_t   wifi_scan_event_group = NULL;  
static esp_netif_t *netif_handle = NULL;
static esp_event_loop_handle_t  user_event_loop_handle = NULL;


static void wifi_scan_show(void){
    esp_err_t ret = ESP_OK;
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
static  void wifi_scan(void){
    esp_err_t ret = ESP_OK;
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
    ret = esp_wifi_scan_start(&wifi_scan_cfg,false);
    ESP_ERROR_CHECK(ret);
}

static void  run_to_event(void* event_handler_arg,esp_event_base_t event_base,int32_t event_id,void* event_data){
    switch(event_id){
        case WIFI_EVENT_STA_START:{
            xEventGroupSetBits(wifi_scan_event_group,WIFI_STA_START);
        }
        break;
        case WIFI_EVENT_SCAN_DONE:{
            xEventGroupSetBits(wifi_scan_event_group,WIFI_SCAN_DONE);
        }
        break;
    }
}
static void app_task(void * arg){
    esp_err_t ret = ESP_OK;
    EventBits_t wifi_bit = 0;
    /* 创建任务事件标志组 */
    wifi_scan_event_group = xEventGroupCreate();
    assert(wifi_scan_event_group);
    /* 给event_loop 注册一个监听函数 */
    ret = esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,run_to_event,NULL);
    ESP_ERROR_CHECK(ret);
    /* 3.WIFI启动 */
    ret = esp_wifi_start();   /* 启动wifi驱动程序 */
    ESP_ERROR_CHECK(ret);
    while(1){
        wifi_bit = xEventGroupWaitBits(wifi_scan_event_group,WIFI_STA_START|WIFI_SCAN_DONE,true,false,portMAX_DELAY);
        if(wifi_bit & WIFI_STA_START){
            wifi_scan();
        }
        else if(wifi_bit & WIFI_SCAN_DONE){
            wifi_scan_show();
        }
    }

}
 
void app_main(void)
{
    esp_err_t   ret = ESP_OK;
    ESP_ERROR_CHECK(nvs_flash_init()); /* 初始化nvs */
    /* 1.WIFI/LwIP 初始化 */
    /* 创建LwIP核心任务,并初始化LwIP相关工作 */
    ret = esp_netif_init();     
    ESP_ERROR_CHECK(ret);
    /* 创建默认的系统事件循环任务，并初始化应用程序事件的回调 */
    ret = esp_event_loop_create_default();  
    ESP_ERROR_CHECK(ret);
    /* 创建网络接口,该函数需要使用系统的事件循环,如果未创建,会出现错误 */
    netif_handle = esp_netif_create_default_wifi_sta();  
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    /* 创建WIFI驱动程序任务,并初始化WIFI驱动程序 */
    ret = esp_wifi_init(&wifi_init_cfg); 
    ESP_ERROR_CHECK(ret);
    /* 创建AppTask用于完成网络任务 */
    ret = xTaskCreate(app_task,"app_task",1024*4,NULL,2,NULL); 
    if(pdPASS != ret){
        ESP_LOGE("wifi_scan","AppTask created failed");
        vTaskDelete(NULL);
        return;
    }
    /* 2.WIFI配置 */
    ret = esp_wifi_set_mode(WIFI_MODE_STA); /* 设置wifi模式 */
    ESP_ERROR_CHECK(ret);
    vTaskDelete(NULL);
}