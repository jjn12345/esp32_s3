#pragma once

#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_check.h"

#define LCD_BSP_PROTOCOL    1

#if LCD_BSP_PROTOCOL
    typedef enum lcd_protocl{
        lcd_spi = 1,
    }lcd_protocl;


    #define PROTOCOL_TYPE   lcd_spi
    #if PROTOCOL_TYPE == lcd_spi 
        #define LCD_BSP_SPI_HOST    SPI2_HOST     
        #define LCD_BSP_SPI_MOSI    GPIO_NUM_40
        #define LCD_BSP_SPI_CS      GPIO_NUM_NC
        #if LCD_BSP_SPI_CS == GPIO_NUM_NC
            #define LCD_CS_USER
            typedef esp_err_t (*lcd_cs_ctrl)(const uint8_t level);
        #endif
        #define LCD_BSP_SPI_CLK     GPIO_NUM_41
        #define LCD_BSP_SPI_DC      GPIO_NUM_39
        #define LCD_BSP_PCLK_HZ     (80 * 1000 * 1000)
        #define LCD_BSP_CMD_BITS    (8)
        #define LCD_BSP_PARAM_BITS  (8)
        #define LCD_BSP_QUEUE_DEPTH (10)
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
/* LCD  on */
void lcd_on(void);
/* LCD off */
void lcd_off(void);
/* LCD set blk */
void lcd_set_blk(const uint32_t bright);
void lcd_draw(int x_s,int y_s,int x_e,int y_e,uint16_t * FrameBuffer);
void lcd_set_color(uint16_t color);