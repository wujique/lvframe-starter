#ifndef DEVICE_PAGE_H
#define DEVICE_PAGE_H

/**
 * device_page — 设备页（普通灯 / 色温灯 / 电动窗帘）作为 Page 对象
 *
 * 创建参数：DevicePageParams*
 * 页面持有设备 id，通过快照读取数据，不持有模型指针。
 */

#include "lvgl/lvgl.h"
#include "../app_bus.h"
#include "lvframe/device/lv_device_store.h"
#include "lvframe/page.h"

typedef struct {
    AppBus*      bus;
    lv_device_store_t* store;
    int          device_id;
} DevicePageParams;

/* 创建设备页 Page 对象 */
Page* device_page_create(DevicePageParams* params);

#endif /* DEVICE_PAGE_H */