#ifndef DEVICE_INFO_PAGE_H
#define DEVICE_INFO_PAGE_H

/**
 * device_info_page — 设备信息覆盖层
 *
 * 不作为独立 Page，而是直接在设备页 root 上创建覆盖层：
 *   - 全屏半透明黑色背景（40% 不透明度），底层设备页内容可透过
 *   - 居中白色圆角卡片，展示设备 ID、名称、类型、安装位置
 *   - 点击任意位置删除覆盖层，回到设备页
 */

#include "lvgl/lvgl.h"
#include "../app_bus.h"
#include "lvframe/device/lv_device_store.h"

/**
 * 在 parent（设备页 root）上创建设备信息覆盖层。
 * 覆盖层是 parent 的子对象，关闭时自行 lv_obj_delete。
 */
void device_info_overlay_open(lv_obj_t* parent, lv_device_store_t* store, int device_id);

#endif /* DEVICE_INFO_PAGE_H */
