#include "lv_ui.h"
#include "ui/ui.h"

void lv_ui_init(void)
{
    lvgl_port_lock(0);
    ui_init();
    
    lvgl_port_unlock();
}
