#ifndef LV_DEVICE_STORE_H
#define LV_DEVICE_STORE_H

#include "lv_device_model.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/osal/lv_os_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * lv_device_store_t — 设备数据仓库
 *
 * 每个设备以 void* 存储（实际指向各自的应用层设备结构体），
 * 通过 lv_device_base_t* 强转访问公共字段。
 * 业务线程是唯一写入方，LVGL 线程通过快照读取。
 */
typedef struct {
    void*             devices[LV_MAX_DEVICES]; /* 指向各设备模型堆内存 */
    int               count;
    int               order[LV_MAX_DEVICES];   /* 显示顺序，存 devices 下标 */
    lv_mutex_t        mutex;
    void*             app_system;  /* 应用层自定义系统数据 */
} lv_device_store_t;

/* ── 生命周期 ── */
void lv_device_store_init(lv_device_store_t* store);
void lv_device_store_deinit(lv_device_store_t* store);

/* ── 底层原语（应用层 store 实现调用，调用前需持锁） ── */

/**
 * 返回下一个可用设备 id（自增最大值 +1）。
 * 调用方须已持有 store->mutex。
 */
int lv_device_store_next_id(lv_device_store_t* store);

/**
 * 将 model 插入 store 并返回其 id；满额时返回 -1。
 * model 必须以 lv_device_base_t 为首成员且已填好 id/type/name。
 * 调用方须已持有 store->mutex。
 */
int lv_device_store_add(lv_device_store_t* store, void* model);

/* ── 通用设备操作 ── */

/** 删除设备（释放堆内存） */
int  lv_device_store_del(lv_device_store_t* store, int id);

/** 调整显示顺序 */
int  lv_device_store_move(lv_device_store_t* store, int id, int new_pos);

/** 在持锁状态下按 id 查找 devices[] 下标，未找到返回 -1 */
int  lv_device_store_find_idx(lv_device_store_t* store, int id);

/* ── 快照接口（LVGL 线程调用） ── */

/**
 * 拷贝所有设备的基础信息（id/type/name）及显示顺序。
 * out/out_order 数组长度须 >= LV_MAX_DEVICES。
 */
int  lv_device_store_snapshot_base(lv_device_store_t* store,
                                   lv_device_base_t* out,
                                   int* out_order,
                                   int* out_count);

#ifdef __cplusplus
}
#endif

#endif /* LV_DEVICE_STORE_H */
