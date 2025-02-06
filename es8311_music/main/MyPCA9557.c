#include "MyPCA9557.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
/*
    PCA9557 通过I2C协议扩展8个IO口
*/
typedef struct {
    i2c_port_t port;
    uint16_t dev_addr;
}pca9557_dev_t;

static char Tag[]   = "PCA9557";
/* PCA9557 I2C_Init */
#if PCA9557_BSP_I2C_CONFIG
static esp_err_t   pca9557_bsp_i2c_init(pca9557_handle_t dev){
    pca9557_dev_t * pca = (pca9557_dev_t*)dev;
    i2c_config_t i2c_cfg = {
        .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL,
        .master.clk_speed = PCA9557_BSP_I2C_SPEED,
        .mode = I2C_MODE_MASTER,
        .scl_io_num = PCA9557_BSP_I2C_SCL,
        .sda_io_num = PCA9557_BSP_I2C_SDA,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(pca->port,&i2c_cfg),\
                        Tag,"i2c init failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(pca->port,i2c_cfg.mode,0,0,0),\
                        Tag,"i2c driver insatll failed");     
    return ESP_OK;
}
#endif



/* PCA9557 I2C_write/read */
static esp_err_t pca9557_bsp_i2c_write_reg(pca9557_handle_t dev,const uint8_t reg,uint8_t data){
    pca9557_dev_t * pca = (pca9557_dev_t*)dev;
    uint8_t writern[] = {reg,data};

    ESP_RETURN_ON_ERROR(i2c_master_write_to_device(pca->port,pca->dev_addr,writern,sizeof(writern),\
                        pdTICKS_TO_MS(1000)),Tag,"i2c write failed");
  
    return ESP_OK;
}

static esp_err_t pca9557_bsp_i2c_read_reg(pca9557_handle_t dev,const uint8_t reg,uint8_t* data){
    pca9557_dev_t * pca = (pca9557_dev_t*)dev;

    ESP_RETURN_ON_ERROR(i2c_master_write_read_device(pca->port,pca->dev_addr,&reg,1,\
                        data,1,pdTICKS_TO_MS(1000)),Tag,"i2c write failed");
    return ESP_OK;
}


/* PCA9557 gpio config */
esp_err_t pca9557_gpio_config(pca9557_handle_t dev,const uint8_t gpio,const uint8_t mode){
    uint8_t cfg_data = 0;
    ESP_RETURN_ON_ERROR(pca9557_bsp_i2c_read_reg(dev,pca9557_configuration_reg,&cfg_data),\
                        Tag,"config failed");
    cfg_data = (mode==pca_input?(cfg_data|gpio):(cfg_data&(~gpio)));        
    ESP_RETURN_ON_ERROR(pca9557_bsp_i2c_write_reg(dev,pca9557_configuration_reg,cfg_data),\
                        Tag,"config failed");
    return ESP_OK;
}
/* PCA9557 gpio write level */
esp_err_t pca9557_gpio_write(pca9557_handle_t dev,const uint8_t gpio,const uint8_t level){
    uint8_t cfg_data = 0;
    ESP_RETURN_ON_ERROR(pca9557_bsp_i2c_read_reg(dev,pca9557_output_port_reg,&cfg_data),\
                        Tag,"level write failed");
    cfg_data = (level?(cfg_data|gpio):(cfg_data&(~gpio)));
    ESP_RETURN_ON_ERROR(pca9557_bsp_i2c_write_reg(dev,pca9557_output_port_reg,cfg_data),\
                        Tag,"level write failed");
    return ESP_OK;
}
/* PCA9557 gpio read level */
esp_err_t pca9557_gpio_read(pca9557_handle_t dev,const uint8_t gpio,uint8_t *level){
    ESP_RETURN_ON_ERROR(pca9557_bsp_i2c_read_reg(dev,pca9557_input_port_reg,level),\
                        Tag,"level read failed");
    return ESP_OK;
}
/* PCA9557 Init */
esp_err_t pca9557_init(pca9557_handle_t dev,uint8_t init_val){
    #if PCA9557_BSP_I2C_CONFIG
        ESP_RETURN_ON_ERROR(pca9557_bsp_i2c_init(dev),Tag,"err code: 1");
    #endif
    /* 将初始值给到扩展io口 */
    ESP_RETURN_ON_ERROR(pca9557_gpio_config(dev,0xff,pca_output),Tag,"err code : 2");
    ESP_RETURN_ON_ERROR(pca9557_gpio_write(dev,0xff,pca_gpio_low),Tag,"err code : 2");
    ESP_RETURN_ON_ERROR(pca9557_gpio_write(dev,init_val,pca_gpio_high),Tag,"err code : 2");
    /* 将gpio恢复为默认值 */
    ESP_RETURN_ON_ERROR(pca9557_gpio_config(dev,0xff,pca_input),Tag,"err code: 3");
    return ESP_OK;
}
/* pca9557 dev create */
pca9557_handle_t pca9557_create(i2c_port_t port ,const uint16_t addr){
    pca9557_dev_t * pca = calloc(1,sizeof(pca9557_dev_t));
    pca->dev_addr = addr;
    pca->port = port;
    return (pca9557_handle_t)pca;
} 
/* pca9557 dev delete */
void pca9557_delete(pca9557_handle_t dev){
    free(dev);
} 