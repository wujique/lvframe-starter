/**
 * font.c - box86 全局字体管理
 */
#include "font.h"
#include "config.h"
#include <stdio.h>

#if LV_USE_FREETYPE
#  include "lvgl/lvgl/src/libs/freetype/lv_freetype.h"
#elif LV_USE_TINY_TTF
#  include "lvgl/lvgl/src/libs/tiny_ttf/lv_tiny_ttf.h"
#endif

static lv_font_t       *s_font_cn  = NULL;
static lv_style_t       s_style_cn;
static int              s_style_init = 0;

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

const lv_font_t *box86_font_get_cn(void)
{
    return s_font_cn;
}

void box86_font_apply(lv_obj_t *obj)
{
    if (!obj || !s_style_init) return;
    lv_obj_add_style(obj, &s_style_cn, LV_PART_MAIN | LV_STATE_DEFAULT);
}
