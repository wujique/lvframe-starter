#ifndef BOX86_SCREENSAVER_H
#define BOX86_SCREENSAVER_H

#include "lvframe/device/lv_device_store.h"

void screensaver_init(lv_device_store_t* store);
void screensaver_wake(void);

#endif
