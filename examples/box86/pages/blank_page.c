/**
 * @file         blank_page.c
 * @brief        息屏页实现：纯黑全屏"息屏"，点击任意处唤醒
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */

#include "blank_page.h"
#include "../screensaver.h"
#include "models/model_store.h"
#include <stdlib.h>

/**
 * @brief        屏幕点击回调，调用 screensaver_wake() 退出息屏
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
 * @brief        页面创建回调，构建黑色全屏息屏 UI
 *
 * @param        page                 当前页面句柄
 * @param        params               model_store_t* 模型仓库
 * @return       void
 */
static void on_create(Page* page, void* params)
{
    model_store_t* store = (model_store_t*)params;

    lv_obj_t* root = page_get_root(page);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(root, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(root, lv_color_white(), LV_PART_MAIN);
    lv_obj_add_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(root, on_screen_clicked, LV_EVENT_CLICKED, page);

    lv_obj_t* lbl = lv_label_create(root);
    lv_label_set_text(lbl, "息屏");
    lv_obj_center(lbl);

    page_set_user_data(page, store);
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
 * @brief        供 page_manager 注册使用的息屏页工厂函数
 *
 * @param        params               model_store_t* 模型仓库
 * @return       Page* 新创建的息屏页对象
 */
Page* blank_page_creator(void* params)
{
    static PageLifecycle lc = {
        .on_create  = on_create,
        .on_destroy = on_destroy,
    };
    return page_create(&lc, params);
}
