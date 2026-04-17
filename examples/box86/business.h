#ifndef BUSINESS_H
#define BUSINESS_H

#include "app_bus.h"
#include "lvframe/device/lv_device_store.h"

typedef struct {
    AppBus*      bus;
    lv_device_store_t* store;
    int          running;
    lv_thread_t  thread;
} Business;

void business_init(Business* biz, AppBus* bus, lv_device_store_t* store);
void business_start(Business* biz);
void business_stop(Business* biz);

#endif /* BUSINESS_H */
