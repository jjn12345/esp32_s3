#include <stdio.h>
#include "MyQmi8658.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define Tag   "APP"
static qmi8658_handler   my_qmi; 
#define TASK_NAME           "GET_ACC_GYRO"
#define TASK_STACK_DEPTH    2048
#define TASK_PRIORITY       5
static TaskHandle_t task1;
static void get_acc_gyro(void*parameters){
    for(;;){
        Qmi8658_get_ga(&my_qmi);
        printf("\r ax: %hd \r ay: %hd \r az: %hd\r",my_qmi.actural.ax,my_qmi.actural.ay,my_qmi.actural.az);
        printf(" gx: %hd \r gy: %hd \r gz: %hd\r",my_qmi.actural.gx,my_qmi.actural.gy,my_qmi.actural.gz);
        fflush(stdout);
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    int ret = 0;
    ret = Qmi8658_Init(&my_qmi,fifo,fifo_16_sample);
    if(ret){
        ESP_LOGE(Tag,"qmi8658 init error ret = %d\r\n", ret);
    }
    else{
        ESP_LOGI(Tag,"qmi8658 init ok ret = %d \r\n", ret);
    }
    while(Qmi8658_cal_ga(&my_qmi)){
        vTaskDelay(80/portTICK_PERIOD_MS);
    }

    xTaskCreate(get_acc_gyro,TASK_NAME,TASK_STACK_DEPTH,NULL,TASK_PRIORITY,&task1);
    vTaskDelete(NULL);
}
