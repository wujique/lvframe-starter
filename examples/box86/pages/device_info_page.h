#ifndef DEVICE_INFO_PAGE_H
#define DEVICE_INFO_PAGE_H

/**
 * device_info_page — 设备信息二级页面
 *
 * 显示设备的基本信息（ID、名称、位置等）。
 * 背景为 40% 透明度（半透明黑色覆盖层），点击任何地方返回上一页。
 *
 * 创建参数：DeviceInfoPageParams*
 */

#include "lvgl/lvgl.h"
#include "../app_bus.h"
#include "lvframe/device/lv_device_store.h"
#include "lvframe/page.h"

typedef struct {
    AppBus*            bus;
    lv_device_store_t* store;
    int                device_id;
} DeviceInfoPageParams;

Page* device_info_page_creator(void* params);

#endif /* DEVICE_INFO_PAGE_H */
