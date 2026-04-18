#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include "lvgl/lvgl.h"
#include "platform/platform.h"
#include "lvframe/page_manager.h"
#include "lvframe/event_bus.h"
#include "app_bus.h"
#include "lvframe/device/lv_device_store.h"
#include "business.h"
#include "pages/home_page.h"
#include "pages/more_settings_page.h"
#include "config.h"

/* 字体引擎头文件 */
#if LV_USE_FREETYPE
#  include "lvgl/lvgl/src/libs/freetype/lv_freetype.h"
#elif LV_USE_TINY_TTF
#  include "lvgl/lvgl/src/libs/tiny_ttf/lv_tiny_ttf.h"
#endif

#define SCREEN_W 480
#define SCREEN_H 480

static AppBus      g_bus;
static lv_device_store_t g_store;
static Business    g_biz;

/* 全局中文字体句柄 */
static lv_font_t  *g_font_cn = NULL;

int main(void)
{
    /* 1. 初始化 LVGL */
    lv_init();

    /* 2. 初始化平台 */
    platform_init(SCREEN_W, SCREEN_H);
    printf("[main] Platform initialized\n");

    /* 3. 加载中文字体，设为 LVGL 默认字体 */
#if LV_USE_FREETYPE
    lv_freetype_init(LV_FREETYPE_CACHE_FT_GLYPH_CNT);
    g_font_cn = lv_freetype_font_create(BOX86_FONT_CN_PATH,
                                        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                        BOX86_FONT_CN_SIZE,
                                        LV_FREETYPE_FONT_STYLE_NORMAL);
#elif LV_USE_TINY_TTF
    g_font_cn = lv_tiny_ttf_create_file(BOX86_FONT_CN_PATH, BOX86_FONT_CN_SIZE);
#endif
    if (g_font_cn) {
        /* 将中文字体应用到活动屏幕的根对象，所有子对象继承 */
        static lv_style_t style_font;
        lv_style_init(&style_font);
        lv_style_set_text_font(&style_font, g_font_cn);
        lv_obj_add_style(lv_screen_active(), &style_font, 0);
        printf("[main] Chinese font loaded: %s size=%d\n",
               BOX86_FONT_CN_PATH, BOX86_FONT_CN_SIZE);
    } else {
        printf("[main] WARNING: Failed to load Chinese font: %s\n", BOX86_FONT_CN_PATH);
    }

    /* 3. 初始化 lvframe */
    page_manager_init();
    page_manager_set_cache_size(3);
    event_bus_init();
    printf("[main] Event bus initialized\n");

    /* 4. 初始化应用层 */
    app_bus_init(&g_bus);
    printf("[main] AppBus initialized\n");
    lv_device_store_init(&g_store);

    /* 5. 创建默认设备：一个普通灯 */
    lv_device_store_add_light(&g_store, "Living Room Light");
    lv_device_store_add_light(&g_store, "bath Room Light");
    /* 6. 启动业务逻辑线程 */
    business_init(&g_biz, &g_bus, &g_store);
    business_start(&g_biz);
    printf("[main] Business thread started\n");

    /* 7. 注册页面 */
    page_manager_register("Home",        home_page_creator);
    page_manager_register("MoreSettings", more_settings_page_creator);

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
