/**
 * font_bench.c - FreeType vs stb_truetype (TinyTTF) 汉字渲染性能基准测试
 *
 * 测量内容：
 *   1. 字体文件加载时间（font init）
 *   2. 首次渲染单个汉字耗时（无缓存）
 *   3. 重复渲染同一汉字耗时（缓存命中）
 *   4. 渲染 100 个不同汉字耗时
 *   5. 堆内存占用（通过 mallinfo）
 *
 * 编译：
 *   # FreeType
 *   gcc -O2 -o font_bench_ft font_bench.c \
 *       -I/path/to/freetype2 -lfreetype -lm -DTEST_FREETYPE
 *
 *   # stb_truetype (TinyTTF)
 *   gcc -O2 -o font_bench_stb font_bench.c -lm -DTEST_STB
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <malloc.h>  /* mallinfo2 */

/* ── 计时工具 ─────────────────────────────────────────────── */
static inline int64_t now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

static inline int64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

/* ── 堆内存快照 ───────────────────────────────────────────── */
static size_t heap_used(void)
{
    struct mallinfo2 mi = mallinfo2();
    return mi.uordblks;
}

/* ═══════════════════════════════════════════════════════════
 * FreeType 测试
 * ═══════════════════════════════════════════════════════════ */
#ifdef TEST_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H

#define FONT_SIZE_PX  20
#define RENDER_REPEAT 1000

/* 测试用汉字（Unicode 码点）*/
static const uint32_t TEST_CHARS[] = {
    0x4E2D, /* 中 */
    0x6587, /* 文 */
    0x6E32, /* 渲 */
    0x67D3, /* 染 */
    0x6D4B, /* 测 */
    0x8BD5, /* 试 */
    0x6027, /* 性 */
    0x80FD, /* 能 */
    0x5206, /* 分 */
    0x6790, /* 析 */
};
#define N_TEST_CHARS (sizeof(TEST_CHARS)/sizeof(TEST_CHARS[0]))

/* 常用汉字表（用于 100 字测试，GB2312 一级常用字范围采样）*/
static const uint32_t COMMON_CHARS_100[] = {
    0x4E2D,0x6587,0x6E32,0x67D3,0x6D4B,0x8BD5,0x6027,0x80FD,0x5206,0x6790,
    0x5927,0x5C0F,0x4EBA,0x5929,0x5730,0x6C34,0x706B,0x9A6C,0x725B,0x7F8A,
    0x5FC3,0x624B,0x53E3,0x773C,0x8033,0x9F3B,0x8138,0x5934,0x8179,0x811A,
    0x6625,0x590F,0x79CB,0x51AC,0x98CE,0x96E8,0x96EA,0x96F7,0x7535,0x706B,
    0x5C71,0x6CB3,0x6D77,0x6E56,0x6797,0x82B1,0x8349,0x6811,0x53F6,0x6839,
    0x7EA2,0x7EFF,0x84DD,0x767D,0x9ED1,0x9EC4,0x7D2B,0x6A59,0x7C89,0x5F69,
    0x4E00,0x4E8C,0x4E09,0x56DB,0x4E94,0x516D,0x4E03,0x516B,0x4E5D,0x5341,
    0x767E,0x5343,0x4E07,0x4EBF,0x5C81,0x5E74,0x6708,0x65E5,0x65F6,0x5206,
    0x5929,0x5730,0x4EBA,0x548C,0x56FD,0x5BB6,0x793E,0x4F1A,0x5B66,0x6821,
    0x5B66,0x751F,0x8001,0x5E08,0x5DE5,0x4EBA,0x519C,0x6C11,0x5E02,0x573A,
};

static void run_freetype_bench(const char *font_path)
{
    int64_t t0, t1;
    size_t  mem0, mem1;
    FT_Library library;
    FT_Face    face;
    FT_Error   err;

    printf("=== FreeType 基准测试 ===\n");
    printf("字体: %s\n", font_path);
    printf("字号: %d px\n\n", FONT_SIZE_PX);

    /* ── 1. 字体加载时间 ── */
    mem0 = heap_used();
    t0 = now_us();
    err = FT_Init_FreeType(&library);
    if (err) { fprintf(stderr, "FT_Init_FreeType failed: %d\n", err); return; }
    err = FT_New_Face(library, font_path, 0, &face);
    if (err) { fprintf(stderr, "FT_New_Face failed: %d\n", err); return; }
    err = FT_Set_Pixel_Sizes(face, 0, FONT_SIZE_PX);
    if (err) { fprintf(stderr, "FT_Set_Pixel_Sizes failed: %d\n", err); return; }
    t1 = now_us();
    mem1 = heap_used();
    printf("[1] 字体加载耗时:        %6lld us\n", (long long)(t1 - t0));
    printf("    堆内存增量:           %6zu KB\n\n", (mem1 - mem0) / 1024);

    /* ── 2. 首次渲染单个汉字（无缓存）── */
    {
        int64_t ns0, ns1;
        uint32_t cp = 0x4E2D; /* '中' */
        FT_UInt glyph_idx = FT_Get_Char_Index(face, cp);

        ns0 = now_ns();
        err = FT_Load_Glyph(face, glyph_idx, FT_LOAD_DEFAULT);
        err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        ns1 = now_ns();
        printf("[2] 首次渲染 U+%04X '中': %6lld ns  (%.3f us)\n",
               cp, (long long)(ns1 - ns0), (ns1 - ns0) / 1000.0);
    }

    /* ── 3. 重复渲染同一汉字（%d 次，统计均值）── */
    {
        uint32_t cp = 0x4E2D;
        FT_UInt glyph_idx = FT_Get_Char_Index(face, cp);
        int64_t total = 0;
        for (int i = 0; i < RENDER_REPEAT; i++) {
            int64_t ns0 = now_ns();
            FT_Load_Glyph(face, glyph_idx, FT_LOAD_DEFAULT);
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
            total += now_ns() - ns0;
        }
        printf("[3] 重复渲染同字均值(%d次): %6lld ns  (%.3f us)\n",
               RENDER_REPEAT, (long long)(total / RENDER_REPEAT),
               (total / RENDER_REPEAT) / 1000.0);
    }

    /* ── 4. 渲染 100 个不同汉字 ── */
    {
        int64_t ns0 = now_ns();
        for (int i = 0; i < 100; i++) {
            FT_UInt gi = FT_Get_Char_Index(face, COMMON_CHARS_100[i]);
            FT_Load_Glyph(face, gi, FT_LOAD_DEFAULT);
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        }
        int64_t elapsed = now_ns() - ns0;
        printf("[4] 渲染 100 个不同汉字:  %6lld us  (均值 %.1f ns/字)\n\n",
               (long long)(elapsed / 1000), (double)elapsed / 100.0);
    }

    mem1 = heap_used();
    printf("    当前堆内存占用:       %6zu KB\n", mem1 / 1024);

    FT_Done_Face(face);
    FT_Done_FreeType(library);
}

int main(int argc, char *argv[])
{
    const char *font = argc > 1 ? argv[1]
        : "assets/font/SourceHanSerifCN-Regular.ttf";
    run_freetype_bench(font);
    return 0;
}
#endif /* TEST_FREETYPE */

/* ═══════════════════════════════════════════════════════════
 * stb_truetype (TinyTTF) 测试
 * ═══════════════════════════════════════════════════════════ */
#ifdef TEST_STB

/* stb_truetype 单头文件实现 */
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert(x)  /* 屏蔽断言，让 CFF 路径继续跑而不是 abort */
#include "../../../lvgl/lvgl/src/libs/tiny_ttf/stb_truetype_htcw.h"

#define FONT_SIZE_PX  20
#define RENDER_REPEAT 1000
#define MAX_FONT_SIZE (32 * 1024 * 1024)  /* 32 MB 上限 */

static const uint32_t COMMON_CHARS_100[] = {
    0x4E2D,0x6587,0x6E32,0x67D3,0x6D4B,0x8BD5,0x6027,0x80FD,0x5206,0x6790,
    0x5927,0x5C0F,0x4EBA,0x5929,0x5730,0x6C34,0x706B,0x9A6C,0x725B,0x7F8A,
    0x5FC3,0x624B,0x53E3,0x773C,0x8033,0x9F3B,0x8138,0x5934,0x8179,0x811A,
    0x6625,0x590F,0x79CB,0x51AC,0x98CE,0x96E8,0x96EA,0x96F7,0x7535,0x706B,
    0x5C71,0x6CB3,0x6D77,0x6E56,0x6797,0x82B1,0x8349,0x6811,0x53F6,0x6839,
    0x7EA2,0x7EFF,0x84DD,0x767D,0x9ED1,0x9EC4,0x7D2B,0x6A59,0x7C89,0x5F69,
    0x4E00,0x4E8C,0x4E09,0x56DB,0x4E94,0x516D,0x4E03,0x516B,0x4E5D,0x5341,
    0x767E,0x5343,0x4E07,0x4EBF,0x5C81,0x5E74,0x6708,0x65E5,0x65F6,0x5206,
    0x5929,0x5730,0x4EBA,0x548C,0x56FD,0x5BB6,0x793E,0x4F1A,0x5B66,0x6821,
    0x5B66,0x751F,0x8001,0x5E08,0x5DE5,0x4EBA,0x519C,0x6C11,0x5E02,0x573A,
};

static void run_stb_bench(const char *font_path)
{
    int64_t t0, t1;
    size_t  mem0, mem1;

    printf("=== stb_truetype (TinyTTF) 基准测试 ===\n");
    printf("字体: %s\n", font_path);
    printf("字号: %d px\n\n", FONT_SIZE_PX);

    /* ── 1. 字体加载时间 ── */
    mem0 = heap_used();
    t0 = now_us();

    FILE *f = fopen(font_path, "rb");
    if (!f) { fprintf(stderr, "Cannot open font: %s\n", font_path); return; }
    fseek(f, 0, SEEK_END);
    long font_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *font_buf = (unsigned char *)malloc(font_size);
    if (!font_buf) { fprintf(stderr, "malloc failed\n"); fclose(f); return; }
    fread(font_buf, 1, font_size, f);
    fclose(f);

    stbtt_fontinfo info;
    int ok = stbtt_InitFont(&info, font_buf, stbtt_GetFontOffsetForIndex(font_buf, 0));
    t1 = now_us();
    mem1 = heap_used();

    printf("[1] 字体加载耗时:        %6lld us\n", (long long)(t1 - t0));
    printf("    字体文件大小:         %6ld KB\n", font_size / 1024);
    printf("    堆内存增量:           %6zu KB\n\n", (mem1 - mem0) / 1024);

    if (!ok) {
        fprintf(stderr, "stbtt_InitFont failed (可能是 CFF/OTF 格式不兼容)\n");
        free(font_buf);
        return;
    }

    float scale = stbtt_ScaleForPixelHeight(&info, FONT_SIZE_PX);
    int w, h, xoff, yoff;

    /* ── 2. 首次渲染单个汉字 ── */
    {
        int64_t ns0, ns1;
        int cp = 0x4E2D; /* '中' */
        int gi = stbtt_FindGlyphIndex(&info, cp);

        ns0 = now_ns();
        unsigned char *bitmap = stbtt_GetGlyphBitmap(&info, scale, scale,
                                                      gi, &w, &h, &xoff, &yoff);
        ns1 = now_ns();
        if (bitmap) stbtt_FreeBitmap(bitmap, NULL);

        printf("[2] 首次渲染 U+%04X '中': %6lld ns  (%.3f us)  bitmap=%dx%d\n",
               cp, (long long)(ns1 - ns0), (ns1 - ns0) / 1000.0, w, h);
    }

    /* ── 3. 重复渲染同一汉字 ── */
    {
        int cp = 0x4E2D;
        int gi = stbtt_FindGlyphIndex(&info, cp);
        int64_t total = 0;
        for (int i = 0; i < RENDER_REPEAT; i++) {
            int64_t ns0 = now_ns();
            unsigned char *bm = stbtt_GetGlyphBitmap(&info, scale, scale,
                                                      gi, &w, &h, &xoff, &yoff);
            total += now_ns() - ns0;
            if (bm) stbtt_FreeBitmap(bm, NULL);
        }
        printf("[3] 重复渲染同字均值(%d次): %6lld ns  (%.3f us)\n",
               RENDER_REPEAT, (long long)(total / RENDER_REPEAT),
               (total / RENDER_REPEAT) / 1000.0);
    }

    /* ── 4. 渲染 100 个不同汉字 ── */
    {
        int64_t ns0 = now_ns();
        int ok_count = 0;
        for (int i = 0; i < 100; i++) {
            int gi = stbtt_FindGlyphIndex(&info, COMMON_CHARS_100[i]);
            unsigned char *bm = stbtt_GetGlyphBitmap(&info, scale, scale,
                                                      gi, &w, &h, &xoff, &yoff);
            if (bm) { ok_count++; stbtt_FreeBitmap(bm, NULL); }
        }
        int64_t elapsed = now_ns() - ns0;
        printf("[4] 渲染 100 个不同汉字:  %6lld us  (均值 %.1f ns/字)  成功:%d/100\n\n",
               (long long)(elapsed / 1000), (double)elapsed / 100.0, ok_count);
    }

    mem1 = heap_used();
    printf("    当前堆内存占用:       %6zu KB\n", mem1 / 1024);

    free(font_buf);
}

int main(int argc, char *argv[])
{
    const char *font = argc > 1 ? argv[1]
        : "assets/font/SourceHanSerifCN-Regular.ttf";
    run_stb_bench(font);
    return 0;
}
#endif /* TEST_STB */
