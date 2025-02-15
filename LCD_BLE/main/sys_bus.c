#include "sys_bus.h"

static void event_process(void*arg) {
    event_bus_t* bus = (event_bus_t*)arg;
    event_t* evt;
    while(1){
        if(xQueueReceive(bus->evt_queue,&evt,pdMS_TO_TICKS(TIMEOUT_EVT_CLEAN_TIME)) == pdPASS){
            xSemaphoreTake(bus->mutex,0);
            for(int i=0; i<bus->sub_count; i++){
                if(evt->type & bus->subscribers[i].filter){
                    bus->subscribers[i].cb(evt,bus->subscribers[i].user_data);
                }
            }
            my_mem_free(bus->pool,evt);
            xSemaphoreGive(bus->mutex);   
        }
    }
}

bool event_bus_init(event_bus_t* bus, mem_pool_t* pool,const uint32_t queue_depth){
    if(!bus || !pool || queue_depth == 0)return false;

    bus->queue_buffer = mem_alloc(pool,queue_depth * sizeof(event_t*));
    if(!bus->queue_buffer){
        return false;
    }
    bus->pxQueueBuffer = mem_alloc(pool,sizeof(StaticQueue_t));
    if(!bus->pxQueueBuffer){
        my_mem_free(pool,bus->queue_buffer);
        return false;
    }
    bus->xMutexBuffer = mem_alloc(pool,sizeof(StaticSemaphore_t));    
    if(!bus->xMutexBuffer){
        my_mem_free(pool,bus->queue_buffer);
        my_mem_free(pool,bus->pxQueueBuffer);
        return false;
    }
    bus->pool = pool;
    bus->sub_count = 0;
    bus->mutex = xSemaphoreCreateMutexStatic(bus->xMutexBuffer);
    bus->evt_queue = xQueueCreateStatic(queue_depth,sizeof(event_t*),\
                                        bus->queue_buffer,bus->pxQueueBuffer);

    if(NULL == bus->mutex || NULL == bus->evt_queue){
        my_mem_free(pool,bus->queue_buffer);
        my_mem_free(pool,bus->pxQueueBuffer);
        my_mem_free(pool,bus->xMutexBuffer);
        return false;
    }
    bus->stack_buffer = mem_alloc(pool,sizeof(StackType_t)*SYS_BUS_PROCESS_STACK_DEPTH);
    bus->task_buffer = mem_alloc(pool,sizeof(StaticTask_t));
    bus->evt_bus_handle= xTaskCreateStatic(event_process,"evt_bus_process",SYS_BUS_PROCESS_STACK_DEPTH,bus,SYS_BUS_PROCESS_PRIORITY,bus->stack_buffer,bus->task_buffer);
    if(NULL == bus->evt_bus_handle){
        my_mem_free(pool,bus->queue_buffer);
        my_mem_free(pool,bus->pxQueueBuffer);
        my_mem_free(pool,bus->xMutexBuffer);
        my_mem_free(pool,bus->stack_buffer);
        my_mem_free(pool,bus->task_buffer);
        return false;
    }
    return true;
}

bool event_subscribe(event_bus_t* bus, event_type_t evt_filter, event_handler cb,void* user_data,const uint32_t delay_time) {
    if(!bus || !cb )return false;
    TickType_t ret = pdPASS;
    
    ret = xSemaphoreTake(bus->mutex,delay_time>0?pdMS_TO_TICKS(delay_time):portMAX_DELAY);
    if(pdPASS != ret)return false;
    if(bus->sub_count >= MAX_SUBSCRIBER_NUM){
        xSemaphoreGive(bus->mutex);
        return false;
    } 
    bus->subscribers[bus->sub_count].cb = cb;
    bus->subscribers[bus->sub_count].filter = evt_filter;
    bus->subscribers[bus->sub_count].user_data= user_data;
    bus->sub_count++;
    xSemaphoreGive(bus->mutex);
    return true;
}

bool event_unsubscribe(event_bus_t* bus,event_handler cb,const uint32_t delay_time) {
    if(!bus || !cb)return false;
    TickType_t ret = pdPASS;

    ret = xSemaphoreTake(bus->mutex,delay_time>0?pdMS_TO_TICKS(delay_time):portMAX_DELAY);
    if(pdPASS != ret)return false; 

    if(bus->sub_count == 0){
        xSemaphoreGive(bus->mutex);
        return false;
    }
    for(int i=0; i<bus->sub_count; i++){
        if(bus->subscribers[i].cb == cb) {
            if(i == bus->sub_count-1){
                bus->subscribers[i].cb = NULL;
                bus->subscribers[i].filter= 0;
                bus->subscribers[i].user_data = NULL;
                bus->sub_count--;     
            }
            else{
                // 前移后续元素
                memmove(&bus->subscribers[i], &bus->subscribers[i+1], 
                    (bus->sub_count - i - 1)*sizeof(bus->subscribers[0]));
                bus->subscribers[bus->sub_count -1].cb = NULL;
                bus->subscribers[bus->sub_count -1].filter = 0;
                bus->subscribers[bus->sub_count -1].user_data = NULL;   
                bus->sub_count--;     
            }         
            ret = xSemaphoreGive(bus->mutex);
            return true;
        }
    }
    return false;
    
}

/*  data不由总线管理   */
bool event_publish(event_bus_t* bus, event_type_t type, void* data,const uint32_t delay_time) {
    event_t* evt = mem_alloc(bus->pool,sizeof(event_t));
    TickType_t ret = 0;
    if(!evt) return false;
    evt->data = data;
    evt->type = type;
    ret = xQueueSend(bus->evt_queue,&evt,delay_time>0?pdMS_TO_TICKS(delay_time):portMAX_DELAY);                   
    if(ret != pdPASS){
        my_mem_free(bus->pool,evt);
        return false;
    }
    return true;
}



void event_bus_deinit(event_bus_t* bus) {
    if (!bus) return;
    // 释放队列内存
    my_mem_free(bus->pool, bus->queue_buffer);
    my_mem_free(bus->pool, bus->pxQueueBuffer);
    // 释放互斥量内存
    my_mem_free(bus->pool, bus->xMutexBuffer);
    bus->evt_queue = NULL;
    bus->mutex = NULL;
    // 清空订阅者
    memset(bus->subscribers, 0, sizeof(bus->subscribers));
    // 删除任务并释放任务栈内存
    vTaskDelete(bus->evt_bus_handle);
    my_mem_free(bus->pool,bus->stack_buffer);
    my_mem_free(bus->pool,bus->task_buffer);
}