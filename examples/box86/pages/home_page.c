#include "home_page.h"
#include "device_page.h"
#include "settings_page.h"
#include "lvframe/page.h"
#include "lvframe/page_manager.h"
#include "lvframe/swipe_container.h"
#include "lvframe/device/lv_device_model.h"
#include "app_bus.h"
#include "app_bus_adapter.h"
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
    AppBus*      bus;
    lv_device_store_t* store;
    lv_obj_t*    tileview;
    lv_obj_t*    settings_cont;
    int          device_ids[LV_MAX_DEVICES];
    Page*        device_pages[LV_MAX_DEVICES];
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

/* ── 设备列表事件 ── */
static int find_tile_idx(HomePageData* d, int device_id)
{
    for (int i = 0; i < d->device_count; i++)
        if (d->device_ids[i] == device_id) return i;
    return -1;
}

static void on_event(Page* page, Event* event)
{
    if (event->type != EVENT_APP_MESSAGE) return;
    HomePageData* d = page_get_user_data(page);
    if (!d) return;

    switch (event->data.user.msg_type) {
    case MSG_BIZ_ADD_DEVICE: {
        if (d->device_count >= LV_MAX_DEVICES) break;
        if (find_tile_idx(d, event->data.user.device_id) >= 0) break;
        lv_obj_t* tile = swipe_container_add_page(d->tileview, d->device_count);
        if (!tile) break;
        DevicePageParams params = {
            .bus = d->bus, .store = d->store,
            .device_id = event->data.user.device_id,
        };
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
            d->device_ids[d->device_count]   = event->data.user.device_id;
            d->device_count++;
        }
        break;
    }
    case MSG_BIZ_DEL_DEVICE: {
        int idx = find_tile_idx(d, event->data.user.device_id);
        if (idx < 0) break;
        int cur = swipe_container_get_current(d->tileview);
        if (cur == idx) {
            if (idx > 0) swipe_container_switch_to(d->tileview, idx - 1);
            else if (d->device_count > 1) swipe_container_switch_to(d->tileview, 1);
        }
        if (d->device_pages[idx]) { page_destroy(d->device_pages[idx]); d->device_pages[idx] = NULL; }
        swipe_container_remove_page(d->tileview, idx);
        memmove(&d->device_ids[idx],   &d->device_ids[idx+1],   (d->device_count-idx-1)*sizeof(int));
        memmove(&d->device_pages[idx], &d->device_pages[idx+1], (d->device_count-idx-1)*sizeof(Page*));
        d->device_count--;
        AppMsg ack = { .type = MSG_UI_DEL_ACK, .device_id = event->data.user.device_id };
        app_bus_send_ui(d->bus, &ack);
        break;
    }
    case MSG_BIZ_REFRESH:
        break;
    case MSG_BIZ_MOVE_DEVICE: {
        lv_device_base_t bases[LV_MAX_DEVICES];
        int order[LV_MAX_DEVICES], count = 0;
        lv_device_store_snapshot_base(d->store, bases, order, &count);
        for (int i = d->device_count-1; i >= 0; i--) {
            if (d->device_pages[i]) { page_destroy(d->device_pages[i]); d->device_pages[i] = NULL; }
            swipe_container_remove_page(d->tileview, i);
        }
        d->device_count = 0;
        for (int i = 0; i < count; i++) {
            lv_device_base_t* b = &bases[order[i]];
            lv_obj_t* tile = swipe_container_add_page(d->tileview, i);
            DevicePageParams params = { .bus = d->bus, .store = d->store, .device_id = b->id };
            Page* device_page = device_page_create(&params);
            if (device_page) {
                lv_obj_t* dp_root = page_get_root(device_page);
                lv_obj_set_parent(dp_root, tile);
                lv_obj_set_style_pad_all(dp_root, 0, LV_PART_MAIN);
                lv_obj_set_style_border_width(dp_root, 0, LV_PART_MAIN);
                lv_obj_set_style_radius(dp_root, 0, LV_PART_MAIN);
                lv_obj_clear_flag(dp_root, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(dp_root, LV_OBJ_FLAG_EVENT_BUBBLE);
                d->device_pages[d->device_count] = device_page;
                d->device_ids[d->device_count]   = b->id;
                d->device_count++;
            }
        }
        break;
    }
    case MSG_BIZ_REFRESH_SYS:
        settings_page_refresh(d->settings_cont);
        break;
    default: break;
    }
}

/* ────────────────────────────────────────────────────────────────
 * 触摸事件：跟手拖拽 + 松手弹出/弹回
 *
 * 注册在 settings_cont 上（settings_cont 覆盖全屏，z-order 高于 tileview）。
 * IDLE 状态时 settings_cont 在屏幕上方（y=-SCREEN_H），但仍接收触摸事件，
 * 通过 LV_EVENT_PRESSING 的 EVENT_BUBBLE 从 tileview 冒泡上来。
 *
 * 解决冒泡：
 *   - settings_cont 自身接收 PRESSED/PRESSING/RELEASED（当它可见时手指直接按在它上面）
 *   - tileview 设置 LV_OBJ_FLAG_EVENT_BUBBLE，PRESSING 冒泡到 root，
 *     root 上的回调处理下拉识别
 * ─────────────────────────────────────────────────────────────── */

/* 松手后的弹出/弹回判断 */
static void do_snap(HomePageData* d)
{
    int32_t cur_y = lv_obj_get_y(d->settings_cont);
    /* cur_y 在 [-SCREEN_H, 0]，越大越靠近展开 */
    bool should_open = (cur_y > -SCREEN_H + PULL_THRESHOLD);

    if (should_open) {
        d->pull_state = PULL_STATE_SETTINGS;
        animate_settings_to(d->settings_cont, 0, NULL);
    } else {
        d->pull_state = PULL_STATE_IDLE;
        animate_settings_to(d->settings_cont, -SCREEN_H, NULL);
    }
}

/* root 上的触摸回调（接收从 tileview 冒泡上来的事件） */
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
            /* 判断主方向 */
            int32_t adx = dx < 0 ? -dx : dx;
            int32_t ady = dy < 0 ? -dy : dy;

            if (adx > VERT_DEAD_ZONE && adx > ady) {
                /* 水平滑动，本次锁定为水平，不处理下拉 */
                d->locked_horiz = true;
                return;
            }
            if (ady < VERT_DEAD_ZONE) return; /* 尚未确定方向 */

            /* 确认为垂直拖拽，检查能否激活 */
            bool can_open  = (d->pull_state == PULL_STATE_IDLE)
                             && (dy > 0)
                             && (swipe_container_get_current(d->tileview) == 0);
            bool can_close = (d->pull_state == PULL_STATE_SETTINGS) && (dy < 0);
            if (!can_open && !can_close) {
                d->locked_horiz = true; /* 不满足条件，本次忽略 */
                return;
            }
            d->drag_active = true;
            d->pull_state  = PULL_STATE_DRAGGING;
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
        }
        return;
    }
}

/* settings_cont 自身也需要接收触摸（展开状态下手指直接按在它上面） */
static void on_settings_touch(lv_event_t* e)
{
    /* 转发给 root 的 on_root_touch 逻辑（共用同一个 HomePageData） */
    on_root_touch(e);
}

static void on_create(Page* page, void* params)
{
    HomePageParams* p = (HomePageParams*)params;
    HomePageData* d   = calloc(1, sizeof(HomePageData));
    d->bus        = p->bus;
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
    /* tileview 的触摸事件冒泡到 root，root 负责识别垂直下拉 */
    lv_obj_add_flag(d->tileview, LV_OBJ_FLAG_EVENT_BUBBLE);

    /* settings_cont：初始置于屏幕上方，z-order 高于 tileview（后创建） */
    SettingsPageParams sp = { .bus = d->bus, .store = d->store };
    d->settings_cont = settings_page_create(root, &sp);
    lv_obj_set_pos(d->settings_cont, 0, -SCREEN_H);

    /* root 接收从 tileview 冒泡上来的触摸事件 */
    lv_obj_add_event_cb(root, on_root_touch, LV_EVENT_PRESSED,    d);
    lv_obj_add_event_cb(root, on_root_touch, LV_EVENT_PRESSING,   d);
    lv_obj_add_event_cb(root, on_root_touch, LV_EVENT_RELEASED,   d);
    lv_obj_add_event_cb(root, on_root_touch, LV_EVENT_PRESS_LOST, d);

    /* settings_cont 展开时，手指直接按在它上面，也要处理上推返回 */
    lv_obj_add_event_cb(d->settings_cont, on_settings_touch, LV_EVENT_PRESSED,    d);
    lv_obj_add_event_cb(d->settings_cont, on_settings_touch, LV_EVENT_PRESSING,   d);
    lv_obj_add_event_cb(d->settings_cont, on_settings_touch, LV_EVENT_RELEASED,   d);
    lv_obj_add_event_cb(d->settings_cont, on_settings_touch, LV_EVENT_PRESS_LOST, d);

    /* 初始化设备页 */
    lv_device_base_t bases[LV_MAX_DEVICES];
    int order[LV_MAX_DEVICES], count = 0;
    lv_device_store_snapshot_base(d->store, bases, order, &count);
    for (int i = 0; i < count; i++) {
        lv_device_base_t* b = &bases[order[i]];
        lv_obj_t* tile = swipe_container_add_page(d->tileview, i);
        DevicePageParams dp = { .bus = d->bus, .store = d->store, .device_id = b->id };
        Page* device_page = device_page_create(&dp);
        if (device_page) {
            lv_obj_t* dp_root = page_get_root(device_page);
            lv_obj_set_parent(dp_root, tile);
            lv_obj_set_style_pad_all(dp_root, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(dp_root, 0, LV_PART_MAIN);
            lv_obj_set_style_radius(dp_root, 0, LV_PART_MAIN);
            lv_obj_clear_flag(dp_root, LV_OBJ_FLAG_HIDDEN);
            /* 允许触摸事件冒泡到 tile → tileview → root */
            lv_obj_add_flag(dp_root, LV_OBJ_FLAG_EVENT_BUBBLE);
            d->device_pages[d->device_count] = device_page;
            d->device_ids[d->device_count]   = b->id;
            d->device_count++;
        }
    }

    app_bus_adapter_init(&g_app_bus_adapter, d->bus);
    event_bus_subscribe(page, EVENT_APP_MESSAGE);
    app_bus_adapter_start_auto_poll(&g_app_bus_adapter, 50);
    app_bus_adapter_poll(&g_app_bus_adapter);
}

static void on_destroy(Page* page)
{
    HomePageData* d = page_get_user_data(page);
    if (d) {
        event_bus_unsubscribe(page, EVENT_APP_MESSAGE);
        app_bus_adapter_deinit(&g_app_bus_adapter);
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
        .on_event   = on_event,
    };
    return page_create(&lc, params);
}
