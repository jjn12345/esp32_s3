#pragma once



/* 该文件用于定义全局事件代码 */
/* WIFI事件 */
typedef enum{
    WIFI_SCAN_RESULT=0x01,
    WIFI_CONNECT_FAIL=0x02,
    WIFI_CONNECT_SUCCESS=0x04,
    WIFI_LOST_CONNECT=0x08,
}wifi_evt_t;
/* UI事件 */
typedef enum{
    UI_ENABLE_WIFI = 0x0100,
    UI_CLOSE_WIFI  = 0x0200,
    UI_CONNECT_WIFI= 0x0400,
}ui_evt_t;


















