#pragma once
#include "view_manager.h"

extern UiScreenOps g_ble_ops;

void view_ble_create(event_bus_t* bus);
void view_ble_hide(void);
void view_ble_show(void);