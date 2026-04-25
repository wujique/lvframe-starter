#include "blank_page.h"
#include "../screensaver.h"
#include <stdlib.h>

static void on_screen_clicked(lv_event_t* e)
{
    (void)e;
    lv_event_stop_bubbling(e);
    screensaver_wake();
}

static void on_create(Page* page, void* params)
{
    lv_device_store_t* store = (lv_device_store_t*)params;

    lv_obj_t* root = page_get_root(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(root, lv_color_white(), LV_PART_MAIN);
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(root, on_screen_clicked, LV_EVENT_CLICKED, page);

    lv_obj_t* lbl = lv_label_create(root);
    lv_label_set_text(lbl, "息屏");
    lv_obj_center(lbl);

    page_set_user_data(page, store);
}

static void on_destroy(Page* page)
{
    (void)page;
}

Page* blank_page_creator(void* params)
{
    static PageLifecycle lc = {
        .on_create  = on_create,
        .on_destroy = on_destroy,
    };
    return page_create(&lc, params);
}
