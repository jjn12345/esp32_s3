#include "esp32_excute.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "lcd.h"
#include "pca9557.h"
#include "esp_camera.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"
#include "llx.h"


static pca9557_handle_t pca;

static char Tag[]   = "lcd_lvgl";

/* I2C     */
#define I2C_PORT            I2C_NUM_0
#define I2C_SCL             GPIO_NUM_2
#define I2C_SDA             GPIO_NUM_1
#define I2C_CLK_FREQ_HZ     (100 * 1000)
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

/* SPI */
#define SPI_HOST            SPI2_HOST
#define SPI_MOSI            GPIO_NUM_40
#define SPI_CLK             GPIO_NUM_41
#define SPI_MAX_TRAN_SIZE   LCD_PIXEL_BYTE
static esp_err_t bsp_spi_init(void){
    esp_err_t ret = ESP_FAIL;
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = SPI_CLK,
        .max_transfer_sz = SPI_MAX_TRAN_SIZE,
        .quadhd_io_num = -1,
        .quadwp_io_num = -1,
    };
    ret = spi_bus_initialize(SPI_HOST,&bus_cfg,SPI_DMA_CH_AUTO);
    ESP_RETURN_ON_ERROR(ret,Tag,"spi init failed");
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

/* lvgl */
#define LV_LCD_H        LCD_W
#define LV_LCD_V        LCD_H
#define LV_BUFF_SIZE    LV_LCD_H * 20 * sizeof(lv_color_t)
static lv_disp_t * disp_handle;
static lv_indev_t* touch_handle;
static esp_lcd_panel_io_handle_t *io_handle = NULL;
static esp_lcd_panel_handle_t *panel_handle = NULL;
static esp_lcd_touch_handle_t *tp = NULL;
static void lvgl_start(void){
    /* lvgl 接口配置 */
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    esp_err_t err = lvgl_port_init(&lvgl_cfg);
    if(ESP_OK != err){
        ESP_LOGE(Tag,"LVGL PORT init fail");
        return;
    }
    /* 将屏幕添加到lvgl接口中 */
    io_handle = lcd_get_io_handle();
    panel_handle = lcd_get_panel_handle();
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = *io_handle,            /* lcd的io执行对象 */
        .panel_handle = *panel_handle,  /* lcd的控制平台执行对象 */
        .buffer_size = LV_BUFF_SIZE,    /* 绘制缓冲区的大小 */
        .double_buffer = true,  /* 是否双缓冲 */
        .hres = LV_LCD_H,       /* 显示器的水平分辨率 */
        .vres = LV_LCD_V,       /* 显示器的垂直分辨率 */
        .monochrome = false,    /* 是否单色显示器 */
        .rotation = {
            .swap_xy = true,   
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,  /* 是否使用DMA 注意：psram和dma不能同时为true*/
            .buff_spiram = true,    /* 是否使用PSRAM 注意：psram和dma不能同时为true */
        }
    };
    disp_handle = lvgl_port_add_disp(&disp_cfg);
    /* 将触摸屏输入设备添加到lvgl接口中 */
    tp = lcd_get_tp_handle();
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp_handle,
        .handle = *tp,
    };
    touch_handle = lvgl_port_add_touch(&touch_cfg); 
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


esp_err_t app_init(void){
    esp_err_t   ret = ESP_OK; 
    /* i2c init */
    ret = bsp_i2c_init();
    ESP_RETURN_ON_ERROR(ret,Tag,"bsp i2c init failed");
    /* spi init */
    ret = bsp_spi_init();
    ESP_RETURN_ON_ERROR(ret,Tag,"bsp spi init failed");
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
    // pca9557_gpio_write(pca,CAMERA_PWDN,pca_gpio_low);
    // ret = bsp_camera_init();
    // ESP_RETURN_ON_ERROR(ret,Tag,"bsp camera init failed");
    /* lvgl start */
    lvgl_start();
    // /* app start task create */
    // ret = xTaskCreate(task_process_start,TASK_START_NAME,TASK_START_STACK_DEPTH,NULL,TASK_START_PRIORITY,&task_start);
    // if(pdPASS != ret){
    //     ESP_LOGE(Tag,"app task created failed");
    //     return ESP_FAIL;
    // }
    return ESP_OK;
}





