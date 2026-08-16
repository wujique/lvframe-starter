/**
 * @file         device_page.h
 * @brief        设备页统一入口，按设备类型（普通灯/色温灯/电动窗帘）分发构建
 *
 * @author       pochard(email@xxx.com)
 * @version      0.3
 * @date         2026-08-16
 * @copyright    Copyright (c) 2026..
 */
#ifndef DEVICE_PAGE_H
#define DEVICE_PAGE_H

#include "lvgl/lvgl.h"
#include "models/model_store.h"
#include "lvframe/page.h"

/**
 * @brief        设备页创建参数
 */
typedef struct {
    model_store_t* store;  /**< 模型仓库 */
    void*          model;  /**< 目标模型指针（object）*/
} DevicePageParams;

/**
 * @brief        创建设备页 Page 对象，根据设备类型自动分发构建逻辑
 */
Page* device_page_create(DevicePageParams* params);

#endif /* DEVICE_PAGE_H */
