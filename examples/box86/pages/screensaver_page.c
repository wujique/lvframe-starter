/**
 * @file         screensaver_page.c
 * @brief        屏保展示页实现：黑底白字"屏保"，点击任意处唤醒
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */

#include "screensaver_page.h"
#include "../screensaver.h"
#include "lvframe/page_manager.h"
#include <stdlib.h>

/**
 * @brief        屏幕点击回调，调用 screensaver_wake() 退出屏保
 *
 * @param        e                    LVGL 事件
 * @return       void
 */
static void on_screen_clicked(lv_event_t* e)
{
    (void)e;
    lv_event_stop_bubbling(e);
    screensaver_wake();
}

/**
 * @brief        页面创建回调，构建黑色全屏屏保 UI
 *
 * @param        page                 当前页面句柄
 * @param        params               保留参数（未使用）
 * @return       void
 */
static void on_create(Page* page, void* params)
{
    (void)params;
    lv_obj_t* root = page_get_root(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(root, lv_color_white(), LV_PART_MAIN);
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(root, on_screen_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t* lbl = lv_label_create(root);
    lv_label_set_text(lbl, "屏保");
    lv_obj_center(lbl);
}

/**
 * @brief        页面销毁回调（无资源需释放）
 *
 * @param        page                 当前页面句柄
 * @return       void
 */
static void on_destroy(Page* page)
{
    (void)page;
}

/**
 * @brief        供 page_manager 注册使用的屏保页工厂函数
 *
 * @param        params               保留参数（未使用）
 * @return       Page* 新创建的屏保页对象
 */
Page* screensaver_page_creator(void* params)
{
    static PageLifecycle lc = {
        .on_create  = on_create,
        .on_destroy = on_destroy,
    };
    return page_create(&lc, params);
}
