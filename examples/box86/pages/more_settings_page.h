#ifndef MORE_SETTINGS_PAGE_H
#define MORE_SETTINGS_PAGE_H

#include "lvgl/lvgl.h"
#include "../app_bus.h"
#include "../models/device_store.h"

typedef struct {
    AppBus*      bus;
    DeviceStore* store;
} MoreSettingsPageParams;

/* 供 page_manager 注册使用 */
struct Page* more_settings_page_creator(void* params);

#endif /* MORE_SETTINGS_PAGE_H */
