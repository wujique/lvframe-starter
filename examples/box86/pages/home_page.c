/**
 * @file         home_page.c
 * @brief        首页实现：tileview 左右滑动。
 *               tile 0 = 卡片首页（一屏 4 张，纵向滚动），tile 1..N = 设备页。
 *               点击卡片开关设备，长按卡片跳到对应设备页；保留下拉设置面板。
 *
 * @author       pochard(email@xxx.com)
 * @version      0.3
 * @date         2026-08-16
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
#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 480
#define SCREEN_H 480

/* 卡片布局：一屏 4 张 */
#define CARD_H      105
#define CARD_GAP    12
#define CARD_PAD    12

/* 下拉触发阈值：松手时 panel 已拖出超过 1/3 屏高则弹出，否则弹回 */
#define PULL_THRESHOLD  (SCREEN_H / 3)
#define SNAP_ANIM_MS    300
#define VERT_DEAD_ZONE  12

typedef enum {
    PULL_STATE_IDLE = 0,
    PULL_STATE_DRAGGING,
    PULL_STATE_SETTINGS,
} PullState;

typedef struct HomePageData HomePageData;

/** 设备卡片信息，挂在卡片 user_data 上 */
typedef struct {
    HomePageData* home;   /**< 首页上下文（长按跳转用）*/
    model_store_t* store;
    void*          model;
    int            type;
    lv_obj_t*      lbl_status;
} CardInfo;

struct HomePageData {
    model_store_t* store;
    lv_obj_t*    root;
    lv_obj_t*    tileview;       /* 左右滑动容器 */
    lv_obj_t*    card_cont;      /* tile 0 的卡片列表容器 */
    lv_obj_t*    settings_cont;  /* 下拉设置面板 */
    void*        models[MODEL_STORE_MAX];       /* 设备模型指针（tile i+1）*/
    Page*        device_pages[MODEL_STORE_MAX]; /* 设备页（tile i+1）*/
    lv_obj_t*    cards[MODEL_STORE_MAX];        /* 卡片（tile 0）*/
    int          device_count;

    PullState    pull_state;
    int32_t      touch_start_x;
    int32_t      touch_start_y;
    int32_t      panel_start_y;
    int32_t      scroll_start_y;
    bool         drag_active;
    bool         locked_horiz;
};

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

/* ── 卡片辅助 ── */
static const char* type_name(int type)
{
    switch (type) {
    case MODEL_TYPE_LIGHT:   return "普通灯";
    case MODEL_TYPE_CCT:     return "色温灯";
    case MODEL_TYPE_CURTAIN: return "电动窗帘";
    default:                 return "未知";
    }
}

static void card_refresh(CardInfo* info)
{
    char status[16];
    bool on = false;

    if (info->type == MODEL_TYPE_CURTAIN) {
        box86_curtain_model_t s;
        if (box86_store_snapshot_curtain(info->store, info->model, &s) == 0) {
            on = s.position > 0;
            snprintf(status, sizeof(status), on ? "OPEN" : "CLOSE");
        } else {
            strcpy(status, "--");
        }
    } else if (info->type == MODEL_TYPE_CCT) {
        box86_cct_light_model_t s;
        if (box86_store_snapshot_cct(info->store, info->model, &s) == 0) {
            on = s.onoffsta;
            snprintf(status, sizeof(status), on ? "ON" : "OFF");
        } else {
            strcpy(status, "--");
        }
    } else {
        box86_light_model_t s;
        if (box86_store_snapshot_light(info->store, info->model, &s) == 0) {
            on = s.onoffsta;
            snprintf(status, sizeof(status), on ? "ON" : "OFF");
        } else {
            strcpy(status, "--");
        }
    }

    lv_label_set_text(info->lbl_status, status);
    lv_obj_set_style_text_color(info->lbl_status,
                                on ? lv_color_hex(0x2E7D32) : lv_color_hex(0x9E9E9E),
                                LV_PART_MAIN);
}

static void card_toggle(CardInfo* info)
{
    lv_slot_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.signal = MSG_UI_SET_PROP;
    msg.object = info->model;

    if (info->type == MODEL_TYPE_CURTAIN) {
        box86_curtain_model_t s;
        if (box86_store_snapshot_curtain(info->store, info->model, &s) < 0) return;
        msg.arg0 = s.position > 0 ? BOX86_CURTAIN_CMD_CLOSE : BOX86_CURTAIN_CMD_OPEN;
        strncpy(msg.field, "command", sizeof(msg.field) - 1);
    } else {
        int onoff = 0;
        if (info->type == MODEL_TYPE_CCT) {
            box86_cct_light_model_t s;
            if (box86_store_snapshot_cct(info->store, info->model, &s) < 0) return;
            onoff = s.onoffsta;
        } else {
            box86_light_model_t s;
            if (box86_store_snapshot_light(info->store, info->model, &s) < 0) return;
            onoff = s.onoffsta;
        }
        msg.arg0 = onoff ? 0 : 1;
        strncpy(msg.field, "onoffsta", sizeof(msg.field) - 1);
    }

    lv_slot_send(&g_dev_slot, &msg);
}

/* ── 卡片事件 ── */
static void on_card_clicked(lv_event_t* e)
{
    lv_obj_t* card = lv_event_get_current_target(e);
    CardInfo* info = (CardInfo*)lv_obj_get_user_data(card);
    if (info) card_toggle(info);
}

static void on_card_long_pressed(lv_event_t* e)
{
    lv_obj_t* card = lv_event_get_current_target(e);
    CardInfo* info = (CardInfo*)lv_obj_get_user_data(card);
    if (!info || !info->home) return;

    /* 跳到该设备对应的 tile（tile 0 是卡片首页） */
    int idx = -1;
    HomePageData* d = info->home;
    for (int i = 0; i < d->device_count; i++) {
        if (d->models[i] == info->model) { idx = i; break; }
    }
    if (idx >= 0) swipe_container_switch_to(d->tileview, idx + 1);
}

/* ── 卡片创建/销毁 ── */
static lv_obj_t* create_card(HomePageData* d, void* model)
{
    CardInfo* info = calloc(1, sizeof(CardInfo));
    info->home  = d;
    info->store = d->store;
    info->model = model;
    info->type  = ((model_base_t*)model)->type;

    lv_obj_t* card = lv_obj_create(d->card_cont);
    lv_obj_set_size(card, LV_PCT(100), CARD_H);
    lv_obj_set_style_radius(card, 12, LV_PART_MAIN);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_user_data(card, info);

    lv_obj_t* lbl_name = lv_label_create(card);
    lv_label_set_text(lbl_name, ((model_base_t*)model)->name);
    lv_obj_align(lbl_name, LV_ALIGN_TOP_LEFT, 16, 14);

    lv_obj_t* lbl_type = lv_label_create(card);
    lv_label_set_text(lbl_type, type_name(info->type));
    lv_obj_set_style_text_color(lbl_type, lv_color_hex(0x8A8A8A), LV_PART_MAIN);
    lv_obj_align(lbl_type, LV_ALIGN_TOP_LEFT, 16, 52);

    info->lbl_status = lv_label_create(card);
    lv_label_set_text(info->lbl_status, "--");
    lv_obj_align(info->lbl_status, LV_ALIGN_RIGHT_MID, -16, 0);

    lv_obj_add_event_cb(card, on_card_clicked, LV_EVENT_SHORT_CLICKED, NULL);
    lv_obj_add_event_cb(card, on_card_long_pressed, LV_EVENT_LONG_PRESSED, NULL);

    card_refresh(info);
    return card;
}

static void destroy_card(lv_obj_t* card)
{
    CardInfo* info = (CardInfo*)lv_obj_get_user_data(card);
    if (info) free(info);
    lv_obj_delete(card);
}

static int find_card_idx(HomePageData* d, void* model)
{
    for (int i = 0; i < d->device_count; i++)
        if (d->models[i] == model) return i;
    return -1;
}

static void card_update(HomePageData* d, void* model)
{
    int idx = find_card_idx(d, model);
    if (idx < 0) return;
    CardInfo* info = (CardInfo*)lv_obj_get_user_data(d->cards[idx]);
    if (info) card_refresh(info);
}

/* 追加一个设备：卡片 + 设备页 tile */
static void append_device(HomePageData* d, void* model)
{
    int n = d->device_count;
    d->cards[n] = create_card(d, model);

    lv_obj_t* tile = swipe_container_add_page(d->tileview, n + 1);
    if (tile) {
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
            lv_obj_add_flag(dp_root, LV_OBJ_FLAG_EVENT_BUBBLE);
            d->device_pages[n] = device_page;
        }
    }
    d->models[n] = model;
    d->device_count++;
}

/* ── 消息处理 ── */
static void on_msg(Page* page, const lv_slot_msg_t* msg)
{
    HomePageData* d = page_get_user_data(page);
    if (!d) return;

    switch (msg->signal) {
    case MSG_DEV_ADD_MODEL: {
        void* model = msg->object;
        if (!model || d->device_count >= MODEL_STORE_MAX) break;
        if (find_card_idx(d, model) >= 0) break;
        append_device(d, model);
        break;
    }
    case MSG_DEV_DEL_MODEL: {
        void* model = msg->object;
        int idx = find_card_idx(d, model);
        if (idx < 0) break;
        int tile_idx = idx + 1;

        int cur = swipe_container_get_current(d->tileview);
        if (cur == tile_idx) {
            if (tile_idx > 1) swipe_container_switch_to(d->tileview, tile_idx - 1);
            else if (d->device_count > 1) swipe_container_switch_to(d->tileview, tile_idx + 1);
        }

        destroy_card(d->cards[idx]);
        if (d->device_pages[idx]) { page_destroy(d->device_pages[idx]); d->device_pages[idx] = NULL; }
        swipe_container_remove_page(d->tileview, tile_idx);

        memmove(&d->cards[idx],        &d->cards[idx+1],        (d->device_count-idx-1)*sizeof(lv_obj_t*));
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
        for (int i = d->device_count - 1; i >= 0; i--) {
            if (d->device_pages[i]) { page_destroy(d->device_pages[i]); d->device_pages[i] = NULL; }
            swipe_container_remove_page(d->tileview, i + 1);
            destroy_card(d->cards[i]);
        }
        d->device_count = 0;

        void* models[MODEL_STORE_MAX];
        int count = 0;
        model_store_snapshot_ordered(d->store, models, &count);
        for (int i = 0; i < count; i++) append_device(d, models[i]);
        break;
    }
    case MSG_DEV_REFRESH_SYS:
        settings_page_refresh(d->settings_cont);
        break;
    case MSG_DEV_LIGHT_ON:
    case MSG_DEV_LIGHT_OFF:
    case MSG_DEV_CCT_TEMP:
    case MSG_DEV_CURTAIN_POS:
        card_update(d, msg->object);
        break;
    default:
        break;
    }
}

/* ── 下拉设置面板触摸 ── */
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
        d->touch_start_x  = pt.x;
        d->touch_start_y  = pt.y;
        d->panel_start_y  = lv_obj_get_y(d->settings_cont);
        d->scroll_start_y = lv_obj_get_scroll_y(d->card_cont);
        d->drag_active    = false;
        d->locked_horiz   = false;
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

            /* 仅首页（tile 0）且卡片列表在顶部时，向下拖拽才触发下拉设置面板 */
            bool can_open  = (d->pull_state == PULL_STATE_IDLE)
                             && (dy > 0)
                             && (swipe_container_get_current(d->tileview) == 0)
                             && (d->scroll_start_y == 0);
            bool can_close = (d->pull_state == PULL_STATE_SETTINGS) && (dy < 0);
            if (!can_open && !can_close) {
                d->locked_horiz = true;
                return;
            }
            d->drag_active = true;
            d->pull_state  = PULL_STATE_DRAGGING;
            lv_obj_set_scroll_dir(d->tileview, LV_DIR_NONE);
            lv_obj_set_scroll_dir(d->card_cont, LV_DIR_NONE);
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
            lv_obj_set_scroll_dir(d->tileview, LV_DIR_HOR);
            lv_obj_set_scroll_dir(d->card_cont, LV_DIR_VER);
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
    d->root = root;
    lv_obj_set_size(root, SCREEN_W, SCREEN_H);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_GESTURE_BUBBLE);

    extern SwipeContainerOps g_tileview_ops;
    swipe_container_register("tileview", &g_tileview_ops);

    d->tileview = swipe_container_create("tileview", root);
    lv_obj_set_size(d->tileview, SCREEN_W, SCREEN_H);
    lv_obj_add_flag(d->tileview, LV_OBJ_FLAG_EVENT_BUBBLE);

    /* tile 0：卡片首页 */
    lv_obj_t* home_tile = swipe_container_add_page(d->tileview, 0);
    d->card_cont = lv_obj_create(home_tile);
    lv_obj_set_size(d->card_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_scrollbar_mode(d->card_cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(d->card_cont, LV_DIR_VER);
    lv_obj_set_flex_flow(d->card_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(d->card_cont, CARD_PAD, LV_PART_MAIN);
    lv_obj_set_style_pad_row(d->card_cont, CARD_GAP, LV_PART_MAIN);
    lv_obj_add_flag(d->card_cont, LV_OBJ_FLAG_EVENT_BUBBLE);
    /* 卡片列表位于 tileview 内，tileview 主题会设置默认字体，
     * 需显式应用中文字体到卡片容器，否则卡片中文不显示 */
    box86_font_apply(d->card_cont);

    /* 下拉设置面板（覆盖在 tileview 之上） */
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

    /* 设备页 tiles 1..N（按显示顺序） */
    void* models[MODEL_STORE_MAX];
    int count = 0;
    model_store_snapshot_ordered(d->store, models, &count);
    for (int i = 0; i < count; i++) append_device(d, models[i]);

    /* 订阅生命周期 + 设备状态信号（object=NULL 全收） */
    page_bind_slot(page, &g_ui_slot, MSG_DEV_ADD_MODEL,   NULL);
    page_bind_slot(page, &g_ui_slot, MSG_DEV_DEL_MODEL,   NULL);
    page_bind_slot(page, &g_ui_slot, MSG_DEV_MOVE_MODEL,  NULL);
    page_bind_slot(page, &g_ui_slot, MSG_DEV_REFRESH_SYS, NULL);
    page_bind_slot(page, &g_ui_slot, MSG_DEV_LIGHT_ON,    NULL);
    page_bind_slot(page, &g_ui_slot, MSG_DEV_LIGHT_OFF,   NULL);
    page_bind_slot(page, &g_ui_slot, MSG_DEV_CCT_TEMP,    NULL);
    page_bind_slot(page, &g_ui_slot, MSG_DEV_CURTAIN_POS, NULL);
}

static void on_destroy(Page* page)
{
    HomePageData* d = page_get_user_data(page);
    if (d) {
        settings_page_destroy(d->settings_cont);
        for (int i = 0; i < d->device_count; i++) {
            if (d->device_pages[i]) { page_destroy(d->device_pages[i]); d->device_pages[i] = NULL; }
            CardInfo* info = (CardInfo*)lv_obj_get_user_data(d->cards[i]);
            if (info) free(info);
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

    /* 切回卡片首页（tile 0）并滚到顶部 */
    swipe_container_switch_to(d->tileview, 0);
    lv_obj_scroll_to_y(d->card_cont, 0, LV_ANIM_OFF);
}
