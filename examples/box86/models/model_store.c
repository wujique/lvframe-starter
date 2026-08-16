/**
 * @file         model_store.c
 * @brief        box86 通用模型仓库实现：设备模型增删改查 + 类型化快照
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#include "model_store.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ── 生命周期 ── */

void model_store_init(model_store_t* s)
{
    memset(s, 0, sizeof(*s));
    lv_mutex_init(&s->mutex);

    /* 常驻 SYSTEM 虚拟模型（id=0） */
    box86_system_model_t* sys = calloc(1, sizeof(*sys));
    if (sys) {
        sys->base.id = 0;
        sys->base.type = MODEL_TYPE_SYSTEM;
        strncpy(sys->base.name, "SYSTEM", MODEL_NAME_MAX - 1);
        sys->base.valid = 1;
        sys->brightness           = 80;
        sys->volume               = 50;
        sys->network_enabled      = 0;
        sys->screensaver_enabled  = 1;
        sys->screensaver_timeout  = 10;
        sys->screensaver_duration = 20;
        sys->wake_action          = 0;
        s->system = sys;
    }
}

void model_store_deinit(model_store_t* s)
{
    lv_mutex_lock(&s->mutex);
    for (int i = 0; i < s->count; i++) {
        free(s->models[i]);
        s->models[i] = NULL;
    }
    s->count = 0;
    if (s->system) {
        free(s->system);
        s->system = NULL;
    }
    lv_mutex_unlock(&s->mutex);
    lv_mutex_delete(&s->mutex);
}

/* ── 通用仓库原语 ── */

int model_store_find(model_store_t* s, void* model)
{
    for (int i = 0; i < s->count; i++) {
        if (s->models[i] == model) return i;
    }
    return -1;
}

void* model_store_get_by_id(model_store_t* s, int id)
{
    lv_mutex_lock(&s->mutex);
    void* ret = NULL;
    for (int i = 0; i < s->count; i++) {
        model_base_t* b = (model_base_t*)s->models[i];
        if (b && b->id == id) { ret = s->models[i]; break; }
    }
    lv_mutex_unlock(&s->mutex);
    return ret;
}

int model_store_add(model_store_t* s, void* model)
{
    if (s->count >= MODEL_STORE_MAX) return -1;

    model_base_t* b = (model_base_t*)model;

    /* 分配 id（max+1） */
    int max_id = 0;
    for (int i = 0; i < s->count; i++) {
        model_base_t* m = (model_base_t*)s->models[i];
        if (m && m->id > max_id) max_id = m->id;
    }
    b->id = max_id + 1;

    int idx = s->count;
    s->models[idx] = model;
    s->order[idx]  = idx;
    s->count++;
    return b->id;
}

int model_store_del(model_store_t* s, void* model)
{
    lv_mutex_lock(&s->mutex);

    int idx = model_store_find(s, model);
    if (idx < 0) { lv_mutex_unlock(&s->mutex); return -1; }

    /* 从 order 中移除 */
    int order_pos = -1;
    for (int i = 0; i < s->count; i++) {
        if (s->order[i] == idx) { order_pos = i; break; }
    }
    if (order_pos >= 0) {
        memmove(&s->order[order_pos], &s->order[order_pos + 1],
                (s->count - order_pos - 1) * sizeof(int));
    }

    /* 释放内存，末尾元素填入空位 */
    free(s->models[idx]);
    int last = s->count - 1;
    if (idx != last) {
        s->models[idx] = s->models[last];
        for (int i = 0; i < last; i++) {
            if (s->order[i] == last) { s->order[i] = idx; break; }
        }
    }
    s->models[last] = NULL;
    s->count--;

    lv_mutex_unlock(&s->mutex);
    return 0;
}

int model_store_move(model_store_t* s, void* model, int new_pos)
{
    lv_mutex_lock(&s->mutex);

    int idx = model_store_find(s, model);
    if (idx < 0 || new_pos < 0 || new_pos >= s->count) {
        lv_mutex_unlock(&s->mutex); return -1;
    }

    int cur_pos = -1;
    for (int i = 0; i < s->count; i++) {
        if (s->order[i] == idx) { cur_pos = i; break; }
    }
    if (cur_pos < 0) { lv_mutex_unlock(&s->mutex); return -1; }

    int val = s->order[cur_pos];
    if (cur_pos < new_pos) {
        memmove(&s->order[cur_pos], &s->order[cur_pos + 1],
                (new_pos - cur_pos) * sizeof(int));
    } else {
        memmove(&s->order[new_pos + 1], &s->order[new_pos],
                (cur_pos - new_pos) * sizeof(int));
    }
    s->order[new_pos] = val;

    lv_mutex_unlock(&s->mutex);
    return 0;
}

int model_store_snapshot_ordered(model_store_t* s, void** out_models, int* out_count)
{
    lv_mutex_lock(&s->mutex);
    *out_count = s->count;
    for (int i = 0; i < s->count; i++) {
        out_models[i] = s->models[s->order[i]];
    }
    lv_mutex_unlock(&s->mutex);
    return 0;
}

box86_system_model_t* model_store_get_system(model_store_t* s)
{
    return s->system;
}

/* ── box86 类型化：添加设备 ── */

int box86_store_add_light(model_store_t* s, const char* name)
{
    lv_mutex_lock(&s->mutex);
    if (s->count >= MODEL_STORE_MAX) { lv_mutex_unlock(&s->mutex); return -1; }

    box86_light_model_t* m = calloc(1, sizeof(*m));
    if (!m) { lv_mutex_unlock(&s->mutex); return -1; }
    m->base.type  = MODEL_TYPE_LIGHT;
    m->base.valid = 1;
    strncpy(m->base.name, name ? name : "Unnamed", MODEL_NAME_MAX - 1);

    int id = model_store_add(s, m);
    lv_mutex_unlock(&s->mutex);
    return id;
}

int box86_store_add_cct(model_store_t* s, const char* name)
{
    lv_mutex_lock(&s->mutex);
    if (s->count >= MODEL_STORE_MAX) { lv_mutex_unlock(&s->mutex); return -1; }

    box86_cct_light_model_t* m = calloc(1, sizeof(*m));
    if (!m) { lv_mutex_unlock(&s->mutex); return -1; }
    m->base.type  = MODEL_TYPE_CCT;
    m->base.valid = 1;
    m->color_temp = 4000;
    strncpy(m->base.name, name ? name : "Unnamed", MODEL_NAME_MAX - 1);

    int id = model_store_add(s, m);
    lv_mutex_unlock(&s->mutex);
    return id;
}

int box86_store_add_curtain(model_store_t* s, const char* name)
{
    lv_mutex_lock(&s->mutex);
    if (s->count >= MODEL_STORE_MAX) { lv_mutex_unlock(&s->mutex); return -1; }

    box86_curtain_model_t* m = calloc(1, sizeof(*m));
    if (!m) { lv_mutex_unlock(&s->mutex); return -1; }
    m->base.type  = MODEL_TYPE_CURTAIN;
    m->base.valid = 1;
    strncpy(m->base.name, name ? name : "Unnamed", MODEL_NAME_MAX - 1);

    int id = model_store_add(s, m);
    lv_mutex_unlock(&s->mutex);
    return id;
}

/* ── box86 类型化：属性写入 ── */

int box86_store_set_light_prop(model_store_t* s, void* model, const char* field, int value)
{
    lv_mutex_lock(&s->mutex);
    model_base_t* base = (model_base_t*)model;
    if (!base || base->type != MODEL_TYPE_LIGHT || !base->valid) {
        lv_mutex_unlock(&s->mutex); return -1;
    }
    box86_light_model_t* m = (box86_light_model_t*)model;
    int ret = 0;
    if (strcmp(field, "onoffsta") == 0) m->onoffsta = value ? 1 : 0;
    else ret = -1;
    lv_mutex_unlock(&s->mutex);
    return ret;
}

int box86_store_set_cct_prop(model_store_t* s, void* model, const char* field, int value)
{
    lv_mutex_lock(&s->mutex);
    model_base_t* base = (model_base_t*)model;
    if (!base || base->type != MODEL_TYPE_CCT || !base->valid) {
        lv_mutex_unlock(&s->mutex); return -1;
    }
    box86_cct_light_model_t* m = (box86_cct_light_model_t*)model;
    int ret = 0;
    if (strcmp(field, "onoffsta") == 0) {
        m->onoffsta = value ? 1 : 0;
    } else if (strcmp(field, "color_temp") == 0) {
        if (value < 2700) value = 2700;
        if (value > 6500) value = 6500;
        m->color_temp = value;
    } else { ret = -1; }
    lv_mutex_unlock(&s->mutex);
    return ret;
}

int box86_store_set_curtain_prop(model_store_t* s, void* model, const char* field, int value)
{
    lv_mutex_lock(&s->mutex);
    model_base_t* base = (model_base_t*)model;
    if (!base || base->type != MODEL_TYPE_CURTAIN || !base->valid) {
        lv_mutex_unlock(&s->mutex); return -1;
    }
    box86_curtain_model_t* m = (box86_curtain_model_t*)model;
    int ret = 0;
    if (strcmp(field, "position") == 0) {
        if (value < 0)   value = 0;
        if (value > 100) value = 100;
        m->position = value;
    } else if (strcmp(field, "command") == 0) {
        m->command = (box86_curtain_cmd_t)value;
    } else { ret = -1; }
    lv_mutex_unlock(&s->mutex);
    return ret;
}

int box86_store_set_system(model_store_t* s, const char* field, int value)
{
    lv_mutex_lock(&s->mutex);
    box86_system_model_t* sys = s->system;
    if (!sys) { lv_mutex_unlock(&s->mutex); return -1; }
    int ret = 0;
    if      (strcmp(field, "brightness")  == 0) sys->brightness          = value < 0 ? 0 : (value > 100 ? 100 : value);
    else if (strcmp(field, "volume")      == 0) sys->volume              = value < 0 ? 0 : (value > 100 ? 100 : value);
    else if (strcmp(field, "network")     == 0) sys->network_enabled     = value ? 1 : 0;
    else if (strcmp(field, "screensaver") == 0) sys->screensaver_enabled = value ? 1 : 0;
    else if (strcmp(field, "sa_timeout")  == 0) sys->screensaver_timeout = value < 5 ? 5 : (value > 60 ? 60 : value);
    else if (strcmp(field, "sa_duration") == 0) sys->screensaver_duration= value < 5 ? 5 : (value > 60 ? 60 : value);
    else if (strcmp(field, "wake_action") == 0) sys->wake_action         = value ? 1 : 0;
    else ret = -1;
    lv_mutex_unlock(&s->mutex);
    return ret;
}

/* ── box86 类型化：快照 ── */

int box86_store_snapshot_light(model_store_t* s, void* model, box86_light_model_t* out)
{
    lv_mutex_lock(&s->mutex);
    model_base_t* base = (model_base_t*)model;
    if (!base || base->type != MODEL_TYPE_LIGHT || !base->valid) {
        lv_mutex_unlock(&s->mutex); return -1;
    }
    *out = *(box86_light_model_t*)model;
    lv_mutex_unlock(&s->mutex);
    return 0;
}

int box86_store_snapshot_cct(model_store_t* s, void* model, box86_cct_light_model_t* out)
{
    lv_mutex_lock(&s->mutex);
    model_base_t* base = (model_base_t*)model;
    if (!base || base->type != MODEL_TYPE_CCT || !base->valid) {
        lv_mutex_unlock(&s->mutex); return -1;
    }
    *out = *(box86_cct_light_model_t*)model;
    lv_mutex_unlock(&s->mutex);
    return 0;
}

int box86_store_snapshot_curtain(model_store_t* s, void* model, box86_curtain_model_t* out)
{
    lv_mutex_lock(&s->mutex);
    model_base_t* base = (model_base_t*)model;
    if (!base || base->type != MODEL_TYPE_CURTAIN || !base->valid) {
        lv_mutex_unlock(&s->mutex); return -1;
    }
    *out = *(box86_curtain_model_t*)model;
    lv_mutex_unlock(&s->mutex);
    return 0;
}

void box86_store_snapshot_system(model_store_t* s, box86_system_model_t* out)
{
    lv_mutex_lock(&s->mutex);
    if (s->system) *out = *s->system;
    else memset(out, 0, sizeof(*out));
    lv_mutex_unlock(&s->mutex);
}
