#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include "lvgl/lvgl.h"
#include "platform/platform.h"
#include "lvframe/page_manager.h"
#include "lvframe/event_bus.h"
#include "app_bus.h"
#include "lvframe/device/lv_device_store.h"
#include "business.h"
#include "models/device_store.h"
#include "pages/home_page.h"
#include "pages/more_settings_page.h"
#include "pages/screensaver_page.h"
#include "pages/blank_page.h"
#include "screensaver.h"
#include "config.h"
#include "font.h"

#define SCREEN_W 480
#define SCREEN_H 480

static AppBus      g_bus;
static lv_device_store_t g_store;
static Business    g_biz;

int main(void)
{
    /* 1. 初始化 LVGL */
    lv_init();

    /* 2. 初始化平台 */
    platform_init(SCREEN_W, SCREEN_H);
    printf("[main] Platform initialized\n");

    /* 3. 加载中文字体 */
    box86_font_init();

    /* 3a. 注册 page root 创建回调，确保每个 page 都应用中文字体 */
    page_set_root_created_cb(box86_font_apply);

    /* 3. 初始化 lvframe */
    page_manager_init();
    page_manager_set_cache_size(3);
    event_bus_init();
    printf("[main] Event bus initialized\n");

    /* 4. 初始化应用层 */
    app_bus_init(&g_bus);
    printf("[main] AppBus initialized\n");
    lv_device_store_init(&g_store);
    box86_store_init_system(&g_store);

    /* 5. 创建默认设备：一个普通灯 */
    box86_store_add_light(&g_store, "Living Room Light");
    box86_store_add_cct(&g_store, "Bedroom CCT Light");
    box86_store_add_curtain(&g_store, "Living Room Curtain");
    /* 6. 启动业务逻辑线程 */
    business_init(&g_biz, &g_bus, &g_store);
    business_start(&g_biz);
    printf("[main] Business thread started\n");

    /* 7. 注册页面 */
    page_manager_register("Home",        home_page_creator);
    page_manager_register("MoreSettings", more_settings_page_creator);
    page_manager_register("Screensaver", screensaver_page_creator);
    page_manager_register("BlankScreen",  blank_page_creator);

    /* 初始化屏保状态机 */
    screensaver_init(&g_store);

    /* 8. 打开首页 */
    HomePageParams hp = { .bus = &g_bus, .store = &g_store };
    int ret = page_manager_open("Home", &hp);
    if (ret != PAGE_MANAGER_OK) {
        printf("Failed to open Home page: error %d\n", ret);
    } else {
        printf("[main] Home page opened successfully\n");
    }

    printf("[main] Entering main loop\n");
    /* 9. LVGL 主循环 */
    while (1) {
        lv_timer_handler();
        platform_delay_ms(5);
    }

    business_stop(&g_biz);
    lv_device_store_deinit(&g_store);
    app_bus_deinit(&g_bus);
    platform_deinit();
    return 0;
}
