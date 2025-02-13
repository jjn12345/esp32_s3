#pragma once 
#include "sys_bus.h"

#define MAX_MODULE_NUMS     8


typedef enum {
    MODULE_STATE_ALLOCATED,   // 未初始化
    MODULE_STATE_READY,       // 初始化完成
    MODULE_STATE_SUSPENDED,   // 已挂起
    MODULE_STATE_ERROR        // 错误状态
} module_state_t;


typedef struct module {
    const uint8_t id;
    uint8_t state;          /*  module_state_t  */
    event_bus_t* bus;       
    void (*init)(struct module* self);
    void (*update)(struct module* self);
    void (*deinit)(struct module* self);
} module_t;

// 模块管理器
typedef struct {
    module_t *modules[MAX_MODULE_NUMS];  // 最多8个模块
    uint8_t count;                      // 模组数量
    SemaphoreHandle_t mutex;            // 管理器级互斥锁  
} module_mgr_t;

// 模块管理接口
bool  module_create(module_t*module);
bool  module_mgr_init(module_mgr_t* mgr);
bool  module_register(module_mgr_t* mgr, module_t* module,const uint32_t delay_time);
bool  module_init_all(module_mgr_t* mgr,const uint32_t delay_time);
bool  module_update_all(module_mgr_t* mgr,const uint32_t delay_time);
bool  module_delete(module_mgr_t* mgr, const uint8_t id,const uint32_t delay_time);
