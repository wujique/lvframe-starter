#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

#include "lvgl/lvgl.h"
#include "../app_bus.h"
#include "lvframe/device/lv_device_store.h"

typedef struct {
    AppBus*      bus;
    lv_device_store_t* store;
} SettingsPageParams;

lv_obj_t* settings_page_create(lv_obj_t* parent, SettingsPageParams* params);
void      settings_page_refresh(lv_obj_t* page);
void      settings_page_destroy(lv_obj_t* page);

#endif /* SETTINGS_PAGE_H */
