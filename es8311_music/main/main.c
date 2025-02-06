#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2s_std.h"     /* esp-idf 的新驱动 */
#include "driver/i2c.h"
#include "MyPCA9557.h"
#include "es8311.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* i2c config */
#define BSP_I2C_PORT    (I2C_NUM_0)
#define BSP_I2C_SDA     (GPIO_NUM_1)
#define BSP_I2C_SCL     (GPIO_NUM_2)
#define BSP_I2C_SPEED   (200000)

/* i2s config */
#define BSP_I2S_NUM             (I2S_NUM_0)                 /* I2S0 */
#define BSP_I2S_ROLE            (I2S_ROLE_MASTER)           /* 主模式 */
#define BSP_I2S_SAMPLE_RATE     (16000)                     /* 采样率 */
#define BSP_I2S_MCLK_MULTIPLE   (256)                       /* MCLK和采样频率的比值 */
#define BSP_I2S_BITS_PER_SAMPLE (I2S_DATA_BIT_WIDTH_16BIT)  /* 数据宽度 */
#define BSP_I2S_SLOT_MODE       (I2S_SLOT_MODE_STEREO)      /* 立体声 */

#define BSP_I2S_BCLK            (GPIO_NUM_14)    
#define BSP_I2S_MCLK            (GPIO_NUM_38)
#define BSP_I2S_DO              (GPIO_NUM_45)
#define BSP_I2S_WS              (GPIO_NUM_13)

#define I2S_MCLK_FREQ           (BSP_I2S_SAMPLE_RATE * BSP_I2S_MCLK_MULTIPLE)   /* MCLK的时钟频率 */


static i2s_chan_handle_t i2s_tx_handle = NULL;
static pca9557_handle_t pca = NULL;
static es8311_handle_t  es = NULL;

static esp_err_t bsp_i2c_init(void){
    char Tag[] = "BSP";
    i2c_config_t i2c_cfg = {
    .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL,
    .master.clk_speed = BSP_I2C_SPEED,
    .mode = I2C_MODE_MASTER,
    .scl_io_num = BSP_I2C_SCL,
    .sda_io_num = BSP_I2C_SDA,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(BSP_I2C_PORT,&i2c_cfg),\
                        Tag,"i2c init failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(BSP_I2C_PORT,i2c_cfg.mode,0,0,0),\
                        Tag,"i2c driver insatll failed");     
    return ESP_OK;
}
static esp_err_t bsp_i2s_init(void){
    char Tag[] = "BSP";
    i2s_chan_config_t i2s_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(BSP_I2S_NUM,BSP_I2S_ROLE);
    i2s_chan_cfg.auto_clear = true; /* 设置 dma 发送缓冲区自动清空 */
    ESP_RETURN_ON_ERROR(i2s_new_channel(&i2s_chan_cfg,&i2s_tx_handle,NULL),Tag,"i2s chan new failed");

    i2s_std_config_t i2s_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(BSP_I2S_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(BSP_I2S_BITS_PER_SAMPLE,BSP_I2S_SLOT_MODE),
        .gpio_cfg = {
            .bclk = BSP_I2S_BCLK,
            .din = -1,
            .dout = BSP_I2S_DO,
            .mclk = BSP_I2S_MCLK,
            .ws = BSP_I2S_WS,
        },
    };
    i2s_std_cfg.clk_cfg.mclk_multiple= BSP_I2S_MCLK_MULTIPLE;     
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(i2s_tx_handle,&i2s_std_cfg),Tag,"i2s init std mode failed");

    return ESP_OK;
}
static esp_err_t bsp_pca9557_init(void){
    pca = pca9557_create(BSP_I2C_PORT,pca9557_addr1);
    ESP_RETURN_ON_FALSE(pca,ESP_FAIL,"PCA9557","dev create failed");
    return pca9557_init(pca,0x05);
}
static esp_err_t bsp_es8311_init(void){
    es = es8311_create(BSP_I2C_PORT,ES8311_ADDRRES_0);
    ESP_RETURN_ON_FALSE(pca,ESP_FAIL,"ES8311","dev create failed");
    es8311_clock_config_t es_clk_cfg = {
        .mclk_frequency = I2S_MCLK_FREQ,
        .mclk_from_mclk_pin = true, /* 因为采用主模式，所以sclk和lrclk由mclk进行分化 */
        .mclk_inverted = false,
        .sclk_inverted = false,
        .sample_frequency = BSP_I2S_SAMPLE_RATE,
    };
    ESP_RETURN_ON_ERROR(es8311_init(es,&es_clk_cfg,ES8311_RESOLUTION_16,ES8311_RESOLUTION_16),\
                        "ES8311","init failed");
    ESP_RETURN_ON_ERROR(es8311_sample_frequency_config(es,I2S_MCLK_FREQ,BSP_I2S_SAMPLE_RATE),\
                        "ES8311","sample frequency config failed");
    ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es,80,NULL),\
                        "ES8311","voice volume set failed");
    ESP_RETURN_ON_ERROR(es8311_microphone_config(es,false),\
                        "ES8311","microphone config failed");
    ESP_RETURN_ON_ERROR(es8311_microphone_gain_set(es,ES8311_MIC_GAIN_12DB),\
                        "ES8311","microphone gain set failed");
    return ESP_OK;
}


/* 
    因为直接通过一个数组去包含pcm文件中的数据并不实际，因为pcm数据可能非常大。
    所以会采用将pcm数据作为二进制文件进行处理，然后在链接的时候，将他包括进来
    所以下面这两句代码就是为了告诉链接器将这个PCM数据作为二进制对象包含到最终
    可执行的文件中去,而这个_binary_canon_pcm_start(举例)就是链接器产生的符号,
    它指向这个PCM数据的起始。
    需要再CMakeLists中添加EMBED_FILES "canon.pcm"
    这表示在组件中嵌入二进制数据文件
 */
extern const uint8_t music_pcm_start[]  asm("_binary_canon_pcm_start");
extern const uint8_t music_pcm_end[]    asm("_binary_canon_pcm_end");

static void music(void * parameter){
    uint8_t * data_ptr = (uint8_t*)music_pcm_start;   /* 创建指针指向pcm数据的起始 */
    esp_err_t ret = ESP_OK;
    size_t  writen = 0;
    /* 开启音频功放 */
    pca9557_gpio_config(pca,pca9557_gpio_num_1,pca_output);
    pca9557_gpio_write(pca,pca9557_gpio_num_1,pca_gpio_high);
    /* 通过预加载数据,可以将数据及时的发送出去 */
    ESP_ERROR_CHECK(i2s_channel_preload_data(i2s_tx_handle,data_ptr,music_pcm_end - data_ptr,&writen));
    data_ptr += writen;

    /* 使能通道 */
    ESP_ERROR_CHECK(i2s_channel_enable(i2s_tx_handle));
    for(;;){
        ret =  i2s_channel_write(i2s_tx_handle,data_ptr,music_pcm_end - data_ptr,&writen,pdMS_TO_TICKS(1000));
        if(ESP_OK != ret){
            ESP_LOGE("MUSIC","TIME_OUT");
            i2s_channel_disable(i2s_tx_handle);
            break;
        }
        if(writen > 0){
            ESP_LOGI("MUSIC", "music played, %d bytes are written.", writen);
        } 
        else{
            ESP_LOGE("MUSIC","music play failed");
            i2s_channel_disable(i2s_tx_handle);
            break;
        }  
        data_ptr = (uint8_t *)music_pcm_start;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    /* i2c */
    bsp_i2c_init();
    /* i2s */
    bsp_i2s_init();
    /* pca9557 */   
    bsp_pca9557_init();
    /* es8311 */    
    bsp_es8311_init();

    /* create music task */
    xTaskCreate(music,"MUSIC",4096,NULL,1,NULL);

    /* delete self */
    vTaskDelete(NULL);
}
