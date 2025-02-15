#include "model.h"
#include "esp_log.h"
bool module_mgr_init(module_mgr_t* mgr){
    if(!mgr)return false;
    mgr->count = 0;
    mgr->mutex = xSemaphoreCreateMutex();
    if(mgr->mutex == NULL){
        return false;
    }
    return true;
}
bool module_register(module_mgr_t* mgr, module_t* module,const uint32_t delay_time){
    if(!mgr || !module)return false;
    TickType_t ret = 0;
    ret = xSemaphoreTake(mgr->mutex,delay_time>0?pdMS_TO_TICKS(delay_time):portMAX_DELAY);
    if(ret == pdPASS){
        for(uint8_t i = 0;i<mgr->count;i++){
            if(mgr->modules[i]->id == module->id){
                xSemaphoreGive(mgr->mutex);
                return false;
            }
        }        
        mgr->modules[mgr->count++] = module;
        xSemaphoreGive(mgr->mutex);
    }
    else{
        return false;
    }
    return true;
}
bool module_init_all(module_mgr_t* mgr,const uint32_t delay_time){
    if(!mgr)return false;
    TickType_t ret = 0;
    ret = xSemaphoreTake(mgr->mutex,delay_time>0?pdMS_TO_TICKS(delay_time):portMAX_DELAY);
    if(ret == pdPASS){
        for(uint8_t i = 0;i < mgr->count;i++){
            if(mgr->modules[i]->init){
                mgr->modules[i]->init(mgr->modules[i]);
                if(mgr->modules[i]->state != MODULE_STATE_ERROR){
                    mgr->modules[i]->state = MODULE_STATE_READY;
                }
                else{
                    ESP_LOGE("model","%d init error",mgr->modules[i]->id);
                }
            }
        }
        xSemaphoreGive(mgr->mutex);
    }
    else{
        return false;
    }
    return true;
}
bool module_update_all(module_mgr_t* mgr,const uint32_t delay_time){
    if(!mgr)return false;
    TickType_t ret = 0;
    ret = xSemaphoreTake(mgr->mutex,delay_time>0?pdMS_TO_TICKS(delay_time):portMAX_DELAY);
    if(ret == pdPASS){
        for(uint8_t i = 0;i < mgr->count;i++){
            if(mgr->modules[i]->update && mgr->modules[i]->state == MODULE_STATE_READY){
                mgr->modules[i]->update(mgr->modules[i]);
            }
        }
        xSemaphoreGive(mgr->mutex);
    }
    else{
        return false;
    }
    return true; 
}
bool module_delete(module_mgr_t* mgr, const uint8_t id,const uint32_t delay_time){
    if(!mgr)return false;
    TickType_t ret = pdPASS;
    if(mgr->count == 0) return false;
    for(int i=0; i<mgr->count; i++){
        if(mgr->modules[i]->id == id){
            ret = xSemaphoreTake(mgr->mutex,delay_time>0?pdMS_TO_TICKS(delay_time):portMAX_DELAY);
            if(pdPASS != ret)return false;   
            if(mgr->modules[i]->deinit){
                mgr->modules[i]->deinit(mgr->modules[i]);
            }
            if(i == mgr->count-1){
                mgr->count--;     
            }
            else{
                // 前移后续元素
                memmove(&mgr->modules[i], &mgr->modules[i+1], 
                    (mgr->count - i - 1)*sizeof(mgr->modules[0]));
                mgr->count--;     
            }         
            ret = xSemaphoreGive(mgr->mutex);
            return true;
        }
    } 
    return false;
}