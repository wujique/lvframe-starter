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
    printf("[TileView] tileview_add_page: container=%p, index=%d\n", container, index);
    uint32_t count = lv_obj_get_child_count(container);
    printf("[TileView] Current child count: %u\n", count);
    lv_obj_t* tile = lv_tileview_add_tile(container, (uint8_t)count, 0, LV_DIR_HOR);
    printf("[TileView] tile created: %p\n", tile);
    if (tile) {
        printf("[TileView] tile size: %d x %d\n", lv_obj_get_width(tile), lv_obj_get_height(tile));
        printf("[TileView] tile hidden: %s\n", lv_obj_has_flag(tile, LV_OBJ_FLAG_HIDDEN) ? "yes" : "no");
        printf("[TileView] parent size: %d x %d\n", lv_obj_get_width(container), lv_obj_get_height(container));
    }
    /* 用 user_data=1 标记为 tile，与 swipe_container 约定一致 */
    lv_obj_set_user_data(tile, (void*)(uintptr_t)1);
    lv_obj_set_scrollbar_mode(tile, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    /* 让触摸事件从 tile 冒泡到 tileview，再冒泡到 root，
     * 以支持 home_page 在 root 上监听垂直拖拽手势 */
    lv_obj_add_flag(tile, LV_OBJ_FLAG_EVENT_BUBBLE);
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

    /* 删除 tile 后，重排剩余 tile 的列坐标，保持连续性。
     * LVGL tileview 中 tile 的位置由 lv_pct(col_id * 100) 决定，
     * 删除中间 tile 后后续 tile 的坐标不会自动更新，
     * 必须手动重新设置。 */
    uint32_t new_count = lv_obj_get_child_count(container);
    for (uint32_t i = 0; i < new_count; i++) {
        lv_obj_t* tile = lv_obj_get_child(container, i);
        lv_obj_set_pos(tile, lv_pct(i * 100), lv_pct(0));
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
