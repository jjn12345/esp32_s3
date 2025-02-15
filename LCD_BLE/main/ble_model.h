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
    module_t base;      /* 必须是第一个      */
}ble_module_t;


extern ble_module_t ble_module;



