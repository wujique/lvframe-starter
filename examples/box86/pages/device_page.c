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

    switch (d->type) {
    case BOX86_DEVICE_TYPE_CCT:
        cct_light_page_build(root, d);
        break;
    case BOX86_DEVICE_TYPE_CURTAIN:
        curtain_page_build(root, d);
        break;
    default:
        light_page_build(root, d);
        break;
    }

    switch (d->type) {
    case BOX86_DEVICE_TYPE_CCT:
        cct_light_page_refresh(d);
        break;
    case BOX86_DEVICE_TYPE_CURTAIN:
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
        case BOX86_DEVICE_TYPE_CCT:
            cct_light_page_refresh(d);
            break;
        case BOX86_DEVICE_TYPE_CURTAIN:
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
        if (d->type == BOX86_DEVICE_TYPE_LIGHT) {
            light_page_destroy(d);
        } else if (d->type == BOX86_DEVICE_TYPE_CCT) {
            cct_light_page_destroy(d);
        }
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
