#include "view_wifi.h"




static lv_obj_t *g_wifi_screen = NULL;
static lv_obj_t *g_switch = NULL;
static lv_obj_t *g_ap_list = NULL;
static lv_obj_t *g_wifi_state_block = NULL;

// WIFI开关事件
static void on_wifi_switch(lv_event_t *e) {
    static uint32_t last_switch_time = 0;
    event_bus_t* bus = (event_bus_t*)lv_event_get_user_data(e);
    lv_obj_t *sw = lv_event_get_target(e);
    bool enable = g_ui_data.wifi_data.flag.wifi_is_enable;
    if((xTaskGetTickCount()-last_switch_time)<pdMS_TO_TICKS(1000)){
        if(enable){
            lv_obj_add_state(sw,LV_STATE_CHECKED);
        }
        else{
            lv_obj_remove_state(sw,LV_STATE_CHECKED);
        }
        return;
    }
    enable = lv_obj_has_state(sw, LV_STATE_CHECKED);
    g_ui_data.wifi_data.flag.wifi_is_enable = enable;
    UI_LOCK();
    lv_obj_set_hidden(g_ap_list, !enable);  // 显示/隐藏AP列表
    if(enable){
        event_publish(bus,UI_ENABLE_WIFI,NULL,1000);
    }
    else{
        lv_obj_t* label = lv_obj_get_child(g_wifi_state_block,1);
        lv_obj_t* label2 = lv_obj_get_child(g_wifi_state_block,0);
        lv_label_set_text_fmt(label2,LV_SYMBOL_WIFI);
        lv_label_set_text_fmt(label,"未连接");
        event_publish(bus,UI_CLOSE_WIFI,NULL,1000);
        g_ui_data.wifi_data.flag.wifi_is_connected = 0;
        lv_obj_clean(g_ap_list);
    }
    last_switch_time = xTaskGetTickCount();
    UI_UNLOCK();
}

// AP点击事件
static void on_ap_click(lv_event_t *e) {
    event_bus_t* bus = (event_bus_t*)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);
    const char *ssid = lv_list_get_btn_text(g_ap_list, btn);
    strncpy(g_ui_data.wifi_data.wifi_counter_info.selected_ssid, ssid, sizeof(g_ui_data.wifi_data.wifi_counter_info.selected_ssid)-1);
    g_ui_data.wifi_data.wifi_counter_info.selected_ssid[sizeof(g_ui_data.wifi_data.wifi_counter_info.selected_ssid)-1] = '\0';
    ui_push(UI_SCREEN_WIFI_CONNECT);  // 进入连接界面
}
//  更新AP列表事件
typedef struct{
    uint8_t ssid[33];
    int16_t rssi;
}wifi_info_t;
typedef struct{
    wifi_info_t* info;
    uint16_t ap_nums;
}scan_info_t;
static  void on_ap_updata(event_t *e,void*arg){
    event_t* bus = (event_bus_t*)arg;
    /* 获取系统总线传递过来的数据 */
    scan_info_t * scan_info;
    scan_info = (scan_info_t*)e->data;
    UI_LOCK();    
    switch(e->type){
        case WIFI_CONNECT_SUCCESS:{
            lv_obj_clean(g_ap_list);
            char ssid[32];
            strcpy(ssid,g_ui_data.wifi_data.wifi_counter_info.selected_ssid);
            lv_obj_t* label = lv_obj_get_child(g_wifi_state_block,1);
            lv_obj_t* label2 = lv_obj_get_child(g_wifi_state_block,0);
            lv_label_set_text_fmt(label2,LV_SYMBOL_WIFI "连接成功");
            lv_label_set_text_fmt(label,ssid);
        }
        break;
        case WIFI_CONNECT_FAIL:{

        }
        break;
        case WIFI_SCAN_RESULT:{
            /* 清除原本的AP列表的内容 */
            lv_obj_clean(g_ap_list);   
            for (int i = 0; i < scan_info->ap_nums; i++) {
                if(g_ui_data.wifi_data.flag.wifi_is_connected){
                    if(!strcmp((char*)scan_info->info[i].ssid,g_ui_data.wifi_data.wifi_counter_info.selected_ssid)){
                        continue;
                    }
                }
                lv_obj_t *btn = lv_list_add_btn(g_ap_list, LV_SYMBOL_WIFI, (char*)scan_info->info[i].ssid);
                lv_obj_add_event_cb(btn, on_ap_click, LV_EVENT_CLICKED, bus);
            }
        }
        break;
    }

    UI_UNLOCK();
}

void view_wifi_create(event_bus_t* bus) {
    if (g_wifi_screen) return;
    UI_LOCK();
    g_wifi_screen = lv_obj_create(NULL);
    lv_obj_set_hidden(g_wifi_screen,true);
    lv_obj_set_flex_flow(g_wifi_screen,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_wifi_screen,LV_FLEX_ALIGN_SPACE_BETWEEN,LV_ALIGN_CENTER,LV_ALIGN_CENTER);
    lv_obj_set_style_pad_row(g_wifi_screen,5,0);
    // 返回按钮（固定在顶部）
    lv_obj_t *back_btn = lv_btn_create(g_wifi_screen);
    lv_obj_add_event_cb(back_btn, ui_pop, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    // WIFI开关行
    lv_obj_t *sw_row = lv_obj_create(g_wifi_screen);
    lv_obj_set_style_pad_top(sw_row,0,0);
    lv_obj_set_style_pad_bottom(sw_row,0,0);
    lv_obj_set_width(sw_row, LV_PCT(100));
    lv_obj_set_flex_grow(sw_row,2);
    // lv_obj_align(sw_row, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *sw_label = lv_label_create(sw_row);
    lv_label_set_text(sw_label, "WIFI:");
    lv_obj_align(sw_label, LV_ALIGN_LEFT_MID, 0, 0);

    g_switch = lv_switch_create(sw_row);
    lv_obj_align(g_switch, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(g_switch, on_wifi_switch, LV_EVENT_VALUE_CHANGED, bus);

    // wifi连接状态栏
    g_wifi_state_block = lv_obj_create(g_wifi_screen);
    lv_obj_set_style_text_font(g_wifi_state_block,&lv_font_chinese,0);
    lv_obj_set_width(g_wifi_state_block,LV_PCT(100));
    lv_obj_set_flex_grow(g_wifi_state_block,2);
    lv_obj_t* symbol = lv_label_create(g_wifi_state_block);
    lv_obj_t* label = lv_label_create(g_wifi_state_block);
    lv_label_set_text(symbol,LV_SYMBOL_WIFI);
    lv_label_set_text(label,"未连接");
    lv_obj_align(symbol,LV_ALIGN_LEFT_MID,0,0);
    lv_obj_align(label,LV_ALIGN_RIGHT_MID,0,0);
    // AP列表（初始隐藏）
    lv_obj_t* ap_list_block = lv_obj_create(g_wifi_screen);
    lv_obj_set_style_text_font(ap_list_block,&lv_font_chinese,0);
    lv_obj_set_style_border_width(ap_list_block,0,0);
    lv_obj_set_flex_grow(ap_list_block,5);
    lv_obj_set_width(ap_list_block,LV_PCT(100));
    lv_obj_set_style_pad_all(ap_list_block,0,0);
    g_ap_list = lv_list_create(ap_list_block);
    lv_obj_set_style_border_width(g_ap_list,0,0);
    lv_obj_set_size(g_ap_list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_hidden(g_ap_list, true);
    UI_UNLOCK();
    /* 订阅wifi扫描完成事件 */
    event_subscribe(bus,WIFI_SCAN_RESULT|WIFI_CONNECT_SUCCESS|WIFI_CONNECT_FAIL,on_ap_updata,bus,1000);
}

void view_wifi_show(void) {
    UI_LOCK();
    bool enable = g_ui_data.wifi_data.flag.wifi_is_enable;
    lv_obj_set_state(g_switch, LV_STATE_CHECKED,enable);
    lv_obj_set_hidden(g_ap_list, !enable);
    lv_obj_set_hidden(g_wifi_screen,false);
    #if IS_ANIMAL
        lv_scr_load_anim(g_wifi_screen, LV_SCR_LOAD_ANIM_OVER_LEFT, 300, 0, false);
    #else
        lv_scr_load(g_wifi_screen);
    #endif
    
    UI_UNLOCK();
}

void view_wifi_hide(void) {
    // 保持界面对象，仅隐藏
    UI_LOCK();
    lv_obj_add_flag(g_wifi_screen,LV_OBJ_FLAG_HIDDEN);
    lv_obj_clean(g_ap_list);  
    UI_UNLOCK();
}
const UiScreenOps g_wifi_ops = {
    .create = view_wifi_create,
    .show = view_wifi_show,
    .hide = view_wifi_hide,
};
