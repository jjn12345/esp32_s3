#include "view_ble.h"






void view_ble_create(event_bus_t* bus){

}
void view_ble_hide(void){

}
void view_ble_show(void){

}


const UiScreenOps g_ble_ops = {
    .create = view_ble_create,
    .show = view_ble_create,
    .hide = view_ble_create,
};
