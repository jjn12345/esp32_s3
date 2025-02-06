#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#define NVS_SET_U16     1
#define NVS_SET_BLOB    0
#define NVS_CONFIG_USER_PARTITION   0

#define u16_key     "my_u16"
#define blob_key    "my_blob"
#define partition_lable "my_nvs"
static const char name_space[] = "my_folder";
static nvs_handle_t my_nvs_handle;

static void mem_print(const unsigned char * address,size_t len){
    for(unsigned char i = 0;i<len;i++){
        printf("%.2x ",address[i]);
    }
    printf("\n");
}
/* 通过nvs迭代器 打印nvs分区中存在的键值对*/
static void nvs_key_val_print(void){
    nvs_iterator_t  it = NULL;
    esp_err_t res = ESP_OK;  
    #if NVS_CONFIG_USER_PARTITION
        nvs_flash_init_partition(partition_lable);    /* 初始化自己创建的nvs分区 */
        res = nvs_entry_find(partition_lable,name_space,NVS_TYPE_ANY,&it);
    #else
        nvs_flash_init(); /* 初始化nvs */
        res = nvs_entry_find("nvs",name_space,NVS_TYPE_ANY,&it);
    #endif
    while(res == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info); // Can omit error check if parameters are guaranteed to be non-NULL
        printf("key '%s', type '%d' \n", info.key, info.type);
        res = nvs_entry_next(&it);
    }
    #if NVS_CONFIG_USER_PARTITION
    nvs_flash_deinit_partition(partition_lable);     /* 去初始化nvs */
    #else
    nvs_flash_deinit();     /* 去初始化nvs */
    #endif      
    fflush(stdout);
}
/* 打印nvs的统计信息 */
static void nvs_state_print(void){
    nvs_stats_t nvs_stats;
    #if NVS_CONFIG_USER_PARTITION
        nvs_flash_init_partition(partition_lable);    /* 初始化自己创建的nvs分区 */
        nvs_get_stats(partition_lable, &nvs_stats);
    #else
        nvs_flash_init(); /* 初始化nvs */
        nvs_get_stats(NULL, &nvs_stats);
    #endif    
    printf("Count: UsedEntries = (%d), FreeEntries = (%d), AvailableEntries = (%d), AllEntries = (%d)\n",
    nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.available_entries, nvs_stats.total_entries);
    #if NVS_CONFIG_USER_PARTITION
    nvs_flash_deinit_partition(partition_lable);     /* 去初始化nvs */
    #else
    nvs_flash_deinit();     /* 去初始化nvs */
    #endif      
}

static nvs_type_t  key_type = NVS_TYPE_ANY;
#define MAX_AP   2
typedef struct AP{
    char ssid[50];
    char pwd[50];
} ap_t;

static void nvs_sample(void){
    esp_err_t   ret = ESP_OK;
    #if NVS_SET_U16
        uint16_t    boot_counter = 0;
        const char key[] = u16_key;
    #endif
    #if NVS_SET_BLOB
        const char key[] = blob_key;
        ap_t aps_get[MAX_AP] = {0};
        ap_t aps_set[MAX_AP] = {0};
    #endif
    #if NVS_CONFIG_USER_PARTITION
    ret = nvs_flash_init_partition(partition_lable);    /* 初始化自己创建的nvs分区 */
    #else
    ret = nvs_flash_init(); /* 初始化nvs */
    #endif
    ESP_ERROR_CHECK(ret);

    #if NVS_CONFIG_USER_PARTITION
    ret = nvs_open_from_partition(partition_lable,name_space,NVS_READWRITE,&my_nvs_handle); /* 打开用户创建的nvs分区中的命名空间*/
    #else
    ret = nvs_open(name_space,NVS_READWRITE,&my_nvs_handle);   /* 打开nvs中对应的命名空间 */
    #endif
    ESP_GOTO_ON_ERROR(ret,err,"nvs","nvs open fail");       
    ret = nvs_find_key(my_nvs_handle,key,&key_type);     /* 查找对应的键,如果没有则写入该键*/
    if(ret != ESP_OK){
        if(ret == ESP_ERR_NVS_NOT_FOUND){
            #if NVS_SET_U16
                ret = nvs_set_u16(my_nvs_handle,key,0x00);   /*  对nvs写入该键值*/
            #endif
            #if NVS_SET_BLOB
                for(int i = 0;i<MAX_AP;i++){
                    strcpy(aps_set[i].ssid,"abc");
                    strcpy(aps_set[i].pwd,"123456");
                }
                ret = nvs_set_blob(my_nvs_handle,key,aps_set,sizeof(aps_set));   /*  对nvs写入blob*/
                ESP_GOTO_ON_ERROR(ret,err,"nvs","nvs set fail");
                nvs_commit(my_nvs_handle);   /* 强制执行nvs的写入 */
            #endif
            ESP_GOTO_ON_ERROR(ret,err,"nvs","nvs set fail");
        }
    }
    else{
        #if NVS_SET_U16
            ret = nvs_get_u16(my_nvs_handle,key,&boot_counter);    /*  对nvs读取u16*/
            ESP_GOTO_ON_ERROR(ret,err,"nvs","nvs get fail");
            ret = nvs_set_u16(my_nvs_handle,key,++boot_counter);   /*  对nvs写入u16*/
            ESP_GOTO_ON_ERROR(ret,err,"nvs","nvs set fail");
            nvs_commit(my_nvs_handle);   /* 强制执行nvs的写入 */
            ESP_LOGI("nvs","boot counter : %d",boot_counter);
            mem_print((unsigned char*)&boot_counter,2);
        #endif
        #if NVS_SET_BLOB       
            size_t len = sizeof(aps_get);
            ret = nvs_get_blob(my_nvs_handle,key,aps_get,&len);    /*  对nvs读取blob*/
            ESP_GOTO_ON_ERROR(ret,err,"nvs","nvs get fail");
            for(int i = 0;i<MAX_AP;i++){
                ESP_LOGI("nvs","ssid:pwd   %s:%s",aps_get[i].ssid,aps_get[i].pwd);
            }
        #endif
    }
    nvs_close(my_nvs_handle);  /* 关闭nvs */
err:
    if(ESP_OK != ret){
        ESP_LOGE("nvs","error: %s",esp_err_to_name(ret));
    }
    #if NVS_CONFIG_USER_PARTITION
    nvs_flash_deinit_partition(partition_lable);     /* 去初始化nvs */
    #else
    nvs_flash_deinit();     /* 去初始化nvs */
    #endif    
}


void app_main(void)
{
    nvs_sample();
    nvs_key_val_print();
    nvs_state_print();
}
