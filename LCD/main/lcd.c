#include "string.h"
#include "lcd.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"


#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

static char Tag[]   = "LCD";
#define LIM(x,min,max)  (x<=min?(min):(x>=max?max:x));   

#if !USED_LEDC
/* 背光引脚初始化 */
static esp_err_t lcd_bk_init(void){
    gpio_config_t gpio_cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << LCD_BSP_BL, /* 注意ULL强转为64位 */
    };
    return gpio_config(&gpio_cfg);
}
/* LCD 显示屏 关 */
void lcd_close(void){
    gpio_set_level(LCD_BSP_BL,1);
}
/* LCD 显示屏 开 */
void lcd_open(void){
    gpio_set_level(LCD_BSP_BL,0);
}
#else
/* 背光渐变初始化 */
static esp_err_t lcd_bk_fade_init(void){
    /* 配置定时器 */
    ledc_timer_config_t timer_cfg = {
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false,
        .duty_resolution = LCD_BSP_LEDC_TIMER_DUTY_RESOLUTION,
        .freq_hz = LCD_BSP_LEDC_TIMER_FREQ_HZ,
        .speed_mode =  LCD_BSP_LEDC_TIMER_SPEED_MODE,
        .timer_num = LCD_BSP_LEDC_TIMER_NUM,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg),Tag,"ledc timer config failed");
    /* 配置定时器通道 */
    ledc_channel_config_t channel_cfg = {
        .channel = LCD_BSP_LEDC_CHANNEL_NUM,
        .duty = 0, /* 初始的值 占空比等于 duty / 2^duty_resolution */
        .gpio_num =  LCD_BSP_BL,
        .intr_type = LEDC_INTR_DISABLE,
        .speed_mode = LCD_BSP_LEDC_CHANNEL_SPEED_MODE,
        .timer_sel = timer_cfg.timer_num,
        .hpoint = LCD_BSP_LEDC_TIMER_PERIOD-1,    /* 高电平时间所占的最大值 */
        .flags.output_invert = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel_cfg),Tag,"ledc channel config failed");
    return ESP_OK;
}
/* 背光渐变设置占空比 */
void lcd_bk_fade_set_duty(uint32_t duty){
    duty = LIM(duty,0,LCD_BSP_LEDC_TIMER_PERIOD);
    ledc_set_duty(LCD_BSP_LEDC_CHANNEL_SPEED_MODE,LCD_BSP_LEDC_CHANNEL_NUM,duty);
    ledc_update_duty(LCD_BSP_LEDC_CHANNEL_SPEED_MODE,LCD_BSP_LEDC_CHANNEL_NUM);
}
/* LCD 显示屏 关 */
void lcd_close(void){
    lcd_bk_fade_set_duty(LCD_BSP_LEDC_TIMER_PERIOD);
}
/* LCD 显示屏 开 */
void lcd_open(void){
    lcd_bk_fade_set_duty(0);
}
#endif
/* SPI 协议 */
#if LCD_BSP_CONFIG
static esp_err_t lcd_bsp_communication_init(void){
    #if LCD_BSP_SPI
        spi_bus_config_t bus_cfg = {
            .mosi_io_num = LCD_BSP_SPI_MOSI,
            .miso_io_num = -1,
            .sclk_io_num = LCD_BSP_SPI_CLK,
            .max_transfer_sz = LCD_W * LCD_H * LCD_BYTE_WIDTH,
            .quadhd_io_num = -1,
            .quadwp_io_num = -1,
        };
        ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_BSP_SPI_HOST,&bus_cfg,SPI_DMA_CH_AUTO),Tag,"spi init failed");
    #endif
    return ESP_OK;
}
#endif

static esp_lcd_panel_io_handle_t   io_handle = NULL;
static esp_lcd_panel_handle_t      panel_handle = NULL;

static esp_err_t lcd_bsp_init(    
    #ifdef LCD_BSP_SPI_CS_NC 
    lcd_ctrl_cs_cb_t lcd_cs_ctrl 
    #else  
    void 
    #endif 
    ){
#if USED_LEDC
    ESP_RETURN_ON_ERROR(lcd_bk_fade_init(),Tag,"BK pin init failed");
#else
    ESP_RETURN_ON_ERROR(lcd_bk_init(),Tag,"BK pin init failed");
#endif

#if LCD_BSP_CONFIG
    ESP_RETURN_ON_ERROR(lcd_bsp_communication_init(),Tag,"protocol init failed");
#endif

#if LCD_BSP_SPI
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = LCD_BSP_SPI_CS,
        .dc_gpio_num = LCD_BSP_DC,
        .spi_mode = 0,
        .lcd_cmd_bits = LCD_CMD_BIT,
        .lcd_param_bits = LCD_PARAM_BIT,
        .pclk_hz = LCD_PIXEL_CLK_HZ,
        .trans_queue_depth = LCD_TRANS_QUEUE_DEPTH,
        .on_color_trans_done = TRANS_DOWN_CALLBACK,
        .user_ctx = CALLBACK_PARAM,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_BSP_SPI_HOST,\
                        &io_cfg,&io_handle),Tag,"new panel failed");
#endif    
    esp_lcd_panel_dev_config_t dev_cfg = {
        .reset_gpio_num = LCD_BSP_RESET,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BYTE_WIDTH * 8,
    };
#if LCD_BSP_ST7789
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(io_handle,&dev_cfg,&panel_handle),Tag,"new panel failed");
#endif
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel_handle),Tag,"reset failed");
    #ifdef LCD_BSP_SPI_CS_NC
        ESP_RETURN_ON_ERROR(lcd_cs_ctrl(0),Tag,"cs low failed");
    #endif
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel_handle),Tag,"lcd init failed");
    return ESP_OK;
}

esp_err_t lcd_init(
    #ifdef LCD_BSP_SPI_CS_NC 
    lcd_ctrl_cs_cb_t lcd_cs_ctrl 
    #else  
    void 
    #endif 
){
    if(ESP_OK != lcd_bsp_init(lcd_cs_ctrl)){   /* lcd 驱动初始化 */
        return ESP_FAIL;
    }
    /*  lcd xy 镜像 */    
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel_handle,LCD_X_MIRROR,LCD_Y_MIRROR),Tag,"mirror failed");
    /* lcd color 翻转 */       
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel_handle,LCD_COLOR_INVERT),Tag,"swap color failed");
    /* lcd xy 翻转 */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(panel_handle,LCD_XY_INVERT),Tag,"swap xy failed");
    /* 开启lcd显示 */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle,true),Tag,"disp failed");
    /* 设置lcd为白色 */
    lcd_set_color(0xffff);
    /* 开启背光 */
    lcd_open();
    return ESP_OK;
}

void lcd_set_color(uint16_t color){
    size_t buffer_size = LCD_W * 2;
    uint16_t *buffer = heap_caps_malloc(buffer_size,MALLOC_CAP_8BIT|MALLOC_CAP_SPIRAM);
    memset(buffer,color,buffer_size);
    for(size_t i = 0;i < LCD_H;i++){
        esp_lcd_panel_draw_bitmap(panel_handle,0,i,LCD_W,i+1,buffer);
    }
    heap_caps_free(buffer);
}

void lcd_draw_pic(int x_s,int y_s,int x_e,int y_e,uint16_t *pic_array){
    size_t buffer_size = (x_e - x_s) * (y_e - y_s) * 2;
    uint16_t *buffer = (uint16_t*)heap_caps_malloc(buffer_size,MALLOC_CAP_8BIT|MALLOC_CAP_SPIRAM);
    if(NULL == buffer){
        ESP_LOGE(Tag,"buffer malloc failde");
        return;
    }
    memcpy(buffer,pic_array,buffer_size);
    esp_lcd_panel_draw_bitmap(panel_handle,x_s,y_s,x_e,y_e,(uint16_t *)buffer);
    heap_caps_free(buffer);
}






