#ifndef DEVICE_INFO_PAGE_H
#define DEVICE_INFO_PAGE_H

/**
 * @file         device_info_page.h
 * @brief        设备信息覆盖层接口：半透明全屏遮罩 + 居中信息卡片
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */

#include "lvgl/lvgl.h"
#include "models/model_store.h"

/**
 * @brief        在 parent（设备页 root）上创建设备信息覆盖层
 *
 * @param        parent               设备页根容器
 * @param        store                模型仓库
 * @param        model                目标模型指针
 */
void device_info_overlay_open(lv_obj_t* parent, model_store_t* store, void* model);

#endif /* DEVICE_INFO_PAGE_H */
