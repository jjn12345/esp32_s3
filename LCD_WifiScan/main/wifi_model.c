#include "wifi_model.h"
#include "string.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

/* wifi task defined */
#define TASK_WIFI_NAME               "wifi"
#define TASK_WIFI_STACK_DEPTH        (6*1024)
#define TASK_WIFI_PRIORITY           (1)
static TaskHandle_t   task_wifi = 0;

static mem_pool_t wifi_pool;
static uint8_t wifi_pool_buffer[MAX_WIFI_POOL_MEM_SIZE];

/* 开始wifi扫描 */
static esp_err_t wifi_scan_start(void){
    static bool is_init = 0;
    esp_err_t ret;
    if(!is_init){
        wifi_country_t wifi_country = {
            .cc = "CN",
            .nchan = 13,
            .schan = 1,
            .policy = WIFI_COUNTRY_POLICY_AUTO,
        };
        ret = esp_wifi_set_country(&wifi_country);
        if(ret != ESP_OK)return ret;
        is_init = 1;
    }
    wifi_scan_config_t wifi_scan_cfg = {
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };
    ret = esp_wifi_scan_start(&wifi_scan_cfg,false);
    return ret;
}

/* wifi连接任务, */

/* wifi扫描任务 */
static void wifi_scan(void*arg){
    EventBits_t wifi_bit = 0;
    wifi_module_t *mod = (wifi_module_t*)arg;
    esp_err_t res = ESP_OK;
    for(;;){
        wifi_scan_start();
        wifi_bit = xEventGroupWaitBits(mod->evt_group,wifi_scan_done,pdTRUE,pdFALSE,pdMS_TO_TICKS(10000));
        ESP_LOGI("WIFI","wifi_bit:%ld",wifi_bit);
        if(wifi_bit == wifi_scan_done){
            esp_wifi_scan_get_ap_num(&mod->res.ap_nums);
            if(mod->res.ap_nums == 0)continue;
            if(mod->res.info){
                my_mem_free(&wifi_pool,mod->res.info);
            }
            mod->res.info = mem_alloc(&wifi_pool,sizeof(wifi_info) * mod->res.ap_nums);
            assert(mod->res.info);
            wifi_ap_record_t* wifi_info_tmp = mem_alloc(&wifi_pool,sizeof(wifi_ap_record_t) * mod->res.ap_nums);
            assert(wifi_info_tmp);
            esp_wifi_scan_get_ap_records(&mod->res.ap_nums,wifi_info_tmp);
            ESP_LOGI("WIFI","AP_NUMS:%d",mod->res.ap_nums);
            for(uint8_t i = 0; i< mod->res.ap_nums;i++){
                strcpy((char*)mod->res.info[i].ssid,(char*)wifi_info_tmp[i].ssid);
                ESP_LOGI("WIFI","SSID:%s",mod->res.info[i].ssid);
                mod->res.info[i].rssi = wifi_info_tmp[i].rssi;
            }  
            event_publish(mod->base.bus,wifi_scan_done,&(mod->res),1000);
            my_mem_free(&wifi_pool,wifi_info_tmp);
        }
    }
}

static void start_wifi(event_t*e,void*arg){
    static bool is_create = 0;
    wifi_mode_t * mod = (wifi_mode_t *)arg;  
    assert(arg);
    event_t* e_tmp = e;
    if(e_tmp->type == 0x100){
        wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&wifi_init_cfg);
        esp_wifi_start();
        if(!is_create){
            is_create = true;
            StaticTask_t* static_task = mem_alloc(&wifi_pool,sizeof(StaticTask_t));
            StackType_t* stack_buffer = mem_alloc(&wifi_pool,sizeof(StackType_t)*4096);
            g_wifi_mod.wifi_scan = xTaskCreateStatic(wifi_scan,"WIFI_SCAN",4096,mod,2,stack_buffer,static_task);
        }
        else{
            vTaskResume(g_wifi_mod.wifi_scan);
        }
        
    }
}
static void  run_to_wifi_event(void* event_handler_arg,
                                esp_event_base_t event_base,
                                int32_t event_id,
                                void* event_data)
{
    wifi_module_t *mod = (wifi_module_t*)event_handler_arg;
    switch(event_id){
        case WIFI_EVENT_SCAN_DONE:{
            ESP_LOGI("WIFI","WIFI_EVENT_SCAN_DONE");
            xEventGroupSetBits(mod->evt_group,wifi_scan_done);
        }
        break;
        case WIFI_EVENT_STA_CONNECTED:{
            ESP_LOGI("WIFI","WIFI_EVENT_STA_CONNECTED");
            xEventGroupSetBits(mod->evt_group,wifi_connected);
        }
        break;
        case WIFI_EVENT_STA_DISCONNECTED:{
            ESP_LOGI("WIFI","WIFI_EVENT_STA_DISCONNECTED");
            xEventGroupSetBits(mod->evt_group,wifi_disconnected);
        }
    }
}
static void my_wifi_close(event_t*e,void*arg){
    wifi_module_t * mod = (wifi_module_t *)arg;
    if(e->type == 0x200){
        vTaskSuspend(mod->wifi_scan);
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_deinit();
    }
}
static void my_wifi_deinit(module_t*self){
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
}
static void wifi_init(module_t*self){
    wifi_module_t* mod = (wifi_module_t*)self;
    if(my_mem_init(&wifi_pool,wifi_pool_buffer,MAX_WIFI_POOL_MEM_SIZE)){
        mod->is_static = 1;
    }
    if(mod->is_static){
        StaticSemaphore_t * StaticMutex = mem_alloc(&wifi_pool,sizeof(StaticSemaphore_t));
        StaticEventGroup_t* StaticEvtGroup = mem_alloc(&wifi_pool,sizeof(StaticEventGroup_t));
        if(StaticMutex == NULL || StaticEvtGroup == NULL){
            mod->is_static = 0;
            if(StaticMutex)my_mem_free(&wifi_pool,StaticMutex);
            if(StaticEvtGroup)my_mem_free(&wifi_pool,StaticEvtGroup);
        }
        else{    
            mod->mutex = xSemaphoreCreateMutexStatic(StaticMutex);
            mod->evt_group = xEventGroupCreateStatic(StaticEvtGroup);
        }
    }
    else{
        mod->mutex = xSemaphoreCreateMutex();
        mod->evt_group = xEventGroupCreate();
        if(mod->mutex == NULL || mod->evt_group == NULL){
            mod->base.state = MODULE_STATE_ERROR;
        }
    }
    esp_netif_t *wifi_netif_handle;
    esp_err_t ret = ESP_OK;
    ret = esp_netif_init();
    if(ret != ESP_OK){
        mod->base.state = MODULE_STATE_ERROR;    
    }
    ret = esp_event_loop_create_default();
    if(ret != ESP_OK){
        mod->base.state = MODULE_STATE_ERROR;    
    }
    wifi_netif_handle = esp_netif_create_default_wifi_sta(); 
    esp_wifi_set_mode(WIFI_MODE_STA);
    /*  注册事件环的回调,用于消息传递  */
    ret = esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,run_to_wifi_event,mod);
    if(ret != ESP_OK){
        mod->base.state = MODULE_STATE_ERROR;    
    }
    /* 以上wifi硬件初始化完毕 */
    /* 开始订阅事件总线中的事件 */
    event_subscribe(mod->base.bus,0x100,start_wifi,mod,0);
    event_subscribe(mod->base.bus,0x200,my_wifi_close,mod,0);
}



wifi_module_t g_wifi_mod = {
    .base = {
        .id = 1,
        .state = MODULE_STATE_ALLOCATED,
        .init = wifi_init,
        .deinit = my_wifi_deinit,
    },
    .is_static = 1,
};
