/**
 * device_store.h - box86 设备数据仓库
 *
 * 基于 lvframe 的 lv_device_store_t 通用存储机制，封装 box86 专用的
 * 设备添加、属性设置和快照接口。业务线程是唯一写入方，UI 线程通过快照读取。
 */

#ifndef BOX86_DEVICE_STORE_H
#define BOX86_DEVICE_STORE_H

#include "device_model.h"
#include "lvframe/device/lv_device_store.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── 添加设备，返回新设备 id，失败返回 -1 ── */
int box86_store_add_light(lv_device_store_t* store, const char* name);
int box86_store_add_cct(lv_device_store_t* store, const char* name);
int box86_store_add_curtain(lv_device_store_t* store, const char* name);

/* ── 设备属性设置（业务线程调用） ── */
int box86_store_set_light_prop(lv_device_store_t* store, int id, const char* field, int value);
int box86_store_set_cct_prop(lv_device_store_t* store, int id, const char* field, int value);
int box86_store_set_curtain_prop(lv_device_store_t* store, int id, const char* field, int value);
int box86_store_set_system(lv_device_store_t* store, const char* field, int value);

/* ── 快照接口（UI 线程调用，加锁后拷贝，线程安全） ── */
int  box86_store_snapshot_light(lv_device_store_t* store, int id, box86_light_model_t* out);
int  box86_store_snapshot_cct(lv_device_store_t* store, int id, box86_cct_light_model_t* out);
int  box86_store_snapshot_curtain(lv_device_store_t* store, int id, box86_curtain_model_t* out);
void box86_store_snapshot_system(lv_device_store_t* store, box86_system_model_t* out);

#ifdef __cplusplus
}
#endif

#endif /* BOX86_DEVICE_STORE_H */
