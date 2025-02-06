#include <stdio.h>
#include "sdkconfig.h"
#include "string.h"
#include "es7210.h"
#include "driver/i2s_tdm.h"
#include "driver/i2c.h"
#include "esp_vfs_fat.h"        /* 用于esp文件系统的轻量级fat驱动 */
#include "format_wav.h"
#include "driver/sdmmc_host.h"  
#include "sdmmc_cmd.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"

/* I2C */
#define BSP_I2C_SDA     GPIO_NUM_1
#define BSP_I2C_SCL     GPIO_NUM_2
#define BSP_I2C_PORT    I2C_NUM_0
#define I2C_CLK_SPEED   100000

/* 一线模式 SD */
#define BSP_SD_D0       GPIO_NUM_21
#define BSP_SD_CMD      GPIO_NUM_48
#define BSP_SD_CLK      GPIO_NUM_47

/* I2S Rx 物理属性 */
#define BSP_I2S_MCLK    GPIO_NUM_38     /* I2S主时钟 */
#define BSP_I2S_BCK     GPIO_NUM_14     /* I2S位时钟即时序时钟 */
#define BSP_I2S_WS      GPIO_NUM_13     /* I2S帧时钟即控制左右声道 */
#define BSP_I2S_DI      GPIO_NUM_12     /* I2S数据输入 */
/* I2S 配置属性 */
#define I2S_TDM_SLOT_MASK   (I2S_TDM_SLOT0 | I2S_TDM_SLOT1)


/* es7210 配置属性 */
#define ES7210_I2C_ADR          0x41        
#define ES7210_I2S_BIT_WIDTH    ES7210_I2S_BITS_16B
#define ES7210_I2S_FORMATE      ES7210_I2S_FMT_I2S
#define ES7210_I2S_MCLK_RATIO   I2S_MCLK_MULTIPLE_256
#define ES7210_I2S_SAMPLE_RATE  48000
/* 日志头 */
#define Tag     "mic"
/* 挂载点 */
#define MOUNT_POINT "/tfcard"
/* WAV 音频文件属性 */
#define I2S_CHAN_NUM            (2) /* I2S通道 */
#define RECODER_TIME_SEC        10  /* 录音时间(s) */
#define WAV_PATH                "/my_wav.wav"   /* 音频文件的路径 */

i2s_chan_handle_t   i2s_rx_chan = NULL;

static esp_err_t sd_mount(void){

    // sdmmc_card_t  *tf_card = (sdmmc_card_t*)malloc(sizeof(sdmmc_card_t));
    sdmmc_card_t  *tf_card;
    /* 配置sdmmc的主机属性 */
    /* 主要是配置SDIO电压、SD槽位号、最大频率、支持的SD协议、高速情况下的皮秒延时以及一堆命令函数 */
    sdmmc_host_t host_cfg = SDMMC_HOST_DEFAULT();    
    host_cfg.flags = SDMMC_HOST_FLAG_1BIT;  
    /* 配置sdmmc的槽位属性 */
    sdmmc_slot_config_t slot_cfg = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_cfg.d0 = BSP_SD_D0;
    slot_cfg.clk = BSP_SD_CLK;
    slot_cfg.cmd = BSP_SD_CMD;  
    slot_cfg.width = 1; /* SD的总线宽度 */   
    /* 
        如果使用esp_vfs_fat_sdmmc_mount挂载SD卡,
            1.  就不能使用以下函数，会导致挂载时的主机初始化失败,
            2.  sdmmc_card_t  * tfcard 不需要额外分配内存
        {
            // 初始化主机 
            ESP_RETURN_ON_ERROR(sdmmc_host_init(),"SD","host init failed"); 
            // 初始化槽位 
            ESP_RETURN_ON_ERROR(sdmmc_host_init_slot(SDMMC_HOST_SLOT_1,&slot_cfg),"SD","slot init failed");
            // 初始化SD卡 
            ESP_RETURN_ON_ERROR(sdmmc_card_init(&host_cfg,tf_card),"SD","card init failed");
        }
    */
    
    esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed = true, /* 打开失败后,是否格式化SD卡 */
        .max_files = 5, /* 可以打开的最大文件数 */   
        .allocation_unit_size = 1024 * 16,  /* 分配单元大小 */
    };
    ESP_RETURN_ON_ERROR(esp_vfs_fat_sdmmc_mount(MOUNT_POINT,&host_cfg,&slot_cfg,&mount_cfg,&tf_card),\
                        "SD","card mount failed");
    sdmmc_card_print_info(stdout,tf_card);
    return ESP_OK;
}
static esp_err_t es7210_config(void){
    /* i2c配置 */
    i2c_config_t    i2c_cfg = {
        .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL,   /* 任何一个时钟源都可以选择 */
        .master.clk_speed = I2C_CLK_SPEED,
        .mode = I2C_MODE_MASTER,
        .scl_io_num = BSP_I2C_SCL,
        .sda_io_num = BSP_I2C_SDA,
        .scl_pullup_en = 1,
        .sda_pullup_en = 1,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(BSP_I2C_PORT,&i2c_cfg),"ES7210","i2c config failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(BSP_I2C_PORT,i2c_cfg.mode,0,0,0),\
                        "ES7210","i2c driver install failed");
    /* es7210配置 */
    es7210_dev_handle_t es7210_handle = NULL;
    es7210_i2c_config_t es7210_i2c_cfg = {
        .i2c_addr = ES7210_I2C_ADR,
        .i2c_port = BSP_I2C_PORT,
    };
    ESP_RETURN_ON_ERROR(es7210_new_codec(&es7210_i2c_cfg,&es7210_handle),\
                        "ES7210","es7210 obj created failed");
    es7210_codec_config_t es7210_cfg = {
        .bit_width = ES7210_I2S_BIT_WIDTH,  /* I2S 有效数据位宽 */
        .flags.tdm_enable = true,           /* I2S 是否使用TDM */
        .i2s_format = ES7210_I2S_FORMATE,   /* 标准I2S模式 */
        .mclk_ratio = ES7210_I2S_MCLK_RATIO,/* 主时钟跟采样率的比例系数 */
        .mic_bias = ES7210_MIC_BIAS_2V87,   /* MIC旁路的电压 */ 
        .mic_gain = ES7210_MIC_GAIN_30DB,   /* MIC的增益 */
        .sample_rate_hz = ES7210_I2S_SAMPLE_RATE,   /* I2S 采样频率 */
    };
    ESP_RETURN_ON_ERROR(es7210_config_codec(es7210_handle,&es7210_cfg),\
                    "ES7210","es7210 codec config failed");
    ESP_RETURN_ON_ERROR(es7210_config_volume(es7210_handle,0),\
                    "ES7210","es7210 codec config failed");  
    return   ESP_OK;                                          
}
static esp_err_t i2s_init(i2s_chan_handle_t  *i2s_chan){
    i2s_chan_config_t i2s_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO,I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&i2s_chan_cfg,NULL,i2s_chan),\
                        "I2S","i2s channel allocate failed");
    i2s_tdm_config_t  i2s_tdm_cfg = {
        .clk_cfg = {
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = ES7210_I2S_MCLK_RATIO,
            .sample_rate_hz = ES7210_I2S_SAMPLE_RATE,
        },
        .gpio_cfg = {
            .bclk = BSP_I2S_BCK,
            .din = BSP_I2S_DI,
            .dout = -1,
            .mclk = BSP_I2S_MCLK,
            .ws = BSP_I2S_WS,
        },
        .slot_cfg = I2S_TDM_PHILIPS_SLOT_DEFAULT_CONFIG(ES7210_I2S_BIT_WIDTH,I2S_SLOT_MODE_STEREO,I2S_TDM_SLOT_MASK),
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_tdm_mode(*i2s_chan,&i2s_tdm_cfg),\
                    "I2S","i2s channel inti tdm mode failed");
    return ESP_OK;
}
static esp_err_t wav_recording(i2s_chan_handle_t  i2s_chan){
    esp_err_t ret = ESP_OK; /* 用于下方的ESP_GOTO_ON_ERROR */
    /* 在单位时间内采样的字节数 */
    uint32_t sample_rate_byte =  ES7210_I2S_SAMPLE_RATE * I2S_CHAN_NUM * ES7210_I2S_BIT_WIDTH / 8;
    /* 在10s内采样的字节数,即音频文件的大小 */
    uint32_t wav_size = sample_rate_byte * RECODER_TIME_SEC;
    /* 在文件中写入wav音频的头数据 */
    wav_header_t wav_head = WAV_HEADER_PCM_DEFAULT(wav_size,ES7210_I2S_BIT_WIDTH,ES7210_I2S_SAMPLE_RATE,I2S_CHAN_NUM);
    FILE * f = fopen(MOUNT_POINT WAV_PATH,"w");
    if(NULL == f){
        ESP_LOGE(Tag,"wav file open failed\r\n");
        return ESP_FAIL;
    }
    ESP_GOTO_ON_FALSE(fwrite(&wav_head,sizeof(wav_head),1,f),ESP_FAIL,err,Tag,"wav head writen failed");
    /* 开始录音 */
    size_t read_size = 0;
    size_t all_read_size = 0;
    static int16_t  wav_buf[4096] = {0};
    i2s_channel_enable(i2s_chan);   /* 使能i2s通道 */
    while(all_read_size < wav_size){
        /* 录音倒计时 */
        if(all_read_size % sample_rate_byte < sizeof(wav_buf)){ /* 判断剩余的可读取的字节数大小是否小于开辟空间的大小 */
            ESP_LOGI(Tag,"recording: %"PRIu32"/%d",all_read_size / sample_rate_byte + 1,RECODER_TIME_SEC );
        }
        /* 从i2s通道中获取数据,即接收录音数据 */
        ESP_GOTO_ON_ERROR(i2s_channel_read(i2s_chan,wav_buf,sizeof(wav_buf),\
                          &read_size,pdMS_TO_TICKS(1000)),err,Tag,"error while reading samples from i2s");
        ESP_GOTO_ON_FALSE(fwrite(wav_buf,read_size,1,f),ESP_FAIL,err,Tag,"error while writting samples to wav file");
        all_read_size += read_size;
    }
err:
    i2s_channel_disable(i2s_chan);
    ESP_LOGI(Tag, "Recording done! Flushing file buffer");
    fclose(f);
    return ret;
}

void app_main(void)
{
    /* 挂载SD卡 */
    if(ESP_OK == sd_mount()){
        ESP_LOGI("SD","sd card mount successful\r\n");
    }
    else{
        vTaskDelete(NULL);
        return;
    }
    /* 初始化i2s */
    if(ESP_OK == i2s_init(&i2s_rx_chan)){
        ESP_LOGI("I2S","i2s init successful\r\n");
    }
    else{
        vTaskDelete(NULL);
        return;
    }
    /* 配置es7210 */
    if(ESP_OK == es7210_config()){
        ESP_LOGI("ES7210","es7210 init successful\r\n");
    }
    else{
        vTaskDelete(NULL);
        return;
    }
    if(ESP_OK == wav_recording(i2s_rx_chan)){
        ESP_LOGI(Tag,"wav recording successful\r\n");
    }
    else{
        vTaskDelete(NULL);
        return;
    }

    while(1){
        printf("\r\nnothing\r\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
