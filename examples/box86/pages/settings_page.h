#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

/**
 * @file         settings_page.h
 * @brief        主页快捷设置面板接口：网络开关 + 进入详细设置
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
} SettingsPageParams;

lv_obj_t* settings_page_create(lv_obj_t* parent, SettingsPageParams* params);
void      settings_page_refresh(lv_obj_t* page);
void      settings_page_destroy(lv_obj_t* page);

#endif /* SETTINGS_PAGE_H */
