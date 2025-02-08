#include "esp32_ui.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
static lv_obj_t * main_screen = NULL;

static void ui_init(void){
    main_screen = lv_screen_active();
}

void ui_size(void){
    lv_obj_t * sub_obj;
    int32_t obj_h = 0;
    int32_t obj_w = 0;
    int32_t content_w;
    int32_t content_h;
    sub_obj = lv_obj_create(main_screen);
    // lv_obj_set_size(sub_obj,120,100);
    lv_obj_set_width(sub_obj,LV_PCT(70));
    lv_obj_set_height(sub_obj,LV_PCT(70));
    lv_obj_update_layout(sub_obj);
    obj_w = lv_obj_get_width(sub_obj);
    obj_h = lv_obj_get_height(sub_obj);
    content_w = lv_obj_get_content_width(sub_obj);
    content_h = lv_obj_get_content_height(sub_obj);
    LV_LOG_USER("obj_w:obj_h --- %d:%d",obj_w,obj_h);
    LV_LOG_USER("content_w:content_h --- %d:%d",content_w,content_h);

}

void ui_pos(void){
    lv_obj_t * sub_obj;
    lv_obj_t * sub_obj2;
    lv_obj_t * lable;
    lv_obj_t * lable2;
    int32_t obj_x = 0;
    int32_t obj_y = 0;
    int32_t obj_x2 = 0;
    int32_t obj_y2 = 0;
    sub_obj = lv_obj_create(main_screen);
    lv_obj_set_size(sub_obj,LV_PCT(30),LV_PCT(30));
    lv_obj_align(sub_obj,LV_ALIGN_CENTER,0,0);
    // sub_obj2 = lv_obj_create(main_screen);
    // lv_obj_set_size(sub_obj2,LV_PCT(20),LV_PCT(20));
    // lv_obj_align(sub_obj2,LV_ALIGN_CENTER,0,0);

    lable = lv_label_create(sub_obj);
    lv_label_set_text_fmt(lable,"test");
    lv_obj_align(lable,LV_ALIGN_CENTER,0,0);

    lv_obj_update_layout(sub_obj);

    obj_x = lv_obj_get_x(sub_obj);
    obj_y = lv_obj_get_y(sub_obj);
    obj_x2 = lv_obj_get_x2(sub_obj);
    obj_y2 = lv_obj_get_y2(sub_obj);
    LV_LOG_USER("obj_x:obj_y --- %d:%d",obj_x,obj_y);
    LV_LOG_USER("obj_x2:obj_y2 --- %d:%d",obj_x2,obj_y2);
    LV_LOG_USER("x_align:y_align --- %d:%d",lv_obj_get_x_aligned(sub_obj),lv_obj_get_y_aligned(sub_obj));
    // lable2 = lv_label_create(sub_obj2);
    // lv_label_set_text_fmt(lable2,"test2");
    // lv_obj_align(lable2,LV_ALIGN_CENTER,0,0);

    // lv_obj_align_to(sub_obj2,sub_obj,LV_ALIGN_OUT_LEFT_MID,0,0);

    // lv_obj_set_x(sub_obj,100);
    // lv_obj_set_y(sub_obj,100);
    // lv_obj_set_pos(sub_obj,100,100);
    // lv_obj_set_align(sub_obj,LV_ALIGN_CENTER);
    // lv_obj_align(sub_obj,LV_ALIGN_CENTER,0,0);

}

void ui_box_model(void){
    uint16_t box_size = 20;

    lv_obj_t * mid_box = lv_obj_create(main_screen);
    lv_obj_t * left_box = lv_obj_create(main_screen);
    lv_obj_t * right_box = lv_obj_create(main_screen);
    lv_obj_t * t_mid_box = lv_obj_create(main_screen);
    lv_obj_t * t_left_box = lv_obj_create(main_screen);
    lv_obj_t * t_right_box= lv_obj_create(main_screen);
    lv_obj_t * b_mid_box = lv_obj_create(main_screen);
    lv_obj_t * b_left_box = lv_obj_create(main_screen);
    lv_obj_t * b_right_box = lv_obj_create(main_screen);

    lv_obj_set_size(mid_box,LV_PCT(box_size),LV_PCT(box_size));
    lv_obj_set_size(left_box,LV_PCT(box_size),LV_PCT(box_size));
    lv_obj_set_size(right_box,LV_PCT(box_size),LV_PCT(box_size));
    lv_obj_set_size(t_mid_box,LV_PCT(box_size),LV_PCT(box_size));
    lv_obj_set_size(t_left_box,LV_PCT(box_size),LV_PCT(box_size));
    lv_obj_set_size(t_right_box,LV_PCT(box_size),LV_PCT(box_size));
    lv_obj_set_size(b_mid_box,LV_PCT(box_size),LV_PCT(box_size));
    lv_obj_set_size(b_left_box,LV_PCT(box_size),LV_PCT(box_size));
    lv_obj_set_size(b_right_box,LV_PCT(box_size),LV_PCT(box_size));

    lv_obj_set_align(mid_box,LV_ALIGN_CENTER);

    lv_obj_align_to(left_box,mid_box,LV_ALIGN_OUT_LEFT_MID,0,0);
    lv_obj_align_to(right_box,mid_box,LV_ALIGN_OUT_RIGHT_MID,0,0);
    lv_obj_align_to(t_mid_box,mid_box,LV_ALIGN_OUT_TOP_MID,0,0);
    lv_obj_align_to(b_mid_box,mid_box,LV_ALIGN_OUT_BOTTOM_MID,0,0);

    lv_obj_align_to(t_left_box,t_mid_box,LV_ALIGN_OUT_LEFT_MID,0,0);
    lv_obj_align_to(t_right_box,t_mid_box,LV_ALIGN_OUT_RIGHT_MID,0,0);
    lv_obj_align_to(b_left_box,b_mid_box,LV_ALIGN_OUT_LEFT_MID,0,0);
    lv_obj_align_to(b_right_box,b_mid_box,LV_ALIGN_OUT_RIGHT_MID,0,0);



}

void lv_example_get_started_1(void)
{
    lv_obj_t * sub_obj;
    lv_obj_t * lable;
    sub_obj = lv_obj_create(main_screen);
    lv_obj_set_size(sub_obj,LV_PCT(50),LV_PCT(50));
    lv_obj_align(sub_obj,LV_ALIGN_CENTER,0,0);
    lable = lv_label_create(sub_obj);
    lv_label_set_text_fmt(lable,"first\nsecond ");
    lv_obj_align(lable,LV_ALIGN_CENTER,0,0);
}

static void ui(void){
    ui_init();
    //lv_example_get_started_1();
    // ui_size();
    // ui_pos();
    ui_box_model();
}


void lvgl_ui(void){
    lvgl_port_lock(0);      
    ui();  
    lvgl_port_unlock();
}













