#include <stdio.h>
#include "string.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* 按键gpio对应参数定义 */
#define KEY_PIN             GPIO_NUM_0
#define INPUT_PIN_SEL       (1ULL << KEY_PIN)    

/* 任务相应参数定义以及变量创建和函数声明 */
#define TASK_NAME           "key_check"
#define TASK_STACK_DEPTH    2048
#define TASK_PRIORITY       1
static TaskHandle_t       task1 = NULL;
static void key_check(void * parameters);
/* 队列变量创建 */
static QueueHandle_t        queue = NULL;
/* GPIO中断服务函数 */
static void IRAM_ATTR key_handler(void *arg){
    static uint32_t key_nums = 0;   /* 按键按下的次数 */ 
    if((uint32_t) arg == KEY_PIN){  /* 说明是该按键触发的外部中断 */
        key_nums++;
        xQueueSendFromISR(queue,&key_nums,NULL); /* 将按键按下的次数传递给队列 */
    }
}
/* gpio初始化函数 */
static void GPIO_Init(void){
    gpio_config_t boot_key = {
        .intr_type = GPIO_INTR_NEGEDGE,         /* 下降沿触发外部中断 */
        .mode = GPIO_MODE_INPUT,                /* 输入模式 */
        .pin_bit_mask = INPUT_PIN_SEL,         /* 引脚口定义为IO0 */
        .pull_down_en = 0,                      /* 下拉电阻失能 */
        .pull_up_en = 1,                        /* 上拉电阻使能 */
    };
    gpio_config(&boot_key); /* 配置gpio */
    gpio_install_isr_service(0);    /* 安装中断服务 */
    gpio_isr_handler_add(KEY_PIN,key_handler,(void*)(KEY_PIN));
}


void app_main(void)
{
    /* 初始化GPIO */
    GPIO_Init();
    /* 创建队列 */
    queue = xQueueCreate(5,sizeof(uint32_t));   /* 创建队列，设置队列缓冲区为5个32字节变量 */
    if(queue == NULL){
        printf("queue created failed \r\n");
        vTaskDelete(NULL);
        return ;
    }
    /* 创建任务 */
    if(pdPASS != xTaskCreate(key_check,TASK_NAME,TASK_STACK_DEPTH,NULL,TASK_PRIORITY,&task1)){
        printf("Task created failed \r\n");
        vTaskDelete(NULL);
        return ;
    }
    printf("Task created successful ,app is running \r\n");
    fflush(stdout);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    /* 删除自身 */
    vTaskDelete(NULL);

}

static void key_check(void * parameters){
    uint32_t key_nums = 0;
    for(;;){
        if(xQueueReceive(queue,&key_nums,1000/portTICK_PERIOD_MS) == pdPASS){
            printf("boot key has pressed & key_nums = %ld\r\n",key_nums);
        }
        else{
            printf("boot key is unpress & key_nums = %ld\r\n",key_nums);    
        }
        fflush(stdout);
    }
}