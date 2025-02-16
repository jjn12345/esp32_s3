#include "wifi_model.h"

typedef enum{
    WIFI_START = 0x01,
    WIFI_SCAN_DONE = 0x02,
    WIFI_STA_CONNECTED = 0x04,
    WIFI_STA_DISCONNECTED = 0x08,
    WIFI_EVT_MAX = 0xff, 
}static_wifi_evt_t;
#define WIFI_PROCESS_WAIT_BITS  (WIFI_START | WIFI_STA_CONNECTED | WIFI_STA_DISCONNECTED)
#define LOCK(mutex,time)        xSemaphoreTake(mutex,time>0?pdMS_TO_TICKS(time):portMAX_DELAY)
#define UNLOCK(mutex)           xSemaphoreGive(mutex)



/* wifi启动扫描 */
static void start_wifi_scan(wifi_module_t *self) {
    static bool is_set_country = false;
    if(is_set_country == false){
        esp_wifi_set_country(&self->wifi_cfg.wifi_country_cfg);
        is_set_country = true;
    }    
    wifi_scan_config_t scan_cfg = {0};
    scan_cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    esp_wifi_scan_start(&scan_cfg, false);
}
/* 启动wifi */
static void wifi_start(wifi_module_t *self){
    static bool is_set_mode = false;
    self->is_enable = 1;
    esp_wifi_init(&self->wifi_cfg.wifi_init_cfg);
    if(false == is_set_mode){
        esp_wifi_set_mode(WIFI_MODE_STA);
        is_set_mode = true;
    }
    esp_wifi_start();
}
/* 关闭wifi 挂起扫描任务 */
static void wifi_close(wifi_module_t *self){
    LOCK(self->mutex,0);
    self->is_enable = 0;
    if(self->is_connected){
        esp_wifi_disconnect();
    }
    if(self->scan_task_handle!=NULL)vTaskSuspend(self->scan_task_handle);
    esp_wifi_stop();
    esp_wifi_deinit();
    UNLOCK(self->mutex);
}
/* wifi连接 */
static void wifi_connect(event_t*e,void*arg){
    wifi_module_t* self = (wifi_module_t*)arg;
    LOCK(self->mutex,0);
    self->is_connecting = 1;
    wifi_counter_t *counter = (wifi_counter_t*)e->data;
    memcpy(&self->now_wifi_counter,counter,sizeof(wifi_counter_t)); /* 深拷贝 */
    memset(counter->password,0,sizeof(counter->password));  /* 清除密码 */
    wifi_config_t  wifi_cfg = {0};
    strcpy((char*)wifi_cfg.sta.ssid,(char*)self->now_wifi_counter.selected_ssid);
    strcpy((char*)wifi_cfg.sta.password,(char*)self->now_wifi_counter.password);
    esp_wifi_set_config(WIFI_IF_STA,&wifi_cfg);
    esp_wifi_connect();
    UNLOCK(self->mutex);
}
/* wifi控制 */
static void wifi_ctrl(event_t*e,void*arg){
    wifi_module_t* self = (wifi_module_t*)arg;
    switch(e->type){
        case UI_ENABLE_WIFI:{
            wifi_start(self);
        }
        break;
        case UI_CLOSE_WIFI:{
            wifi_close(self);
        }
        break;
    }
}
/* 获取wifi事件 */
static void run_to_wifi_evt(void* event_handler_arg,
                            esp_event_base_t event_base,
                            int32_t event_id,
                            void* event_data)
{
    if(event_base != WIFI_EVENT)return;
    wifi_module_t *mod = (wifi_module_t*)event_handler_arg;
    switch(event_id){
        case WIFI_EVENT_SCAN_DONE:{
            ESP_LOGI("wifi","WIFI_SCAN_DONE");
            xEventGroupSetBits(mod->evt_group,WIFI_SCAN_DONE);
        }
        break;
        case WIFI_EVENT_STA_START:{
            ESP_LOGI("wifi","WIFI_START");
            xEventGroupSetBits(mod->evt_group,WIFI_START); 
        }
        break;
        case WIFI_EVENT_STA_DISCONNECTED:{
            ESP_LOGI("wifi","WIFI_STA_DISCONNECTED");
            xEventGroupSetBits(mod->evt_group,WIFI_STA_DISCONNECTED); 
        }
        break;
        case WIFI_EVENT_STA_CONNECTED:{
            ESP_LOGI("wifi","WIFI_STA_CONNECTED");
            xEventGroupSetBits(mod->evt_group,WIFI_STA_CONNECTED); 
        }
        break;
    }
}     

/* wifi扫描任务 */
static void wifi_scan_task(void*arg){
    wifi_module_t* self = (wifi_module_t*)arg;
    EventBits_t bit = 0;
    static scan_info_t* scan_info;
    scan_info = malloc(sizeof(scan_info_t));
    scan_info->ap_nums = 0;
    scan_info->info = NULL;
    while(1){  
        ESP_LOGI("wifi","wifi_scan_task is running");
        LOCK(self->mutex,0);
        start_wifi_scan(self);
        bit = xEventGroupWaitBits(self->evt_group,WIFI_SCAN_DONE,true,false,pdMS_TO_TICKS(3000));
        if(bit == WIFI_SCAN_DONE){
            /* 清空数据 */
            if(scan_info->info != NULL){
                free(scan_info->info);
                scan_info->info = NULL;
            }
            uint16_t ap_nums = 0;
            wifi_ap_record_t * ap_records;
            /* 获取数据 */
            esp_wifi_scan_get_ap_num(&ap_nums);
            ap_records = malloc(ap_nums*sizeof(wifi_ap_record_t));
            esp_wifi_scan_get_ap_records(&ap_nums,ap_records);
            /* 将数据复制到scan_info */
            scan_info->ap_nums = ap_nums;
            scan_info->info = malloc(ap_nums*sizeof(wifi_info_t));
            for(uint16_t i = 0;i<ap_nums;i++){
                strncpy((char*)scan_info->info[i].ssid,(char*)ap_records[i].ssid,sizeof(scan_info->info[i].ssid)-1);
                scan_info->info[i].ssid[sizeof(scan_info->info[i].ssid)-1] = '\0';
                scan_info->info[i].rssi = ap_records[i].rssi;
            }
            if(ap_nums == 0)continue;
            /* 把数据发布出去 */
            event_publish(self->base.bus,WIFI_SCAN_RESULT,scan_info,1000);
            /* 清除临时数据 */
            if(ap_records)free(ap_records);    
        }
        UNLOCK(self->mutex);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* wifi重连任务 */
static void wifi_reconnect_task(void*arg){
    uint32_t max_reconnect_nums = 3;            /* 最大重连次数 */
    wifi_module_t* self = (wifi_module_t*)arg;
    bool enable = 0;
    bool is_reconnect = 0;
    while(1){
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI("wifi","wifi_reconnect_task is running");
        max_reconnect_nums--;
        LOCK(self->mutex,0);
        enable = self->is_reconnect;
        is_reconnect = self->is_reconnect;
        UNLOCK(self->mutex);
        if(!enable || !max_reconnect_nums||!is_reconnect){
            if(enable){
                vTaskResume(self->scan_task_handle);
            }
            vTaskDelete(NULL);
            return;
        }
        esp_wifi_connect();
        self->is_connecting = 1;
    }
}   
/* wifi处理任务 */
static void wifi_process(void*arg){
    EventBits_t bits = 0;
    wifi_module_t *mod = (wifi_module_t*)arg;
    for(;;){
        bits =  xEventGroupWaitBits(mod->evt_group,WIFI_PROCESS_WAIT_BITS,true,false,portMAX_DELAY);
        switch(bits & WIFI_EVT_MAX){
            case WIFI_START:{
                ESP_LOGI("wifi_process","WIFI_START");
                if(mod->scan_task_handle){
                    vTaskResume(mod->scan_task_handle);
                }
                else{
                    xTaskCreate(wifi_scan_task,"scan_task",6144,mod,3,&mod->scan_task_handle);
                }
            }
            break;
            case WIFI_STA_CONNECTED:{
                ESP_LOGI("wifi_process","WIFI_STA_CONNECTED");
                mod->is_connected = 1;
                mod->is_connecting = 0;
                if(mod->is_reconnect)mod->is_reconnect = 0;
                event_publish(mod->base.bus,WIFI_CONNECT_SUCCESS,&mod->now_wifi_counter,1000);
                ESP_LOGI("wifi_process","WIFI_STA_CONNECTED finish");
            }
            break;
            case WIFI_STA_DISCONNECTED:{
                ESP_LOGI("wifi_process","WIFI_STA_DISCONNECTED");
                if(mod->is_connecting){
                    if(mod->is_reconnect){

                    }
                    else{
                        event_publish(mod->base.bus,WIFI_CONNECT_FAIL,NULL,1000);
                    }
                    mod->is_connecting = 0;
                }
                else{
                    if(mod->is_enable){
                        LOCK(mod->mutex,0);
                        vTaskSuspend(mod->scan_task_handle);
                        xTaskCreate(wifi_reconnect_task,"wifi reconnect",6144,mod,6,&mod->reconnect_task_handle);
                        mod->is_reconnect = 1;  /* 重连标志置一 */
                        event_publish(mod->base.bus,WIFI_LOST_CONNECT,NULL,1000);
                        UNLOCK(mod->mutex);
                    }
                    else{
                        mod->is_reconnect = 0;
                    }
                }
            }
            break;
        }
    }
}


static void wifi_init(module_t *self){
    wifi_module_t *mod = (wifi_module_t*)self;
    esp_err_t ret = ESP_OK;
    /* 创建互斥信号量 */
    mod->mutex = xSemaphoreCreateMutex();
    if(NULL == mod->mutex){
        ESP_LOGE("wifi","1");
        self->state = MODULE_STATE_ERROR;
        return;
    }
    /* 创建事件标志组 */
    mod->evt_group = xEventGroupCreate();
    if(NULL == mod->evt_group){
        ESP_LOGE("wifi","2");
        vSemaphoreDelete(mod->mutex);
        self->state = MODULE_STATE_ERROR;
        return;
    }
    /* 配置为站点模式 */
    /* 初始化lwif */
    ret = esp_netif_init();
    if(ESP_OK != ret){
        ESP_LOGE("wifi","3");
        vSemaphoreDelete(mod->mutex);
        vEventGroupDelete(mod->evt_group);
        self->state = MODULE_STATE_ERROR;
        return;
    }
    /* 创建默认事件循环 */
    ret = esp_event_loop_create_default();
    if(ESP_OK != ret){
        ESP_LOGE("wifi","4");
        vSemaphoreDelete(mod->mutex);
        vEventGroupDelete(mod->evt_group);
        esp_netif_deinit();
        self->state = MODULE_STATE_ERROR;
        return;
    }    
    /*  向事件循环注册对应的wifi执行函数 */
    esp_netif_create_default_wifi_sta();
    /* 向事件循环注册当前任务，进行消息传递 */
    esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,run_to_wifi_evt,mod);

    /* 初始化wifi的各项配置 */
    mod->wifi_cfg.wifi_init_cfg = (wifi_init_config_t)WIFI_INIT_CONFIG_DEFAULT();
    /* 配置wifi城市配置 */
    mod->wifi_cfg.wifi_country_cfg = (wifi_country_t){
        .cc = "CN",
        .nchan = 13,
        .schan = 1,
        .policy = WIFI_COUNTRY_POLICY_AUTO,
    };
    /* 订阅事件 */
    ret = event_subscribe(self->bus,UI_ENABLE_WIFI|UI_CLOSE_WIFI,wifi_ctrl,mod,1000);
    if(ret == false){
        ESP_LOGE("wifi","5");
        goto err;
    }
    ret = event_subscribe(self->bus,UI_CONNECT_WIFI,wifi_connect,mod,1000);
    if(ret == false){
        ESP_LOGE("wifi","6");
        goto err;
    }
    /* 创建wifi处理函数 */
    ret = xTaskCreate(wifi_process,"wifi process",6144,mod,5,&mod->process_task_handle);
    if(pdPASS!=ret){
        ESP_LOGE("wifi","7");
        goto err;
    }
    return;
err:
    vSemaphoreDelete(mod->mutex);
    vEventGroupDelete(mod->evt_group);
    esp_wifi_deinit();
    esp_netif_deinit();
    esp_event_loop_delete_default();
    vTaskDelete(mod->process_task_handle);
    self->state = MODULE_STATE_ERROR;    
    return;
}

/* 创建wifi模块 */
wifi_module_t wifi_module = {
    .base = {
        .id = 0x02,
        .state = MODULE_STATE_ALLOCATED,           
        .init = wifi_init,
    },
    .is_connected = 0,
    .is_enable = 0,
};





