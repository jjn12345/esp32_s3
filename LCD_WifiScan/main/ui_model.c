#include "ui_model.h"
#include "view_home.h"
#include "view_wifi.h"
#include "view_wifi_connect.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"



void ui_model_init(module_t * self){
    ui_register_screen(UI_SCREEN_HOME,view_home_create,view_home_show,view_home_hide);
    ui_register_screen(UI_SCREEN_WIFI,view_wifi_create,view_wifi_show,view_wifi_hide);
    ui_register_screen(UI_SCREEN_WIFI_CONNECT,view_wifi_connect_create,view_wifi_connect_show,view_wifi_connect_hide);
    ui_init(self);
}   

ui_module_t ui_model = {
    .base = {
        .id = 0x01,
        .init = ui_model_init,
        .state = MODULE_STATE_ALLOCATED,
    },
    .ui_data = &g_ui_data,
};

