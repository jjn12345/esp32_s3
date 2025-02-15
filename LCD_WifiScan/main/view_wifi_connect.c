#include "view_wifi_connect.h"



/* 全局屏幕 */
static lv_obj_t* g_wifi_connect_screen;
static lv_obj_t* g_ssid = NULL;
static lv_obj_t* g_kb = NULL;
static lv_obj_t* g_ta = NULL;

// 连接结果事件
static void func(lv_timer_t *t) {
    lv_obj_t*obj =  lv_timer_get_user_data(t);
    lv_obj_del(obj);
    lv_timer_del(t);
}
static void connect_res_cb(event_t*e,void*arg) {

    if(g_ui_data.wifi_data.flag.wifi_is_enable == 0)return;
    UI_LOCK();
    lv_obj_t *toast = lv_label_create(lv_layer_top());
    lv_obj_set_style_text_font(toast,&lv_font_chinese,0);
    if(e->type == WIFI_CONNECT_FAIL){
        lv_label_set_text(toast, "连接失败!");
        g_ui_data.wifi_data.flag.wifi_is_connected = 0;
    }
    else{
        lv_label_set_text(toast, "连接成功!");
        g_ui_data.wifi_data.flag.wifi_is_connected = 1;
    }
    lv_obj_align(toast, LV_ALIGN_CENTER, 0, 0);
    lv_timer_create(func, 1500, toast);
    UI_UNLOCK();
}

//  连接按钮
static void connect_btn_cb(lv_event_t *e) {
    event_bus_t* bus = lv_event_get_user_data(e);
    event_publish(bus,UI_CONNECT_WIFI,&g_ui_data.wifi_data.wifi_counter_info,1000);
    ESP_LOGI("wifi_connect","SSID:%s\nPWD:%s",g_ui_data.wifi_data.wifi_counter_info.selected_ssid,g_ui_data.wifi_data.wifi_counter_info.password);
    ui_pop();
}

// 输入框焦点事件（动态创建/销毁键盘）
static void on_ta_focus(lv_event_t *e) {
    lv_event_code_t code= lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    UI_LOCK();
    if (code == LV_EVENT_FOCUSED) {
        g_kb = lv_keyboard_create(lv_layer_top());  // 顶层显示键盘
        lv_keyboard_set_textarea(g_kb, ta);
    } else if (code == LV_EVENT_DEFOCUSED) {
        strncpy(g_ui_data.wifi_data.wifi_counter_info.password,lv_textarea_get_text(ta),sizeof(g_ui_data.wifi_data.wifi_counter_info.password)-1);
        g_ui_data.wifi_data.wifi_counter_info.password[sizeof(g_ui_data.wifi_data.wifi_counter_info.password)-1] = '\0';
        lv_obj_del(g_kb);
        g_kb = NULL;
    }
    UI_UNLOCK();
}

void view_wifi_connect_create(event_bus_t* bus){
    if (g_wifi_connect_screen) return;
    UI_LOCK();
    g_wifi_connect_screen = lv_obj_create(NULL);
    lv_obj_set_style_text_font(g_wifi_connect_screen,&lv_font_chinese,0);
    lv_obj_set_hidden(g_wifi_connect_screen,true);
    lv_obj_set_flex_flow(g_wifi_connect_screen,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_wifi_connect_screen,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(g_wifi_connect_screen,0,0);
    /* 创建标题,显示连接的ssid */
    char ssid[33];
    strncpy(ssid, g_ui_data.wifi_data.wifi_counter_info.selected_ssid, sizeof(ssid)-1);
    ssid[sizeof(ssid)-1] = '\0';
    lv_obj_t* label_row = lv_obj_create(g_wifi_connect_screen);
    /* 对标题进行样式设计 */
    static lv_style_t row_style;
    lv_style_init(&row_style);
    lv_style_set_align(&row_style,LV_ALIGN_TOP_MID);
    lv_style_set_border_width(&row_style,0);;
    lv_style_set_width(&row_style,LV_PCT(100));
    lv_style_set_pad_column(&row_style,0);
    lv_style_set_pad_all(&row_style,0);
    lv_obj_add_style(label_row,&row_style,0);
    
    lv_obj_set_height(label_row,LV_PCT(20));
    lv_obj_t* label1 = lv_label_create(label_row);
    g_ssid = lv_label_create(label_row);
    lv_obj_align(label1,LV_ALIGN_LEFT_MID,0,0);
    lv_obj_align_to(g_ssid,label1,LV_ALIGN_TOP_RIGHT,60,0);
    lv_label_set_text(label1,"SSID:");
    lv_label_set_text(g_ssid,ssid);
    /* 创建单行密码输入框 */
    lv_obj_t* ta_row = lv_obj_create(g_wifi_connect_screen);
    lv_obj_add_style(ta_row,&row_style,0);
    lv_obj_set_flex_grow(ta_row,2);
    lv_obj_t* label3 = lv_label_create(ta_row);
    lv_label_set_text(label3,"PWD:");
    g_ta = lv_textarea_create(ta_row);
    lv_textarea_set_one_line(g_ta,true);
    lv_textarea_set_password_mode(g_ta,true);
    lv_textarea_set_placeholder_text(g_ta,"请输入密码");
    lv_obj_set_size(g_ta,LV_PCT(80),LV_SIZE_CONTENT);
    
    lv_obj_align(g_ta,LV_ALIGN_RIGHT_MID,0,0);
    lv_obj_align(label3,LV_ALIGN_LEFT_MID,0,0);
    /* 创建两个按钮 */
    lv_obj_t* btn_row = lv_obj_create(g_wifi_connect_screen);
    lv_obj_add_style(btn_row,&row_style,0);
    lv_obj_set_flex_grow(btn_row,3);
    lv_obj_t* cancel_btn = lv_button_create(btn_row);
    lv_obj_t* ok_btn = lv_button_create(btn_row);
    lv_obj_t* cancel_label = lv_label_create(cancel_btn);
    lv_obj_t* ok_label = lv_label_create(ok_btn);
    lv_label_set_text(cancel_label,"取消");
    lv_label_set_text(ok_label,"连接");
    lv_obj_align(cancel_btn,LV_ALIGN_CENTER,-80,0);
    lv_obj_align(ok_btn,LV_ALIGN_CENTER,80,0);
    /* 添加回调事件 */
    /* 取消按钮 */
    lv_obj_add_event_cb(cancel_btn,ui_pop,LV_EVENT_CLICKED,NULL);
    /* 连接按钮 */
    lv_obj_add_event_cb(ok_btn,connect_btn_cb,LV_EVENT_CLICKED,bus);
    /* 文本区域按钮 */
    lv_obj_add_event_cb(g_ta,on_ta_focus,LV_EVENT_FOCUSED,NULL);
    lv_obj_add_event_cb(g_ta,on_ta_focus,LV_EVENT_DEFOCUSED,NULL);
    UI_UNLOCK();
    /* 订阅事件 */
    event_subscribe(bus,WIFI_CONNECT_FAIL,connect_res_cb,0,1000);
    event_subscribe(bus,WIFI_CONNECT_SUCCESS,connect_res_cb,0,1000);
}
void view_wifi_connect_hide(void){
    // 保持界面对象，仅隐藏
    UI_LOCK();
    lv_obj_add_flag(g_wifi_connect_screen,LV_OBJ_FLAG_HIDDEN);
    lv_textarea_set_text(g_ta,"");
    UI_UNLOCK();
}
void view_wifi_connect_show(void){
    // 保持界面对象，仅隐藏
    UI_LOCK();
    lv_obj_remove_flag(g_wifi_connect_screen,LV_OBJ_FLAG_HIDDEN);
    /* 更新ssid */
    char ssid[33];
    strncpy(ssid, g_ui_data.wifi_data.wifi_counter_info.selected_ssid, sizeof(ssid)-1);
    ssid[sizeof(ssid)-1] = '\0';
    lv_label_set_text(g_ssid,ssid);
    #if IS_ANIMAL
        lv_scr_load_anim(g_wifi_screen, LV_SCR_LOAD_ANIM_OVER_LEFT, 300, 0, false);
    #else
        lv_scr_load(g_wifi_connect_screen);
    #endif
    UI_UNLOCK();
}



const UiScreenOps g_wifi_connect_ops = {
    .create = view_wifi_connect_create,
    .show = view_wifi_connect_show,
    .hide = view_wifi_connect_hide,
};

