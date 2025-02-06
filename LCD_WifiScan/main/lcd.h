#pragma once
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_check.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"          
#include "esp_lcd_panel_vendor.h"       


#define LCD_BSP_PROTOCOL        (0)
#define LCD_TP_BSP_PROTOCOL     (0)
#define LCD_TP_CONFIG_ENABLE    (1)

#if LCD_TP_CONFIG_ENABLE
#include "esp_lcd_touch_ft5x06.h"
#endif


typedef enum lcd_protocl{
    lcd_spi = 1,
    lcd_i2c,
}lcd_protocl;

#define PROTOCOL_TYPE               lcd_spi

#if PROTOCOL_TYPE == lcd_spi 
    #define LCD_BSP_SPI_HOST    SPI2_HOST  
    #define LCD_BSP_SPI_DC      GPIO_NUM_39
    #define LCD_BSP_SPI_CS      GPIO_NUM_NC
    #if LCD_BSP_SPI_CS == GPIO_NUM_NC
        #define LCD_CS_USER
        typedef esp_err_t (*lcd_cs_ctrl)(const uint8_t level);
    #endif
    #define LCD_BSP_CMD_BITS    (8)
    #define LCD_BSP_PARAM_BITS  (8)
    #define LCD_BSP_PCLK_HZ     (80 * 1000 * 1000)
    #define LCD_BSP_QUEUE_DEPTH (10)
    #if LCD_BSP_PROTOCOL
        #define LCD_BSP_SPI_MOSI    GPIO_NUM_40
        #define LCD_BSP_SPI_CLK     GPIO_NUM_41
    #endif
#endif

#if LCD_TP_CONFIG_ENABLE    
    #define LCD_TP_BSP_PROTOCOL_TYPE    lcd_i2c
    /* 复位引脚 */
    #define LCD_TP_BSP_RESET        GPIO_NUM_NC
    #if LCD_TP_BSP_RESET == GPIO_NUM_NC
        #define LCD_TP_RESET_ACTIVE   (0)
    #else
        #define LCD_TP_RESET_ACTIVE   (1)
    #endif

    /* 中断引脚 */
    #define LCD_TP_BSP_INT      GPIO_NUM_NC
    #if LCD_TP_BSP_INT == GPIO_NUM_NC
        #define LCD_TP_INT_ACTIVE   (0)
    #else
        #define LCD_TP_INT_ACTIVE   (1)
    #endif

    
    /* 触摸IC的数字协议接口 */
    #if LCD_TP_BSP_PROTOCOL_TYPE == lcd_i2c 
        #define LCD_BSP_I2C_PORT        (I2C_NUM_0)  
        #if LCD_TP_BSP_PROTOCOL  
            #define LCD_BSP_TP_I2C_SDA      (GPIO_NUM_1)
            #define LCD_BSP_TP_I2C_SCL      (GPIO_NUM_2)
            #define LCD_BSP_TP_I2C_SPEED    (400000)
        #endif
    #endif
#endif

#define LCD_BSP_BLK         GPIO_NUM_42
#define LCD_BSP_RESET       GPIO_NUM_NC

#define LCD_H   (240)
#define LCD_W   (320)
#define LCD_PIXEL_BYTE   LCD_H*LCD_W*2

#define DRIVER_TYPE    st7789
typedef enum lcd_driver{
    st7789 = 1,
}lcd_driver;


#define LCD_BSP_PIXEL_BIT       (16)

/* 背光渐变开关 */
#define LCD_BLK_IS_FADE         1

#if LCD_BLK_IS_FADE
    /* ledc timer */
    #define LCD_LEDC_TIMER_SPEED_MODE       LEDC_LOW_SPEED_MODE
    #define LCD_LEDC_TIMER_NUM              LEDC_TIMER_0
    #define LCD_LEDC_TIMER_DUTY_RESOLUTION  LEDC_TIMER_12_BIT
    #define LCD_LEDC_TIMER_CLK              (1 * 1000)
    #define LCD_LEDC_TIMER_CLK_SOURCE       LEDC_AUTO_CLK
    #define LCD_LEDC_PERIOD                 (4096)  /* 2^12 */
    /* ledc channel */
    #define LCD_LEDC_CHANNEL                LEDC_CHANNEL_0

    #define LCD_ON_DUTY     LCD_LEDC_PERIOD
    #define LCD_OFF_DUTY    (0)
#endif

/* LCD Config */
#define LCD_MIRROR_X            1
#define LCD_MIRROR_Y            0
#define LCD_XY_INVERT           1
#define LCD_COLOR_INVERT        1

esp_err_t lcd_init(
    #ifdef LCD_CS_USER
        lcd_cs_ctrl cs_ctrl
    #else
        void 
    #endif
);
void lcd_on(void);
void lcd_off(void);
void lcd_set_blk(const uint32_t bright);
void lcd_draw(int x_s,int y_s,int x_e,int y_e,uint16_t * FrameBuffer);
void lcd_set_color(uint16_t color);
esp_lcd_panel_handle_t * lcd_get_panel_handle(void);
esp_lcd_panel_io_handle_t * lcd_get_io_handle(void);
#if LCD_TP_CONFIG_ENABLE
esp_lcd_touch_handle_t * lcd_get_tp_handle(void);
#endif