#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"

#define TASK_NAME           "HELLO_WORLD"
#define TASK_PRIORITY       1
#define TASK_STACK_DEPTH    2048
TaskHandle_t   task1;
static void hello_world(void* parameters){
    uint8_t nums = 0;
    for(;;){
        printf("hello_world_nums = %d\r\n",++nums);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if(nums == 10){
            printf("Restarting now.\n");
            fflush(stdout);
            esp_restart();
        }
    }
}


void app_main(void)
{
    printf("app is running\r\n");
    esp_chip_info_t chip_info ={0}; 
    uint32_t flash_size;
    char esp_type[255];
    esp_chip_info(&chip_info);
    switch(chip_info.model){
       case  CHIP_ESP32  : {
            sprintf(esp_type,"esp32");
        }
        break;
       case  CHIP_ESP32S2: {
            sprintf(esp_type,"esp32s2");
        }//!< ESP32-S2
        break;
       case  CHIP_ESP32S3: {
            sprintf(esp_type,"esp32s3");
        }//!< ESP32-S3
        break;
       case  CHIP_ESP32C3: {
            sprintf(esp_type,"esp32c3");
        }//!< ESP32-C3
        break;
       case  CHIP_ESP32C2: {
            sprintf(esp_type,"esp32c2");
        } //!< ESP32-C2
        break;
       case  CHIP_ESP32C6: {
            sprintf(esp_type,"esp32c6");
        } //!< ESP32-C6
        break;
       case  CHIP_ESP32H2: {
            sprintf(esp_type,"esp32h2");
        } //!< ESP32-H2
        break;
       case  CHIP_ESP32P4: {
            sprintf(esp_type,"esp32p4");
        } //!< ESP32-P4
        break;
       case  CHIP_POSIX_LINUX:{
            sprintf(esp_type,"linux simulator");
       } //!< The code is running on POSIX/Linux simulator
       break;
    }
    printf("This is %s Chip with %d core(s),%s%s%s%s",
        esp_type,
        chip_info.cores,
        (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
        (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
        (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
        (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");
    if(esp_flash_get_size(NULL,&flash_size) != ESP_OK){
        printf("Get flash size failed");
        return;
    }
    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
           
    fflush(stdout);
    xTaskCreate((TaskFunction_t) hello_world,TASK_NAME,TASK_STACK_DEPTH,NULL,TASK_PRIORITY,&task1);
    vTaskDelete(NULL);
}
