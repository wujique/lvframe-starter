/**
 * font.h - box86 全局字体访问接口
 */
#ifndef BOX86_FONT_H
#define BOX86_FONT_H

#include "lvgl/lvgl.h"

/**
 * 初始化中文字体（在 platform_init 后调用）
 * @return 0 成功，-1 失败
 */
int box86_font_init(void);

/**
 * 获取中文字体指针（可能为 NULL）
 */
const lv_font_t *box86_font_get_cn(void);

/**
 * 将中文字体应用到指定对象（对象自身 + 所有子对象通过继承获得）
 * 应在每个 page root 创建后调用。
 */
void box86_font_apply(lv_obj_t *obj);

#endif /* BOX86_FONT_H */
