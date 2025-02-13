#pragma once
#include "mem_manage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define TIMEOUT_EVT_CLEAN_TIME      5000        /*  超时任务自动清理的时间  */
#define MAX_SUBSCRIBER_NUM          8    
typedef uint32_t event_type_t;
typedef struct {
    event_type_t type;
    void* data;       // 使用内存池分配
} event_t;

typedef void (*event_handler)(event_t*e,void*arg);

// 事件总线结构体
typedef struct {
    mem_pool_t* pool;
    struct {
        event_handler cb;
        event_type_t filter;
        void* user_data;
    } subscribers[MAX_SUBSCRIBER_NUM];  // 最多8个订阅者
    uint8_t sub_count;
    QueueHandle_t evt_queue;  /* 事件队列,当事件触发后 */
    SemaphoreHandle_t mutex;
} event_bus_t;

// 总线操作
bool  event_bus_init(event_bus_t* bus, mem_pool_t* pool,const uint32_t queue_depth);
bool  event_unsubscribe(event_bus_t* bus,event_handler cb,const uint32_t delay_time); 
bool  event_subscribe(event_bus_t* bus, event_type_t evt_filter, event_handler cb,void* user_data,const uint32_t delay_time);
bool  event_publish(event_bus_t* bus, event_type_t type, void* data,const uint32_t deley_time);
void  event_process(event_bus_t* bus);






