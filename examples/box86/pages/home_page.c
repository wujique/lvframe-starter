#include "home_page.h"
#include "device_page.h"
#include "settings_page.h"
#include "lvframe/page.h"
#include "lvframe/page_manager.h"
#include "lvframe/swipe_container.h"
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 480
#define SCREEN_H 480

typedef struct {
    AppBus*      bus;
    DeviceStore* store;
    lv_obj_t*    tileview;
    lv_obj_t*    settings_cont;
    int          device_ids[MAX_DEVICES];
    int          device_count;
} HomePageData;

static int find_tile_idx(HomePageData* d, int device_id)
{
    for (int i = 0; i < d->device_count; i++) {
        if (d->device_ids[i] == device_id) return i;
    }
    return -1;
}

static void poll_biz_msgs(lv_timer_t* timer)
{
    HomePageData* d = lv_timer_get_user_data(timer);
    AppMsg msg;

    while (app_bus_recv_biz(d->bus, &msg)) {
        switch (msg.type) {

        case MSG_BIZ_ADD_DEVICE: {
            lv_obj_t* tile = swipe_container_add_page(d->tileview, d->device_count);
            DevicePageParams params = {
                .bus       = d->bus,
                .store     = d->store,
                .device_id = msg.device_id,
            };
            device_page_create(tile, &params);
            d->device_ids[d->device_count++] = msg.device_id;
            break;
        }

        case MSG_BIZ_DEL_DEVICE: {
            int idx = find_tile_idx(d, msg.device_id);
            if (idx < 0) break;

            lv_obj_t* tile = lv_obj_get_child(d->tileview, idx);
            if (tile) {
                device_page_destroy(tile);
                swipe_container_remove_page(d->tileview, idx);
            }

            memmove(&d->device_ids[idx], &d->device_ids[idx + 1],
                    (d->device_count - idx - 1) * sizeof(int));
            d->device_count--;

            AppMsg ack = { .type = MSG_UI_DEL_ACK, .device_id = msg.device_id };
            app_bus_send_ui(d->bus, &ack);
            break;
        }

        case MSG_BIZ_REFRESH: {
            int idx = find_tile_idx(d, msg.device_id);
            if (idx < 0) break;
            lv_obj_t* tile = lv_obj_get_child(d->tileview, idx);
            if (tile) device_page_refresh(tile);
            break;
        }

        case MSG_BIZ_MOVE_DEVICE: {
            /* 读快照，按新顺序重建所有 tile */
            DeviceBase bases[MAX_DEVICES];
            int order[MAX_DEVICES], count = 0;
            device_store_snapshot_base(d->store, bases, order, &count);

            for (int i = d->device_count - 1; i >= 0; i--) {
                lv_obj_t* tile = lv_obj_get_child(d->tileview, i);
                if (tile) { device_page_destroy(tile); swipe_container_remove_page(d->tileview, i); }
            }
            d->device_count = 0;

            for (int i = 0; i < count; i++) {
                DeviceBase* b = &bases[order[i]];
                lv_obj_t* tile = swipe_container_add_page(d->tileview, i);
                DevicePageParams params = { .bus = d->bus, .store = d->store, .device_id = b->id };
                device_page_create(tile, &params);
                d->device_ids[d->device_count++] = b->id;
            }
            break;
        }

        case MSG_BIZ_REFRESH_SYS:
            settings_page_refresh(d->settings_cont);
            break;

        default:
            break;
        }
    }
}

static void on_tileview_gesture(lv_event_t* e)
{
    HomePageData* d = lv_event_get_user_data(e);
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

    if (dir == LV_DIR_BOTTOM && swipe_container_get_current(d->tileview) == 0) {
        lv_obj_remove_flag(d->settings_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(d->tileview, LV_OBJ_FLAG_HIDDEN);
    }
}

static void on_settings_gesture(lv_event_t* e)
{
    HomePageData* d = lv_event_get_user_data(e);
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

    if (dir == LV_DIR_TOP) {
        lv_obj_add_flag(d->settings_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(d->tileview, LV_OBJ_FLAG_HIDDEN);
    }
}

static void on_create(Page* page, void* params)
{
    HomePageParams* p = (HomePageParams*)params;
    HomePageData* d = calloc(1, sizeof(HomePageData));
    d->bus   = p->bus;
    d->store = p->store;
    page_set_user_data(page, d);

    lv_obj_t* root = page_get_root(page);
    lv_obj_set_size(root, SCREEN_W, SCREEN_H);

    extern SwipeContainerOps g_tileview_ops;
    swipe_container_register("tileview", &g_tileview_ops);

    d->tileview = swipe_container_create("tileview", root);
    lv_obj_set_size(d->tileview, SCREEN_W, SCREEN_H);
    lv_obj_add_event_cb(d->tileview, on_tileview_gesture, LV_EVENT_GESTURE, d);

    SettingsPageParams sp = { .bus = d->bus, .store = d->store };
    d->settings_cont = settings_page_create(root, &sp);
    lv_obj_add_flag(d->settings_cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(d->settings_cont, on_settings_gesture, LV_EVENT_GESTURE, d);

    /* 读快照，创建初始设备页 */
    DeviceBase bases[MAX_DEVICES];
    int order[MAX_DEVICES], count = 0;
    device_store_snapshot_base(d->store, bases, order, &count);
    for (int i = 0; i < count; i++) {
        DeviceBase* b = &bases[order[i]];
        lv_obj_t* tile = swipe_container_add_page(d->tileview, i);
        DevicePageParams dp = { .bus = d->bus, .store = d->store, .device_id = b->id };
        device_page_create(tile, &dp);
        d->device_ids[d->device_count++] = b->id;
    }

    lv_timer_create(poll_biz_msgs, 50, d);
}

static void on_destroy(Page* page)
{
    HomePageData* d = page_get_user_data(page);
    if (d) {
        settings_page_destroy(d->settings_cont);
        free(d);
    }
}

Page* home_page_creator(void* params)
{
    static PageLifecycle lc = {
        .on_create  = on_create,
        .on_destroy = on_destroy,
    };
    return page_create(&lc, params);
}
