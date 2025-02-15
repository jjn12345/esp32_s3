#pragma once
#include "view_manager.h"

typedef struct{
    module_t    base;           /* 必须放在最上面 */
    ui_data_t*  ui_data;        /* 指向全局ui数据 */   
}ui_module_t;


extern ui_module_t ui_model;


