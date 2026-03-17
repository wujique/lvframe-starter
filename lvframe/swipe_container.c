#include "swipe_container.h"
#include <string.h>
#include <stdio.h>

#define MAX_IMPL 5

typedef struct {
    char name[20];
    SwipeContainerOps ops;
} ContainerImpl;

static ContainerImpl g_impls[MAX_IMPL];
static int g_impl_count = 0;

void swipe_container_register(const char* name, SwipeContainerOps* ops) {
    if (g_impl_count >= MAX_IMPL) return;
    
    ContainerImpl* impl = &g_impls[g_impl_count++];
    strncpy(impl->name, name, 19);
    impl->ops = *ops;
}

static SwipeContainerOps* find_ops(const char* name) {
    for (int i = 0; i < g_impl_count; i++) {
        if (strcmp(g_impls[i].name, name) == 0) {
            return &g_impls[i].ops;
        }
    }
    return NULL;
}

lv_obj_t* swipe_container_create(const char* name, lv_obj_t* parent) {
    SwipeContainerOps* ops = find_ops(name);
    if (ops && ops->create) {
        return ops->create(parent);
    }
    return NULL;
}

lv_obj_t* swipe_container_add_page(lv_obj_t* container, int index) {
    // 获取容器对应的ops（需要将ops存储在容器user_data中）
    SwipeContainerOps* ops = lv_obj_get_user_data(container);
    if (ops && ops->add_page) {
        return ops->add_page(container, index);
    }
    return NULL;
}

void swipe_container_remove_page(lv_obj_t* container, int index) {
    SwipeContainerOps* ops = lv_obj_get_user_data(container);
    if (ops && ops->remove_page) {
        ops->remove_page(container, index);
    }
}

void swipe_container_switch_to(lv_obj_t* container, int index) {
    SwipeContainerOps* ops = lv_obj_get_user_data(container);
    if (ops && ops->switch_to) {
        ops->switch_to(container, index);
    }
}

int swipe_container_get_current(lv_obj_t* container) {
    SwipeContainerOps* ops = lv_obj_get_user_data(container);
    if (ops && ops->get_current) {
        return ops->get_current(container);
    }
    return -1;
}

int swipe_container_get_count(lv_obj_t* container) {
    SwipeContainerOps* ops = lv_obj_get_user_data(container);
    if (ops && ops->get_count) {
        return ops->get_count(container);
    }
    return 0;
}