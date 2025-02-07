#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp32_excute.h"
#include "demos/lv_demos.h"


void app_main(void)
{
    esp_err_t ret = ESP_OK;
    ret = app_init();
    if(ESP_OK != ret){
        ESP_LOGE("app_main","app init failed");
    }
     lv_demo_benchmark(); 
    // lv_demo_keypad_encoder(); 
    // lv_demo_music(); 
    // lv_demo_stress(); 
    //lv_demo_widgets();

}
