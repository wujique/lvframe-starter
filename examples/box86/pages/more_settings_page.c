#include "more_settings_page.h"
#include "lvframe/page.h"
#include "lvframe/page_manager.h"
#include <stdlib.h>

typedef struct {
    AppBus*      bus;
    lv_device_store_t* store;
} MoreSettingsData;

static void on_back_clicked(lv_event_t* e)
{
    (void)e;
    page_manager_back();
}

static void on_create(Page* page, void* params)
{
    MoreSettingsPageParams* p = (MoreSettingsPageParams*)params;
    MoreSettingsData* d = calloc(1, sizeof(MoreSettingsData));
    d->bus   = p->bus;
    d->store = p->store;
    page_set_user_data(page, d);

    lv_obj_t* root = page_get_root(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));

    /* 返回按钮 */
    lv_obj_t* btn_back = lv_button_create(root);
    lv_obj_set_size(btn_back, 60, 40);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t* lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "< Back");
    lv_obj_center(lbl_back);
    lv_obj_add_event_cb(btn_back, on_back_clicked, LV_EVENT_CLICKED, NULL);

    /* 可滚动列表 */
    lv_obj_t* list = lv_list_create(root);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100) - 60);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 60);

    lv_list_add_button(list, NULL, "Volume");
    lv_list_add_button(list, NULL, "Display");
}

static void on_destroy(Page* page)
{
    MoreSettingsData* d = page_get_user_data(page);
    if (d) free(d);
}

Page* more_settings_page_creator(void* params)
{
    static PageLifecycle lc = {
        .on_create  = on_create,
        .on_destroy = on_destroy,
    };
    return page_create(&lc, params);
}
