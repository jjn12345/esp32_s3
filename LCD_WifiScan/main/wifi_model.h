#pragma once
#include "model.h"
#include "esp_wifi.h"

#define MAX_WIFI_POOL_MEM_SIZE          8192


typedef enum{
    wifi_start = 0x01,
    wifi_scan_done = 0x02,
    wifi_connect = 0x04,
    wifi_disconnected = 0x08,
    wifi_connected = 0x10,
    wifi_event_max = 0xff,
}WifiEventType_t;

typedef struct{
    char ssid[33];
    int8_t  rssi;
}wifi_info;
typedef struct{
    wifi_info* info;
    uint16_t  ap_nums;
}scan_res;
typedef struct{
    module_t base;
    scan_res res;
    SemaphoreHandle_t mutex;
    EventGroupHandle_t evt_group;
    TaskHandle_t wifi_scan;
    bool is_static;
}wifi_module_t;


extern wifi_module_t g_wifi_mod;



