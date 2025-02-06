#include "lcd.h"
#include "string.h"
static const char Tag[] = "lcd";
static lcd_protocl bsp_protocl = PROTOCOL_TYPE;
#if LCD_TP_CONFIG_ENABLE
    static lcd_protocl tp_bsp_protocl = LCD_TP_BSP_PROTOCOL_TYPE;
    #ifdef LCD_TP_BSP_PROTOCOL_TYPE
        #if LCD_TP_BSP_PROTOCOL_TYPE == lcd_i2c && LCD_TP_BSP_PROTOCOL
            static i2c_master_bus_handle_t tp_i2c_bus_handle = NULL;
        #endif
    #endif
#endif
static lcd_driver bsp_lcd_driver = DRIVER_TYPE;
#if LCD_BSP_PROTOCOL
    static esp_err_t lcd_bsp_spi_init(void){
        esp_err_t ret = ESP_FAIL;
        spi_bus_config_t bus_cfg = {
            .mosi_io_num = LCD_BSP_SPI_MOSI,
            .miso_io_num = -1,
            .sclk_io_num = LCD_BSP_SPI_CLK,
            .max_transfer_sz = LCD_PIXEL_BYTE,
            .quadhd_io_num = -1,
            .quadwp_io_num = -1,
        };
        ret = spi_bus_initialize(LCD_BSP_SPI_HOST,&bus_cfg,SPI_DMA_CH_AUTO);
        ESP_RETURN_ON_ERROR(ret,Tag,"spi init failed");
        return ESP_OK;
    }
    static esp_err_t lcd_bsp_i2c_init(void){
        esp_err_t ret = ESP_FAIL;
        i2c_master_bus_config_t bus_cfg = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .flags = {
                .enable_internal_pullup = true,
            },
            .glitch_ignore_cnt = 7,
            .i2c_port = LCD_BSP_I2C_PORT,
            .scl_io_num = LCD_BSP_TP_I2C_SCL,
            .sda_io_num = LCD_BSP_TP_I2C_SDA,
            .trans_queue_depth = 10,
        };
        ret = i2c_new_master_bus(&bus_cfg,&tp_i2c_bus_handle);
        ESP_RETURN_ON_ERROR(ret,Tag,"i2c init failed");
        return ESP_OK;
    }
    /* 通信协议总初始化 */
    static esp_err_t lcd_bsp_protocal_init(const uint8_t protocal){
        esp_err_t ret = ESP_FAIL;
        switch(protocal){
            case lcd_spi:{
                ret = lcd_bsp_spi_init();
            }
            break;
            case lcd_i2c:{
                ret = lcd_bsp_i2c_init();
            }
            break;
        }
        return ret;
    }
#endif
#if LCD_TP_CONFIG_ENABLE
    #if LCD_TP_BSP_PROTOCOL
        /* 触摸屏通信协议总初始化 */
        static esp_err_t lcd_tp_bsp_protocal_init(const uint8_t protocal){
            if (LCD_TP_BSP_PROTOCOL_TYPE == PROTOCOL_TYPE){
                return ESP_OK;
            }
            esp_err_t ret = ESP_FAIL;
            switch(protocal){
                case lcd_spi:{
                    ret = lcd_bsp_spi_init();
                }
                break;
                case lcd_i2c:{
                    ret = lcd_bsp_i2c_init();
                }
                break;
            }
            return ret;
        }
    #endif
#endif
#if LCD_TP_CONFIG_ENABLE
static esp_lcd_touch_handle_t tp = NULL;
static esp_err_t lcd_bsp_tp_init(const uint8_t protocol){
    esp_err_t ret = ESP_FAIL;
    #if LCD_TP_BSP_PROTOCOL
        ESP_RETURN_ON_ERROR(lcd_tp_bsp_protocal_init(tp_bsp_protocl),Tag,"tp protocal init failed");
    #endif  
    
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    switch(protocol){
        case lcd_i2c:{
            #if LCD_TP_BSP_PROTOCOL
            ret = esp_lcd_new_panel_io_i2c(tp_i2c_bus_handle,&tp_io_config,&tp_io_handle);
            #else
            ret = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)LCD_BSP_I2C_PORT,&tp_io_config,&tp_io_handle);
            #endif
            ESP_RETURN_ON_ERROR(ret,Tag,"tp new panel io i2c failed");
        }
        break;
        case lcd_spi:{
            ESP_LOGI(Tag,"tp spi init no realized");
            return ESP_FAIL;
        }
        break;
    }
    esp_lcd_touch_config_t tp_config = {
        .x_max = LCD_H,
        .y_max = LCD_W,
        .rst_gpio_num = LCD_TP_BSP_RESET,
        .int_gpio_num = LCD_TP_BSP_INT,
        .levels = {
            .interrupt = LCD_TP_INT_ACTIVE,
            .reset = LCD_TP_RESET_ACTIVE,
        },
        .flags = {
            .mirror_x = LCD_MIRROR_X,
            .mirror_y = LCD_MIRROR_Y,
            .swap_xy = LCD_XY_INVERT,
        },
    };
    ret = esp_lcd_touch_new_i2c_ft5x06(tp_io_handle,&tp_config,&tp);
    ESP_RETURN_ON_ERROR(ret,Tag,"tp new i2c ft5x06 failed");
    return ESP_OK;
}    
esp_lcd_touch_handle_t * lcd_get_tp_handle(void){
    if(NULL == tp){
        ESP_LOGE(Tag,"tp No initialization ");
        return NULL;
    }
    return &tp;
}
#endif

/* lcd panel 执行对象 */
static esp_lcd_panel_handle_t lcd_panel_t = NULL;
static esp_lcd_panel_io_handle_t lcd_io_handle = NULL;
/* LCD 平台初始化 */
static esp_err_t lcd_bsp_panel_init(const uint8_t protocal,const uint8_t driver){
    esp_err_t ret = ESP_FAIL;
    
#if LCD_BSP_PROTOCOL
    ESP_RETURN_ON_ERROR(lcd_bsp_protocal_init(bsp_protocl),Tag,"protocal init failed");
#endif
    switch(protocal){
        case lcd_spi:{
            /* 将lcd设备连接到spi总线上 */
            esp_lcd_panel_io_spi_config_t spi_io_cfg = {
                .cs_gpio_num = LCD_BSP_SPI_CS,
                .dc_gpio_num = LCD_BSP_SPI_DC,
                .lcd_cmd_bits = LCD_BSP_CMD_BITS,
                .lcd_param_bits = LCD_BSP_PARAM_BITS,
                .on_color_trans_done = NULL,
                .pclk_hz = LCD_BSP_PCLK_HZ,
                .spi_mode = 0,  
                .trans_queue_depth = LCD_BSP_QUEUE_DEPTH,
            };
            ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_BSP_SPI_HOST,&spi_io_cfg,&lcd_io_handle);
            ESP_RETURN_ON_ERROR(ret,Tag,"new panel io failed");
        }
        break;
    }
    /* 安装对应屏幕驱动程序 */
    esp_lcd_panel_dev_config_t dev_cfg = {
        .bits_per_pixel = LCD_BSP_PIXEL_BIT,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .reset_gpio_num = LCD_BSP_RESET,
    };
    switch(driver){
        case st7789:{
            ret = esp_lcd_new_panel_st7789(lcd_io_handle,&dev_cfg,&lcd_panel_t);
        }
        break;
    }
    ESP_RETURN_ON_ERROR(ret,Tag,"driver install failed");
    return ESP_OK;
}
/* LCD 背光引脚初始化 */
static esp_err_t lcd_blk_init(void){
    esp_err_t ret = ESP_FAIL;
    #if LCD_BLK_IS_FADE
        /* 配置ledc时钟 */
        ledc_timer_config_t timer_cfg = {
            .deconfigure = false,
            .speed_mode = LCD_LEDC_TIMER_SPEED_MODE,
            .timer_num = LCD_LEDC_TIMER_NUM,
            .duty_resolution = LCD_LEDC_TIMER_DUTY_RESOLUTION,
            .freq_hz = LCD_LEDC_TIMER_CLK,
            .clk_cfg = LCD_LEDC_TIMER_CLK_SOURCE,
        };
        ret = ledc_timer_config(&timer_cfg);
        ESP_RETURN_ON_ERROR(ret,Tag,"ledc timer config failed");
        ledc_channel_config_t chan_cfg = {
            .gpio_num = LCD_BSP_BLK,
            .duty = 0,
            .channel = LCD_LEDC_CHANNEL,
            .timer_sel = timer_cfg.timer_num,
            .flags.output_invert = 0,
            .hpoint = LCD_LEDC_PERIOD - 1,
        };
        ret = ledc_channel_config(&chan_cfg);
        ESP_RETURN_ON_ERROR(ret,Tag,"ledc channel config failed");
    #else
        gpio_config_t blk_cfg = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL<<LCD_BSP_BLK),
        };
        ret = gpio_config(&blk_cfg);
        ESP_RETURN_ON_ERROR(ret,Tag,"blk pin config failed");
    #endif    
    return ESP_OK;
}
/* LCD 执行函数 */
/* 获取LCD panel 执行对象 */
esp_lcd_panel_handle_t * lcd_get_panel_handle(void){
    if(NULL == lcd_panel_t){
        ESP_LOGE(Tag,"lcd_panel_t No initialization ");
        return NULL;
    }
    return &lcd_panel_t;
}
/* 获取LCD io 执行对象 */
esp_lcd_panel_io_handle_t * lcd_get_io_handle(void){
    if(NULL == lcd_io_handle){
        ESP_LOGE(Tag,"lcd_io_handle No initialization ");
        return NULL;
    }
    return &lcd_io_handle;
}
/* LCD on */
void lcd_on(void){
    #if LCD_BLK_IS_FADE
        ledc_set_duty(LCD_LEDC_TIMER_SPEED_MODE,LCD_LEDC_CHANNEL,LCD_ON_DUTY);
    #else
        gpio_set_level(LCD_BSP_BLK,0);
    #endif    
}
/* LCD off */
void lcd_off(void){
    #if LCD_BLK_IS_FADE
        ledc_set_duty(LCD_LEDC_TIMER_SPEED_MODE,LCD_LEDC_CHANNEL,LCD_OFF_DUTY);
    #else
        gpio_set_level(LCD_BSP_BLK,1);
    #endif  

}
/* LCD set blk */
void lcd_set_blk(const uint32_t bright){
    ledc_set_duty(LCD_LEDC_TIMER_SPEED_MODE,LCD_LEDC_CHANNEL,bright);
}
/* LCD  draw Framebuffer */
void lcd_draw(int x_s,int y_s,int x_e,int y_e,uint16_t * FrameBuffer){
    // size_t buffer_size = (x_e - x_s) * (y_e - y_s) * 2;
    // uint16_t *tmp_buffer = heap_caps_malloc(buffer_size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT); 
    // if(NULL == tmp_buffer){
    //     ESP_LOGI(Tag,"draw failed");
    //     return;
    // }
    // memcpy(tmp_buffer,FrameBuffer,buffer_size);
    // esp_lcd_panel_draw_bitmap(lcd_panel_t,x_s,y_s,x_e,y_e,tmp_buffer);
    // heap_caps_free(tmp_buffer);
     esp_lcd_panel_draw_bitmap(lcd_panel_t,x_s,y_s,x_e,y_e,FrameBuffer);
}
/* LCD  set color */
void lcd_set_color(uint16_t color){
    size_t buffer_size = LCD_W * 2;
    uint16_t *buffer = heap_caps_malloc(buffer_size,MALLOC_CAP_8BIT|MALLOC_CAP_SPIRAM);
    if(NULL == buffer){
        ESP_LOGI(Tag,"set color failed");
        return;
    }
    memset(buffer,color,buffer_size);
    for(size_t i = 0;i < LCD_H;i++){
        esp_lcd_panel_draw_bitmap(lcd_panel_t,0,i,LCD_W,i+1,buffer);
    }
    heap_caps_free(buffer);
}
/* LCD 初始化 */
esp_err_t lcd_init(
    #ifdef LCD_CS_USER
        lcd_cs_ctrl cs_ctrl
    #else
        void 
    #endif
){
    esp_err_t ret = ESP_FAIL;
    /* lcd bsp init */
    ESP_RETURN_ON_ERROR(lcd_bsp_panel_init(bsp_protocl,bsp_lcd_driver),Tag,"bsp panel init failed");
    ESP_RETURN_ON_ERROR(lcd_blk_init(),Tag,"bsp blk init failed");
    /* lcd resetd */
    ret = esp_lcd_panel_reset(lcd_panel_t);
    ESP_RETURN_ON_ERROR(ret,Tag,"lcd reset failed");
    /* low cs if spi */
    #ifdef LCD_CS_USER
        ESP_RETURN_ON_ERROR(cs_ctrl(0),Tag,"cs low failed"); /* 拉低cs引脚 */
    #endif
    /* lcd init  */
    ret = esp_lcd_panel_init(lcd_panel_t);
    ESP_RETURN_ON_ERROR(ret,Tag,"lcd init failed");
    /*  lcd xy 镜像 */   
    ret = esp_lcd_panel_mirror(lcd_panel_t,LCD_MIRROR_X,LCD_MIRROR_Y);
    ESP_RETURN_ON_ERROR(ret,Tag,"mirror failed");
    /* lcd color 翻转 */       
    ret = esp_lcd_panel_invert_color(lcd_panel_t,LCD_COLOR_INVERT);
    ESP_RETURN_ON_ERROR(ret,Tag,"invert color failed");
    /* lcd xy 翻转 */
    ret = esp_lcd_panel_swap_xy(lcd_panel_t,LCD_XY_INVERT);
    ESP_RETURN_ON_ERROR(ret,Tag,"invert xy failed");
    /* tp init */
    #if LCD_TP_CONFIG_ENABLE
        ret = lcd_bsp_tp_init(tp_bsp_protocl);
        ESP_RETURN_ON_ERROR(ret,Tag,"lcd tp init failed");
    #endif
    /* lcd display on */ 
    ret = esp_lcd_panel_disp_on_off(lcd_panel_t,1);
    ESP_RETURN_ON_ERROR(ret,Tag,"lcd disp failed");
    /* lcd set color */
    lcd_set_color(0xffff);
    /* lcd blk on */
    lcd_on();
    return ESP_OK;
}


