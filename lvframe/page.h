/**
 * @file         page.h
 * @brief        页面基础结构和生命周期接口定义
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */

#ifndef PAGE_H
#define PAGE_H

#include "../lvgl/lvgl/lvgl.h"
#include "slot.h"

/**
 * @brief        页面生命周期状态枚举
 */
typedef enum {
    PAGE_STATE_NONE = 0,     /**< 初始无效状态 */
    PAGE_STATE_CREATED,      /**< 页面已创建，on_create 已调用 */
    PAGE_STATE_STARTED,      /**< 页面已启动，on_start 已调用 */
    PAGE_STATE_RESUMED,      /**< 页面处于前台，on_resume 已调用 */
    PAGE_STATE_PAUSED,       /**< 页面被覆盖，on_pause 已调用 */
    PAGE_STATE_STOPPED,      /**< 页面已停止，on_stop 已调用 */
    PAGE_STATE_DESTROYED     /**< 页面已销毁，on_destroy 已调用 */
} PageState;

typedef struct Page Page;

/**
 * @brief        页面生命周期回调函数集合
 */
typedef struct {
    void (*on_create)(Page* page, void* params);        /**< 页面首次创建时调用 */
    void (*on_start)(Page* page);                       /**< 页面从停止恢复时调用 */
    void (*on_resume)(Page* page);                      /**< 页面进入前台时调用 */
    void (*on_pause)(Page* page);                       /**< 页面被覆盖进入后台时调用 */
    void (*on_stop)(Page* page);                        /**< 页面停止时调用 */
    void (*on_destroy)(Page* page);                     /**< 页面销毁时调用 */
    void (*on_msg)(Page* page, const lv_slot_msg_t* msg); /**< 收到订阅信号（object 已由槽过滤）*/
} PageLifecycle;

/**
 * @brief        页面对象结构体
 */
struct Page {
    lv_obj_t*     root;        /**< LVGL 根对象，作为页面内容的父容器 */
    PageState     state;       /**< 当前生命周期状态 */
    void*         user_data;   /**< 应用层页面私有数据（含其绑定的对象等，由应用定义）*/
    lv_slot_t*    slot;        /**< 绑定槽（page_destroy 自动退订全部）*/
    PageLifecycle lifecycle;   /**< 生命周期回调函数集 */
};

/**
 * @brief        创建页面，分配内存、创建根对象并调用 on_create
 */
Page* page_create(PageLifecycle* lifecycle, void* params);

/**
 * @brief        销毁页面：先自动退订绑定槽，再调 on_destroy、删根对象、释放内存
 */
void  page_destroy(Page* page);

/**
 * @brief        获取页面的 LVGL 根对象
 */
lv_obj_t* page_get_root(Page* page);

/**
 * @brief        设置页面的用户自定义数据指针
 */
void  page_set_user_data(Page* page, void* data);

/**
 * @brief        获取页面的用户自定义数据指针
 */
void* page_get_user_data(Page* page);

/**
 * @brief        按信号 + 对象订阅槽，可多次调用；内部 handler 转发到 page 的 on_msg
 *
 * 记录 slot 供 page_destroy 自动退订全部。
 */
int   page_bind_slot(Page* page, lv_slot_t* slot, int signal, void* object);

/**
 * @brief        按 (signal, object) 单条退订（内部 handler 固定为 page_slot_dispatch）
 */
int   page_unbind_slot(Page* page, lv_slot_t* slot, int signal, void* object);

/**
 * @brief        注册 root 对象创建后的回调，可用于应用层设置全局字体等
 */
void page_set_root_created_cb(void (*cb)(lv_obj_t *root));

#endif /* PAGE_H */
