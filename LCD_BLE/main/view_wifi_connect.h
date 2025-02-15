#pragma once 
#include "view_manager.h"


extern const UiScreenOps g_wifi_connect_ops;

void view_wifi_connect_create(event_bus_t* bus);
void view_wifi_connect_hide(void);
void view_wifi_connect_show(void);