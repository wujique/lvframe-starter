/**
 * @file         swipe_container.c
 * @brief        滑动容器抽象层实现，维护一个具名实现注册表，并将调用转发给对应的 ops
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */
#include "swipe_container.h"
#include <string.h>
#include <stdio.h>

/** 最多支持注册的实现数量 */
#define MAX_IMPL 5

/**
 * @brief        内部注册表条目，保存实现名称及其操作接口
 */
typedef struct {
    char name[20];
    SwipeContainerOps ops;
} ContainerImpl;

static ContainerImpl g_impls[MAX_IMPL];
static int g_impl_count = 0;

/**
 * @brief        注册一个具名滑动容器实现；若同名已存在则更新
 *
 * @param        name                 实现名称（最长 19 字符）
 * @param        ops                  操作接口指针
 * @return       void
 */
void swipe_container_register(const char* name, SwipeContainerOps* ops) {
    if (g_impl_count >= MAX_IMPL) return;

    for (int i = 0; i < g_impl_count; i++) {
        if (strcmp(g_impls[i].name, name) == 0) {
            g_impls[i].ops = *ops;
            printf("[SwipeContainer] updated existing impl '%s'\n", name);
            return;
        }
    }

    ContainerImpl* impl = &g_impls[g_impl_count++];
    strncpy(impl->name, name, 19);
    impl->ops = *ops;
    printf("[SwipeContainer] registered impl '%s'\n", name);
}

/**
 * @brief        在注册表中按名称查找操作接口
 *
 * @param        name                 实现名称
 * @return       SwipeContainerOps* 找到时返回指针，否则返回 NULL
 */
static SwipeContainerOps* find_ops(const char* name) {
    for (int i = 0; i < g_impl_count; i++) {
        if (strcmp(g_impls[i].name, name) == 0) {
            return &g_impls[i].ops;
        }
    }
    return NULL;
}

/**
 * @brief        使用指定实现名称创建滑动容器
 *
 * @param        name                 已注册的实现名称
 * @param        parent               父 LVGL 对象
 * @return       lv_obj_t* 容器根对象，实现未找到时返回 NULL
 */
lv_obj_t* swipe_container_create(const char* name, lv_obj_t* parent) {
    SwipeContainerOps* ops = find_ops(name);
    if (ops && ops->create) {
        return ops->create(parent);
    }
    return NULL;
}

/**
 * @brief        向容器指定索引位置添加一页
 *
 * @param        container            容器对象（user_data 存储 ops 指针）
 * @param        index                页索引
 * @return       lv_obj_t* 新建页对象，失败返回 NULL
 */
lv_obj_t* swipe_container_add_page(lv_obj_t* container, int index) {
    // 获取容器对应的ops（需要将ops存储在容器user_data中）
    SwipeContainerOps* ops = lv_obj_get_user_data(container);
    printf("[SwipeContainer] add_page: container=%p, index=%d, ops=%p\n", container, index, ops);
    if (ops && ops->add_page) {
        printf("[SwipeContainer] Calling ops->add_page\n");
        lv_obj_t* result = ops->add_page(container, index);
        printf("[SwipeContainer] add_page returned %p\n", result);
        return result;
    }
    printf("[SwipeContainer] ERROR: No ops or add_page function\n");
    return NULL;
}

/**
 * @brief        移除容器中指定索引处的页
 *
 * @param        container            容器对象
 * @param        index                要移除的页索引
 * @return       void
 */
void swipe_container_remove_page(lv_obj_t* container, int index) {
    SwipeContainerOps* ops = lv_obj_get_user_data(container);
    if (ops && ops->remove_page) {
        ops->remove_page(container, index);
    }
}

/**
 * @brief        切换容器到指定索引的页
 *
 * @param        container            容器对象
 * @param        index                目标页索引
 * @return       void
 */
void swipe_container_switch_to(lv_obj_t* container, int index) {
    SwipeContainerOps* ops = lv_obj_get_user_data(container);
    if (ops && ops->switch_to) {
        ops->switch_to(container, index);
    }
}

/**
 * @brief        获取容器当前显示页的索引
 *
 * @param        container            容器对象
 * @return       int 当前页索引，无效时返回 -1
 */
int swipe_container_get_current(lv_obj_t* container) {
    SwipeContainerOps* ops = lv_obj_get_user_data(container);
    if (ops && ops->get_current) {
        return ops->get_current(container);
    }
    return -1;
}

/**
 * @brief        获取容器中的总页数
 *
 * @param        container            容器对象
 * @return       int 总页数，无效时返回 0
 */
int swipe_container_get_count(lv_obj_t* container) {
    SwipeContainerOps* ops = lv_obj_get_user_data(container);
    if (ops && ops->get_count) {
        return ops->get_count(container);
    }
    return 0;
}
