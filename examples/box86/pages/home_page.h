#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include "lvframe/page.h"
#include "../app_bus.h"
#include "lvframe/device/lv_device_store.h"

typedef struct {
    AppBus*      bus;
    lv_device_store_t* store;
} HomePageParams;

Page* home_page_creator(void* params);

void home_page_reset_to_first(Page* home_page);

#endif /* HOME_PAGE_H */
