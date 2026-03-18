#include "device_store.h"
#include <string.h>
#include <stdlib.h>

void device_store_init(DeviceStore* store)
{
    memset(store, 0, sizeof(DeviceStore));
    lv_mutex_init(&store->mutex);
    store->system.brightness     = 80;
    store->system.volume         = 50;
    store->system.network_enabled = 0;
}

void device_store_deinit(DeviceStore* store)
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

int device_store_find_idx(DeviceStore* store, int id)
{
    for (int i = 0; i < store->count; i++) {
        DeviceBase* b = (DeviceBase*)store->devices[i];
        if (b && b->id == id) return i;
    }
    return -1;
}

static int next_id(DeviceStore* store)
{
    int max_id = 0;
    for (int i = 0; i < store->count; i++) {
        DeviceBase* b = (DeviceBase*)store->devices[i];
        if (b && b->id > max_id) max_id = b->id;
    }
    return max_id + 1;
}

static int store_add(DeviceStore* store, void* model)
{
    if (store->count >= MAX_DEVICES) return -1;
    int idx = store->count;
    store->devices[idx] = model;
    store->order[idx]   = idx;
    store->count++;
    return ((DeviceBase*)model)->id;
}

/* ── 添加接口 ── */

int device_store_add_light(DeviceStore* store, const char* name)
{
    lv_mutex_lock(&store->mutex);
    if (store->count >= MAX_DEVICES) { lv_mutex_unlock(&store->mutex); return -1; }

    LightModel* m = calloc(1, sizeof(LightModel));
    m->base.id   = next_id(store);
    m->base.type = DEVICE_TYPE_LIGHT;
    strncpy(m->base.name, name ? name : "Unnamed", DEVICE_NAME_MAX - 1);

    int id = store_add(store, m);
    lv_mutex_unlock(&store->mutex);
    return id;
}

int device_store_add_cct(DeviceStore* store, const char* name)
{
    lv_mutex_lock(&store->mutex);
    if (store->count >= MAX_DEVICES) { lv_mutex_unlock(&store->mutex); return -1; }

    CctLightModel* m = calloc(1, sizeof(CctLightModel));
    m->base.id   = next_id(store);
    m->base.type = DEVICE_TYPE_CCT;
    strncpy(m->base.name, name ? name : "Unnamed", DEVICE_NAME_MAX - 1);
    m->color_temp = 4000;

    int id = store_add(store, m);
    lv_mutex_unlock(&store->mutex);
    return id;
}

int device_store_add_curtain(DeviceStore* store, const char* name)
{
    lv_mutex_lock(&store->mutex);
    if (store->count >= MAX_DEVICES) { lv_mutex_unlock(&store->mutex); return -1; }

    CurtainModel* m = calloc(1, sizeof(CurtainModel));
    m->base.id   = next_id(store);
    m->base.type = DEVICE_TYPE_CURTAIN;
    strncpy(m->base.name, name ? name : "Unnamed", DEVICE_NAME_MAX - 1);

    int id = store_add(store, m);
    lv_mutex_unlock(&store->mutex);
    return id;
}

/* ── 删除 ── */

int device_store_del(DeviceStore* store, int id)
{
    lv_mutex_lock(&store->mutex);

    int idx = device_store_find_idx(store, id);
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

int device_store_move(DeviceStore* store, int id, int new_pos)
{
    lv_mutex_lock(&store->mutex);

    int idx = device_store_find_idx(store, id);
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

/* ── 属性设置 ── */

int device_store_set_light_prop(DeviceStore* store, int id, const char* field, int value)
{
    lv_mutex_lock(&store->mutex);
    int idx = device_store_find_idx(store, id);
    if (idx < 0) { lv_mutex_unlock(&store->mutex); return -1; }

    DeviceBase* base = (DeviceBase*)store->devices[idx];
    if (base->type != DEVICE_TYPE_LIGHT) { lv_mutex_unlock(&store->mutex); return -1; }

    LightModel* m = (LightModel*)store->devices[idx];
    int ret = 0;
    if (strcmp(field, "onoffsta") == 0) m->onoffsta = value ? 1 : 0;
    else ret = -1;

    lv_mutex_unlock(&store->mutex);
    return ret;
}

int device_store_set_cct_prop(DeviceStore* store, int id, const char* field, int value)
{
    lv_mutex_lock(&store->mutex);
    int idx = device_store_find_idx(store, id);
    if (idx < 0) { lv_mutex_unlock(&store->mutex); return -1; }

    DeviceBase* base = (DeviceBase*)store->devices[idx];
    if (base->type != DEVICE_TYPE_CCT) { lv_mutex_unlock(&store->mutex); return -1; }

    CctLightModel* m = (CctLightModel*)store->devices[idx];
    int ret = 0;
    if (strcmp(field, "onoffsta") == 0) {
        m->onoffsta = value ? 1 : 0;
    } else if (strcmp(field, "color_temp") == 0) {
        if (value < 2700) value = 2700;
        if (value > 6500) value = 6500;
        m->color_temp = value;
    } else { ret = -1; }

    lv_mutex_unlock(&store->mutex);
    return ret;
}

int device_store_set_curtain_prop(DeviceStore* store, int id, const char* field, int value)
{
    lv_mutex_lock(&store->mutex);
    int idx = device_store_find_idx(store, id);
    if (idx < 0) { lv_mutex_unlock(&store->mutex); return -1; }

    DeviceBase* base = (DeviceBase*)store->devices[idx];
    if (base->type != DEVICE_TYPE_CURTAIN) { lv_mutex_unlock(&store->mutex); return -1; }

    CurtainModel* m = (CurtainModel*)store->devices[idx];
    int ret = 0;
    if (strcmp(field, "position") == 0) {
        if (value < 0)   value = 0;
        if (value > 100) value = 100;
        m->position = value;
    } else if (strcmp(field, "command") == 0) {
        m->command = (CurtainCmd)value;
    } else { ret = -1; }

    lv_mutex_unlock(&store->mutex);
    return ret;
}

int device_store_set_system(DeviceStore* store, const char* field, int value)
{
    lv_mutex_lock(&store->mutex);
    int ret = 0;
    if      (strcmp(field, "brightness") == 0) store->system.brightness     = value < 0 ? 0 : (value > 100 ? 100 : value);
    else if (strcmp(field, "volume")     == 0) store->system.volume         = value < 0 ? 0 : (value > 100 ? 100 : value);
    else if (strcmp(field, "network")    == 0) store->system.network_enabled = value ? 1 : 0;
    else ret = -1;
    lv_mutex_unlock(&store->mutex);
    return ret;
}

/* ── 快照接口 ── */

int device_store_snapshot_base(DeviceStore* store, DeviceBase* out, int* out_order, int* out_count)
{
    lv_mutex_lock(&store->mutex);
    *out_count = store->count;
    for (int i = 0; i < store->count; i++) {
        out[i] = *(DeviceBase*)store->devices[i];
    }
    memcpy(out_order, store->order, store->count * sizeof(int));
    lv_mutex_unlock(&store->mutex);
    return 0;
}

int device_store_snapshot_light(DeviceStore* store, int id, LightModel* out)
{
    lv_mutex_lock(&store->mutex);
    int idx = device_store_find_idx(store, id);
    if (idx < 0 || ((DeviceBase*)store->devices[idx])->type != DEVICE_TYPE_LIGHT) {
        lv_mutex_unlock(&store->mutex); return -1;
    }
    *out = *(LightModel*)store->devices[idx];
    lv_mutex_unlock(&store->mutex);
    return 0;
}

int device_store_snapshot_cct(DeviceStore* store, int id, CctLightModel* out)
{
    lv_mutex_lock(&store->mutex);
    int idx = device_store_find_idx(store, id);
    if (idx < 0 || ((DeviceBase*)store->devices[idx])->type != DEVICE_TYPE_CCT) {
        lv_mutex_unlock(&store->mutex); return -1;
    }
    *out = *(CctLightModel*)store->devices[idx];
    lv_mutex_unlock(&store->mutex);
    return 0;
}

int device_store_snapshot_curtain(DeviceStore* store, int id, CurtainModel* out)
{
    lv_mutex_lock(&store->mutex);
    int idx = device_store_find_idx(store, id);
    if (idx < 0 || ((DeviceBase*)store->devices[idx])->type != DEVICE_TYPE_CURTAIN) {
        lv_mutex_unlock(&store->mutex); return -1;
    }
    *out = *(CurtainModel*)store->devices[idx];
    lv_mutex_unlock(&store->mutex);
    return 0;
}

void device_store_snapshot_system(DeviceStore* store, SystemModel* out)
{
    lv_mutex_lock(&store->mutex);
    *out = store->system;
    lv_mutex_unlock(&store->mutex);
}
