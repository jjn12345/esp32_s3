#pragma once
#include "model.h"
#include "esp_lvgl_port.h"

typedef enum{
    view_wifi_start = 0x100,
    view_wifi_close = 0x200,
    view_wifi_connect = 0x300,
}ViewEventType_t;

typedef struct{
    char ssid[33];
    char pwd[64];
}wifi_counter;

typedef struct{
    module_t base;
    lv_obj_t* wifi_info;
}view_module_t;

extern view_module_t g_view_module;









