#include "more_settings_page.h"
#include "models/device_store.h"
#include "lvframe/page.h"
#include "lvframe/page_manager.h"
#include "app_bus.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    AppBus*              bus;
    lv_device_store_t*   store;
    lv_obj_t*            sw_saver;
    lv_obj_t*            slider_timeout;
    lv_obj_t*            lbl_timeout;
    lv_obj_t*            slider_duration;
    lv_obj_t*            lbl_duration;
    lv_obj_t*            btn_wake_action;
    lv_obj_t*            lbl_wake_action;
} MoreSettingsData;

static void send_system_set(MoreSettingsData* d, const char* field, int value)
{
    AppMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.type      = MSG_UI_SET_SYSTEM;
    msg.device_id = -1;
    msg.value     = value;
    strncpy(msg.field, field, sizeof(msg.field) - 1);
    app_bus_send_ui(d->bus, &msg);
}

/* ── 屏保开关 ── */
static void on_saver_toggle(lv_event_t* e)
{
    MoreSettingsData* d = lv_event_get_user_data(e);
    box86_system_model_t sys;
    box86_store_snapshot_system(d->store, &sys);
    int new_val = sys.screensaver_enabled ? 0 : 1;
    send_system_set(d, "screensaver", new_val);
    if (new_val) {
        lv_obj_set_style_bg_color(d->sw_saver, lv_color_hex(0xFFB300), 0);
        lv_label_set_text(lv_obj_get_child(d->sw_saver, 0), "屏保: 开");
    } else {
        lv_obj_set_style_bg_color(d->sw_saver, lv_color_hex(0x607D8B), 0);
        lv_label_set_text(lv_obj_get_child(d->sw_saver, 0), "屏保: 关");
    }
}

/* ── 待机时间滑块 ── */
static void on_timeout_changed(lv_event_t* e)
{
    MoreSettingsData* d = lv_event_get_user_data(e);
    int val = (int)lv_slider_get_value(d->slider_timeout);
    char buf[16];
    snprintf(buf, sizeof(buf), "%ds", val);
    lv_label_set_text(d->lbl_timeout, buf);
    send_system_set(d, "sa_timeout", val);
}

/* ── 屏保维持滑块 ── */
static void on_duration_changed(lv_event_t* e)
{
    MoreSettingsData* d = lv_event_get_user_data(e);
    int val = (int)lv_slider_get_value(d->slider_duration);
    char buf[16];
    snprintf(buf, sizeof(buf), "%ds", val);
    lv_label_set_text(d->lbl_duration, buf);
    send_system_set(d, "sa_duration", val);
}

/* ── 唤醒行为切换 ── */
static void on_wake_action_clicked(lv_event_t* e)
{
    MoreSettingsData* d = lv_event_get_user_data(e);
    box86_system_model_t sys;
    box86_store_snapshot_system(d->store, &sys);
    int new_val = sys.wake_action ? 0 : 1;
    send_system_set(d, "wake_action", new_val);
    lv_label_set_text(d->lbl_wake_action, new_val ? "唤醒: 返回首页" : "唤醒: 返回屏保");
}

/* ── 返回按钮 ── */
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
    lv_obj_set_scrollbar_mode(root, LV_SCROLLBAR_MODE_ACTIVE);

    /* 返回按钮 */
    lv_obj_t* btn_back = lv_button_create(root);
    lv_obj_set_size(btn_back, 60, 40);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t* lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "< Back");
    lv_obj_center(lbl_back);
    lv_obj_add_event_cb(btn_back, on_back_clicked, LV_EVENT_CLICKED, NULL);

    int y = 70;

    /* 屏保开关 */
    d->sw_saver = lv_button_create(root);
    lv_obj_set_size(d->sw_saver, 140, 50);
    lv_obj_align(d->sw_saver, LV_ALIGN_TOP_RIGHT, -20, y);
    lv_obj_t* lbl_saver_btn = lv_label_create(d->sw_saver);
    lv_label_set_text(lbl_saver_btn, "屏保: 开");
    lv_obj_center(lbl_saver_btn);
    lv_obj_add_event_cb(d->sw_saver, on_saver_toggle, LV_EVENT_CLICKED, d);
    y += 50;

    /* 待机时间 */
    lv_obj_t* lbl_timeout_title = lv_label_create(root);
    lv_label_set_text(lbl_timeout_title, "待机时间:");
    lv_obj_align(lbl_timeout_title, LV_ALIGN_TOP_LEFT, 20, y);

    d->lbl_timeout = lv_label_create(root);
    lv_label_set_text(d->lbl_timeout, "10s");
    lv_obj_align(d->lbl_timeout, LV_ALIGN_TOP_RIGHT, -20, y);

    d->slider_timeout = lv_slider_create(root);
    lv_slider_set_range(d->slider_timeout, 5, 60);
    lv_obj_set_width(d->slider_timeout, 440);
    lv_obj_align(d->slider_timeout, LV_ALIGN_TOP_LEFT, 20, y + 22);
    lv_obj_add_event_cb(d->slider_timeout, on_timeout_changed, LV_EVENT_VALUE_CHANGED, d);
    y += 65;

    /* 屏保维持时间 */
    lv_obj_t* lbl_duration_title = lv_label_create(root);
    lv_label_set_text(lbl_duration_title, "屏保维持:");
    lv_obj_align(lbl_duration_title, LV_ALIGN_TOP_LEFT, 20, y);

    d->lbl_duration = lv_label_create(root);
    lv_label_set_text(d->lbl_duration, "20s");
    lv_obj_align(d->lbl_duration, LV_ALIGN_TOP_RIGHT, -20, y);

    d->slider_duration = lv_slider_create(root);
    lv_slider_set_range(d->slider_duration, 5, 60);
    lv_obj_set_width(d->slider_duration, 440);
    lv_obj_align(d->slider_duration, LV_ALIGN_TOP_LEFT, 20, y + 22);
    lv_obj_add_event_cb(d->slider_duration, on_duration_changed, LV_EVENT_VALUE_CHANGED, d);
    y += 65;

    /* 唤醒行为 */
    d->btn_wake_action = lv_button_create(root);
    lv_obj_set_size(d->btn_wake_action, 300, 50);
    lv_obj_align(d->btn_wake_action, LV_ALIGN_TOP_MID, 0, y);
    d->lbl_wake_action = lv_label_create(d->btn_wake_action);
    lv_label_set_text(d->lbl_wake_action, "唤醒: 返回屏保");
    lv_obj_center(d->lbl_wake_action);
    lv_obj_add_event_cb(d->btn_wake_action, on_wake_action_clicked, LV_EVENT_CLICKED, d);

    /* 刷新显示当前值 */
    box86_system_model_t sys;
    box86_store_snapshot_system(d->store, &sys);
    if (sys.screensaver_enabled) {
        lv_obj_set_style_bg_color(d->sw_saver, lv_color_hex(0xFFB300), 0);
        lv_label_set_text(lv_obj_get_child(d->sw_saver, 0), "屏保: 开");
    } else {
        lv_obj_set_style_bg_color(d->sw_saver, lv_color_hex(0x607D8B), 0);
        lv_label_set_text(lv_obj_get_child(d->sw_saver, 0), "屏保: 关");
    }
    lv_slider_set_value(d->slider_timeout, sys.screensaver_timeout, LV_ANIM_OFF);
    lv_slider_set_value(d->slider_duration, sys.screensaver_duration, LV_ANIM_OFF);
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%ds", sys.screensaver_timeout);
        lv_label_set_text(d->lbl_timeout, buf);
        snprintf(buf, sizeof(buf), "%ds", sys.screensaver_duration);
        lv_label_set_text(d->lbl_duration, buf);
    }
    lv_label_set_text(d->lbl_wake_action, sys.wake_action ? "唤醒: 返回首页" : "唤醒: 返回屏保");
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
