#pragma once
#include "model.h"
#include "esp_wifi.h"
#include "app_evt_def.h"
#include "string.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

typedef struct{
    char selected_ssid[32];  // 当前选中的AP名称
    char password[64];       // 输入的密码
}wifi_counter;
typedef struct{
    wifi_init_config_t  wifi_init_cfg;
    wifi_country_t wifi_country_cfg;
}wifi_cfg_t;
typedef struct{
    module_t base;      /* 必须是第一个      */
    bool is_enable;     /* 是否启动wifi     */
    bool is_connected;  /* 是否连接         */
    TaskHandle_t scan_task_handle;      /* 扫描任务句柄 */
    TaskHandle_t process_task_handle;   /* 总处理任务句柄 */
    SemaphoreHandle_t   mutex;            /* 互斥信号量 */
    EventGroupHandle_t  evt_group;        /* 事件标志组 */
    wifi_cfg_t          wifi_cfg;       /* wifi配置 */
}wifi_module_t;


extern wifi_module_t wifi_module;