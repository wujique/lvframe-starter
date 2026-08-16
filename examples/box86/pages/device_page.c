/**
 * @file         device_page.c
 * @brief        设备页按设备类型分发构建与生命周期管理，绑定槽并按 object 匹配
 *
 * @author       pochard(email@xxx.com)
 * @version      0.3
 * @date         2026-08-16
 * @copyright    Copyright (c) 2026..
 */

#include "device_page_internal.h"
#include "msg.h"
#include "slots.h"
#include "models/model_base.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void on_create(Page* page, void* params)
{
    DevicePageParams* p = (DevicePageParams*)params;
    if (!p || !p->store || !p->model) return;

    DevicePageData* d = calloc(1, sizeof(DevicePageData));
    if (!d) return;

    d->store = p->store;
    d->model = p->model;
    d->type  = ((model_base_t*)d->model)->type;

    page_set_user_data(page, d);

    lv_obj_t* root = page_get_root(page);

    switch (d->type) {
    case MODEL_TYPE_CCT:
        cct_light_page_build(root, d);
        break;
    case MODEL_TYPE_CURTAIN:
        curtain_page_build(root, d);
        break;
    default:
        light_page_build(root, d);
        break;
    }

    /* 按信号 + 对象订阅（object = 该模型指针，由槽过滤） */
    switch (d->type) {
    case MODEL_TYPE_CCT:
        page_bind_slot(page, &g_ui_slot, MSG_DEV_LIGHT_ON,  d->model);
        page_bind_slot(page, &g_ui_slot, MSG_DEV_LIGHT_OFF, d->model);
        page_bind_slot(page, &g_ui_slot, MSG_DEV_CCT_TEMP,  d->model);
        break;
    case MODEL_TYPE_CURTAIN:
        page_bind_slot(page, &g_ui_slot, MSG_DEV_CURTAIN_POS, d->model);
        break;
    default:
        page_bind_slot(page, &g_ui_slot, MSG_DEV_LIGHT_ON,  d->model);
        page_bind_slot(page, &g_ui_slot, MSG_DEV_LIGHT_OFF, d->model);
        break;
    }

    switch (d->type) {
    case MODEL_TYPE_CCT:     cct_light_page_refresh(d); break;
    case MODEL_TYPE_CURTAIN: curtain_page_refresh(d);   break;
    default:                 light_page_refresh(d);     break;
    }

    printf("[DevicePage] on_create completed, model=%p, type=%d\n", d->model, d->type);
}

static void on_msg(Page* page, const lv_slot_msg_t* msg)
{
    DevicePageData* d = page_get_user_data(page);
    if (!d) return;

    switch (msg->signal) {
    case MSG_DEV_LIGHT_ON:
    case MSG_DEV_LIGHT_OFF:
    case MSG_DEV_CCT_TEMP:
    case MSG_DEV_CURTAIN_POS:
        switch (d->type) {
        case MODEL_TYPE_CCT:     cct_light_page_refresh(d); break;
        case MODEL_TYPE_CURTAIN: curtain_page_refresh(d);   break;
        default:                 light_page_refresh(d);     break;
        }
        break;
    default:
        break;
    }
}

static void on_destroy(Page* page)
{
    DevicePageData* d = page_get_user_data(page);
    if (d) {
        if (d->type == MODEL_TYPE_LIGHT) {
            light_page_destroy(d);
        } else if (d->type == MODEL_TYPE_CCT) {
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
        .on_msg     = on_msg,
    };
    return page_create(&lc, params);
}
