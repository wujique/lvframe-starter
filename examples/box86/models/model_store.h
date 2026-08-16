/**
 * @file         model_store.h
 * @brief        box86 通用模型仓库：DEV 线程唯一写方，UI 线程通过快照读取
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#ifndef BOX86_MODEL_STORE_H
#define BOX86_MODEL_STORE_H

#include "model_base.h"
#include "device_model.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/osal/lv_os_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief        模型仓库：设备模型数组 + 显示顺序 + 常驻 SYSTEM 模型
 */
typedef struct {
    void*                  models[MODEL_STORE_MAX]; /**< 设备模型指针数组（灯/色温灯/窗帘）*/
    int                    count;                   /**< 当前设备模型数量 */
    int                    order[MODEL_STORE_MAX];  /**< 显示顺序，存 models 下标（仅设备模型参与）*/
    box86_system_model_t*  system;                  /**< 常驻 SYSTEM 虚拟模型（id=0）*/
    lv_mutex_t             mutex;                   /**< 保护 models/count/order/system 的互斥锁 */
} model_store_t;

/* ── 生命周期 ── */

void model_store_init(model_store_t* s);
void model_store_deinit(model_store_t* s);

/* ── 通用仓库原语（DEV 线程调用） ── */

/** 将 model 插入仓库并分配 id（max+1），满额返回 -1（调用方须已持锁或内部加锁） */
int  model_store_add(model_store_t* s, void* model);

/** 按模型指针删除并释放堆内存 */
int  model_store_del(model_store_t* s, void* model);

/** 调整指定模型的显示顺序位置 */
int  model_store_move(model_store_t* s, void* model, int new_pos);

/** 按模型指针查找 devices 下标（持锁内使用） */
int  model_store_find(model_store_t* s, void* model);

/** 按 id 查找模型指针（线程安全） */
void* model_store_get_by_id(model_store_t* s, int id);

/** 拷贝所有设备模型指针（按显示顺序）快照（UI 线程调用） */
int  model_store_snapshot_ordered(model_store_t* s, void** out_models, int* out_count);

/** 取常驻 SYSTEM 模型指针 */
box86_system_model_t* model_store_get_system(model_store_t* s);

/* ── box86 类型化：添加设备（DEV） ── */

int  box86_store_add_light(model_store_t* s, const char* name);
int  box86_store_add_cct(model_store_t* s, const char* name);
int  box86_store_add_curtain(model_store_t* s, const char* name);

/* ── box86 类型化：属性写入（DEV） ── */

int  box86_store_set_light_prop(model_store_t* s, void* model, const char* field, int value);
int  box86_store_set_cct_prop(model_store_t* s, void* model, const char* field, int value);
int  box86_store_set_curtain_prop(model_store_t* s, void* model, const char* field, int value);
int  box86_store_set_system(model_store_t* s, const char* field, int value);

/* ── box86 类型化：快照（UI） ── */

int  box86_store_snapshot_light(model_store_t* s, void* model, box86_light_model_t* out);
int  box86_store_snapshot_cct(model_store_t* s, void* model, box86_cct_light_model_t* out);
int  box86_store_snapshot_curtain(model_store_t* s, void* model, box86_curtain_model_t* out);
void box86_store_snapshot_system(model_store_t* s, box86_system_model_t* out);

#ifdef __cplusplus
}
#endif

#endif /* BOX86_MODEL_STORE_H */
