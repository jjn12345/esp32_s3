#include <stdio.h>
#include "string.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "MyPCA9557.h"
#include "lcd.h"
#include "llx.h"
#include "yingwu.h"
#define PCA_GPIO_NUM        (pca9557_gpio_num_0|pca9557_gpio_num_1|pca9557_gpio_num_2)

pca9557_handle_t pca = NULL;
static esp_err_t lcd_ctrl_cs(const uint8_t level){
    return pca9557_gpio_write(pca,pca9557_gpio_num_0,level);
}
void app_main(void)
{
    pca = pca9557_create(I2C_NUM_0,pca9557_addr1);
    pca9557_init(pca,0x05);
    pca9557_gpio_config(pca,PCA_GPIO_NUM,pca_output);
    lcd_init(lcd_ctrl_cs);
    lcd_draw_pic(0,0,LCD_W,LCD_H,(uint16_t*)llx);
    while(1){

    }

}
