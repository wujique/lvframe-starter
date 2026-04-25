#include "lv_device_store.h"
#include <string.h>
#include <stdlib.h>

void lv_device_store_init(lv_device_store_t* store)
{
    memset(store, 0, sizeof(lv_device_store_t));
    lv_mutex_init(&store->mutex);
}

void lv_device_store_deinit(lv_device_store_t* store)
{
    lv_mutex_lock(&store->mutex);
    for (int i = 0; i < store->count; i++) {
        free(store->devices[i]);
        store->devices[i] = NULL;
    }
    store->count = 0;
    lv_mutex_unlock(&store->mutex);
    lv_mutex_delete(&store->mutex);
}

int lv_device_store_find_idx(lv_device_store_t* store, int id)
{
    for (int i = 0; i < store->count; i++) {
        lv_device_base_t* b = (lv_device_base_t*)store->devices[i];
        if (b && b->id == id) return i;
    }
    return -1;
}

/* 调用方须已持锁 */
int lv_device_store_next_id(lv_device_store_t* store)
{
    int max_id = 0;
    for (int i = 0; i < store->count; i++) {
        lv_device_base_t* b = (lv_device_base_t*)store->devices[i];
        if (b && b->id > max_id) max_id = b->id;
    }
    return max_id + 1;
}

/* 调用方须已持锁 */
int lv_device_store_add(lv_device_store_t* store, void* model)
{
    if (store->count >= LV_MAX_DEVICES) return -1;
    int idx = store->count;
    store->devices[idx] = model;
    store->order[idx]   = idx;
    store->count++;
    return ((lv_device_base_t*)model)->id;
}

/* ── 删除 ── */

int lv_device_store_del(lv_device_store_t* store, int id)
{
    lv_mutex_lock(&store->mutex);

    int idx = lv_device_store_find_idx(store, id);
    if (idx < 0) { lv_mutex_unlock(&store->mutex); return -1; }

    /* 从 order 中移除 */
    int order_pos = -1;
    for (int i = 0; i < store->count; i++) {
        if (store->order[i] == idx) { order_pos = i; break; }
    }
    if (order_pos >= 0) {
        memmove(&store->order[order_pos], &store->order[order_pos + 1],
                (store->count - order_pos - 1) * sizeof(int));
    }

    /* 释放内存，末尾元素填入空位 */
    free(store->devices[idx]);
    int last = store->count - 1;
    if (idx != last) {
        store->devices[idx] = store->devices[last];
        for (int i = 0; i < last; i++) {
            if (store->order[i] == last) { store->order[i] = idx; break; }
        }
    }
    store->devices[last] = NULL;
    store->count--;

    lv_mutex_unlock(&store->mutex);
    return 0;
}

/* ── 排序 ── */

int lv_device_store_move(lv_device_store_t* store, int id, int new_pos)
{
    lv_mutex_lock(&store->mutex);

    int idx = lv_device_store_find_idx(store, id);
    if (idx < 0 || new_pos < 0 || new_pos >= store->count) {
        lv_mutex_unlock(&store->mutex); return -1;
    }

    int cur_pos = -1;
    for (int i = 0; i < store->count; i++) {
        if (store->order[i] == idx) { cur_pos = i; break; }
    }
    if (cur_pos < 0) { lv_mutex_unlock(&store->mutex); return -1; }

    int val = store->order[cur_pos];
    if (cur_pos < new_pos) {
        memmove(&store->order[cur_pos], &store->order[cur_pos + 1],
                (new_pos - cur_pos) * sizeof(int));
    } else {
        memmove(&store->order[new_pos + 1], &store->order[new_pos],
                (cur_pos - new_pos) * sizeof(int));
    }
    store->order[new_pos] = val;

    lv_mutex_unlock(&store->mutex);
    return 0;
}

/* ── 快照接口 ── */

int lv_device_store_snapshot_base(lv_device_store_t* store, lv_device_base_t* out, int* out_order, int* out_count)
{
    lv_mutex_lock(&store->mutex);
    *out_count = store->count;
    for (int i = 0; i < store->count; i++) {
        out[i] = *(lv_device_base_t*)store->devices[i];
    }
    memcpy(out_order, store->order, store->count * sizeof(int));
    lv_mutex_unlock(&store->mutex);
    return 0;
}
