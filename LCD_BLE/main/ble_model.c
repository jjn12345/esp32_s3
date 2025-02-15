#include "ble_model.h"





static void ble_init(module_t*self){
    



}



ble_module_t ble_module = {
    .base = {
        .id = 0x03,
        .init = ble_init,
        .state = MODULE_STATE_ALLOCATED,
    },
}

