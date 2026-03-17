/**
 * platform_sdl.c - SDL2 平台实现（Ubuntu PC 模拟器）
 *
 * 依赖：SDL2
 * 用途：PC 端开发调试，模拟目标设备 UI
 */

#include "../platform.h"
#include <SDL2/SDL.h>

/* 模拟屏幕分辨率，根据目标设备调整 */
#define SDL_HOR_RES  480
#define SDL_VER_RES  320

/* TODO: 实现 SDL 显示 flush 回调 */
static void sdl_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    /* SDL 渲染实现 */
    lv_display_flush_ready(disp);
}

/* TODO: 实现 SDL 触摸/鼠标输入读取回调 */
static void sdl_indev_read_cb(lv_indev_t* indev, lv_indev_data_t* data)
{
    /* SDL 鼠标事件读取实现 */
}

void platform_init(void)
{
    /* 初始化 SDL */
    SDL_Init(SDL_INIT_VIDEO);

    /* 创建 LVGL 显示 */
    lv_display_t* disp = lv_display_create(SDL_HOR_RES, SDL_VER_RES);
    lv_display_set_flush_cb(disp, sdl_flush_cb);

    /* 创建 LVGL 输入设备 */
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, sdl_indev_read_cb);

    /* tick 由 SDL 定时器或主循环提供，见各工程 main.c */
}

void platform_deinit(void)
{
    SDL_Quit();
}

void platform_delay_ms(uint32_t ms)
{
    SDL_Delay(ms);
}
