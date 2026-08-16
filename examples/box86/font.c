/**
 * @file         font.c
 * @brief        box86 全局中文字体管理实现，根据编译配置选用 FreeType 或 TinyTTF
 *
 * @author       pochard(email@xxx.com)
 * @version      0.1
 * @date         2026-05-16
 * @copyright    Copyright (c) 2026..
 */
#include "font.h"
#include "config.h"
#include <stdio.h>

#if LV_USE_FREETYPE
#  include "lvgl/lvgl/src/libs/freetype/lv_freetype.h"
#elif LV_USE_TINY_TTF
#  include "lvgl/lvgl/src/libs/tiny_ttf/lv_tiny_ttf.h"
#endif

static lv_font_t  *s_font_cn   = NULL; /**< 已加载的中文字体指针 */
static lv_style_t  s_style_cn;         /**< 应用中文字体的样式对象 */
static int         s_style_init = 0;   /**< 样式是否已初始化标志 */

/**
 * @brief        初始化中文字体，根据编译宏选择 FreeType 或 TinyTTF 加载字体文件
 *
 * @return       int 成功返回 0，加载失败返回 -1
 */
int box86_font_init(void)
{
#if LV_USE_FREETYPE
    lv_freetype_init(LV_FREETYPE_CACHE_FT_GLYPH_CNT);
    s_font_cn = lv_freetype_font_create(BOX86_FONT_CN_PATH,
                                        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                        BOX86_FONT_CN_SIZE,
                                        LV_FREETYPE_FONT_STYLE_NORMAL);
#elif LV_USE_TINY_TTF
    s_font_cn = lv_tiny_ttf_create_file(BOX86_FONT_CN_PATH, BOX86_FONT_CN_SIZE);
#endif

    if (!s_font_cn) {
        printf("[font] WARNING: Failed to load Chinese font: %s\n", BOX86_FONT_CN_PATH);
        return -1;
    }

    lv_style_init(&s_style_cn);
    lv_style_set_text_font(&s_style_cn, s_font_cn);
    s_style_init = 1;
    printf("[font] Chinese font loaded: %s size=%d\n", BOX86_FONT_CN_PATH, BOX86_FONT_CN_SIZE);
    return 0;
}

/**
 * @brief        获取已加载的中文字体指针
 *
 * @return       const lv_font_t* 字体指针，未加载时返回 NULL
 */
const lv_font_t *box86_font_get_cn(void)
{
    return s_font_cn;
}

/**
 * @brief        将中文字体样式追加到指定对象，子对象通过样式继承自动生效
 *
 * @param        obj                  目标 LVGL 对象
 * @return       void
 */
void box86_font_apply(lv_obj_t *obj)
{
    if (!obj || !s_style_init) return;
    lv_obj_add_style(obj, &s_style_cn, LV_PART_MAIN | LV_STATE_DEFAULT);
}
