#include "device_page_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void on_toggle(lv_event_t* e)
{
    DevicePageData* d = lv_event_get_user_data(e);
    if (!d) return;

    lv_cct_light_model_t snap;
    if (lv_device_store_snapshot_cct(d->store, d->device_id, &snap) < 0) return;

    AppMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.type      = MSG_UI_SET_PROP;
    msg.device_id = d->device_id;
    msg.value     = snap.onoffsta ? 0 : 1;
    strncpy(msg.field, "onoffsta", sizeof(msg.field) - 1);
    app_bus_send_ui(d->bus, &msg);
}

static void on_slider_cct(lv_event_t* e)
{
    DevicePageData* d = lv_event_get_user_data(e);
    if (!d) return;

    int val = (int)lv_slider_get_value(d->slider_cct);

    AppMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.type      = MSG_UI_SET_PROP;
    msg.device_id = d->device_id;
    msg.value     = val;
    strncpy(msg.field, "color_temp", sizeof(msg.field) - 1);
    app_bus_send_ui(d->bus, &msg);

    char buf[16];
    snprintf(buf, sizeof(buf), "%dK", val);
    lv_label_set_text(d->lbl_cct, buf);
}

void cct_light_page_build(lv_obj_t* root, DevicePageData* d)
{
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0xE3F2FD), LV_PART_MAIN); /* 浅蓝 */
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);

    d->lbl_name = lv_label_create(root);
    lv_label_set_text(d->lbl_name, "...");
    lv_obj_align(d->lbl_name, LV_ALIGN_TOP_MID, 0, 20);

    d->btn_toggle = lv_button_create(root);
    lv_obj_set_size(d->btn_toggle, 120, 50);
    lv_obj_align(d->btn_toggle, LV_ALIGN_CENTER, 0, -40);
    lv_obj_t* lbl = lv_label_create(d->btn_toggle);
    lv_label_set_text(lbl, "On/Off");
    lv_obj_center(lbl);
    lv_obj_add_event_cb(d->btn_toggle, on_toggle, LV_EVENT_CLICKED, d);

    d->lbl_status = lv_label_create(root);
    lv_label_set_text(d->lbl_status, "---");
    lv_obj_align(d->lbl_status, LV_ALIGN_CENTER, 0, 20);

    d->slider_cct = lv_slider_create(root);
    lv_slider_set_range(d->slider_cct, 2700, 6500);
    lv_slider_set_value(d->slider_cct, 4000, LV_ANIM_OFF);
    lv_obj_set_width(d->slider_cct, 200);
    lv_obj_align(d->slider_cct, LV_ALIGN_CENTER, 0, 80);
    lv_obj_add_event_cb(d->slider_cct, on_slider_cct, LV_EVENT_VALUE_CHANGED, d);

    d->lbl_cct = lv_label_create(root);
    lv_label_set_text(d->lbl_cct, "4000K");
    lv_obj_align(d->lbl_cct, LV_ALIGN_CENTER, 0, 115);
}

void cct_light_page_refresh(DevicePageData* d)
{
    lv_cct_light_model_t snap;
    if (lv_device_store_snapshot_cct(d->store, d->device_id, &snap) < 0) return;
    lv_label_set_text(d->lbl_name, snap.base.name);
    lv_label_set_text(d->lbl_status, snap.onoffsta ? "ON" : "OFF");
    lv_slider_set_value(d->slider_cct, snap.color_temp, LV_ANIM_OFF);
    char buf[16];
    snprintf(buf, sizeof(buf), "%dK", snap.color_temp);
    lv_label_set_text(d->lbl_cct, buf);
}
