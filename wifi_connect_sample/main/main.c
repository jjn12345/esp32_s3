#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define WIFI_SCAN_DONE          BIT0
#define WIFI_STA_CONNECTED      BIT1
#define WIFI_STA_DISCONNECTED   BIT2
#define WIFI_START              BIT3
#define WIFI_DEFAULT_AP         BIT4
#define WIFI_EVT_BIT            (WIFI_SCAN_DONE | WIFI_STA_CONNECTED | WIFI_STA_DISCONNECTED | WIFI_DEFAULT_AP | WIFI_START)

static char Tag[] = "wifi_connect";
static EventGroupHandle_t wifi_evt_group = NULL;
static esp_netif_t* esp_netif_handle = NULL;




static void  run_to_wifi_event( void* event_handler_arg,esp_event_base_t event_base,
                                int32_t event_id,void* event_data)
{   
    if(event_base != WIFI_EVENT){
        return;
    }
    switch(event_id){
        case WIFI_EVENT_STA_START:{
            xEventGroupSetBits(wifi_evt_group,WIFI_START);
        }
        break;
        case WIFI_EVENT_SCAN_DONE:{
            xEventGroupSetBits(wifi_evt_group,WIFI_SCAN_DONE);
        }
        break;
        case WIFI_EVENT_STA_CONNECTED:{
            xEventGroupSetBits(wifi_evt_group,WIFI_STA_CONNECTED);
        }
        break;
        case WIFI_EVENT_STA_DISCONNECTED:{
            xEventGroupSetBits(wifi_evt_group,WIFI_STA_DISCONNECTED);
        }
        break;
    }
}
static void start_wifi_scan() {
    wifi_scan_config_t scan_cfg = {0};
    scan_cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_cfg, false));
}
typedef struct{
    uint8_t ssid[32];                        
    uint8_t password[64];                   
}wifi_info_t;

wifi_info_t save_wifi = {
    "Xiaomi 12",
    "123456789"
};
static void task_process_wifi(void * arg){
    EventBits_t wifi_flag = 0;
    uint16_t ap_nums = 0;
    wifi_ap_record_t *scan_ap = NULL;
    wifi_info_t *wifi_default_ap = &save_wifi;
    wifi_info_t *get_ap = NULL;
    wifi_evt_group = xEventGroupCreate();
    esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,run_to_wifi_event,NULL);

    for(;;){
        wifi_flag = xEventGroupWaitBits(wifi_evt_group,WIFI_EVT_BIT,true,false,portMAX_DELAY);
        switch(wifi_flag & WIFI_EVT_BIT){
            case WIFI_SCAN_DONE:{
                /* 释放之前获取的ap信息 */
                if(get_ap){
                    free(get_ap);
                    get_ap = NULL;
                }
                /* 获取扫描到的ap数量 */
                esp_wifi_scan_get_ap_num(&ap_nums);
                /* 根据获取到的ap数量动态分配scan_ap所需内存 */
                scan_ap = malloc(sizeof(wifi_ap_record_t) * ap_nums);
                assert(scan_ap);
                get_ap = malloc(sizeof(wifi_info_t) * ap_nums);
                assert(get_ap);
                /* 获取记录 */
                esp_wifi_scan_get_ap_records(&ap_nums,scan_ap);
                /* 判断是否存在默认AP,并获取扫描到的AP的SSID */
                bool default_found = false;
                for(int i = 0;i < ap_nums;i++){
                    ESP_LOGI(Tag,"%d:  SSID:%s",i,scan_ap[i].ssid);
                    if(wifi_default_ap){
                        if(strncmp((char*)scan_ap[i].ssid,(char*)wifi_default_ap->ssid,sizeof(wifi_default_ap->ssid)) == 0){
                            default_found = true;
                        }
                    }
                    memcpy(get_ap[i].ssid,scan_ap->ssid,strlen((char*)scan_ap->ssid)+1);
                }
                /* 释放内存并触发事件 */
                free(scan_ap);  
                scan_ap = NULL; 
                if(default_found){
                    xEventGroupSetBits(wifi_evt_group, WIFI_DEFAULT_AP);
                }
            }
            break;
            case WIFI_STA_CONNECTED:{
                wifi_ap_record_t ap_info;
                esp_wifi_sta_get_ap_info(&ap_info);
                ESP_LOGI(Tag,"AP_CONNECT_OK,SSID:%s",ap_info.ssid);
            }
            break;
            case WIFI_STA_DISCONNECTED:{
                static int retry_count = 0;
                const int max_retry = 5;
                if (retry_count < max_retry) {
                    vTaskDelay(pdMS_TO_TICKS(1000 * (1 << retry_count))); // 1s, 2s, 4s...
                    ESP_LOGI(Tag, "Retry connect (%d/%d)", retry_count+1, max_retry);
                    ESP_ERROR_CHECK(esp_wifi_connect());
                    retry_count++;
                }else{
                    ESP_LOGW(Tag, "Max retries reached");
                    retry_count = 0;
                }
            }
            break;
            case WIFI_DEFAULT_AP:{
                wifi_config_t wifi_cfg = {
                    .sta.scan_method = WIFI_ALL_CHANNEL_SCAN,
                    .sta.failure_retry_cnt = 0,
                    .sta.threshold.authmode = WIFI_AUTH_WPA2_PSK,
                };
                strncpy((char*)wifi_cfg.sta.ssid, (char*)wifi_default_ap->ssid, sizeof(wifi_cfg.sta.ssid) - 1);
                strncpy((char*)wifi_cfg.sta.password, (char*)wifi_default_ap->password, sizeof(wifi_cfg.sta.password) - 1);
                wifi_cfg.sta.ssid[sizeof(wifi_cfg.sta.ssid) - 1] = '\0'; // 确保终止符
                wifi_cfg.sta.password[sizeof(wifi_cfg.sta.password) - 1] = '\0'; // 确保终止符
                ESP_LOGI(Tag,"SSID:%s PWD:%s",wifi_cfg.sta.ssid,wifi_cfg.sta.password);
                esp_wifi_set_config(WIFI_IF_STA,&wifi_cfg);
                ESP_ERROR_CHECK(esp_wifi_connect());
            }
            break;
            case WIFI_START:{
                /* 启动wifi扫描 */
                start_wifi_scan();
            }
            break;
        }
    }
}




void app_main(void)
{
    esp_err_t res = ESP_OK;
    /* 初始化nvs */
    res = nvs_flash_init();
    if(res != ESP_OK){
        ESP_LOGE(Tag,"NVS:ERROR %s",esp_err_to_name(res));
        nvs_flash_erase();
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    /* 初始化WIFI */
    res = esp_netif_init();
    ESP_ERROR_CHECK(res);
    res = esp_event_loop_create_default();
    ESP_ERROR_CHECK(res);
    esp_netif_handle = esp_netif_create_default_wifi_sta();
    assert(esp_netif_handle);
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    res = esp_wifi_init(&wifi_init_cfg);
    ESP_ERROR_CHECK(res);
    /* 创建AppTask */
    res = xTaskCreate(task_process_wifi,"wifi_process",1024*4,NULL,4,NULL);
    if(pdPASS != res){
        esp_wifi_deinit();
        vTaskDelete(NULL);
        return;
    }    
    /* 配置WIFI */
    res = esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_ERROR_CHECK(res);
    wifi_country_t wifi_country_cfg = {
        .cc = "CN",
        .nchan = 13,
        .schan = 1,
        .policy = WIFI_COUNTRY_POLICY_AUTO,
    };
    res = esp_wifi_set_country(&wifi_country_cfg);
    ESP_ERROR_CHECK(res);

    /* 启动WIFI驱动任务 */
    res = esp_wifi_start();
    ESP_ERROR_CHECK(res);

    vTaskDelete(NULL);
}
