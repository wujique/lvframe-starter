#include "device_page_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void on_curtain_btn(lv_event_t* e)
{
    DevicePageData* d = lv_event_get_user_data(e);
    lv_obj_t* btn = lv_event_get_target(e);
    if (!d) return;

    CurtainCmd cmd = CURTAIN_CMD_NONE;
    if      (btn == d->btn_open)  cmd = CURTAIN_CMD_OPEN;
    else if (btn == d->btn_close) cmd = CURTAIN_CMD_CLOSE;
    else if (btn == d->btn_stop)  cmd = CURTAIN_CMD_STOP;
    if (cmd == CURTAIN_CMD_NONE) return;

    AppMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.type      = MSG_UI_SET_PROP;
    msg.device_id = d->device_id;
    msg.value     = (int)cmd;
    strncpy(msg.field, "command", sizeof(msg.field) - 1);
    app_bus_send_ui(d->bus, &msg);
}

lv_obj_t* curtain_page_build(lv_obj_t* tile, DevicePageData* d)
{
    lv_obj_set_size(tile, LV_PCT(100), LV_PCT(100));

    d->lbl_name = lv_label_create(tile);
    lv_label_set_text(d->lbl_name, "...");
    lv_obj_align(d->lbl_name, LV_ALIGN_TOP_MID, 0, 20);

    d->lbl_status = lv_label_create(tile);
    lv_label_set_text(d->lbl_status, "Position: --");
    lv_obj_align(d->lbl_status, LV_ALIGN_CENTER, 0, -50);

    const char* labels[3] = {"Open", "Close", "Stop"};
    lv_obj_t**  btns[3]   = {&d->btn_open, &d->btn_close, &d->btn_stop};
    for (int i = 0; i < 3; i++) {
        *btns[i] = lv_button_create(tile);
        lv_obj_set_size(*btns[i], 90, 50);
        lv_obj_align(*btns[i], LV_ALIGN_CENTER, (i - 1) * 110, 20);
        lv_obj_t* lbl = lv_label_create(*btns[i]);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(*btns[i], on_curtain_btn, LV_EVENT_CLICKED, d);
    }

    return tile;
}

void curtain_page_refresh(lv_obj_t* tile, DevicePageData* d)
{
    (void)tile;
    CurtainModel snap;
    if (device_store_snapshot_curtain(d->store, d->device_id, &snap) < 0) return;
    lv_label_set_text(d->lbl_name, snap.base.name);
    char buf[32];
    snprintf(buf, sizeof(buf), "Position: %d%%", snap.position);
    lv_label_set_text(d->lbl_status, buf);
}
