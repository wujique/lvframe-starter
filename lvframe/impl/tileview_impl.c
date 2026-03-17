/**
 * tileview_impl.c - 基于 lv_tileview 的滑动容器实现（适配 LVGL v9.4）
 */

#include "../swipe_container.h"
#include <stdio.h>

static lv_obj_t* tileview_create(lv_obj_t* parent) {
    lv_obj_t* tv = lv_tileview_create(parent);

    /* 存储 ops 指针到 user_data，供 swipe_container 调度层使用 */
    extern SwipeContainerOps g_tileview_ops;
    lv_obj_set_user_data(tv, &g_tileview_ops);

    return tv;
}

static lv_obj_t* tileview_add_page(lv_obj_t* container, int index) {
    /* v9.4: lv_tileview_add_tile 签名不变，但 col_id 用当前 tile 数量 */
    uint32_t count = lv_obj_get_child_count(container);
    lv_obj_t* tile = lv_tileview_add_tile(container, (uint8_t)count, 0, LV_DIR_HOR);
    /* 用 user_data=1 标记为 tile，与 swipe_container 约定一致 */
    lv_obj_set_user_data(tile, (void*)(uintptr_t)1);
    return tile;
}

static void tileview_remove_page(lv_obj_t* container, int index) {
    /* v9.4: lv_obj_get_child(obj, index) 直接按索引取子对象 */
    uint32_t count = lv_obj_get_child_count(container);
    if ((uint32_t)index >= count) return;

    lv_obj_t* target = lv_obj_get_child(container, index);
    if (target) {
        lv_obj_delete(target);
    }
}

static void tileview_switch_to(lv_obj_t* container, int index) {
    /* v9.4: lv_tileview_set_tile_by_index(tv, col, row, anim) */
    lv_tileview_set_tile_by_index(container, (uint32_t)index, 0, LV_ANIM_ON);
}

static int tileview_get_current(lv_obj_t* container) {
    /* v9.4: lv_tileview_get_tile_active 返回当前 tile 对象，再取其索引 */
    lv_obj_t* active = lv_tileview_get_tile_active(container);
    if (!active) return 0;
    return (int)lv_obj_get_index(active);
}

static int tileview_get_count(lv_obj_t* container) {
    return (int)lv_obj_get_child_count(container);
}

SwipeContainerOps g_tileview_ops = {
    .create      = tileview_create,
    .add_page    = tileview_add_page,
    .remove_page = tileview_remove_page,
    .switch_to   = tileview_switch_to,
    .get_current = tileview_get_current,
    .get_count   = tileview_get_count
};
