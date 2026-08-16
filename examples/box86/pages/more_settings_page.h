#ifndef MORE_SETTINGS_PAGE_H
#define MORE_SETTINGS_PAGE_H

/**
 * @file         more_settings_page.h
 * @brief        屏保详细设置页接口：屏保开关/待机时间/维持时间/唤醒行为
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */

#include "lvgl/lvgl.h"
#include "models/model_store.h"

typedef struct {
    model_store_t* store;
} MoreSettingsPageParams;

/**
 * @brief        供 page_manager 注册使用的页面工厂函数
 */
struct Page* more_settings_page_creator(void* params);

#endif /* MORE_SETTINGS_PAGE_H */
