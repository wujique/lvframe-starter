#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include "lvframe/page.h"
#include "../app_bus.h"
#include "../models/device_store.h"

typedef struct {
    AppBus*      bus;
    DeviceStore* store;
} HomePageParams;

Page* home_page_creator(void* params);

#endif /* HOME_PAGE_H */
