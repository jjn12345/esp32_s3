#pragma once
#include "view_manager.h"

extern const UiScreenOps g_wifi_ops; 


void view_wifi_create(event_bus_t* bus);
void view_wifi_hide(void);
void view_wifi_show(void);