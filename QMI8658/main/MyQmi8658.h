#pragma once

/* qmi8658 底层i2c相应参数 */
#define BSP_I2C_Port_Num    0
#define BSP_I2C_Freq_HZ     400000
#define BSP_SCL_IO          GPIO_NUM_2
#define BSP_SDA_IO          GPIO_NUM_1
/* qmi8658 寄存器定义 */
#define QMI8658_SENSOR_ADDR       0x6A
#define QMI8658_ID                0x05
/* qmi8658 寄存器 */
typedef enum qmi8658_reg
{
    QMI8658_WHO_AM_I,
    QMI8658_REVISION_ID,
    QMI8658_CTRL1,
    QMI8658_CTRL2,
    QMI8658_CTRL3,
    QMI8658_CTRL4,
    QMI8658_CTRL5,
    QMI8658_CTRL6,
    QMI8658_CTRL7,
    QMI8658_CTRL8,
    QMI8658_CTRL9,
    QMI8658_CATL1_L,
    QMI8658_CATL1_H,
    QMI8658_CATL2_L,
    QMI8658_CATL2_H,
    QMI8658_CATL3_L,
    QMI8658_CATL3_H,
    QMI8658_CATL4_L,
    QMI8658_CATL4_H,
    QMI8658_FIFO_WTM_TH,
    QMI8658_FIFO_CTRL,
    QMI8658_FIFO_SMPL_CNT,
    QMI8658_FIFO_STATUS,
    QMI8658_FIFO_DATA,
    QMI8658_I2CM_STATUS = 44,
    QMI8658_STATUSINT,
    QMI8658_STATUS0,
    QMI8658_STATUS1,
    QMI8658_TIMESTAMP_LOW,
    QMI8658_TIMESTAMP_MID,
    QMI8658_TIMESTAMP_HIGH,
    QMI8658_TEMP_L,
    QMI8658_TEMP_H,
    QMI8658_AX_L,
    QMI8658_AX_H,
    QMI8658_AY_L,
    QMI8658_AY_H,
    QMI8658_AZ_L,
    QMI8658_AZ_H,
    QMI8658_GX_L,
    QMI8658_GX_H,
    QMI8658_GY_L,
    QMI8658_GY_H,
    QMI8658_GZ_L,
    QMI8658_GZ_H,
    QMI8658_MX_L,
    QMI8658_MX_H,
    QMI8658_MY_L,
    QMI8658_MY_H,
    QMI8658_MZ_L,
    QMI8658_MZ_H,
    QMI8658_dQW_L = 73,
    QMI8658_dQW_H,
    QMI8658_dQX_L,
    QMI8658_dQX_H,
    QMI8658_dQY_L,
    QMI8658_dQY_H,
    QMI8658_dQZ_L,
    QMI8658_dQZ_H,
    QMI8658_dVX_L,
    QMI8658_dVX_H,
    QMI8658_dVY_L,
    QMI8658_dVY_H,
    QMI8658_dVZ_L,
    QMI8658_dVZ_H,
    QMI8658_AE_REG1,
    QMI8658_AE_REG2,
    QMI8658_RESET = 96
}qmi8658_reg;
/* fifo模式 */
typedef enum fifo_mode{
    disable = 0,
    fifo,
    stream,
}fifo_mode;
/* FIFO Sample Size */
typedef enum fifo_sample_size{
    fifo_16_sample = 0,
    fifo_32_sample,
    fifo_64_sample,
    fifo_128_sample,
}fifo_sample_size;
typedef struct{
    /* 标志 */
    uint8_t fifo_enable;    /* 是否启用fifo */
    uint8_t fifo_size;      /* fifo采样个数 */
    uint8_t is_cal;         /* 是否经过校准 */
    /* 数据 */
    struct{
        int16_t ax_raw;
        int16_t ay_raw;
        int16_t az_raw;
        int16_t gx_raw;
        int16_t gy_raw;
        int16_t gz_raw;
    }raw_data;
    struct{
        int32_t ax_cal;
        int32_t ay_cal;
        int32_t az_cal;
        int32_t gx_cal;
        int32_t gy_cal;
        int32_t gz_cal;
    }cal_data;
    struct{
        int16_t ax;
        int16_t ay;
        int16_t az;
        int16_t gx;
        int16_t gy;
        int16_t gz;
    }actural;
    struct{
      float pit;
      float roll;
      float yaw;  
    }angle;
}qmi8658_handler;
typedef qmi8658_handler*    h_qmi;
/* Qmi8658的初始化 */
int Qmi8658_Init(h_qmi qmi,fifo_mode mode,fifo_sample_size size);
/* 获取Qmi8658的加速度 */
void Qmi8658_get_acell(h_qmi qmi);
/* 获取Qmi8658的角速度 */
void Qmi8658_get_gyro(h_qmi qmi);
/* 获取Qmi8658的加速度和角速度 */
uint8_t Qmi8658_get_ga(h_qmi qmi);
/* 矫正Qmi8658的加速度和角速度 */
uint8_t Qmi8658_cal_ga(h_qmi qmi);