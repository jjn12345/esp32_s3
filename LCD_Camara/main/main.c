#include <stdio.h>
#include "esp_err.h"
#include "esp_check.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "lcd.h"
#include "pca9557.h"
#include "esp_camera.h"
#include "llx.h"
static pca9557_handle_t pca;

static char Tag[]   = "lcd_camera";

/* I2C     */
#define I2C_PORT            I2C_NUM_0
#define I2C_SCL             GPIO_NUM_2
#define I2C_SDA             GPIO_NUM_1
#define I2C_CLK_FREQ_HZ     (200 * 1000)
static esp_err_t bsp_i2c_init(void){
    esp_err_t ret = ESP_OK;
    i2c_config_t    i2c_cfg = {
        .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL,
        .master.clk_speed = I2C_CLK_FREQ_HZ,
        .mode = I2C_MODE_MASTER,
        .scl_io_num = I2C_SCL,
        .scl_pullup_en = 1,
        .sda_pullup_en = 1,
        .sda_io_num = I2C_SDA,
    };
    ret = i2c_param_config(I2C_PORT,&i2c_cfg);
    ESP_RETURN_ON_ERROR(ret,"i2c","i2c param config failed");
    ret = i2c_driver_install(I2C_PORT,i2c_cfg.mode,0,0,0);
    ESP_RETURN_ON_ERROR(ret,"i2c","i2c driver install failed");
    return ESP_OK;
}

/* lcd  */
#define I2C_PORT    I2C_NUM_0
#define LCD_CS      pca9557_gpio_num_0
static esp_err_t cs_ctrl(const uint8_t level){
    return pca9557_gpio_write(pca,LCD_CS,level);
}

/* 摄像头   */
#define CAMERA_PWDN                 pca9557_gpio_num_2
#define CAMERA_TRANS_FREQ_HZ        (24 * 1000 * 1000)
#define CAMERA_LEDT_TIMER_NUM       LEDC_TIMER_0
#define CAMERA_LEDC_CHANNEL         LEDC_CHANNEL_1
#define CAMERA_FRAME_BUFFER_COUNT   (2)
#define CAMERA_FRAME_BUFFER_SIZE    FRAMESIZE_QVGA
#define CAMERA_PIXEL_FORMAT         PIXFORMAT_RGB565
/* DVP */
#define CAMERA_BSP_DVP_D0           GPIO_NUM_16
#define CAMERA_BSP_DVP_D1           GPIO_NUM_18
#define CAMERA_BSP_DVP_D2           GPIO_NUM_8
#define CAMERA_BSP_DVP_D3           GPIO_NUM_17
#define CAMERA_BSP_DVP_D4           GPIO_NUM_15
#define CAMERA_BSP_DVP_D5           GPIO_NUM_6
#define CAMERA_BSP_DVP_D6           GPIO_NUM_4
#define CAMERA_BSP_DVP_D7           GPIO_NUM_9
#define CAMERA_BSP_DVP_PCLK         GPIO_NUM_7
#define CAMERA_BSP_DVP_XCLK         GPIO_NUM_5
#define CAMERA_BSP_DVP_HSYNC        GPIO_NUM_46
#define CAMERA_BSP_DVP_VSYNC        GPIO_NUM_3
/* SCCB */
#define CAMERA_BSP_SCCB_SCL         I2C_SCL
#define CAMERA_BSP_SCCB_SDA         I2C_SDA
#define CAMERA_BSP_SCCB_PORT        I2C_PORT

#define CAMERA_BSP_PWND             GPIO_NUM_NC
#define CAMERA_BSP_RESET            GPIO_NUM_NC

static esp_err_t bsp_camera_init(void){
    esp_err_t ret = ESP_OK;
    camera_config_t camera_example_config = {
            .pin_pwdn       = CAMERA_BSP_PWND,
            .pin_reset      = CAMERA_BSP_RESET,
            .pin_xclk       = CAMERA_BSP_DVP_XCLK,
            .pin_sccb_sda   = CAMERA_BSP_SCCB_SDA,
            .pin_sccb_scl   = CAMERA_BSP_SCCB_SCL,
            .pin_d7         = CAMERA_BSP_DVP_D7,
            .pin_d6         = CAMERA_BSP_DVP_D6,
            .pin_d5         = CAMERA_BSP_DVP_D5,
            .pin_d4         = CAMERA_BSP_DVP_D4,
            .pin_d3         = CAMERA_BSP_DVP_D3,
            .pin_d2         = CAMERA_BSP_DVP_D2,
            .pin_d1         = CAMERA_BSP_DVP_D1,
            .pin_d0         = CAMERA_BSP_DVP_D0,
            .pin_vsync      = CAMERA_BSP_DVP_VSYNC,
            .pin_href       = CAMERA_BSP_DVP_HSYNC,
            .pin_pclk       = CAMERA_BSP_DVP_PCLK,
            .xclk_freq_hz   = CAMERA_TRANS_FREQ_HZ,
            .ledc_timer     = CAMERA_LEDT_TIMER_NUM,
            .ledc_channel   = CAMERA_LEDC_CHANNEL,
            .pixel_format   = CAMERA_PIXEL_FORMAT,
            .frame_size     = CAMERA_FRAME_BUFFER_SIZE,
            .jpeg_quality   = 10,
            .fb_count       = CAMERA_FRAME_BUFFER_COUNT,
            .grab_mode      = CAMERA_GRAB_WHEN_EMPTY,
            .fb_location    = CAMERA_FB_IN_PSRAM,
    };
    ret = esp_camera_init(&camera_example_config);
    ESP_RETURN_ON_ERROR(ret,Tag,"camera init failed");
    return ESP_OK;
}

/* App Task defined */
#define TASK_START_NAME             "start"
#define TASK_START_STACK_DEPTH      (4*1024)
#define TASK_START_PRIORITY         (10)
static TaskHandle_t    task_start = 0;
static void task_process_start(void * arg);

#define TASK_LCD_NAME               "lcd"
#define TASK_LCD_STACK_DEPTH        (4*1024)
#define TASK_LCD_PRIORITY           (5)
static TaskHandle_t    task_lcd = 0;
static void task_process_lcd(void * arg);

#define TASK_CAMERA_NAME               "camera"
#define TASK_CAMERA_STACK_DEPTH        (4*1024)
#define TASK_CAMERA_PRIORITY           (6)
static TaskHandle_t   task_camera = 0;
static void task_process_camera(void * arg);

/* app task  */
static QueueHandle_t xQueueLcdFrame = NULL;
#define LCD_FRAME_LEN   CAMERA_FRAME_BUFFER_COUNT
static void task_process_start(void * arg){
    BaseType_t  ret = pdPASS;
    /* app task init */
    xQueueLcdFrame = xQueueCreate(LCD_FRAME_LEN,sizeof(camera_fb_t));
    if(NULL == xQueueLcdFrame){
        ESP_LOGE(Tag,"app queue created failed");
    }
    ret = xTaskCreatePinnedToCore(task_process_lcd,TASK_LCD_NAME,TASK_LCD_STACK_DEPTH,NULL,TASK_LCD_PRIORITY,&task_lcd,0);
    if(pdPASS != ret){
        ESP_LOGE(Tag,"task lcd created failed");
        vTaskDelete(NULL);
        return;
    }
    ret = xTaskCreatePinnedToCore(task_process_camera,TASK_CAMERA_NAME,TASK_CAMERA_STACK_DEPTH,NULL,TASK_CAMERA_PRIORITY,&task_camera,1);
    if(pdPASS != ret){
        ESP_LOGE(Tag,"task camera created failed");
        vTaskDelete(NULL);
        return;
    }
    vTaskDelete(NULL);
}
static void task_process_camera(void * arg){
    camera_fb_t * frame = NULL;
    for(;;){

        frame = esp_camera_fb_get();
        if (!frame) {
            ESP_LOGE(Tag, "Frame buffer could not be acquired");
            return;
        }
        xQueueSend(xQueueLcdFrame,&frame,portMAX_DELAY);
    }
}
static void task_process_lcd(void * arg){
    camera_fb_t * frame = NULL;
    for(;;){
        if(xQueueReceive(xQueueLcdFrame,&frame,portMAX_DELAY)){
            lcd_draw(0,0,frame->width,frame->height,(uint16_t*)frame->buf);
            esp_camera_fb_return(frame);
        }
    }
}

static esp_err_t app_init(void){
    esp_err_t   ret = ESP_OK; 
    /* i2c init */
    ret = bsp_i2c_init();
    ESP_RETURN_ON_ERROR(ret,Tag,"bsp i2c init failed");
    /* pca9557 init */
    pca = pca9557_create(I2C_PORT,pca9557_addr1);
    ret = pca9557_init(pca,0x05);
    ret = pca9557_gpio_config(pca,LCD_CS|CAMERA_PWDN,pca_output);
    ESP_RETURN_ON_ERROR(ret,Tag,"bsp pca init failed");
    
    /* lcd init */
    ret = lcd_init(cs_ctrl);
    lcd_draw(0,0,LCD_W,LCD_H,(uint16_t*)llx);
    ESP_RETURN_ON_ERROR(ret,Tag,"bsp lcd init failed");
    /* camera init */
    pca9557_gpio_write(pca,CAMERA_PWDN,pca_gpio_low);
    ret = bsp_camera_init();
    ESP_RETURN_ON_ERROR(ret,Tag,"bsp camera init failed");

    /* app start task create */
    ret = xTaskCreate(task_process_start,TASK_START_NAME,TASK_START_STACK_DEPTH,NULL,TASK_START_PRIORITY,&task_start);
    if(pdPASS != ret){
        ESP_LOGE(Tag,"app task created failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}


void app_main(void)
{
    esp_err_t ret = ESP_OK;
    ret = app_init();
    if(ESP_OK != ret){
        ESP_LOGE(Tag,"app init failed");
    }
    vTaskDelete(NULL);
}
