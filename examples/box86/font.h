/**
 * @file         font.h
 * @brief        box86 全局中文字体加载与应用接口，支持 FreeType 和 TinyTTF 两种后端
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */
#ifndef BOX86_FONT_H
#define BOX86_FONT_H

#include "lvgl/lvgl.h"

/**
 * @brief        初始化中文字体，须在 platform_init 之后调用
 *
 * @return       int 成功返回 0，加载失败返回 -1
 */
int box86_font_init(void);

/**
 * @brief        获取已加载的中文字体指针
 *
 * @return       const lv_font_t* 字体指针，未加载时返回 NULL
 */
const lv_font_t *box86_font_get_cn(void);

/**
 * @brief        将中文字体样式应用到指定对象，子对象通过继承自动获得该字体
 *
 * @param        obj                  目标 LVGL 对象（通常为页面根对象）
 * @return       void
 */
void box86_font_apply(lv_obj_t *obj);

#endif /* BOX86_FONT_H */
