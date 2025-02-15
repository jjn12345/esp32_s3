#include "view_manager.h"
/* 线程互斥锁定义 */
#define VIEW_LOCK()       xSemaphoreTake(sMutex,0)
#define VIEW_UNLOCK()     xSemaphoreGive(sMutex)
/* UI的屏幕执行函数 */
static UiScreenOps  g_UIOps[UI_SCREEN_COUNT];
/* UI栈 */
static UiScreenID   g_UIStack[UI_STACK_DEPTH];
/* 栈顶指针 */
static int g_s_stack_top = -1;
/* 互斥信号量 */
static SemaphoreHandle_t  sMutex;
/* 获取到系统总线 */
static  event_bus_t* sys_bus;
/* 全局UI数据 */
ui_data_t  g_ui_data;
/* 注册屏幕 */
bool ui_register_screen(UiScreenID id,ui_create create,ui_show show,ui_hide hide){
    if(id >= UI_SCREEN_COUNT)return false;
    g_UIOps[id].create = create;
    g_UIOps[id].hide = hide;
    g_UIOps[id].show = show;
    g_UIOps[id].is_created = false;
    g_UIOps[id].is_persistent = (id == UI_SCREEN_HOME);
    return true;
}
/* 创建初始界面 */
bool ui_init(module_t* self){
    sMutex = xSemaphoreCreateMutex();
    if(!sMutex){
        return false;
    }
    sys_bus = self->bus;
    ui_push(UI_SCREEN_HOME);
    return true;
} 
/* 出栈 */
void ui_pop(void){
    VIEW_LOCK();
    if(g_s_stack_top < 0){
        VIEW_UNLOCK();
        return;
    }
    UiScreenOps* old_UI = &g_UIOps[g_UIStack[g_s_stack_top]];
    // 销毁非持久界面
    if (!old_UI->is_persistent) {
        if (old_UI->hide) old_UI->hide();
        old_UI->is_created = false;
    }
    g_s_stack_top--;
    // 显示上一界面
    if (g_s_stack_top >= 0) {
        UiScreenOps *prev_screen = &g_UIOps[g_UIStack[g_s_stack_top]];
        if(prev_screen->show)prev_screen->show();
    }
    VIEW_UNLOCK();
}  
/* 压栈 */
void ui_push(const uint8_t screen_id){
    VIEW_LOCK();
    if(screen_id >= UI_SCREEN_COUNT || g_s_stack_top >= UI_STACK_DEPTH-1){
        VIEW_UNLOCK();
        return ;
    }
    /* 该界面未创建 */
    UiScreenOps* new_UI = &g_UIOps[screen_id];
    if(!new_UI){
        VIEW_UNLOCK();
        return ;
    }
    if(!new_UI->is_created){
        if(new_UI->create){
            new_UI->create(sys_bus);
            new_UI->is_created = true;
        }
        else{
            VIEW_UNLOCK();
            return;
        }
    }    


    /* 栈非空 */
    if(g_s_stack_top >= 0){
         UiScreenOps* old_UI= &g_UIOps[g_UIStack[g_s_stack_top]];
        /* 把当前的隐藏掉 */
        if(old_UI->hide)old_UI->hide();
    }
    if(new_UI->show){
        new_UI->show();
    }
    g_UIStack[++g_s_stack_top] = screen_id;
    VIEW_UNLOCK();
} 
/* 设置是否隐藏 */
void lv_obj_set_hidden(lv_obj_t*obj,bool enable){
    if(enable){
        lv_obj_add_flag(obj,LV_OBJ_FLAG_HIDDEN);
    }
    else{
        lv_obj_remove_flag(obj,LV_OBJ_FLAG_HIDDEN);
    }
}




