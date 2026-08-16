/**
 * @file         main.c
 * @brief        应用程序入口：完成 LVGL、平台、字体、页面栈、双槽、模型仓库、
 *               业务线程、页面及屏保的初始化，并进入主循环
 *
 * @author       pochard(email@xxx.com)
 * @version      0.2
 * @date         2026-08-15
 * @copyright    Copyright (c) 2026..
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include "lvgl/lvgl.h"
#include "platform/platform.h"
#include "lvframe/page_manager.h"
#include "lvframe/slot.h"
#include "slots.h"
#include "msg.h"
#include "models/model_store.h"
#include "business.h"
#include "pages/home_page.h"
#include "pages/more_settings_page.h"
#include "pages/screensaver_page.h"
#include "pages/blank_page.h"
#include "screensaver.h"
#include "config.h"
#include "font.h"

#define SCREEN_W 480
#define SCREEN_H 480

static model_store_t g_store; /**< 模型仓库 */
static Business      g_biz;   /**< 业务逻辑上下文 */

static lv_timer_t*   g_ui_slot_timer = NULL; /**< g_ui_slot 消费定时器 */

/**
 * @brief        g_ui_slot 消费定时器回调（UI 线程，50ms）
 */
static void ui_slot_timer_cb(lv_timer_t* timer)
{
    (void)timer;
    lv_slot_process(&g_ui_slot);
}

/**
 * @brief        程序入口
 */
int main(void)
{
    /* 1. LVGL 与平台 */
    lv_init();
    platform_init(SCREEN_W, SCREEN_H);

    /* 2. 中文字体 + 页面 root 字体注入 */
    box86_font_init();
    page_set_root_created_cb(box86_font_apply);

    /* 3. lvframe：页面管理器 + 双槽 */
    page_manager_init();
    page_manager_set_cache_size(3);
    box86_slots_init();

    /* 4. 模型仓库 + 默认设备 */
    model_store_init(&g_store);
    box86_store_add_light(&g_store, "Living Room Light");
    box86_store_add_cct(&g_store, "Bedroom CCT Light");
    box86_store_add_curtain(&g_store, "Living Room Curtain");

    /* 5. 业务线程 */
    business_init(&g_biz, &g_dev_slot, &g_ui_slot, &g_store);
    business_start(&g_biz);

    /* 6. 注册页面 */
    page_manager_register("Home",        home_page_creator);
    page_manager_register("MoreSettings", more_settings_page_creator);
    page_manager_register("Screensaver", screensaver_page_creator);
    page_manager_register("BlankScreen",  blank_page_creator);

    /* 7. 屏保状态机 */
    screensaver_init(&g_store);

    /* 8. g_ui_slot 消费定时器（UI 线程 50ms） */
    g_ui_slot_timer = lv_timer_create(ui_slot_timer_cb, 50, NULL);

    /* 9. 打开首页 */
    HomePageParams hp = { .store = &g_store };
    int ret = page_manager_open("Home", &hp);
    if (ret != PAGE_MANAGER_OK) {
        printf("Failed to open Home page: error %d\n", ret);
    }

    printf("[main] Entering main loop\n");
    while (1) {
        lv_timer_handler();
        platform_delay_ms(5);
    }

    return 0;
}
