/**
 * @file         page.c
 * @brief        页面基础操作实现：创建、销毁、属性存取、槽绑定/退订
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */

#include "page.h"
#include <stdlib.h>
#include <stdio.h>

/** root 创建回调，由应用层通过 page_set_root_created_cb 注册 */
static void (*s_root_created_cb)(lv_obj_t *root) = NULL;

/**
 * @brief        槽内部派发：把消息转发到 page 的 on_msg
 */
static void page_slot_dispatch(void* ctx, const lv_slot_msg_t* msg)
{
    Page* page = (Page*)ctx;
    if (page && page->lifecycle.on_msg) {
        page->lifecycle.on_msg(page, msg);
    }
}

void page_set_root_created_cb(void (*cb)(lv_obj_t *root))
{
    s_root_created_cb = cb;
}

Page* page_create(PageLifecycle* lifecycle, void* params) {
    Page* page = (Page*)calloc(1, sizeof(Page));
    if (!page) return NULL;

    page->root = lv_obj_create(lv_scr_act());
    lv_obj_add_flag(page->root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_scrollbar_mode(page->root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(page->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(page->root, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(page->root, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(page->root, 0, LV_PART_MAIN);

    if (s_root_created_cb) s_root_created_cb(page->root);

    if (lifecycle) {
        page->lifecycle = *lifecycle;
    }

    page->state = PAGE_STATE_CREATED;

    printf("Page: created page=%p, root=%p, state=CREATED\n", page, page->root);

    if (page->lifecycle.on_create) {
        page->lifecycle.on_create(page, params);
    }

    return page;
}

void page_destroy(Page* page) {
    if (!page) return;

    printf("Page: destroying page=%p, root=%p, state=%d\n", page, page->root, page->state);

    /* 自动退订绑定槽的全部订阅 */
    if (page->slot) {
        lv_slot_unsubscribe_all(page->slot, page);
        page->slot = NULL;
    }

    if (page->lifecycle.on_destroy) {
        page->lifecycle.on_destroy(page);
    }

    if (page->root) {
        lv_obj_delete(page->root);
    }

    free(page);
}

lv_obj_t* page_get_root(Page* page) {
    return page ? page->root : NULL;
}

void page_set_user_data(Page* page, void* data) {
    if (page) page->user_data = data;
}

void* page_get_user_data(Page* page) {
    return page ? page->user_data : NULL;
}

int page_bind_slot(Page* page, lv_slot_t* slot, int signal, void* object) {
    if (!page || !slot) return -1;

    int ret = lv_slot_subscribe(slot, signal, object, page_slot_dispatch, page);
    if (ret == 0) {
        page->slot = slot; /* 记录槽供 page_destroy 自动退订 */
    }
    return ret;
}

int page_unbind_slot(Page* page, lv_slot_t* slot, int signal, void* object) {
    if (!page || !slot) return -1;
    return lv_slot_unsubscribe_one(slot, signal, object, page_slot_dispatch, page);
}
