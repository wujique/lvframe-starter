#include "device_page_internal.h"
#include <string.h>
#include <stdlib.h>

static void on_toggle(lv_event_t* e)
{
    DevicePageData* d = lv_event_get_user_data(e);
    if (!d) return;

    box86_light_model_t snap;
    if (box86_store_snapshot_light(d->store, d->device_id, &snap) < 0) return;

    AppMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.type      = MSG_UI_SET_PROP;
    msg.device_id = d->device_id;
    msg.value     = snap.onoffsta ? 0 : 1;
    strncpy(msg.field, "onoffsta", sizeof(msg.field) - 1);
    app_bus_send_ui(d->bus, &msg);
}

void light_page_build(lv_obj_t* root, DevicePageData* d)
{
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0xFFFDE7), LV_PART_MAIN); /* 暖黄 */
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);

    d->lbl_name = lv_label_create(root);
    lv_label_set_text(d->lbl_name, "...");
    lv_obj_align(d->lbl_name, LV_ALIGN_TOP_MID, 0, 20);

    d->btn_toggle = lv_button_create(root);
    lv_obj_set_size(d->btn_toggle, 120, 50);
    lv_obj_align(d->btn_toggle, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t* lbl = lv_label_create(d->btn_toggle);
    lv_label_set_text(lbl, "开/关");
    lv_obj_center(lbl);
    lv_obj_add_event_cb(d->btn_toggle, on_toggle, LV_EVENT_CLICKED, d);

    d->lbl_status = lv_label_create(root);
    lv_label_set_text(d->lbl_status, "---");
    lv_obj_align(d->lbl_status, LV_ALIGN_CENTER, 0, 60);

    /* 汉字显示测试 */
    lv_obj_t* lbl_test = lv_label_create(root);
    lv_label_set_text(lbl_test, "汉字测试：普通灯控制面板\n亮度调节 · 开关状态 · 场景模式");
    lv_obj_set_width(lbl_test, LV_PCT(90));
    lv_label_set_long_mode(lbl_test, LV_LABEL_LONG_WRAP);
    lv_obj_align(lbl_test, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void light_page_refresh(DevicePageData* d)
{
    box86_light_model_t snap;
    if (box86_store_snapshot_light(d->store, d->device_id, &snap) < 0) return;
    lv_label_set_text(d->lbl_name, snap.base.name);
    lv_label_set_text(d->lbl_status, snap.onoffsta ? "已开启" : "已关闭");
}
