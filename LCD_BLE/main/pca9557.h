#pragma once

#include "esp_types.h"
#include "esp_err.h"

#include "driver/i2c.h"

typedef void *pca9557_handle_t;

#define PCA9557_BSP_I2C_CONFIG  0
/* 采用本文件中底层I2C初始化 */
#if PCA9557_BSP_I2C_CONFIG
    #define PCA9557_BSP_I2C_SDA     (GPIO_NUM_1)
    #define PCA9557_BSP_I2C_SCL     (GPIO_NUM_2)
    #define PCA9557_BSP_I2C_SPEED   (200000)
#endif

/* 0 0 1 1 A2 A1 A0 */
typedef enum pca9557_addr{
    pca9557_addr0 = 24,         /* 0011 000 */
    pca9557_addr1,              /* 0011 001 */
    pca9557_addr2,              /* 0011 010 */
    pca9557_addr3,              /* 0011 011 */
    pca9557_addr4,              /* 0011 100 */
    pca9557_addr5,              /* 0011 101 */
    pca9557_addr6,              /* 0011 110 */
    pca9557_addr7,              /* 0011 111 */
}pca9557_addr;

typedef enum pca9557_reg{
    pca9557_input_port_reg = 0,         /* 输入IO寄存器 */
    pca9557_output_port_reg,            /* 输出IO寄存器 */
    pca9557_polarity_inversion_reg,     /* IO极性反转寄存器 */
    pca9557_configuration_reg           /* IO方向配置寄存器 */
}pca9557_reg;

typedef enum pca9557_gpio_mode{
    pca_output = 0,
    pca_input,
}pca9557_gpio_mode;

typedef enum pca9557_gpio_level{
    pca_gpio_low = 0,
    pca_gpio_high,
}pca9557_gpio_level;

typedef enum pca9557_gpio_mask{
    pca9557_gpio_num_0 = 1<<0,
    pca9557_gpio_num_1 = 1<<1,
    pca9557_gpio_num_2 = 1<<2,
    pca9557_gpio_num_3 = 1<<3,
    pca9557_gpio_num_4 = 1<<4,
    pca9557_gpio_num_5 = 1<<5,
    pca9557_gpio_num_6 = 1<<6,
    pca9557_gpio_num_7 = 1<<7,
}pca9557_gpio_mask;




/* PCA9557 gpio config */
esp_err_t pca9557_gpio_config(pca9557_handle_t dev,const uint8_t gpio,const uint8_t mode);
/* PCA9557 gpio write level */
esp_err_t pca9557_gpio_write(pca9557_handle_t dev,const uint8_t gpio,const uint8_t level);
/* PCA9557 gpio read level */
esp_err_t pca9557_gpio_read(pca9557_handle_t dev,const uint8_t gpio,uint8_t *level);
/* PCA9557 Init */
esp_err_t pca9557_init(pca9557_handle_t dev,uint8_t init_val);
/* pca9557 dev create */
pca9557_handle_t pca9557_create(i2c_port_t port ,const uint16_t addr);
/* pca9557 dev delete */
void pca9557_delete(pca9557_handle_t dev);