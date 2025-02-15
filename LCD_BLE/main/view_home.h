#pragma once
#include "view_manager.h"

extern const UiScreenOps g_home_ops;


void view_home_create(event_bus_t* bus);
void view_home_hide(void);
void view_home_show(void);

