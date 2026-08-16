/**
 * @file         swipe_container.h
 * @brief        滑动容器抽象层，通过注册不同实现（ops）支持多种滑动控件后端
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */
#ifndef SWIPE_CONTAINER_H
#define SWIPE_CONTAINER_H

#include "../lvgl/lvgl/lvgl.h"

/**
 * @brief        滑动容器操作接口，由具体实现注册
 */
typedef struct {
    lv_obj_t* (*create)(lv_obj_t* parent);                    /**< 创建容器并返回根对象 */
    lv_obj_t* (*add_page)(lv_obj_t* container, int index);    /**< 在指定索引处添加一页并返回页对象 */
    void      (*remove_page)(lv_obj_t* container, int index); /**< 移除指定索引处的页 */
    void      (*switch_to)(lv_obj_t* container, int index);   /**< 切换到指定索引的页 */
    int       (*get_current)(lv_obj_t* container);            /**< 获取当前页索引 */
    int       (*get_count)(lv_obj_t* container);              /**< 获取总页数 */
} SwipeContainerOps;

/**
 * @brief        注册一个具名滑动容器实现
 *
 * @param        name                 实现名称（唯一标识符）
 * @param        ops                  指向操作接口结构体的指针
 * @return       void
 */
void swipe_container_register(const char* name, SwipeContainerOps* ops);

/**
 * @brief        使用指定实现名称创建滑动容器
 *
 * @param        name                 已注册的实现名称
 * @param        parent               父 LVGL 对象
 * @return       lv_obj_t* 容器根对象，若实现未找到则返回 NULL
 */
lv_obj_t* swipe_container_create(const char* name, lv_obj_t* parent);

/**
 * @brief        向容器指定索引位置添加一页
 *
 * @param        container            容器对象（user_data 中存储了 ops 指针）
 * @param        index                要添加的页索引
 * @return       lv_obj_t* 新建页对象，失败返回 NULL
 */
lv_obj_t* swipe_container_add_page(lv_obj_t* container, int index);

/**
 * @brief        移除容器中指定索引处的页
 *
 * @param        container            容器对象
 * @param        index                要移除的页索引
 * @return       void
 */
void swipe_container_remove_page(lv_obj_t* container, int index);

/**
 * @brief        切换容器到指定索引的页
 *
 * @param        container            容器对象
 * @param        index                目标页索引
 * @return       void
 */
void swipe_container_switch_to(lv_obj_t* container, int index);

/**
 * @brief        获取容器当前显示页的索引
 *
 * @param        container            容器对象
 * @return       int 当前页索引，若无效则返回 -1
 */
int swipe_container_get_current(lv_obj_t* container);

/**
 * @brief        获取容器中的总页数
 *
 * @param        container            容器对象
 * @return       int 总页数，若无效则返回 0
 */
int swipe_container_get_count(lv_obj_t* container);

#endif
