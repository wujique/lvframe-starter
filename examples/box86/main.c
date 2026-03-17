/**
 * box86/main.c - box86 工程入口
 *
 * 支持平台：
 *   -DPLATFORM=sdl     Ubuntu SDL 模拟器
 *   -DPLATFORM=rk3506  RK3506 Linux
 *   -DPLATFORM=rtos    RTOS（预留）
 */

#include "lvgl/lvgl.h"
#include "platform/platform.h"
#include "lvframe/page_manager.h"
#include "lvframe/event_bus.h"

/* TODO: 注册工程页面 */
/* #include "pages/home_page.h" */

int main(void)
{
    /* 1. 初始化 LVGL */
    lv_init();

    /* 2. 初始化平台（显示驱动、输入驱动、tick） */
    platform_init();

    /* 3. 初始化 lvframe */
    page_manager_init();
    page_manager_set_cache_size(3);
    event_bus_init();

    /* 4. 注册页面 */
    /* page_manager_register("HomePage", home_page_create); */

    /* 5. 打开首页 */
    /* page_manager_open("HomePage", NULL); */

    /* 6. LVGL 主循环 */
    while (1) {
        lv_timer_handler();
        platform_delay_ms(5);
    }

    platform_deinit();
    return 0;
}
