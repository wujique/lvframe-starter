# 项目结构规划

## 概述

本目录是一个基于 LVGL 的嵌入式 UI 开发工作区。包含：
- **lvframe**：自研的 LVGL UI 框架（页面管理、事件总线、设备管理等）
- **platform**：平台适配层，屏蔽不同硬件/OS 差异
- **examples**：基于 lvframe 开发的具体 UI 应用工程（多工程并存）
- **lvgl**：LVGL 源码（v9.4，手动拷贝到此目录）

---

## 目录结构

```
.
├── lvframe/                        # LvFrame UI 框架（与平台无关）
│   ├── page.h / page.c
│   ├── page_manager.h / page_manager.c
│   ├── event_bus.h / event_bus.c
│   ├── swipe_container.h / swipe_container.c
│   ├── device_manager.h / device_manager.c
│   ├── impl/
│   │   └── tileview_impl.c
│   ├── README.md
│   ├── 详细设计文档.md
│   └── lvgl框架.md
│
├── platform/                       # 平台适配层（HAL）
│   ├── platform.h                  # 统一平台接口定义
│   ├── sdl/                        # SDL 平台（Ubuntu PC 模拟器）
│   │   ├── platform_sdl.c          # SDL 显示/输入驱动 + LVGL tick
│   │   └── CMakeLists.txt
│   ├── rk3506/                     # RK3506 Linux 平台
│   │   ├── platform_rk3506.c       # framebuffer/DRM 显示 + 触摸输入驱动
│   │   └── CMakeLists.txt
│   └── rtos/                       # RTOS 平台（预留，未来扩展）
│       ├── platform_rtos.c
│       └── CMakeLists.txt
│
├── lvgl/                           # LVGL v9.4 源码（手动拷贝）
│   └── ...
│
├── examples/                       # 应用工程目录（多工程并存）
│   ├── box86/                      # box86 工程（RK3506 Linux UI 程序）
│   │   ├── main.c                  # 应用入口
│   │   ├── pages/                  # 页面实现
│   │   ├── models/                 # 数据模型
│   │   ├── assets/                 # 图片、字体等资源（工程独立管理）
│   │   └── CMakeLists.txt
│   └── <other_app>/                # 其他工程（结构相同）
│
├── CMakeLists.txt                  # 顶层构建文件
└── PROJECT_STRUCTURE.md            # 本文档
```

---

## 各模块说明

### lvframe/
与平台完全无关的 UI 框架层。不包含任何平台驱动、SDL、RTOS 相关代码。
所有工程均可直接引用此目录下的头文件和源文件。

### platform/
平台适配层（HAL，Hardware Abstraction Layer）。

定义统一接口 `platform.h`，包括：
- LVGL 显示驱动注册（`lv_display_t`）
- LVGL 输入驱动注册（`lv_indev_t`）
- 系统 tick 提供（`lv_tick_inc`）
- 平台初始化/反初始化接口

三个平台实现：

| 子目录 | 平台 | 用途 |
|--------|------|------|
| `sdl/` | Ubuntu + SDL2 | PC 端开发调试，模拟目标设备 UI |
| `rk3506/` | RK3506 + Linux | 目标硬件，framebuffer 或 DRM 驱动 |
| `rtos/` | RTOS（预留） | 未来移植到 RTOS 平台时扩展 |

切换平台只需在编译时传入 `-DPLATFORM=sdl` 或 `-DPLATFORM=rk3506` 参数。

### lvgl/
LVGL v9.4 源码目录，由开发者手动下载并拷贝到此处。
顶层 CMakeLists.txt 统一引用，各工程无需重复配置。

### examples/
多工程并存的应用目录，每个子目录是一个独立的 UI 应用工程。

每个工程的标准结构：
- `main.c`：应用入口，调用 platform 初始化，注册 lvframe 页面，启动 LVGL 主循环
- `pages/`：各页面实现（基于 lvframe 的 `Page` / `PageLifecycle`）
- `models/`：业务数据模型（与 UI 解耦）
- `assets/`：图片、字体等静态资源（每个工程独立管理）
- `CMakeLists.txt`：工程构建文件，通过 `PLATFORM` 参数决定链接哪个平台实现

---

## 构建方式

每个工程通过 `PLATFORM` 参数决定运行在哪个平台，与其他工程无关。

```bash
# box86 工程 —— 在 PC 上用 SDL 模拟器运行
cmake -B build -DAPP=box86 -DPLATFORM=sdl
cmake --build build
./build/box86

# box86 工程 —— 交叉编译到 RK3506 Linux
cmake -B build_rk -DAPP=box86 -DPLATFORM=rk3506 -DCMAKE_TOOLCHAIN_FILE=rk3506.cmake
cmake --build build_rk

# box86 工程 —— 未来编译到 RTOS（预留）
cmake -B build_rtos -DAPP=box86 -DPLATFORM=rtos -DCMAKE_TOOLCHAIN_FILE=rtos.cmake
cmake --build build_rtos
```

---

## LVGL 版本说明

使用 LVGL **v9.4**。

v9.x 相比 v7/v8 的主要变化（影响本项目的部分）：
- 显示驱动 API 改为 `lv_display_create()` / `lv_display_set_flush_cb()`
- 输入驱动 API 改为 `lv_indev_create()` / `lv_indev_set_read_cb()`
- `lv_task_t` 改为 `lv_timer_t`
- `lv_mutex_t` 线程 API 有所调整

> 注意：lvframe 现有代码基于 v7 API 编写，迁移到 v9.4 时需要同步更新 platform 层和 lvframe 内部的 LVGL API 调用。

---

## 开发日志

### 2026-03-17 — music demo 编译运行

**目标**：将 LVGL 内置的 music demo 作为独立工程跑在 SDL 模拟器上。

**操作步骤**：

1. 从 `lvgl/lvgl/demos/music/` 拷贝所有源文件到 `examples/music/`。

2. 创建 `examples/music/lv_conf.h`，关键配置：
   - `LV_COLOR_DEPTH 32`
   - `LV_USE_SDL 1` + `LV_SDL_RENDER_MODE LV_DISPLAY_RENDER_MODE_PARTIAL`
   - `LV_USE_OS LV_OS_PTHREAD`
   - `LV_USE_DEMO_MUSIC 1` / `LV_DEMO_MUSIC_AUTO_PLAY 1`
   - 启用 Montserrat 12/14/16/24 字体

3. 创建 `examples/music/main.c`：
   ```c
   lv_init();
   lv_display_t *disp = lv_sdl_window_create(480, 272);
   lv_sdl_window_set_title(disp, "LVGL Music Demo");
   lv_indev_t *mouse = lv_sdl_mouse_create();
   lv_demo_music();
   while (1) { /* lv_timer_handler + nanosleep */ }
   ```
   注意：需要在文件顶部加 `#define _POSIX_C_SOURCE 200809L` 才能使用 `nanosleep`。

4. 创建 `examples/music/CMakeLists.txt`，关键点：
   - 用 `get_filename_component(ROOT_DIR ... ABSOLUTE)` 获取工作区根目录绝对路径
   - 全局 `include_directories(${ROOT_DIR}/lvgl)` 让 assets/*.c 也能找到 `lvgl/lvgl.h`
   - `add_compile_definitions(LV_CONF_INCLUDE_SIMPLE)` + `include_directories(${CMAKE_SOURCE_DIR})` 让 LVGL 找到本工程的 `lv_conf.h`
   - 链接 `lvgl SDL2::SDL2 m pthread`，不需要 `lvgl::examples`

5. 修正 demo 源码中的 include 路径（原始路径基于 LVGL 内部目录结构）：
   - `lv_demo_music.h`：`../lv_demos.h` → `lvgl/lvgl.h`
   - `lv_demo_music_main.c`：`../../lvgl_private.h` → `lvgl/lvgl_private.h`

**编译命令**：
```bash
cd examples/music
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
./build/music
```

**踩坑记录**：
- `ROOT_DIR` 必须用 `get_filename_component(...ABSOLUTE)` 转为绝对路径，否则 CMake 子目录中相对路径解析会出错
- include 路径应设为 `ROOT_DIR/lvgl`（而非 `ROOT_DIR`），因为 `lvgl.h` 实际位于 `ROOT_DIR/lvgl/lvgl/lvgl.h`，`#include "lvgl/lvgl.h"` 才能正确解析
- `nanosleep` 需要 `_POSIX_C_SOURCE 200809L` 和 `#include <time.h>`
