#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp32_excute.h"
#include "esp32_ui.h"
#include "demos/lv_demos.h"
#include "string.h"


void app_main(void)
{
    esp_err_t ret = ESP_OK;
    ret = app_init();
    if(ESP_OK != ret){
        ESP_LOGE("app_main","app init failed");
    }
    /* lvgl start */
    lvgl_start();
    lvgl_ui();
    vTaskDelete(NULL);
}
