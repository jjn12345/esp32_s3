#pragma once
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"


esp_err_t app_init(void);
void my_wifi_init(void);
void my_wifi_deinit(void);
void lvgl_start(void);