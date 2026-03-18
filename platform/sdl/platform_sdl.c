/**
 * platform_sdl.c - SDL2 平台实现（Ubuntu PC 模拟器）
 *
 * 使用 LVGL 内置 SDL 驱动（LV_USE_SDL=1）
 * platform 库的 include dir 是 ROOT_DIR，所以路径从 ROOT_DIR 开始
 */

#include "../platform.h"
#include "lvgl/lvgl/src/drivers/sdl/lv_sdl_window.h"
#include "lvgl/lvgl/src/drivers/sdl/lv_sdl_mouse.h"
#include <SDL2/SDL.h>

void platform_init(uint32_t hor_res, uint32_t ver_res)
{
    lv_display_t* disp = lv_sdl_window_create((int32_t)hor_res, (int32_t)ver_res);
    lv_sdl_window_set_title(disp, "box86 UI");

    lv_indev_t* mouse = lv_sdl_mouse_create();
    (void)mouse;
}

void platform_deinit(void)
{
    lv_sdl_quit();
}

void platform_delay_ms(uint32_t ms)
{
    SDL_Delay(ms);
}
