/**
 * @file         curtain_page.c
 * @brief        电动窗帘设备页：开/关/停按钮控制
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */

#include "device_page_internal.h"
#include "msg.h"
#include "slots.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief        窗帘按钮点击回调，向业务层发送开/关/停命令
 *
 * @param        e                    LVGL 事件
 * @return       void
 */
static void on_curtain_btn(lv_event_t* e)
{
    DevicePageData* d = lv_event_get_user_data(e);
    lv_obj_t* btn = lv_event_get_target(e);
    if (!d) return;

    box86_curtain_cmd_t cmd = BOX86_CURTAIN_CMD_NONE;
    if      (btn == d->btn_open)  cmd = BOX86_CURTAIN_CMD_OPEN;
    else if (btn == d->btn_close) cmd = BOX86_CURTAIN_CMD_CLOSE;
    else if (btn == d->btn_stop)  cmd = BOX86_CURTAIN_CMD_STOP;
    if (cmd == BOX86_CURTAIN_CMD_NONE) return;

    lv_slot_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.signal = MSG_UI_SET_PROP;
    msg.object = d->model;
    msg.arg0   = (int)cmd;
    strncpy(msg.field, "command", sizeof(msg.field) - 1);
    lv_slot_send(&g_dev_slot, &msg);
}

/**
 * @brief        构建电动窗帘设备页 UI
 *
 * @param        root                 页面根容器
 * @param        d                    设备页共享数据
 * @return       void
 */
void curtain_page_build(lv_obj_t* root, DevicePageData* d)
{
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_hex(0xE8F5E9), LV_PART_MAIN); /* 浅绿 */
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);

    d->lbl_name = lv_label_create(root);
    lv_label_set_text(d->lbl_name, "...");
    lv_obj_align(d->lbl_name, LV_ALIGN_TOP_MID, 0, 20);

    d->lbl_status = lv_label_create(root);
    lv_label_set_text(d->lbl_status, "Position: --");
    lv_obj_align(d->lbl_status, LV_ALIGN_CENTER, 0, -50);

    const char* labels[3] = {"Open", "Close", "Stop"};
    lv_obj_t**  btns[3]   = {&d->btn_open, &d->btn_close, &d->btn_stop};
    for (int i = 0; i < 3; i++) {
        *btns[i] = lv_button_create(root);
        lv_obj_set_size(*btns[i], 90, 50);
        lv_obj_align(*btns[i], LV_ALIGN_CENTER, (i - 1) * 110, 20);
        lv_obj_t* lbl = lv_label_create(*btns[i]);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(*btns[i], on_curtain_btn, LV_EVENT_CLICKED, d);
    }
}

/**
 * @brief        刷新电动窗帘设备页数据（名称/位置百分比）
 *
 * @param        d                    设备页共享数据
 * @return       void
 */
void curtain_page_refresh(DevicePageData* d)
{
    box86_curtain_model_t snap;
    if (box86_store_snapshot_curtain(d->store, d->model, &snap) < 0) return;
    lv_label_set_text(d->lbl_name, snap.base.name);
    char buf[32];
    snprintf(buf, sizeof(buf), "Position: %d%%", snap.position);
    lv_label_set_text(d->lbl_status, buf);
}
