#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "wifi_model.h"
#include "view.h"
#include "model.h"
#include "demos/lv_demos.h"
#include "string.h"
#include "esp32_excute.h"
#include <esp_system.h>
// 打印内存使用情况
void displayMemoryUsage() {
    size_t totalDRAM = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t freeDRAM = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t usedDRAM = totalDRAM - freeDRAM;

    size_t totalPSRAM = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t freePSRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t usedPSRAM = totalPSRAM - freePSRAM;

    size_t totalDMA = heap_caps_get_total_size(MALLOC_CAP_DMA);
    size_t freeDMA = heap_caps_get_free_size(MALLOC_CAP_DMA);
    size_t usedDMA = totalDMA - freeDMA;

    size_t DRAM_largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    size_t PSRAM_largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    size_t DMA_largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);

    float dramUsagePercentage = (float)usedDRAM / totalDRAM * 100;
    float psramUsagePercentage = (float)usedPSRAM / totalPSRAM * 100;
    float dmaUsagePercentage = (float)usedDMA / totalDMA * 100;

    ESP_LOGI("app_main", "DRAM Total: %zu bytes, Used: %zu bytes, Free: %zu bytes,  DRAM_Largest_block: %zu bytes", totalDRAM, usedDRAM, freeDRAM, DRAM_largest_block);
    ESP_LOGI("app_main", "DRAM Used: %.2f%%", dramUsagePercentage);
    ESP_LOGI("app_main", "PSRAM Total: %zu bytes, Used: %zu bytes, Free: %zu bytes, PSRAM_Largest_block: %zu bytes", totalPSRAM, usedPSRAM, freePSRAM, PSRAM_largest_block);
    ESP_LOGI("app_main", "PSRAM Used: %.2f%%", psramUsagePercentage);
    ESP_LOGI("app_main", "DMA Total: %zu bytes, Used: %zu bytes, Free: %zu bytes, DMA_Largest_block: %zu bytes", totalDMA, usedDMA, freeDMA, DMA_largest_block);
    ESP_LOGI("app_main", "DMA Used: %.2f%%", dmaUsagePercentage);    
} 
static void sys_monitor(void*arg){
    while(1){
        displayMemoryUsage();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static module_mgr_t g_mod_mgr;
static event_bus_t  sys_bus;
static mem_pool_t   sys_bus_pool;
static uint8_t sys_bus_buffer[4096];
void app_main(void)
{
    app_init();
    my_mem_init(&sys_bus_pool,sys_bus_buffer,4096);     /* 内存池初始化 */
    event_bus_init(&sys_bus,&sys_bus_pool,10);          /* 系统总线初始化 */
    module_mgr_init(&g_mod_mgr);                        /* 初始化模组管理器 */
    /* 模块绑定总线 */
    g_wifi_mod.base.bus = &sys_bus;
    g_view_module.base.bus = &sys_bus;
    module_register(&g_mod_mgr,&g_wifi_mod,1000);       /* 注册wifi模组 */
    module_register(&g_mod_mgr,&g_view_module,1000);    /* 注册ui模组 */
    module_init_all(&g_mod_mgr,1000);                   /* 初始化全部模组 */
    xTaskCreate(sys_monitor,"monitor",4096,NULL,3,NULL);
    // event_publish(&sys_bus,0x01,NULL,1000);          /* 模拟发布wifi_start事件 */
    /* lvgl start */
    
    // lvgl_ui();
    for(;;){
        event_process(&sys_bus);
    }
}
