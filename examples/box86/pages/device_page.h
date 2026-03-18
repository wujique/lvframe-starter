#ifndef DEVICE_PAGE_H
#define DEVICE_PAGE_H

/**
 * device_page — 设备页（普通灯 / 色温灯 / 电动窗帘）
 *
 * 创建参数：DevicePageParams*
 * 页面持有设备 id，通过快照读取数据，不持有模型指针。
 */

#include "lvgl/lvgl.h"
#include "../app_bus.h"
#include "../models/device_store.h"

typedef struct {
    AppBus*      bus;
    DeviceStore* store;
    int          device_id;
} DevicePageParams;

/* 在 tileview 的一个 tile 上创建设备页内容，返回根容器 */
lv_obj_t* device_page_create(lv_obj_t* tile, DevicePageParams* params);

/* 刷新设备页显示（从快照读取） */
void device_page_refresh(lv_obj_t* tile);

/* 销毁设备页（清理 user_data） */
void device_page_destroy(lv_obj_t* tile);

#endif /* DEVICE_PAGE_H */
