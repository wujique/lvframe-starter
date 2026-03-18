#include "device_page_internal.h"
#include <string.h>
#include <stdlib.h>

static void on_toggle(lv_event_t* e)
{
    DevicePageData* d = lv_event_get_user_data(e);
    if (!d) return;

    LightModel snap;
    if (device_store_snapshot_light(d->store, d->device_id, &snap) < 0) return;

    AppMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.type      = MSG_UI_SET_PROP;
    msg.device_id = d->device_id;
    msg.value     = snap.onoffsta ? 0 : 1;
    strncpy(msg.field, "onoffsta", sizeof(msg.field) - 1);
    app_bus_send_ui(d->bus, &msg);
}

lv_obj_t* light_page_build(lv_obj_t* tile, DevicePageData* d)
{
    lv_obj_set_size(tile, LV_PCT(100), LV_PCT(100));

    d->lbl_name = lv_label_create(tile);
    lv_label_set_text(d->lbl_name, "...");
    lv_obj_align(d->lbl_name, LV_ALIGN_TOP_MID, 0, 20);

    d->btn_toggle = lv_button_create(tile);
    lv_obj_set_size(d->btn_toggle, 120, 50);
    lv_obj_align(d->btn_toggle, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t* lbl = lv_label_create(d->btn_toggle);
    lv_label_set_text(lbl, "On/Off");
    lv_obj_center(lbl);
    lv_obj_add_event_cb(d->btn_toggle, on_toggle, LV_EVENT_CLICKED, d);

    d->lbl_status = lv_label_create(tile);
    lv_label_set_text(d->lbl_status, "---");
    lv_obj_align(d->lbl_status, LV_ALIGN_CENTER, 0, 60);

    return tile;
}

void light_page_refresh(lv_obj_t* tile, DevicePageData* d)
{
    (void)tile;
    LightModel snap;
    if (device_store_snapshot_light(d->store, d->device_id, &snap) < 0) return;
    lv_label_set_text(d->lbl_name, snap.base.name);
    lv_label_set_text(d->lbl_status, snap.onoffsta ? "ON" : "OFF");
}
