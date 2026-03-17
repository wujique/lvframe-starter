/**
 * main.c - music demo SDL 模拟器入口
 *
 * 编译：
 *   cd examples/music
 *   cmake -B build -DPLATFORM=sdl
 *   cmake --build build
 *   ./build/music
 */

#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include "lvgl/lvgl.h"
#include "lvgl/src/drivers/sdl/lv_sdl_window.h"
#include "lvgl/src/drivers/sdl/lv_sdl_mouse.h"
#include "lv_demo_music.h"

/* 屏幕分辨率，与 music demo 默认尺寸匹配 */
#define MUSIC_HOR_RES  480
#define MUSIC_VER_RES  272

int main(void)
{
    /* 1. 初始化 LVGL */
    lv_init();

    /* 2. 创建 SDL 窗口显示 */
    lv_display_t* disp = lv_sdl_window_create(MUSIC_HOR_RES, MUSIC_VER_RES);
    lv_sdl_window_set_title(disp, "LVGL Music Demo");

    /* 3. 创建鼠标输入设备 */
    lv_indev_t* mouse = lv_sdl_mouse_create();
    (void)mouse;

    /* 4. 启动 music demo */
    lv_demo_music();

    /* 5. 主循环 */
    while (1) {
        uint32_t ms = lv_timer_handler();
        if (ms > 0) {
            struct timespec ts = { 0, (long)ms * 1000000L };
            nanosleep(&ts, NULL);
        }
    }

    return 0;
}
