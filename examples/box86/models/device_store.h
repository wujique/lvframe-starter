#ifndef DEVICE_STORE_H
#define DEVICE_STORE_H

#include "device_model.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/osal/lv_os_private.h"

/**
 * device_store — 设备数据仓库
 *
 * 每个设备以 void* 存储（实际指向各自的 XxxModel），
 * 通过 DeviceBase* 强转访问公共字段。
 * 业务线程是唯一写入方，LVGL 线程通过快照读取。
 */
typedef struct {
    void*       devices[MAX_DEVICES]; /* 指向各设备模型堆内存 */
    int         count;
    int         order[MAX_DEVICES];   /* 显示顺序，存 devices 下标 */
    SystemModel system;
    lv_mutex_t  mutex;
} DeviceStore;

void device_store_init(DeviceStore* store);
void device_store_deinit(DeviceStore* store);

/* 添加设备，返回新设备 id，失败返回 -1 */
int  device_store_add_light(DeviceStore* store, const char* name);
int  device_store_add_cct(DeviceStore* store, const char* name);
int  device_store_add_curtain(DeviceStore* store, const char* name);

/* 删除设备（释放堆内存） */
int  device_store_del(DeviceStore* store, int id);

/* 调整顺序 */
int  device_store_move(DeviceStore* store, int id, int new_pos);

/* 各设备属性设置（业务线程调用） */
int  device_store_set_light_prop(DeviceStore* store, int id, const char* field, int value);
int  device_store_set_cct_prop(DeviceStore* store, int id, const char* field, int value);
int  device_store_set_curtain_prop(DeviceStore* store, int id, const char* field, int value);
int  device_store_set_system(DeviceStore* store, const char* field, int value);

/**
 * 快照接口（LVGL 线程调用）
 *
 * 返回公共基础信息数组，供 UI 遍历设备列表用。
 * 需要具体设备数据时，用 device_store_snapshot_xxx 按 id 获取。
 */
int  device_store_snapshot_base(DeviceStore* store, DeviceBase* out, int* out_order, int* out_count);
int  device_store_snapshot_light(DeviceStore* store, int id, LightModel* out);
int  device_store_snapshot_cct(DeviceStore* store, int id, CctLightModel* out);
int  device_store_snapshot_curtain(DeviceStore* store, int id, CurtainModel* out);
void device_store_snapshot_system(DeviceStore* store, SystemModel* out);

/* 内部辅助（调用方需持锁） */
int  device_store_find_idx(DeviceStore* store, int id);

#endif /* DEVICE_STORE_H */
