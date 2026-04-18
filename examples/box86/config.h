/**
 * config.h - box86 应用配置
 *
 * 由 CMake 在构建时通过 -DBOX86_ASSETS_PATH=... 传入，
 * 或使用下方的默认值（SDL 模拟器下直接引用工程内 assets 目录）。
 *
 * 各平台 assets 路径：
 *   SDL 模拟器  : <工程目录>/assets  （开发阶段直接引用源码树）
 *   RK3506 Linux: /usr/share/box86/assets （部署到目标机后的路径）
 */

#ifndef BOX86_CONFIG_H
#define BOX86_CONFIG_H

/* assets 根目录，由 CMake 注入，默认指向工程内 assets/ */
#ifndef BOX86_ASSETS_PATH
#define BOX86_ASSETS_PATH "./assets"
#endif

/* 字体文件路径
 * TinyTTF（SDL 模拟器）通过 LVGL 虚拟文件系统读取，需要 "A:" 驱动前缀
 * FreeType（rk3506）直接用 fopen，使用原生绝对路径
 */
#ifdef BOX86_USE_FREETYPE
#  define BOX86_FONT_CN_PATH  BOX86_ASSETS_PATH "/font/SourceHanSerifCN-Regular.otf"
#else
#  define BOX86_FONT_CN_PATH  "A:" BOX86_ASSETS_PATH "/font/SourceHanSerifCN-Regular.otf"
#endif

/* 中文字体默认大小 */
#define BOX86_FONT_CN_SIZE  20

#endif /* BOX86_CONFIG_H */
