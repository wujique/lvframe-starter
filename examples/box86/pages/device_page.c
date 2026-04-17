#include "device_page_internal.h"
#include "lvframe/event_bus.h"
#include "app_bus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void on_create(Page* page, void* params)
{
    DevicePageParams* p = (DevicePageParams*)params;
    if (!p || !p->store) {
        page_set_model_valid(page, 0);
        return;
    }

    DevicePageData* d = calloc(1, sizeof(DevicePageData));
    if (!d) {
        page_set_model_valid(page, 0);
        return;
    }

    d->bus = p->bus;
    d->store = p->store;
    d->device_id = p->device_id;

    lv_device_base_t bases[LV_MAX_DEVICES];
    int order[LV_MAX_DEVICES], count = 0;
    lv_device_store_snapshot_base(d->store, bases, order, &count);
    for (int i = 0; i < count; i++) {
        if (bases[i].id == d->device_id) {
            d->type = bases[i].type;
            break;
        }
    }

    page_set_user_data(page, d);

    lv_obj_t* root = page_get_root(page);

    /* 按设备类型设置不同的浅色背景 */
    static const lv_color_t bg_colors[] = {
        [LV_DEVICE_TYPE_LIGHT]   = {.red = 0xFF, .green = 0xFD, .blue = 0xE7}, /* 暖黄：普通灯   #FFFDE7 */
        [LV_DEVICE_TYPE_CCT]     = {.red = 0xE3, .green = 0xF2, .blue = 0xFD}, /* 浅蓝：色温灯   #E3F2FD */
        [LV_DEVICE_TYPE_CURTAIN] = {.red = 0xE8, .green = 0xF5, .blue = 0xE9}, /* 浅绿：窗帘     #E8F5E9 */
    };
    lv_color_t bg = lv_color_hex(0xF5F5F5); /* 浅灰：默认 */
    if ((int)d->type < (int)(sizeof(bg_colors) / sizeof(bg_colors[0])))
        bg = bg_colors[d->type];
    lv_obj_set_style_bg_color(root, bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);

    switch (d->type) {
    case LV_DEVICE_TYPE_CCT:
        cct_light_page_build(root, d);
        break;
    case LV_DEVICE_TYPE_CURTAIN:
        curtain_page_build(root, d);
        break;
    default:
        light_page_build(root, d);
        break;
    }

    switch (d->type) {
    case LV_DEVICE_TYPE_CCT:
        cct_light_page_refresh(d);
        break;
    case LV_DEVICE_TYPE_CURTAIN:
        curtain_page_refresh(d);
        break;
    default:
        light_page_refresh(d);
        break;
    }

    event_bus_subscribe(page, EVENT_APP_MESSAGE);
    printf("[DevicePage] on_create completed, device_id=%d, type=%d\n", d->device_id, d->type);
}

static void on_event(Page* page, Event* event)
{
    if (event->type != EVENT_APP_MESSAGE) return;

    DevicePageData* d = page_get_user_data(page);
    if (!d) return;

    if (event->data.user.device_id != d->device_id) return;

    switch (event->data.user.msg_type) {
    case MSG_BIZ_REFRESH:
        switch (d->type) {
        case LV_DEVICE_TYPE_CCT:
            cct_light_page_refresh(d);
            break;
        case LV_DEVICE_TYPE_CURTAIN:
            curtain_page_refresh(d);
            break;
        default:
            light_page_refresh(d);
            break;
        }
        break;
    case MSG_BIZ_DEL_DEVICE:
        page_set_model_valid(page, 0);
        break;
    default:
        break;
    }
}

static void on_destroy(Page* page)
{
    DevicePageData* d = page_get_user_data(page);
    if (d) {
        event_bus_unsubscribe(page, EVENT_APP_MESSAGE);
        free(d);
    }
}

Page* device_page_create(DevicePageParams* params)
{
    static PageLifecycle lc = {
        .on_create  = on_create,
        .on_destroy = on_destroy,
        .on_event   = on_event,
    };
    return page_create(&lc, params);
}
