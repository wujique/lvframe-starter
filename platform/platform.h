#ifndef PLATFORM_H
#define PLATFORM_H

/**
 * platform.h - 统一平台接口定义
 *
 * 每个平台实现（sdl / rk3506 / rtos）都必须实现以下接口。
 * 应用层和 lvframe 只依赖此头文件，不直接引用任何平台细节。
 */

#include "lvgl/lvgl/lvgl.h"

/**
 * 平台初始化
 * 负责：创建 lv_display_t、lv_indev_t，启动 lv_tick_inc 定时器
 *
 * @param hor_res  屏幕水平分辨率（像素），由各工程 main.c 传入
 * @param ver_res  屏幕垂直分辨率（像素），由各工程 main.c 传入
 */
void platform_init(uint32_t hor_res, uint32_t ver_res);

/**
 * 平台反初始化（释放资源）
 */
void platform_deinit(void);

/**
 * 平台主循环延时（毫秒）
 * 在 LVGL 主循环中调用，避免 CPU 空转
 */
void platform_delay_ms(uint32_t ms);

#endif /* PLATFORM_H */
