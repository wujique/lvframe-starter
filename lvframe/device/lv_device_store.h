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
 * 每个设备以 void* 存储（实际指向各自的 lv_xxx_model_t），
 * 通过 lv_device_base_t* 强转访问公共字段。
 * 业务线程是唯一写入方，LVGL 线程通过快照读取。
 */
typedef struct {
    void*       devices[LV_MAX_DEVICES]; /* 指向各设备模型堆内存 */
    int         count;
    int         order[LV_MAX_DEVICES];   /* 显示顺序，存 devices 下标 */
    lv_system_model_t system;
    lv_mutex_t  mutex;
} lv_device_store_t;

/**
 * 初始化设备仓库
 */
void lv_device_store_init(lv_device_store_t* store);

/**
 * 反初始化设备仓库，释放所有设备内存
 */
void lv_device_store_deinit(lv_device_store_t* store);

/* 添加设备，返回新设备 id，失败返回 -1 */
int  lv_device_store_add_light(lv_device_store_t* store, const char* name);
int  lv_device_store_add_cct(lv_device_store_t* store, const char* name);
int  lv_device_store_add_curtain(lv_device_store_t* store, const char* name);

/* 删除设备（释放堆内存） */
int  lv_device_store_del(lv_device_store_t* store, int id);

/* 调整显示顺序 */
int  lv_device_store_move(lv_device_store_t* store, int id, int new_pos);

/* 各设备属性设置（业务线程调用） */
int  lv_device_store_set_light_prop(lv_device_store_t* store, int id, const char* field, int value);
int  lv_device_store_set_cct_prop(lv_device_store_t* store, int id, const char* field, int value);
int  lv_device_store_set_curtain_prop(lv_device_store_t* store, int id, const char* field, int value);
int  lv_device_store_set_system(lv_device_store_t* store, const char* field, int value);

/**
 * 快照接口（LVGL 线程调用）
 *
 * 返回公共基础信息数组，供 UI 遍历设备列表用。
 * 需要具体设备数据时，用 lv_device_store_snapshot_xxx 按 id 获取。
 */
int  lv_device_store_snapshot_base(lv_device_store_t* store, lv_device_base_t* out, int* out_order, int* out_count);
int  lv_device_store_snapshot_light(lv_device_store_t* store, int id, lv_light_model_t* out);
int  lv_device_store_snapshot_cct(lv_device_store_t* store, int id, lv_cct_light_model_t* out);
int  lv_device_store_snapshot_curtain(lv_device_store_t* store, int id, lv_curtain_model_t* out);
void lv_device_store_snapshot_system(lv_device_store_t* store, lv_system_model_t* out);

/* 内部辅助（调用方需持锁） */
int  lv_device_store_find_idx(lv_device_store_t* store, int id);

#ifdef __cplusplus
}
#endif

#endif /* LV_DEVICE_STORE_H */