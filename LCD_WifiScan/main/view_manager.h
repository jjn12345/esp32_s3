#pragma once
#include "model.h"
#include "esp_lvgl_port.h"
#include "app_evt_def.h"

LV_FONT_DECLARE(lv_font_chinese)    /* 声明中文字库 */
#define UI_LOCK()       lvgl_port_lock(0)
#define UI_UNLOCK()     lvgl_port_unlock()
#define IS_ANIMAL       0

/* 全局UI数据管理 */
/* wifi数据 */

typedef struct{
    struct {
        char selected_ssid[32];  // 当前选中的AP名称
        char password[64];       // 输入的密码        
    }wifi_counter_info;
    struct{
        bool wifi_is_enable;
        bool wifi_is_connected;
    }flag;
}wifi_data_t;

typedef struct{
    /* wifi 状态 */
    wifi_data_t wifi_data;
}ui_data_t;
extern ui_data_t  g_ui_data;
/* view 页面ID */
typedef enum{
    UI_SCREEN_HOME = 0,             /* 主界面 */
    UI_SCREEN_WIFI,                 /* wifi界面 */
    UI_SCREEN_WIFI_CONNECT,         /* wifi连接界面 */
    UI_SCREEN_COUNT,                /* 界面总数 */
}UiScreenID;

/* view 操作函数 */
typedef void(*ui_create)(event_bus_t*bus);
typedef void(*ui_show)(void);
typedef void(*ui_hide)(void);
typedef struct{
    const uint8_t  ScreenID;
    bool  is_created;           /* 是否创建了该界面，用于按需创建，节省内存 */
    bool  is_persistent;        /* 是否常驻界面，一般为主界面 */  
    ui_create   create;         /* 界面创建函数 */
    ui_show     show;           /* 界面显示函数 */
    ui_hide     hide;           /* 界面隐藏函数(隐藏界面的同时,删除一些不必要的对象，释放内存) */
}UiScreenOps;

/* view  全局界面栈,使用出栈、入栈进行实现界面的返回上一级和进入下一级*/
#define UI_STACK_DEPTH      UI_SCREEN_COUNT



/* 界面注册函数 */
bool ui_register_screen(UiScreenID id,ui_create create,ui_show show,ui_hide hide);
/* 创建初始界面 */
bool ui_init(module_t* self); 
/* 出栈 */
void ui_pop(void); 
/* 压栈 */
void ui_push(const uint8_t screen_id);
/* 设置是否隐藏 */
void lv_obj_set_hidden(lv_obj_t*obj,bool enable);







