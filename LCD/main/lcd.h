#pragma once

#include "esp_types.h"
#include "esp_err.h"
#include "driver/ledc.h"

/* 是否采用本文件中的通信协议初始化 */
#define LCD_BSP_CONFIG  1   
/* lcd的通信协议 */
#define LCD_BSP_SPI     1
/* lcd的芯片 */
#define LCD_BSP_ST7789  1

#if LCD_BSP_CONFIG
    #if LCD_BSP_SPI
        #define LCD_BSP_SPI_HOST        SPI2_HOST
        #define LCD_BSP_SPI_MOSI        GPIO_NUM_40
        #define LCD_BSP_SPI_CLK         GPIO_NUM_41
    #endif
#endif

#if LCD_BSP_SPI
    #define LCD_BSP_SPI_CS          GPIO_NUM_NC
    #define LCD_BSP_DC              GPIO_NUM_39
    #if LCD_BSP_SPI_CS == GPIO_NUM_NC
        typedef esp_err_t(*lcd_ctrl_cs_cb_t)(const uint8_t level);
        #define LCD_BSP_SPI_CS_NC
    #endif
#endif


#define LCD_BSP_RESET           GPIO_NUM_NC         /* 复位引脚 */
#define LCD_BSP_BL              GPIO_NUM_42         /* 背光引脚 */

/* lcd */
#define LCD_W                   320                 /* lcd 宽 */
#define LCD_H                   240                 /* lcd 长 */
#define LCD_BYTE_WIDTH          2                   /* lcd 位宽 565 */
#define LCD_CMD_BIT             8                   /* lcd 命令的长度   8bit */
#define LCD_PARAM_BIT           8                   /* lcd 参数的长度   8bit */
#define LCD_PIXEL_CLK_HZ        (20 * 1000 * 1000)  /* lcd 像素时钟的频率 */
#define LCD_TRANS_QUEUE_DEPTH   10                  /* spi 传输队列深度  */
#define TRANS_DOWN_CALLBACK     NULL                /* 颜色数据传输完后的回调函数 */
#define CALLBACK_PARAM          NULL                /* 传递给回调函数的变量 */
#define LCD_XY_INVERT           true                /* xy是否翻转 */
#define LCD_COLOR_INVERT        true                /* 颜色是否翻转 */
#define LCD_X_MIRROR            true               /* x轴是否镜像 */
#define LCD_Y_MIRROR            false                /* y轴是否镜像 */
#define USED_LEDC               1
/* ledc timer */
#define LCD_BSP_LEDC_TIMER_SPEED_MODE       LEDC_LOW_SPEED_MODE /* 背光的ledc Timer速度模式 */
#define LCD_BSP_LEDC_TIMER_NUM              LEDC_TIMER_0        /* 背光的ledc Timer的次序 */
#define LCD_BSP_LEDC_TIMER_FREQ_HZ          1000                /* 背光的ledc Timer频率 */
#define LCD_BSP_LEDC_TIMER_DUTY_RESOLUTION  LEDC_TIMER_10_BIT   /* 背光的ledc Timer占空比分辨率 */

/* ledc channel */
#define LCD_BSP_LEDC_CHANNEL_NUM            LEDC_CHANNEL_0
#define LCD_BSP_LEDC_CHANNEL_SPEED_MODE     LEDC_LOW_SPEED_MODE
#define LCD_BSP_LEDC_TIMER_PERIOD           1024    // 2^10

/* LCD 显示屏初始化 */
esp_err_t lcd_init(
    #ifdef LCD_BSP_SPI_CS_NC 
    lcd_ctrl_cs_cb_t lcd_cs_ctrl 
    #else  
    void 
    #endif 
);

/* LCD 显示屏显示颜色 */
void lcd_set_color(uint16_t color);
/* LCD 显示屏显示图片 */
void lcd_draw_pic(int x_s,int y_s,int x_e,int y_e,uint16_t *pic_array);
#if !USED_LEDC
/* LCD 显示屏 关 不使用ledc*/
void lcd_close(void);
/* LCD 显示屏 开 不使用ledc*/
void lcd_open(void);
#else
/* 背光渐变设置占空比 */
void lcd_bk_fade_set_duty(uint32_t duty);
/* LCD 显示屏 关 使用ledc*/
void lcd_close(void);
/* LCD 显示屏 开 使用ledc*/
void lcd_open(void);
#endif