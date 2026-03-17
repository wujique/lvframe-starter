#ifndef SWIPE_CONTAINER_H
#define SWIPE_CONTAINER_H

#include "lvgl.h"

typedef struct {
    lv_obj_t* (*create)(lv_obj_t* parent);
    lv_obj_t* (*add_page)(lv_obj_t* container, int index);
    void (*remove_page)(lv_obj_t* container, int index);
    void (*switch_to)(lv_obj_t* container, int index);
    int (*get_current)(lv_obj_t* container);
    int (*get_count)(lv_obj_t* container);
} SwipeContainerOps;

void swipe_container_register(const char* name, SwipeContainerOps* ops);
lv_obj_t* swipe_container_create(const char* name, lv_obj_t* parent);
lv_obj_t* swipe_container_add_page(lv_obj_t* container, int index);
void swipe_container_remove_page(lv_obj_t* container, int index);
void swipe_container_switch_to(lv_obj_t* container, int index);
int swipe_container_get_current(lv_obj_t* container);
int swipe_container_get_count(lv_obj_t* container);

#endif