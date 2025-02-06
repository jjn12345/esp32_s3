#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"
#include "MyQmi8658.h"

#define TAG                 "QMI8658_INIT"
#define Delay_time          1000



/* 初始化i2c */
static esp_err_t Qmi8658_Bsp_I2C_Init(void){
    i2c_port_t  qmi8658_i2c_port = BSP_I2C_Port_Num;
    i2c_config_t    qmi8658_i2c_config = {
        .mode = I2C_MODE_MASTER,
        .master.clk_speed =  BSP_I2C_Freq_HZ,
        .scl_io_num = BSP_SCL_IO,
        .sda_io_num = BSP_SDA_IO,
        .scl_pullup_en = 1,
        .sda_pullup_en = 1,
    };
    i2c_param_config(qmi8658_i2c_port,&qmi8658_i2c_config);
    return i2c_driver_install(qmi8658_i2c_port,qmi8658_i2c_config.mode,0,0,0);
}
/* i2c写单个字节 */
static esp_err_t qmi8658_i2c_write_byte(uint8_t reg,uint8_t data){
    esp_err_t ret = 0;
    uint8_t writer_buffer[2] = {reg,data};
    ret = i2c_master_write_to_device(BSP_I2C_Port_Num,QMI8658_SENSOR_ADDR,\
                                     writer_buffer,sizeof(writer_buffer) / sizeof(writer_buffer[0]),\
                                     Delay_time / portTICK_PERIOD_MS);
    return ret;
}
/* i2c读多个字节 */
static esp_err_t qmi8658_i2c_read(uint8_t reg,uint8_t* data,uint8_t len){
    return i2c_master_write_read_device(BSP_I2C_Port_Num,QMI8658_SENSOR_ADDR,\
                                 &reg,1,data,len,Delay_time / portTICK_PERIOD_MS);
}

/* 设置FIFO工作模式 */
static void Qmi8658_set_fifo_mode(h_qmi qmi,fifo_mode mode){
    uint8_t tmp = 0;
    qmi8658_i2c_read(QMI8658_FIFO_CTRL,&tmp,1);
    tmp = (tmp & (~(0x03))) | mode;
    qmi8658_i2c_write_byte(QMI8658_FIFO_CTRL,tmp);
    qmi->fifo_enable = mode;
}
/* 设置FIFO采样数据个数 */
static void Qmi8658_set_fifo_sample_size(h_qmi qmi,fifo_sample_size sample_size){
    uint8_t tmp = 0;
    qmi8658_i2c_read(QMI8658_FIFO_CTRL,&tmp,1);
    tmp = (tmp & (~((0x03) << 2))) | (sample_size<<2);
    qmi8658_i2c_write_byte(QMI8658_FIFO_CTRL,tmp);
    switch(sample_size){
        case fifo_16_sample:qmi->fifo_size = 16;break;
        case fifo_32_sample:qmi->fifo_size = 32;break;
        case fifo_64_sample:qmi->fifo_size = 64;break;
        case fifo_128_sample:qmi->fifo_size = 128;break;
    }
}
/* 获取FIFO实际采样数据个数 */
static uint16_t Qmi8658_get_fifo_sample_size(void){
    uint8_t fifo_sample_lsb  = 0;
    uint8_t fifo_sample_hsb  = 0;
    uint16_t fifo_sample_count = 0;
    qmi8658_i2c_read(QMI8658_FIFO_SMPL_CNT,&fifo_sample_lsb,1);
    qmi8658_i2c_read(QMI8658_FIFO_STATUS,&fifo_sample_hsb,1);
    fifo_sample_hsb &= (0x03);
    fifo_sample_count =  ((uint16_t)(fifo_sample_hsb<<8)|fifo_sample_lsb) * 2;
    return fifo_sample_count;
}
/* 清除FIFO读模式 */
static void Qmi8658_clear_fifo_r_mode(void){
    uint8_t tmp = 0;
    qmi8658_i2c_read(QMI8658_FIFO_CTRL,&tmp,1);
    tmp &= ~(1<<7);
    qmi8658_i2c_write_byte(QMI8658_FIFO_CTRL,tmp);
}
// void Qmi8658_getAngle(float)


/* 初始化Qmi8658 */
int Qmi8658_Init(h_qmi qmi,fifo_mode mode,fifo_sample_size size){
    uint8_t data = 0;
    int ret = 0;
    /* i2c 初始化 */
    if(ESP_OK != Qmi8658_Bsp_I2C_Init()){
        ret = -1;
        ESP_LOGE(TAG,"device i2c init error\r\n");
    }
    else{
        ESP_LOGI(TAG,"device i2c init ok\r\n");
    }
    /* 设备id读取 */
    if(ESP_OK != qmi8658_i2c_read(QMI8658_WHO_AM_I,&data,1)){
        ret = -2;
        ESP_LOGE(TAG,"device id error\r\n");
    }
    if(data != QMI8658_ID){
        ret = -2;
        ESP_LOGE(TAG,"device id error\r\n");
    }
    else{
        ESP_LOGI(TAG,"device id ok\r\n");
    }
    /*  复位设备 */
    ret = qmi8658_i2c_write_byte(QMI8658_RESET, 0xb0); 
    vTaskDelay(10 / portTICK_PERIOD_MS);
    /* 设置设备参数 */
    ret = qmi8658_i2c_write_byte(QMI8658_CTRL1, 0x40); // CTRL1 设置地址自动增加
    ret = qmi8658_i2c_write_byte(QMI8658_CTRL7, 0x03); // CTRL7 允许加速度和陀螺仪
    ret = qmi8658_i2c_write_byte(QMI8658_CTRL2, 0x95); // CTRL2 设置ACC 4g 250Hz
    ret = qmi8658_i2c_write_byte(QMI8658_CTRL3, 0xd5); // CTRL3 设置GRY 512dps 250Hz
    if(ESP_OK != ret){
        ret = -3;
        ESP_LOGE(TAG,"device set error\r\n");
    }
    else{
        ret = 0;
        ESP_LOGI(TAG,"device set OK\r\n");
    }
    Qmi8658_set_fifo_sample_size(qmi,size);  /* 设置fifo大小为16个采样数据 */
    Qmi8658_set_fifo_mode(qmi,mode);    /* 设置fifo工作模式为fifo */
    return ret;
}
/* 获取Qmi8658的加速度 */
void Qmi8658_get_acell(h_qmi qmi){
    int16_t tmp[3];
    uint8_t status = 0;
    qmi8658_i2c_read(QMI8658_STATUS0, &status, 1); /* 读取状态寄存器 */
    if(status & 0x01){  /* 如果数据准备好了 */
        qmi8658_i2c_read(QMI8658_AX_L,(uint8_t *)tmp,6);
        qmi->raw_data.ax_raw = tmp[0];
        qmi->raw_data.ay_raw = tmp[1];
        qmi->raw_data.az_raw = tmp[2];
        qmi->actural.ax = tmp[0] - qmi->cal_data.ax_cal;
        qmi->actural.ay = tmp[1] - qmi->cal_data.ay_cal;
        qmi->actural.az = tmp[2] - qmi->cal_data.az_cal;        
        if(!qmi->is_cal){
            ESP_LOGI(TAG,"data is uncal");
        }
    }

}
/* 获取Qmi8658的角速度 */
void Qmi8658_get_gyro(h_qmi qmi){
    int16_t tmp[3];
    uint8_t status = 0;
    qmi8658_i2c_read(QMI8658_STATUS0, &status, 1); /* 读取状态寄存器 */
    if(status & 0x02){  /* 如果数据准备好了 */
        qmi8658_i2c_read(QMI8658_GX_L,(uint8_t *)tmp,6);
        qmi->raw_data.gx_raw = tmp[0];
        qmi->raw_data.gy_raw = tmp[1];
        qmi->raw_data.gz_raw = tmp[2];
        qmi->actural.gx = tmp[0] - qmi->cal_data.gx_cal;
        qmi->actural.gy = tmp[1] - qmi->cal_data.gy_cal;
        qmi->actural.gz = tmp[2] - qmi->cal_data.gz_cal;        
        if(!qmi->is_cal){
            ESP_LOGI(TAG,"data is uncal");
        }
    }
}
/* 获取Qmi8658的加速度和角速度 */
uint8_t Qmi8658_get_ga(h_qmi qmi){
    uint8_t status = 0;
    int16_t tmp[6];
    if(qmi->fifo_enable){   /* 使用fifo */
        uint16_t sample_size = 0;
        qmi8658_i2c_read(QMI8658_FIFO_STATUS,&status,1);
        if(status & 0x80){
            sample_size = Qmi8658_get_fifo_sample_size() / (uint16_t)12;
            qmi8658_i2c_write_byte(QMI8658_CTRL9,0x05);
            while(1){
                qmi8658_i2c_read(QMI8658_STATUSINT,&status,1);
                if(status & 0x80){
                    qmi8658_i2c_write_byte(QMI8658_CTRL9,0x00);
                    break;
                }
            }
            while(sample_size--){
                qmi8658_i2c_read(QMI8658_FIFO_DATA,(uint8_t*)tmp,12);
            }
            Qmi8658_clear_fifo_r_mode();
            qmi->raw_data.ax_raw = tmp[0];
            qmi->raw_data.ay_raw = tmp[1];
            qmi->raw_data.az_raw = tmp[2];
            qmi->raw_data.gx_raw = tmp[3];
            qmi->raw_data.gy_raw = tmp[4];
            qmi->raw_data.gz_raw = tmp[5];
            if(!qmi->is_cal){
                ESP_LOGI(TAG,"data is calabrating");
                qmi->cal_data.ax_cal += (int32_t)qmi->raw_data.ax_raw;
                qmi->cal_data.ay_cal += (int32_t)qmi->raw_data.ay_raw;
                qmi->cal_data.az_cal += (int32_t)qmi->raw_data.az_raw;  
                qmi->cal_data.gx_cal += (int32_t)qmi->raw_data.gx_raw;
                qmi->cal_data.gy_cal += (int32_t)qmi->raw_data.gy_raw;
                qmi->cal_data.gz_cal += (int32_t)qmi->raw_data.gz_raw;   
            }
            else{
                qmi->actural.ax = qmi->raw_data.ax_raw - (int16_t)qmi->cal_data.ax_cal;
                qmi->actural.ay = qmi->raw_data.ay_raw - (int16_t)qmi->cal_data.ay_cal;
                qmi->actural.az = qmi->raw_data.az_raw - (int16_t)qmi->cal_data.az_cal;  
                qmi->actural.gx = qmi->raw_data.gx_raw - (int16_t)qmi->cal_data.gx_cal;
                qmi->actural.gy = qmi->raw_data.gy_raw - (int16_t)qmi->cal_data.gy_cal;
                qmi->actural.gz = qmi->raw_data.gz_raw - (int16_t)qmi->cal_data.gz_cal;   
            }
        }
        else{
            return 1;
        }
    }
    else{   /* 不使用fifo */
        qmi8658_i2c_read(QMI8658_STATUS0, &status, 1); /* 读取状态寄存器 */
        if(status & 0x03){  /* 如果数据准备好了 */
            qmi8658_i2c_read(QMI8658_AX_L,(uint8_t *)tmp,12);
            qmi->raw_data.ax_raw = tmp[0];
            qmi->raw_data.ay_raw = tmp[1];
            qmi->raw_data.az_raw = tmp[2];
            qmi->actural.ax = qmi->raw_data.ax_raw - qmi->cal_data.ax_cal;
            qmi->actural.ay = qmi->raw_data.ay_raw - qmi->cal_data.ay_cal;
            qmi->actural.az = qmi->raw_data.az_raw - qmi->cal_data.az_cal;  
            qmi->raw_data.gx_raw = tmp[3];
            qmi->raw_data.gy_raw = tmp[4];
            qmi->raw_data.gz_raw = tmp[5];
            qmi->actural.gx = qmi->raw_data.gx_raw - qmi->cal_data.gx_cal;
            qmi->actural.gy = qmi->raw_data.gy_raw - qmi->cal_data.gy_cal;
            qmi->actural.gz = qmi->raw_data.gz_raw - qmi->cal_data.gz_cal;     
            if(!qmi->is_cal){
                ESP_LOGI(TAG,"data is uncal");
            }
        }      
        else{
            return 1;
        }
    }
    return 0;
}
/* 矫正Qmi8658的加速度和角速度 */
uint8_t Qmi8658_cal_ga(h_qmi qmi){
    static uint16_t j = 0;
    if(j >= 10){
        qmi->is_cal = 1;
        qmi->cal_data.ax_cal /= j;
        qmi->cal_data.ay_cal /= j;
        qmi->cal_data.az_cal /= j;
        qmi->cal_data.gx_cal /= j;
        qmi->cal_data.gy_cal /= j;
        qmi->cal_data.gz_cal /= j;
        return ESP_OK;
    }
    else{
        if(!Qmi8658_get_ga(qmi)){
            ++j;
        }
        return ESP_FAIL;
    }
}

/* 获取角度 */




