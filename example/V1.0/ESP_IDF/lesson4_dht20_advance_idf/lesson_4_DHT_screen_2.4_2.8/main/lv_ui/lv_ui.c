#include "lv_ui.h"
#include "ui/ui.h"

// lv_obj_t *scr;

// lv_obj_t *ui_test;

// lv_obj_t *ui_page_zw_label;

void lv_ui_init(void)
{
    lvgl_port_lock(0);

    // scr = lv_scr_act(); /* 获取活动屏幕 */
    // ui_test = lv_obj_create(scr);
    // lv_obj_set_size(ui_test, LV_HOR_RES, LV_VER_RES);
    // ui_page_zw_label = lv_label_create(ui_test);
    // lv_label_set_text(ui_page_zw_label, "test");
    // lv_obj_set_width(ui_page_zw_label, 150);
    // lv_obj_set_style_text_align(ui_page_zw_label, LV_TEXT_ALIGN_CENTER, 0);
    // lv_obj_align(ui_page_zw_label, LV_ALIGN_CENTER, 0, 0);
    ui_init();
    
    lvgl_port_unlock();
}
