#include "esp32_ui.h"
#include "esp32_excute.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
static lv_obj_t* root_screen = NULL;     /*根屏幕,后续所有屏幕基于此屏幕创建 */
static void ui_init(void){
    root_screen = lv_screen_active();
}
/* demo 1 */
#include "link_list.h"
/* 页面枚举和全局变量 */
typedef enum{
    top_block = 0,
    main_block,
    bottom_block,
    right_block,
    block_count,
}page_block_t;
typedef enum{
    main_screen = 0,
    wifi_screen,
    screen_count,
}screen_t;
#define BLOCK_BORDER_WIDTH        0         /* 区块的边框宽度 */
static lv_obj_t* page_block[block_count];                   /* 整个页面的区块 */
static lv_style_t indicator_style;                          /* 屏幕指示器的统一样式 */
static lv_obj_t**indicator;                                 /* 屏幕指示器(根据分支节点数动态创建) */
static lv_obj_t* screen[screen_count];                      /* 屏幕 */
#define current_screen  (screen_t)(screen_list.head->data)    /* 当前屏幕永远是屏幕链表的头结点的数据 */
static dlist_t  screen_list = {0};
/* 内存池 */
static uint8_t ui_mem_buffer[4096];
static mem_pool_t ui_mem_pool;
static void block_init(void){
    /* 创建page_block */
    page_block[top_block] = lv_obj_create(root_screen);
    page_block[main_block] = lv_obj_create(root_screen);
    page_block[bottom_block] = lv_obj_create(root_screen);
    page_block[right_block] = lv_obj_create(root_screen);
    /* 对页面区块进行布局 */
    /* 设置区块大小 */
    lv_obj_set_size(page_block[top_block],LV_PCT(50),LV_PCT(25));
    lv_obj_set_width(page_block[main_block],LV_PCT(50));
    lv_obj_set_flex_grow(page_block[main_block],1);
    lv_obj_set_size(page_block[bottom_block],LV_PCT(50),LV_PCT(10));
    lv_obj_set_size(page_block[right_block],LV_PCT(50),LV_PCT(100));
    /* 设置区块间距 */
    lv_obj_set_style_pad_column(root_screen, 0, 0);
    lv_obj_set_style_pad_row(root_screen, 0, 0);
    /* 设置区块透明 */
    static lv_style_t block_style;
    lv_style_init(&block_style);
    lv_style_set_border_width(&block_style,BLOCK_BORDER_WIDTH);
    lv_style_set_bg_opa(&block_style,LV_OPA_0);
    lv_style_set_pad_all(&block_style,0);
    for(uint32_t i = 0;i < block_count;i++){
        lv_obj_add_style(page_block[i],&block_style,0);
    }
}
/* 实现主区块的界面切换 */
/* 回退到上一个屏幕 */
static void main_block_out_screen(void){
    if(current_screen == main_screen){
        return;
    }
    lv_obj_add_flag(screen[current_screen],LV_OBJ_FLAG_HIDDEN);
    dlist_remove(&screen_list,screen_list.head);    /* 删除头结点 */
    lv_led_on(indicator[screen_list.count-1]);
    lv_obj_delete(indicator[screen_list.count]);
    lv_obj_remove_flag(screen[current_screen],LV_OBJ_FLAG_HIDDEN);
    LV_LOG_USER("%d",current_screen);
}
/* 进入下一个屏幕 */
static void main_block_in_screen(screen_t in_screen){
    if(in_screen == current_screen)return;
    lv_obj_add_flag(screen[current_screen],LV_OBJ_FLAG_HIDDEN);
    dlist_insert_head(&screen_list,(void*)in_screen);                   /* 插入头结点 */
    indicator = realloc(indicator,sizeof(lv_obj_t*)*screen_list.count);
    indicator[screen_list.count-1] = lv_led_create(page_block[bottom_block]);
    lv_obj_add_style(indicator[screen_list.count-1],&indicator_style,0);
    lv_led_on(indicator[screen_list.count-1]);
    lv_led_off(indicator[screen_list.count-2]);
    lv_obj_remove_flag(screen[current_screen],LV_OBJ_FLAG_HIDDEN);
    LV_LOG_USER("%d",current_screen);
}
/* 顶部区块的界面创建 */
/* 返回按钮的回调函数 */
static void bk_btn_cb(lv_event_t* e){
    lv_obj_t * bk_btn = lv_event_get_target_obj(e);
    lv_event_code_t code = lv_event_get_code(e);
    switch(code){
        case LV_EVENT_RELEASED:{
            main_block_out_screen();
        }
        break;       
        default:break;
    }
}
static void top_block_create(void){
    lv_obj_t* back_btn = lv_button_create(page_block[top_block]);
    lv_obj_t* label = lv_label_create(back_btn);
    lv_obj_set_size(back_btn,LV_PCT(40),LV_SIZE_CONTENT);
    lv_obj_align(back_btn,LV_ALIGN_LEFT_MID,10,0);
    lv_label_set_text(label,"BACK");
    lv_obj_center(label);
    lv_obj_add_flag(back_btn,LV_OBJ_FLAG_FLOATING);
    lv_obj_add_event_cb(back_btn,bk_btn_cb,LV_EVENT_ALL,NULL);
}
/* 底部区块的界面创建 */
static void bottom_block_create(void){
    lv_obj_set_flex_flow(page_block[bottom_block],LV_FLEX_FLOW_ROW);
    indicator = malloc(sizeof(lv_obj_t*) * 1);
    indicator[0] = lv_led_create(page_block[bottom_block]);
    lv_obj_set_flex_align(page_block[bottom_block],LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    lv_style_init(&indicator_style);
    lv_style_set_size(&indicator_style,10,10);
    lv_style_set_radius(&indicator_style,LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&indicator_style,lv_color_hex(0x22a5f1));
    lv_obj_add_style(indicator[0],&indicator_style,0);
}
/* 主区块的界面创建 */
static void main_btn_cb(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target_obj(e);
    void* user_data = lv_event_get_user_data(e);
    screen_t next_screen;
    if(user_data){
        next_screen = (screen_t)user_data;
    }
    else{
        next_screen = main_screen;
    }
    switch(code){
        case LV_EVENT_PRESSING :{
             lv_obj_set_style_bg_color(btn,lv_color_hex(0x7a9fff),0);
        }
        break;
        case LV_EVENT_RELEASED:{
            lv_obj_remove_local_style_prop(btn,LV_STYLE_BG_COLOR,0);
            if(next_screen != main_screen)main_block_in_screen(next_screen);
        }
        break;
        default:break;
    }
}
static void main_screen_create(void){
    lv_obj_t* task_list = lv_list_create(screen[main_screen]);
    lv_obj_t* btn;
    lv_obj_set_style_border_width(task_list,0,0);
    lv_obj_set_size(task_list,LV_PCT(100),LV_PCT(100));
    btn = lv_list_add_button(task_list,LV_SYMBOL_WIFI,"WIFI");
    lv_obj_add_event_cb(btn,main_btn_cb,LV_EVENT_ALL,wifi_screen);
    btn = lv_list_add_button(task_list,NULL,"NONE");
    lv_obj_add_event_cb(btn,main_btn_cb,LV_EVENT_ALL,NULL);
    btn = lv_list_add_button(task_list,NULL,"NONE");
    lv_obj_add_event_cb(btn,main_btn_cb,LV_EVENT_ALL,NULL);
    btn = lv_list_add_button(task_list,NULL,"NONE");
    lv_obj_add_event_cb(btn,main_btn_cb,LV_EVENT_ALL,NULL);
    btn = lv_list_add_button(task_list,NULL,"NONE");
    lv_obj_add_event_cb(btn,main_btn_cb,LV_EVENT_ALL,NULL);
    btn = lv_list_add_button(task_list,NULL,"NONE");
    lv_obj_add_event_cb(btn,main_btn_cb,LV_EVENT_ALL,NULL);
    btn = lv_list_add_button(task_list,NULL,"NONE");
    lv_obj_add_event_cb(btn,main_btn_cb,LV_EVENT_ALL,NULL);
    btn = lv_list_add_button(task_list,NULL,"NONE");
    lv_obj_add_event_cb(btn,main_btn_cb,LV_EVENT_ALL,NULL);
}
static void wifi_switch_cb(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* sw = lv_event_get_target_obj(e);
    if(code == LV_EVENT_VALUE_CHANGED){
        if(lv_obj_has_state(sw,LV_STATE_CHECKED)){
            my_wifi_init();
        }
        else{
            my_wifi_deinit();
        }
    }
}
static void wifi_screen_create(void){
    lv_obj_t* wifi_label  = lv_label_create(screen[wifi_screen]);
    lv_obj_t* wifi_switch = lv_switch_create(screen[wifi_screen]);
    lv_obj_t* wifi_info   = lv_list_create(page_block[right_block]);
    lv_obj_t* wifi_tip    = lv_label_create(screen[wifi_screen]);
    lv_obj_set_flex_flow(screen[wifi_screen],LV_FLEX_FLOW_ROW_WRAP);
    lv_label_set_text(wifi_label,LV_SYMBOL_WIFI "WIFI");
    lv_obj_set_style_text_font(wifi_label,&lv_font_montserrat_20,0);
    lv_obj_set_flex_align(screen[wifi_screen],LV_FLEX_ALIGN_SPACE_BETWEEN,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_START);
    lv_obj_set_size(wifi_info,LV_PCT(100),LV_PCT(100));
    lv_obj_center(wifi_tip);
    lv_label_set_text(wifi_tip,"WIFI disconnected");
    lv_obj_add_event_cb(wifi_switch,wifi_switch_cb,LV_EVENT_VALUE_CHANGED,wifi_info);
}
static void main_block_create(void){
    static lv_style_t screen_style;
    lv_style_init(&screen_style);
    lv_style_set_size(&screen_style,LV_PCT(100),LV_PCT(100));
    lv_style_set_pad_all(&screen_style,0);
    lv_style_set_border_width(&screen_style,0);
    for(uint32_t i = 0;i<screen_count;i++){
        screen[i] = lv_obj_create(page_block[main_block]);
        lv_obj_add_flag(screen[i],LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_style(screen[i],&screen_style,0);
    }
    lv_obj_remove_flag(screen[main_screen],LV_OBJ_FLAG_HIDDEN);
    main_screen_create();
    wifi_screen_create();
}
void ui_demo1(void){
    my_mem_init(&ui_mem_pool,ui_mem_buffer,4096);  /* 分配内存池 */
    dlist_init(&screen_list,&ui_mem_pool);      /* 初始化链表,绑定内存池 */
    dlist_insert_head(&screen_list,main_screen); /* 将当前屏幕插入链表 */
    /* 将rootscreen设置为flex_col  */
    lv_obj_set_flex_flow(root_screen,LV_FLEX_FLOW_COLUMN_WRAP);
    /* 区块布局 */
    block_init();
    /* 创建各区块的内容 */
    top_block_create();
    bottom_block_create();
    main_block_create();
}




void lvgl_ui(void){
    lvgl_port_lock(0); 
    ui_init();     
    ui_demo1();  
    lvgl_port_unlock();
}













