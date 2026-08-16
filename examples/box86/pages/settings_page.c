/**
 * @file         settings_page.c
 * @brief        主页快捷设置面板实现：网络开关 + 跳转详细设置
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */

#include "settings_page.h"
#include "more_settings_page.h"
#include "models/model_store.h"
#include "msg.h"
#include "slots.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lvframe/page_manager.h"

typedef struct {
    model_store_t* store;
    lv_obj_t*      btn_network;
    lv_obj_t*      lbl_network;
} SettingsPageData;

/**
 * @brief        网络开关按钮点击回调，乐观更新并发送消息
 *
 * @param        e                    LVGL 事件
 * @return       void
 */
static void on_network_toggle(lv_event_t* e)
{
    SettingsPageData* d = lv_event_get_user_data(e);
    box86_system_model_t sys;
    box86_store_snapshot_system(d->store, &sys);

    lv_slot_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.signal = MSG_UI_SET_SYSTEM;
    msg.object = model_store_get_system(d->store);
    msg.arg0   = sys.network_enabled ? 0 : 1;
    strncpy(msg.field, "network", sizeof(msg.field) - 1);
    lv_slot_send(&g_dev_slot, &msg);

    /* 乐观更新按钮颜色 */
    int new_val = sys.network_enabled ? 0 : 1;
    if (new_val) {
        lv_obj_set_style_bg_color(d->btn_network, lv_color_hex(0xFFB300), 0);
    } else {
        lv_obj_set_style_bg_color(d->btn_network, lv_color_hex(0x607D8B), 0);
    }
    lv_label_set_text(d->lbl_network, new_val ? "Network: ON" : "Network: OFF");
}

/**
 * @brief        更多设置按钮点击回调，通过 page_manager 跳转 MoreSettings 页
 *
 * @param        e                    LVGL 事件
 * @return       void
 */
static void on_more_settings(lv_event_t* e)
{
    SettingsPageData* d = lv_event_get_user_data(e);
    MoreSettingsPageParams params = { .store = d->store };
    /* 使用 page_manager 跳转 */
    extern int page_manager_open(const char* name, void* params);
    int ret = page_manager_open("MoreSettings", &params);
    if (ret != PAGE_MANAGER_OK) {
        printf("Failed to open MoreSettings page: error %d\n", ret);
    }
}

/**
 * @brief        创建快捷设置面板控件
 *
 * @param        parent               父容器
 * @param        params               SettingsPageParams* 创建参数
 * @return       lv_obj_t* 面板根容器
 */
lv_obj_t* settings_page_create(lv_obj_t* parent, SettingsPageParams* params)
{
    SettingsPageData* d = calloc(1, sizeof(SettingsPageData));
    d->store = params->store;

    lv_obj_t* cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(cont, d);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Network toggle button */
    d->btn_network = lv_button_create(cont);
    lv_obj_set_size(d->btn_network, 140, 60);
    d->lbl_network = lv_label_create(d->btn_network);
    lv_obj_center(d->lbl_network);
    lv_obj_add_event_cb(d->btn_network, on_network_toggle, LV_EVENT_CLICKED, d);

    /* More settings button */
    lv_obj_t* btn_more = lv_button_create(cont);
    lv_obj_set_size(btn_more, 140, 60);
    lv_obj_t* lbl_more = lv_label_create(btn_more);
    lv_label_set_text(lbl_more, "More Settings");
    lv_obj_center(lbl_more);
    lv_obj_add_event_cb(btn_more, on_more_settings, LV_EVENT_CLICKED, d);

    settings_page_refresh(cont);
    return cont;
}

/**
 * @brief        刷新快捷设置面板显示（从快照读取系统状态）
 *
 * @param        page                 面板根容器
 * @return       void
 */
void settings_page_refresh(lv_obj_t* page)
{
    SettingsPageData* d = lv_obj_get_user_data(page);
    if (!d) return;
    box86_system_model_t sys;
    box86_store_snapshot_system(d->store, &sys);
    if (sys.network_enabled) {
        lv_obj_set_style_bg_color(d->btn_network, lv_color_hex(0xFFB300), 0);
        lv_label_set_text(d->lbl_network, "Network: ON");
    } else {
        lv_obj_set_style_bg_color(d->btn_network, lv_color_hex(0x607D8B), 0);
        lv_label_set_text(d->lbl_network, "Network: OFF");
    }
}

/**
 * @brief        销毁快捷设置面板，释放内部数据
 *
 * @param        page                 面板根容器
 * @return       void
 */
void settings_page_destroy(lv_obj_t* page)
{
    SettingsPageData* d = lv_obj_get_user_data(page);
    if (d) { free(d); lv_obj_set_user_data(page, NULL); }
}
