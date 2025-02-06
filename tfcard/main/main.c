#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#define Tag             "SD"
#define MOUNT_PONIT     "/sdcard"   /* 挂载点路径 */
#define BSP_SD_CLK      47
#define BSP_SD_CMD      48
#define BSP_SD_D0       21

static sdmmc_card_t *tf_card;    /*  创建sdmmc对象 */

/* 挂载SD卡 */
static void sd_card_mount(void){
    esp_err_t   ret = 0;
    sdmmc_host_t    host_cfg = SDMMC_HOST_DEFAULT(); /* SDMMC的主机接口配置 */
    sdmmc_slot_config_t slot_cfg = SDMMC_SLOT_CONFIG_DEFAULT(); /* SDMMC的插槽配置 */
    esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed = true,         // 如果挂载失败，是否该sd卡格式化为fat分区
        .max_files  = 5,                        // 最大可以打开的文件数目
        .allocation_unit_size =   16 * 1024,    // 格式化时的分配单元大小
    };

    slot_cfg.width = 1; /* 一线sd模式 */
    slot_cfg.clk = BSP_SD_CLK;
    slot_cfg.cmd = BSP_SD_CMD;
    slot_cfg.d0  = BSP_SD_D0;
    slot_cfg.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    ret = esp_vfs_fat_sdmmc_mount(MOUNT_PONIT,&host_cfg,&slot_cfg,&mount_cfg,&tf_card);
    if(ret == ESP_OK){
        ESP_LOGI(Tag,"tf card mount successful\r\n");
    }
    else{
        if(ret == ESP_FAIL){
            ESP_LOGE(Tag,"tf card mount failed\r\n");
        }
        else{
            ESP_LOGE(Tag,"tf card init failed(%s)\r\n",esp_err_to_name(ret));
        }
    }
}
/* 在SD卡的文件中写入数据 */
static esp_err_t sd_card_w_file(const char* path,char*data){
    FILE * f = fopen(path,"w"); /* 打开文件路径 */
    if(f == NULL){
        ESP_LOGE(Tag,"file open failed\r\n");
        return ESP_FAIL;
    }
    fprintf(f,data);
    fclose(f);
    ESP_LOGI(Tag,"file write successful\r\n");
    return ESP_OK;
}
/* 在SD卡的文件中读取指定字节数的数据 */
static esp_err_t sd_card_r_file(const char* path,char*data,uint32_t len){
    FILE * f = fopen(path,"r"); /* 打开文件路径 */
    uint32_t read_size = 0;
    if(f == NULL){
        ESP_LOGE(Tag,"file open failed\r\n");
        return ESP_FAIL;
    }
    read_size = fread(data,1,len,f);
    if(read_size != len){
        if(feof(f)){
            ESP_LOGI(Tag,"end of file\r\n");
        }
        else if(ferror(f)){
            ESP_LOGI(Tag,"read file failed\r\n");
            fclose(f);
            return ESP_FAIL;
        }
    }
    fclose(f);
    ESP_LOGI(Tag,"file read successful\r\n");
    return ESP_OK;
}
/* 读取文件的大小 */
static esp_err_t sd_card_get_file_size(const char* path,long* size){
    FILE * f = fopen(path,"r");
    if(f == NULL){
        ESP_LOGE(Tag,"file open failed\r\n");
        return ESP_FAIL;
    }
    if(fseek(f,0,SEEK_END)){ /* 将文件指针指向文件末尾 */
        ESP_LOGE(Tag,"file pointer seek failed\r\n");
        fclose(f);
        return ESP_FAIL;
    }   
    *size = ftell(f);
    if(*size == -1){
        ESP_LOGE(Tag,"file size get failed\r\n");
        fclose(f);
        return ESP_FAIL;
    }
    fclose(f);
    ESP_LOGI(Tag,"file size get successful\r\n");
    return ESP_OK;
} 


char tmp[255] = {0};
long file_size = 0;
void app_main(void)
{
    sd_card_mount();
    char * file_test = MOUNT_PONIT "/test.txt";     /* 指定文件路径 */
    sd_card_w_file(file_test,"Test测试");  /* 在文件中写入数据 */
    sd_card_get_file_size(file_test,&file_size);
    if(file_size != -1){
        sd_card_r_file(file_test,tmp,file_size);
    }
    printf("file size : %ld \r\nread data : %s\r\n",file_size,tmp); 

    fflush(stdout);
    esp_vfs_fat_sdmmc_unmount();
    while(1){
           
    }
}
