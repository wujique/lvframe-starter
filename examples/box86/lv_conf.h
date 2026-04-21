/**
 * lv_conf.h for box86 (SDL simulator)
 */

#if 1

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16

#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_CLIB

#define LV_STDINT_INCLUDE    <stdint.h>
#define LV_STDDEF_INCLUDE    <stddef.h>
#define LV_STDBOOL_INCLUDE   <stdbool.h>
#define LV_INTTYPES_INCLUDE  <inttypes.h>
#define LV_LIMITS_INCLUDE    <limits.h>
#define LV_STDARG_INCLUDE    <stdarg.h>

#define LV_DEF_REFR_PERIOD  16
#define LV_DPI_DEF 130

#define LV_USE_OS   LV_OS_PTHREAD

#define LV_DRAW_BUF_STRIDE_ALIGN  1
#define LV_DRAW_BUF_ALIGN         4
#define LV_DRAW_TRANSFORM_USE_MATRIX 0
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE  (24 * 1024)
#define LV_DRAW_LAYER_MAX_MEMORY 0
#define LV_DRAW_THREAD_STACK_SIZE (64 * 1024)
#define LV_DRAW_THREAD_PRIO LV_THREAD_PRIO_HIGH
#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW
    #define LV_DRAW_SW_SUPPORT_RGB565       1
    #define LV_DRAW_SW_SUPPORT_RGB565_SWAPPED 1
    #define LV_DRAW_SW_SUPPORT_RGB565A8     1
    #define LV_DRAW_SW_SUPPORT_RGB888       1
    #define LV_DRAW_SW_SUPPORT_XRGB8888     1
    #define LV_DRAW_SW_SUPPORT_ARGB8888     1
    #define LV_DRAW_SW_SUPPORT_ARGB8888_PREMULTIPLIED 1
    #define LV_DRAW_SW_SUPPORT_L8           1
    #define LV_DRAW_SW_SUPPORT_AL88         1
    #define LV_DRAW_SW_SUPPORT_A8           1
    #define LV_DRAW_SW_SUPPORT_I1           1
    #define LV_DRAW_SW_I1_LUM_THRESHOLD     127
    #define LV_DRAW_SW_DRAW_UNIT_CNT        1
    #define LV_USE_DRAW_ARM2D_SYNC          0
    #define LV_USE_NATIVE_HELIUM_ASM        0
    #define LV_DRAW_SW_COMPLEX              1
    #if LV_DRAW_SW_COMPLEX
        #define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
        #define LV_DRAW_SW_CIRCLE_CACHE_SIZE 4
    #endif
    #define LV_USE_DRAW_SW_ASM  LV_DRAW_SW_ASM_NONE
    #define LV_USE_DRAW_SW_COMPLEX_GRADIENTS 0
#endif

#define LV_USE_NEMA_GFX     0
#define LV_USE_PXP          0
#define LV_USE_G2D          0
#define LV_USE_DRAW_DAVE2D  0
#define LV_USE_DRAW_SDL     0
#define LV_USE_DRAW_VG_LITE 0
#define LV_USE_DRAW_DMA2D   0
#define LV_USE_DRAW_OPENGLES 0
#define LV_USE_PPA          0
#define LV_USE_DRAW_EVE     0

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1
#define LV_LOG_USE_TIMESTAMP 1
#define LV_LOG_USE_FILE_LINE 1
#define LV_LOG_TRACE_MEM        0
#define LV_LOG_TRACE_TIMER      0
#define LV_LOG_TRACE_INDEV      0
#define LV_LOG_TRACE_DISP_REFR  0
#define LV_LOG_TRACE_EVENT      0
#define LV_LOG_TRACE_OBJ_CREATE 0
#define LV_LOG_TRACE_LAYOUT     0
#define LV_LOG_TRACE_ANIM       0
#define LV_LOG_TRACE_CACHE      0

#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);

#define LV_USE_REFR_DEBUG           0
#define LV_USE_LAYER_DEBUG          0
#define LV_USE_PARALLEL_DRAW_DEBUG  0

#define LV_ENABLE_GLOBAL_CUSTOM     0
#define LV_CACHE_DEF_SIZE           0
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 0
#define LV_GRADIENT_MAX_STOPS       2
#define LV_COLOR_MIX_ROUND_OFS      0
#define LV_OBJ_STYLE_CACHE          0
#define LV_USE_OBJ_ID               0
#define LV_USE_OBJ_NAME             0
#define LV_OBJ_ID_AUTO_ASSIGN       LV_USE_OBJ_ID
#define LV_USE_OBJ_ID_BUILTIN       1
#define LV_USE_OBJ_PROPERTY         0
#define LV_USE_OBJ_PROPERTY_NAME    1
#define LV_USE_GESTURE_RECOGNITION  0

#define LV_BIG_ENDIAN_SYSTEM        0
#define LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TIMER_HANDLER
#define LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_FAST_MEM
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning
#define LV_ATTRIBUTE_EXTERN_DATA
#define LV_USE_FLOAT                0
#define LV_USE_MATRIX               0
#define LV_USE_PRIVATE_API          0

#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_CUSTOM_DECLARE
#define LV_FONT_DEFAULT &lv_font_montserrat_14
#define LV_FONT_FMT_TXT_LARGE       0
#define LV_USE_FONT_COMPRESSED      0
#define LV_USE_FONT_PLACEHOLDER     1

#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS          " ,.;:-_)]}"
#define LV_TXT_LINE_BREAK_LONG_LEN  0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_USE_BIDI                 0
#define LV_USE_ARABIC_PERSIAN_CHARS 0
#define LV_TXT_COLOR_CMD            "#"

#define LV_WIDGETS_HAS_DEFAULT_VALUE 1
#define LV_USE_ANIMIMG    0
#define LV_USE_ARC        1
#define LV_USE_ARCLABEL   0
#define LV_USE_BAR        1
#define LV_USE_BUTTON     1
#define LV_USE_BUTTONMATRIX 0
#define LV_USE_CALENDAR   0
#define LV_USE_CANVAS     1
#define LV_USE_CHART      0
#define LV_USE_CHECKBOX   0
#define LV_USE_DROPDOWN   0
#define LV_USE_IMAGE      1
#define LV_USE_IMAGEBUTTON 0
#define LV_USE_KEYBOARD   0
#define LV_USE_LABEL      1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION  1
    #define LV_LABEL_LONG_TXT_HINT   1
    #define LV_LABEL_WAIT_CHAR_COUNT 3
#endif
#define LV_USE_LED        0
#define LV_USE_LINE       0
#define LV_USE_LIST       1
#define LV_USE_LOTTIE     0
#define LV_USE_MENU       0
#define LV_USE_MSGBOX     0
#define LV_USE_ROLLER     0
#define LV_USE_SCALE      0
#define LV_USE_SLIDER     1
#define LV_USE_SPAN       0
#define LV_USE_SPINBOX    0
#define LV_USE_SPINNER    0
#define LV_USE_SWITCH     0
#define LV_USE_TABLE      0
#define LV_USE_TABVIEW    0
#define LV_USE_TEXTAREA   0
#define LV_USE_TILEVIEW   1
#define LV_USE_WIN        0
#define LV_USE_3DTEXTURE  0

#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK 0
    #define LV_THEME_DEFAULT_GROW 1
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif
#define LV_USE_THEME_SIMPLE  1
#define LV_USE_THEME_MONO    1

#define LV_USE_FLEX   1
#define LV_USE_GRID   1

/* SDL 驱动 */
#define LV_USE_SDL              1
#define LV_SDL_INCLUDE_PATH     <SDL2/SDL.h>
#define LV_SDL_RENDER_MODE      LV_DISPLAY_RENDER_MODE_PARTIAL
#define LV_SDL_BUF_COUNT        2
#define LV_SDL_FULLSCREEN       0
#define LV_SDL_DIRECT_EXIT      1
#define LV_SDL_MOUSEWHEEL_MODE  LV_SDL_MOUSEWHEEL_MODE_ENCODER

#define LV_USE_SNAPSHOT     0
#define LV_USE_SYSMON       0
#define LV_USE_PROFILER     0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR  0
#define LV_USE_OBSERVER     1
#define LV_USE_IMGFONT      0
/* 文件系统：POSIX 驱动（用于 TinyTTF/FreeType 从文件加载字体） */
#define LV_USE_FS_POSIX 1
#if LV_USE_FS_POSIX
    #define LV_FS_POSIX_LETTER 'A'  /**< 驱动字母，路径前缀：A:/abs/path */
    #define LV_FS_POSIX_PATH   ""   /**< 空：使用绝对路径 */
    #define LV_FS_POSIX_CACHE_SIZE 0
#endif

/* FreeType：用于 rk3506 等目标平台（需要系统预装 libfreetype） */
#ifdef BOX86_USE_FREETYPE
#  define LV_USE_FREETYPE   1
#  if LV_USE_FREETYPE
#    define LV_FREETYPE_USE_LVGL_PORT  0
#    define LV_FREETYPE_CACHE_FT_GLYPH_CNT 256
#  endif
#  define LV_USE_TINY_TTF   0
#else
/* TinyTTF：内置纯 C 实现，用于 SDL 模拟器（无需系统 freetype 库） */
#  define LV_USE_FREETYPE   0
#  define LV_USE_TINY_TTF   1
#  if LV_USE_TINY_TTF
#    define LV_TINY_TTF_FILE_SUPPORT      1
#    define LV_TINY_TTF_CACHE_GLYPH_CNT  256
#    define LV_TINY_TTF_CACHE_KERNING_CNT 256
#  endif
#endif
#define LV_USE_LODEPNG      0
#define LV_USE_LIBPNG       0
#define LV_USE_TJPGD        0
#define LV_USE_LIBJPEG_TURBO 0
#define LV_USE_BMP          0
#define LV_USE_RLE          0
#define LV_USE_QRCODE       0
#define LV_USE_BARCODE      0
#define LV_USE_RLOTTIE      0
#define LV_USE_THORVG_INTERNAL 0
#define LV_USE_THORVG_EXTERNAL 0
#define LV_USE_LZ4_INTERNAL 0
#define LV_USE_LZ4_EXTERNAL 0
#define LV_USE_VECTOR_GRAPHIC 0
#define LV_USE_FFMPEG       0
/* LV_USE_TINY_TTF 已在上方 FreeType/TinyTTF 选择块中定义 */
#define LV_USE_XML          0

/* 不使用 demo */
#define LV_USE_DEMO_MUSIC           0
#define LV_USE_DEMO_WIDGETS         0
#define LV_USE_DEMO_BENCHMARK       0
#define LV_USE_DEMO_STRESS          0

#endif /* LV_CONF_H */
#endif /* Enable content */
