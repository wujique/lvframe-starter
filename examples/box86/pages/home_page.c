/**
 * @file         home_page.c
 * @brief        主页实现：基于 tileview 的水平设备页切换 + 下拉设置面板，
 *               订阅 g_ui_slot 的生命周期信号动态增删设备页
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#include "home_page.h"
#include "device_page.h"
#include "settings_page.h"
#include "lvframe/page.h"
#include "lvframe/page_manager.h"
#include "lvframe/swipe_container.h"
#include "msg.h"
#include "slots.h"
#include "models/model_base.h"
#include "models/model_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 480
#define SCREEN_H 480

/* 下拉触发阈值：松手时 panel 已拖出超过 1/3 屏高则弹出，否则弹回 */
#define PULL_THRESHOLD  (SCREEN_H / 3)
/* 弹出/弹回动画时长(ms) */
#define SNAP_ANIM_MS    300
/* 识别为垂直拖拽的最小垂直位移(px) */
#define VERT_DEAD_ZONE  12

typedef enum {
    PULL_STATE_IDLE = 0,   /* 主页正常显示，设置页收起 */
    PULL_STATE_DRAGGING,   /* 手指拖拽中 */
    PULL_STATE_SETTINGS,   /* 设置页完全展开 */
} PullState;

typedef struct {
    model_store_t* store;
    lv_obj_t*    tileview;
    lv_obj_t*    settings_cont;
    void*        models[MODEL_STORE_MAX];       /* 设备模型指针（按显示顺序）*/
    Page*        device_pages[MODEL_STORE_MAX]; /* 对应设备页 */
    int          device_count;

    PullState    pull_state;
    int32_t      touch_start_y;   /* 本次按下时的手指 y */
    int32_t      touch_start_x;   /* 本次按下时的手指 x（用于排除水平滑动） */
    int32_t      panel_start_y;   /* 本次按下时 settings_cont 的 y */
    bool         drag_active;     /* 已确认为垂直拖拽 */
    bool         locked_horiz;    /* 本次已确认为水平（排除） */
} HomePageData;

/* ── 动画 ── */
static void settings_y_anim_cb(void* obj, int32_t v)
{
    lv_obj_set_y((lv_obj_t*)obj, v);
}

static void animate_settings_to(lv_obj_t* cont, int32_t to_y,
                                 lv_anim_completed_cb_t done_cb)
{
    int32_t from_y = lv_obj_get_y(cont);
    if (from_y == to_y) {
        if (done_cb) {
            lv_anim_t tmp;
            lv_anim_init(&tmp);
            tmp.var = cont;
            done_cb(&tmp);
        }
        return;
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, cont);
    lv_anim_set_exec_cb(&a, settings_y_anim_cb);
    lv_anim_set_values(&a, from_y, to_y);
    lv_anim_set_duration(&a, SNAP_ANIM_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    if (done_cb) lv_anim_set_completed_cb(&a, done_cb);
    lv_anim_start(&a);
}

/* ── 设备列表 ── */
static int find_tile_idx(HomePageData* d, void* model)
{
    for (int i = 0; i < d->device_count; i++)
        if (d->models[i] == model) return i;
    return -1;
}

/* 把一个模型对应的设备页追加到 tileview 末尾 */
static void append_device_page(HomePageData* d, void* model)
{
    lv_obj_t* tile = swipe_container_add_page(d->tileview, d->device_count);
    if (!tile) return;
    DevicePageParams params = { .store = d->store, .model = model };
    Page* device_page = device_page_create(&params);
    if (device_page) {
        lv_obj_t* dp_root = page_get_root(device_page);
        lv_obj_set_parent(dp_root, tile);
        lv_obj_set_size(dp_root, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_pad_all(dp_root, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(dp_root, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(dp_root, 0, LV_PART_MAIN);
        lv_obj_clear_flag(dp_root, LV_OBJ_FLAG_HIDDEN);
        /* device_page root 的触摸事件需要能冒泡到 tile → tileview → home root */
        lv_obj_add_flag(dp_root, LV_OBJ_FLAG_EVENT_BUBBLE);
        d->device_pages[d->device_count] = device_page;
        d->models[d->device_count]       = model;
        d->device_count++;
    }
}

static void on_msg(Page* page, const lv_slot_msg_t* msg)
{
    HomePageData* d = page_get_user_data(page);
    if (!d) return;

    switch (msg->signal) {
    case MSG_DEV_ADD_MODEL: {
        void* model = msg->object;
        if (!model || d->device_count >= MODEL_STORE_MAX) break;
        if (find_tile_idx(d, model) >= 0) break;
        append_device_page(d, model);
        break;
    }
    case MSG_DEV_DEL_MODEL: {
        void* model = msg->object;
        int idx = find_tile_idx(d, model);
        if (idx < 0) break;
        int cur = swipe_container_get_current(d->tileview);
        if (cur == idx) {
            if (idx > 0) swipe_container_switch_to(d->tileview, idx - 1);
            else if (d->device_count > 1) swipe_container_switch_to(d->tileview, 1);
        }
        if (d->device_pages[idx]) { page_destroy(d->device_pages[idx]); d->device_pages[idx] = NULL; }
        swipe_container_remove_page(d->tileview, idx);
        memmove(&d->models[idx],       &d->models[idx+1],       (d->device_count-idx-1)*sizeof(void*));
        memmove(&d->device_pages[idx], &d->device_pages[idx+1], (d->device_count-idx-1)*sizeof(Page*));
        d->device_count--;

        lv_slot_msg_t ack;
        memset(&ack, 0, sizeof(ack));
        ack.signal = MSG_UI_DEL_ACK;
        ack.object = model;
        lv_slot_send(&g_dev_slot, &ack);
        break;
    }
    case MSG_DEV_MOVE_MODEL: {
        void* models[MODEL_STORE_MAX];
        int count = 0;
        model_store_snapshot_ordered(d->store, models, &count);
        for (int i = d->device_count-1; i >= 0; i--) {
            if (d->device_pages[i]) { page_destroy(d->device_pages[i]); d->device_pages[i] = NULL; }
            swipe_container_remove_page(d->tileview, i);
        }
        d->device_count = 0;
        for (int i = 0; i < count; i++) {
            append_device_page(d, models[i]);
        }
        break;
    }
    case MSG_DEV_REFRESH_SYS:
        settings_page_refresh(d->settings_cont);
        break;
    default:
        break;
    }
}

/* ────────────────────────────────────────────────────────────────
 * 触摸事件：跟手拖拽 + 松手弹出/弹回
 * ─────────────────────────────────────────────────────────────── */

static void do_snap(HomePageData* d)
{
    int32_t cur_y = lv_obj_get_y(d->settings_cont);
    bool should_open = (cur_y > -SCREEN_H + PULL_THRESHOLD);

    if (should_open) {
        d->pull_state = PULL_STATE_SETTINGS;
        animate_settings_to(d->settings_cont, 0, NULL);
    } else {
        d->pull_state = PULL_STATE_IDLE;
        animate_settings_to(d->settings_cont, -SCREEN_H, NULL);
    }
}

static void on_root_touch(lv_event_t* e)
{
    HomePageData* d    = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t* indev  = lv_indev_active();
    if (!indev) return;

    lv_point_t pt;
    lv_indev_get_point(indev, &pt);

    if (code == LV_EVENT_PRESSED) {
        d->touch_start_x = pt.x;
        d->touch_start_y = pt.y;
        d->panel_start_y = lv_obj_get_y(d->settings_cont);
        d->drag_active   = false;
        d->locked_horiz  = false;
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        int32_t dx = pt.x - d->touch_start_x;
        int32_t dy = pt.y - d->touch_start_y;

        if (!d->drag_active && !d->locked_horiz) {
            int32_t adx = dx < 0 ? -dx : dx;
            int32_t ady = dy < 0 ? -dy : dy;

            if (adx > VERT_DEAD_ZONE && adx > ady) {
                d->locked_horiz = true;
                return;
            }
            if (ady < VERT_DEAD_ZONE) return;

            bool can_open  = (d->pull_state == PULL_STATE_IDLE)
                             && (dy > 0)
                             && (swipe_container_get_current(d->tileview) == 0);
            bool can_close = (d->pull_state == PULL_STATE_SETTINGS) && (dy < 0);
            if (!can_open && !can_close) {
                d->locked_horiz = true;
                return;
            }
            d->drag_active = true;
            d->pull_state  = PULL_STATE_DRAGGING;
            /* 锁定 tileview 水平滚动，避免斜着拖时同时左右滑页 */
            lv_obj_set_scroll_dir(d->tileview, LV_DIR_NONE);
        }

        if (!d->drag_active) return;

        int32_t new_y = d->panel_start_y + dy;
        if (new_y >  0)        new_y = 0;
        if (new_y < -SCREEN_H) new_y = -SCREEN_H;
        lv_obj_set_y(d->settings_cont, new_y);
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (d->drag_active) {
            d->drag_active = false;
            do_snap(d);
            /* 恢复 tileview 水平滑动 */
            lv_obj_set_scroll_dir(d->tileview, LV_DIR_HOR);
        }
        return;
    }
}

static void on_settings_touch(lv_event_t* e)
{
    on_root_touch(e);
}

static void on_create(Page* page, void* params)
{
    HomePageParams* p = (HomePageParams*)params;
    HomePageData* d   = calloc(1, sizeof(HomePageData));
    d->store      = p->store;
    d->pull_state = PULL_STATE_IDLE;
    page_set_user_data(page, d);

    lv_obj_t* root = page_get_root(page);
    lv_obj_set_size(root, SCREEN_W, SCREEN_H);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_GESTURE_BUBBLE);

    extern SwipeContainerOps g_tileview_ops;
    swipe_container_register("tileview", &g_tileview_ops);

    d->tileview = swipe_container_create("tileview", root);
    lv_obj_set_size(d->tileview, SCREEN_W, SCREEN_H);
    lv_obj_add_flag(d->tileview, LV_OBJ_FLAG_EVENT_BUBBLE);

    SettingsPageParams sp = { .store = d->store };
    d->settings_cont = settings_page_create(root, &sp);
    lv_obj_set_pos(d->settings_cont, 0, -SCREEN_H);

    lv_obj_add_event_cb(root, on_root_touch, LV_EVENT_PRESSED,    d);
    lv_obj_add_event_cb(root, on_root_touch, LV_EVENT_PRESSING,   d);
    lv_obj_add_event_cb(root, on_root_touch, LV_EVENT_RELEASED,   d);
    lv_obj_add_event_cb(root, on_root_touch, LV_EVENT_PRESS_LOST, d);

    lv_obj_add_event_cb(d->settings_cont, on_settings_touch, LV_EVENT_PRESSED,    d);
    lv_obj_add_event_cb(d->settings_cont, on_settings_touch, LV_EVENT_PRESSING,   d);
    lv_obj_add_event_cb(d->settings_cont, on_settings_touch, LV_EVENT_RELEASED,   d);
    lv_obj_add_event_cb(d->settings_cont, on_settings_touch, LV_EVENT_PRESS_LOST, d);

    /* 初始化设备页（按显示顺序） */
    void* models[MODEL_STORE_MAX];
    int count = 0;
    model_store_snapshot_ordered(d->store, models, &count);
    for (int i = 0; i < count; i++) {
        append_device_page(d, models[i]);
    }

    /* 订阅生命周期信号（object=NULL 全收） */
    page_bind_slot(page, &g_ui_slot, MSG_DEV_ADD_MODEL,   NULL);
    page_bind_slot(page, &g_ui_slot, MSG_DEV_DEL_MODEL,   NULL);
    page_bind_slot(page, &g_ui_slot, MSG_DEV_MOVE_MODEL,  NULL);
    page_bind_slot(page, &g_ui_slot, MSG_DEV_REFRESH_SYS, NULL);
}

static void on_destroy(Page* page)
{
    HomePageData* d = page_get_user_data(page);
    if (d) {
        settings_page_destroy(d->settings_cont);
        for (int i = 0; i < d->device_count; i++) {
            if (d->device_pages[i]) { page_destroy(d->device_pages[i]); d->device_pages[i] = NULL; }
        }
        free(d);
    }
}

Page* home_page_creator(void* params)
{
    static PageLifecycle lc = {
        .on_create  = on_create,
        .on_destroy = on_destroy,
        .on_msg     = on_msg,
    };
    return page_create(&lc, params);
}

void home_page_reset_to_first(Page* home_page)
{
    HomePageData* d = page_get_user_data(home_page);
    if (!d) return;

    lv_obj_set_y(d->settings_cont, -SCREEN_H);
    d->pull_state = PULL_STATE_IDLE;

    swipe_container_switch_to(d->tileview, 0);
}
