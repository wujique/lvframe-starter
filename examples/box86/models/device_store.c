/**
 * device_store.c - box86 设备数据仓库实现
 */

#include "device_store.h"
#include <string.h>
#include <stdlib.h>

/* ── 添加设备 ── */

int box86_store_add_light(lv_device_store_t* store, const char* name)
{
    lv_mutex_lock(&store->mutex);
    if (store->count >= LV_MAX_DEVICES) { lv_mutex_unlock(&store->mutex); return -1; }

    box86_light_model_t* m = calloc(1, sizeof(box86_light_model_t));
    if (!m) { lv_mutex_unlock(&store->mutex); return -1; }
    m->base.id   = lv_device_store_next_id(store);
    m->base.type = BOX86_DEVICE_TYPE_LIGHT;
    strncpy(m->base.name, name ? name : "Unnamed", LV_DEVICE_NAME_MAX - 1);

    int id = lv_device_store_add(store, m);
    lv_mutex_unlock(&store->mutex);
    return id;
}

int box86_store_add_cct(lv_device_store_t* store, const char* name)
{
    lv_mutex_lock(&store->mutex);
    if (store->count >= LV_MAX_DEVICES) { lv_mutex_unlock(&store->mutex); return -1; }

    box86_cct_light_model_t* m = calloc(1, sizeof(box86_cct_light_model_t));
    if (!m) { lv_mutex_unlock(&store->mutex); return -1; }
    m->base.id   = lv_device_store_next_id(store);
    m->base.type = BOX86_DEVICE_TYPE_CCT;
    strncpy(m->base.name, name ? name : "Unnamed", LV_DEVICE_NAME_MAX - 1);
    m->color_temp = 4000;

    int id = lv_device_store_add(store, m);
    lv_mutex_unlock(&store->mutex);
    return id;
}

int box86_store_add_curtain(lv_device_store_t* store, const char* name)
{
    lv_mutex_lock(&store->mutex);
    if (store->count >= LV_MAX_DEVICES) { lv_mutex_unlock(&store->mutex); return -1; }

    box86_curtain_model_t* m = calloc(1, sizeof(box86_curtain_model_t));
    if (!m) { lv_mutex_unlock(&store->mutex); return -1; }
    m->base.id   = lv_device_store_next_id(store);
    m->base.type = BOX86_DEVICE_TYPE_CURTAIN;
    strncpy(m->base.name, name ? name : "Unnamed", LV_DEVICE_NAME_MAX - 1);

    int id = lv_device_store_add(store, m);
    lv_mutex_unlock(&store->mutex);
    return id;
}

/* ── 属性设置 ── */

int box86_store_set_light_prop(lv_device_store_t* store, int id, const char* field, int value)
{
    lv_mutex_lock(&store->mutex);
    int idx = lv_device_store_find_idx(store, id);
    if (idx < 0) { lv_mutex_unlock(&store->mutex); return -1; }

    lv_device_base_t* base = (lv_device_base_t*)store->devices[idx];
    if (base->type != BOX86_DEVICE_TYPE_LIGHT) { lv_mutex_unlock(&store->mutex); return -1; }

    box86_light_model_t* m = (box86_light_model_t*)store->devices[idx];
    int ret = 0;
    if (strcmp(field, "onoffsta") == 0) m->onoffsta = value ? 1 : 0;
    else ret = -1;

    lv_mutex_unlock(&store->mutex);
    return ret;
}

int box86_store_set_cct_prop(lv_device_store_t* store, int id, const char* field, int value)
{
    lv_mutex_lock(&store->mutex);
    int idx = lv_device_store_find_idx(store, id);
    if (idx < 0) { lv_mutex_unlock(&store->mutex); return -1; }

    lv_device_base_t* base = (lv_device_base_t*)store->devices[idx];
    if (base->type != BOX86_DEVICE_TYPE_CCT) { lv_mutex_unlock(&store->mutex); return -1; }

    box86_cct_light_model_t* m = (box86_cct_light_model_t*)store->devices[idx];
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

int box86_store_set_curtain_prop(lv_device_store_t* store, int id, const char* field, int value)
{
    lv_mutex_lock(&store->mutex);
    int idx = lv_device_store_find_idx(store, id);
    if (idx < 0) { lv_mutex_unlock(&store->mutex); return -1; }

    lv_device_base_t* base = (lv_device_base_t*)store->devices[idx];
    if (base->type != BOX86_DEVICE_TYPE_CURTAIN) { lv_mutex_unlock(&store->mutex); return -1; }

    box86_curtain_model_t* m = (box86_curtain_model_t*)store->devices[idx];
    int ret = 0;
    if (strcmp(field, "position") == 0) {
        if (value < 0)   value = 0;
        if (value > 100) value = 100;
        m->position = value;
    } else if (strcmp(field, "command") == 0) {
        m->command = (box86_curtain_cmd_t)value;
    } else { ret = -1; }

    lv_mutex_unlock(&store->mutex);
    return ret;
}

int box86_store_set_system(lv_device_store_t* store, const char* field, int value)
{
    lv_mutex_lock(&store->mutex);
    box86_system_model_t* sys = (box86_system_model_t*)store->app_system;
    if (!sys) { lv_mutex_unlock(&store->mutex); return -1; }
    int ret = 0;
    if      (strcmp(field, "brightness")       == 0) sys->brightness          = value < 0 ? 0 : (value > 100 ? 100 : value);
    else if (strcmp(field, "volume")           == 0) sys->volume              = value < 0 ? 0 : (value > 100 ? 100 : value);
    else if (strcmp(field, "network")          == 0) sys->network_enabled     = value ? 1 : 0;
    else if (strcmp(field, "screensaver")      == 0) sys->screensaver_enabled = value ? 1 : 0;
    else if (strcmp(field, "sa_timeout")       == 0) sys->screensaver_timeout = value < 5 ? 5 : (value > 60 ? 60 : value);
    else if (strcmp(field, "sa_duration")      == 0) sys->screensaver_duration= value < 5 ? 5 : (value > 60 ? 60 : value);
    else if (strcmp(field, "wake_action")      == 0) sys->wake_action         = value ? 1 : 0;
    else ret = -1;
    lv_mutex_unlock(&store->mutex);
    return ret;
}

/* ── 快照接口 ── */

int box86_store_snapshot_light(lv_device_store_t* store, int id, box86_light_model_t* out)
{
    lv_mutex_lock(&store->mutex);
    int idx = lv_device_store_find_idx(store, id);
    if (idx < 0 || ((lv_device_base_t*)store->devices[idx])->type != BOX86_DEVICE_TYPE_LIGHT) {
        lv_mutex_unlock(&store->mutex); return -1;
    }
    *out = *(box86_light_model_t*)store->devices[idx];
    lv_mutex_unlock(&store->mutex);
    return 0;
}

int box86_store_snapshot_cct(lv_device_store_t* store, int id, box86_cct_light_model_t* out)
{
    lv_mutex_lock(&store->mutex);
    int idx = lv_device_store_find_idx(store, id);
    if (idx < 0 || ((lv_device_base_t*)store->devices[idx])->type != BOX86_DEVICE_TYPE_CCT) {
        lv_mutex_unlock(&store->mutex); return -1;
    }
    *out = *(box86_cct_light_model_t*)store->devices[idx];
    lv_mutex_unlock(&store->mutex);
    return 0;
}

int box86_store_snapshot_curtain(lv_device_store_t* store, int id, box86_curtain_model_t* out)
{
    lv_mutex_lock(&store->mutex);
    int idx = lv_device_store_find_idx(store, id);
    if (idx < 0 || ((lv_device_base_t*)store->devices[idx])->type != BOX86_DEVICE_TYPE_CURTAIN) {
        lv_mutex_unlock(&store->mutex); return -1;
    }
    *out = *(box86_curtain_model_t*)store->devices[idx];
    lv_mutex_unlock(&store->mutex);
    return 0;
}

void box86_store_snapshot_system(lv_device_store_t* store, box86_system_model_t* out)
{
    lv_mutex_lock(&store->mutex);
    box86_system_model_t* sys = (box86_system_model_t*)store->app_system;
    if (sys) *out = *sys;
    else memset(out, 0, sizeof(*out));
    lv_mutex_unlock(&store->mutex);
}

/* ── 系统参数初始化 ── */
void box86_store_init_system(lv_device_store_t* store)
{
    lv_mutex_lock(&store->mutex);
    box86_system_model_t* sys = calloc(1, sizeof(box86_system_model_t));
    if (sys) {
        sys->brightness           = 80;
        sys->volume               = 50;
        sys->network_enabled      = 0;
        sys->screensaver_enabled  = 1;
        sys->screensaver_timeout  = 10;
        sys->screensaver_duration = 20;
        sys->wake_action          = 0;
        store->app_system = sys;
    }
    lv_mutex_unlock(&store->mutex);
}
