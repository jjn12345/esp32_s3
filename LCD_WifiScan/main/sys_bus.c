#include "sys_bus.h"


bool event_bus_init(event_bus_t* bus, mem_pool_t* pool,const uint32_t queue_depth){
    if(!bus || !pool || queue_depth == 0)return false;

    uint8_t *queue_buffer = mem_alloc(pool,queue_depth * sizeof(event_t*));
    if(!queue_buffer){
        return false;
    }
    StaticQueue_t* pxQueueBuffer = mem_alloc(pool,sizeof(StaticQueue_t));
    if(!queue_buffer){
        my_mem_free(pool,queue_buffer);
        return false;
    }
    StaticSemaphore_t* xMutexBuffer = mem_alloc(pool,sizeof(StaticSemaphore_t));    
    if(!xMutexBuffer){
        my_mem_free(pool,queue_buffer);
        my_mem_free(pool,pxQueueBuffer);
        return false;
    }
    bus->pool = pool;
    bus->sub_count = 0;
    bus->mutex = xSemaphoreCreateMutexStatic(xMutexBuffer);
    bus->evt_queue = xQueueCreateStatic(queue_depth,sizeof(event_t*),\
                                        queue_buffer,pxQueueBuffer);

    if(NULL == bus->mutex || NULL == bus->evt_queue){
        my_mem_free(pool,queue_buffer);
        my_mem_free(pool,pxQueueBuffer);
        my_mem_free(pool,xMutexBuffer);
        return false;
    }
    return true;
}

bool event_subscribe(event_bus_t* bus, event_type_t evt_filter, event_handler cb,void* user_data,const uint32_t delay_time) {
    if(!bus || !cb )return false;
    TickType_t ret = pdPASS;
    if(bus->sub_count >= MAX_SUBSCRIBER_NUM) return false;
    ret = xSemaphoreTake(bus->mutex,delay_time>0?pdMS_TO_TICKS(delay_time):portMAX_DELAY);
    if(pdPASS != ret)return false;
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
    if(bus->sub_count == 0) return false;
    for(int i=0; i<bus->sub_count; i++){
        if(bus->subscribers[i].cb == cb) {
            ret = xSemaphoreTake(bus->mutex,delay_time>0?pdMS_TO_TICKS(delay_time):portMAX_DELAY);
            if(pdPASS != ret)return false;   
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
                bus->sub_count--;     
            }         
            ret = xSemaphoreGive(bus->mutex);
            return true;
        }
    }
    return false;
    
}

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

void event_process(event_bus_t* bus) {
    event_t* evt;
    if(xQueueReceive(bus->evt_queue,&evt,pdMS_TO_TICKS(TIMEOUT_EVT_CLEAN_TIME)) == pdPASS){
        for(int i=0; i<bus->sub_count; i++){
            if(evt->type & bus->subscribers[i].filter){
                bus->subscribers[i].cb(evt,bus->subscribers[i].user_data);
            }
        }
        my_mem_free(bus->pool,evt);
    }
    
}