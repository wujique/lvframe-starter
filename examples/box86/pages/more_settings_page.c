/**
 * @file         more_settings_page.c
 * @brief        屏保详细设置页实现：屏保开关/待机时间/维持时间/唤醒行为
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */

#include "more_settings_page.h"
#include "models/model_store.h"
#include "lvframe/page.h"
#include "lvframe/page_manager.h"
#include "msg.h"
#include "slots.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    model_store_t*       store;
    lv_obj_t*            sw_saver;
    lv_obj_t*            slider_timeout;
    lv_obj_t*            lbl_timeout;
    lv_obj_t*            slider_duration;
    lv_obj_t*            lbl_duration;
    lv_obj_t*            btn_wake_action;
    lv_obj_t*            lbl_wake_action;
} MoreSettingsData;

static void refresh_settings(MoreSettingsData* d);

/**
 * @brief        发送系统属性设置消息到业务层
 *
 * @param        d                    页面私有数据
 * @param        field                属性字段名
 * @param        value                属性新值
 * @return       void
 */
static void send_system_set(MoreSettingsData* d, const char* field, int value)
{
    lv_slot_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.signal = MSG_UI_SET_SYSTEM;
    msg.object = model_store_get_system(d->store);
    msg.arg0   = value;
    strncpy(msg.field, field, sizeof(msg.field) - 1);
    lv_slot_send(&g_dev_slot, &msg);
}

/* ── 屏保开关 ── */
/**
 * @brief        屏保开关按钮点击回调，切换屏保启用状态
 *
 * @param        e                    LVGL 事件
 * @return       void
 */
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
/**
 * @brief        待机时间滑块值变化回调，更新标签并发送消息
 *
 * @param        e                    LVGL 事件
 * @return       void
 */
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
/**
 * @brief        屏保维持时间滑块值变化回调，更新标签并发送消息
 *
 * @param        e                    LVGL 事件
 * @return       void
 */
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
/**
 * @brief        唤醒行为切换按钮回调，在"返回首页"和"返回屏保"间切换
 *
 * @param        e                    LVGL 事件
 * @return       void
 */
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
/**
 * @brief        返回按钮点击回调，调用 page_manager_back() 返回上一页
 *
 * @param        e                    LVGL 事件
 * @return       void
 */
static void on_back_clicked(lv_event_t* e)
{
    (void)e;
    page_manager_back();
}

/**
 * @brief        页面创建回调，构建屏保详细设置 UI
 *
 * @param        page                 当前页面句柄
 * @param        params               MoreSettingsPageParams* 创建参数
 * @return       void
 */
static void on_create(Page* page, void* params)
{
    MoreSettingsPageParams* p = (MoreSettingsPageParams*)params;
    MoreSettingsData* d = calloc(1, sizeof(MoreSettingsData));
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
    refresh_settings(d);

    /* 订阅系统刷新信号 */
    page_bind_slot(page, &g_ui_slot, MSG_DEV_REFRESH_SYS, NULL);
}

/**
 * @brief        从 SYSTEM 模型快照刷新控件
 */
static void refresh_settings(MoreSettingsData* d)
{
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

static void on_msg(Page* page, const lv_slot_msg_t* msg)
{
    (void)msg;
    MoreSettingsData* d = page_get_user_data(page);
    if (!d) return;
    refresh_settings(d);
}

/**
 * @brief        页面销毁回调，释放私有数据堆内存
 *
 * @param        page                 当前页面句柄
 * @return       void
 */
static void on_destroy(Page* page)
{
    MoreSettingsData* d = page_get_user_data(page);
    if (d) free(d);
}

/**
 * @brief        供 page_manager 注册使用的页面工厂函数
 *
 * @param        params               MoreSettingsPageParams* 创建参数
 * @return       Page* 新创建的页面对象
 */
Page* more_settings_page_creator(void* params)
{
    static PageLifecycle lc = {
        .on_create  = on_create,
        .on_destroy = on_destroy,
        .on_msg     = on_msg,
    };
    return page_create(&lc, params);
}
