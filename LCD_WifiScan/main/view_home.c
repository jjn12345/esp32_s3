#include "view_home.h"
/* 全局主界面屏幕 */
static lv_obj_t* g_home_screen;

static void home_list_btn_cb(lv_event_t*e){
    uint8_t target = (uint8_t)lv_event_get_user_data(e);
    if(target != UI_SCREEN_HOME){
        ui_push(target);  // 使用栈式切换
    }  
}
void view_home_create(event_bus_t* bus){
    UI_LOCK();
    /* 创建全局主界面 */
    g_home_screen = lv_obj_create(NULL);
    /* 默认隐藏 */
    lv_obj_set_hidden(g_home_screen,true);
    /* 设置屏幕样式 */
    lv_obj_set_style_text_font(g_home_screen,&lv_font_chinese,0);
    /* 创建一个菜单列表 */
    lv_obj_t* menu_list = lv_list_create(g_home_screen);
    lv_obj_set_width(menu_list, LV_PCT(100));
    const char* menu_name[] = {"Wifi","Ble","Setting"};
    const uint8_t screen_id[] = {UI_SCREEN_WIFI,UI_SCREEN_HOME,UI_SCREEN_HOME};
    for(uint8_t i = 0;i<3;i++){
        lv_obj_t*btn = lv_list_add_btn(menu_list,NULL,menu_name[i]);
        lv_obj_add_event(btn,home_list_btn_cb,LV_EVENT_CLICKED,screen_id[i]);
    }
    UI_UNLOCK();
}   
void view_home_hide(void){
    UI_LOCK();
    lv_obj_set_hidden(g_home_screen,true);
    UI_UNLOCK();
}
void view_home_show(void){
    // 无需销毁，保持常驻
    UI_LOCK();
    lv_obj_set_hidden(g_home_screen,false);
    #if IS_ANIMAL
        lv_scr_load_anim(g_home_screen, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 400, 0, false);
    #else
        lv_scr_load(g_home_screen);
    #endif
    UI_UNLOCK();
}
const UiScreenOps g_home_ops = {
    .create = view_home_create,
    .show = view_home_show,
    .hide = view_home_hide,
};
