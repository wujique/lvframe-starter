# 项目结构规划

## 概述

本目录是一个基于 LVGL 的嵌入式 UI 开发工作区。包含：
- **lvframe**：自研的 LVGL UI 框架（页面管理、事件总线、设备管理等）
- **platform**：平台适配层，仅支持 SDL 模拟器平台（用于 PC 端开发调试）
- **examples**：基于 lvframe 开发的具体 UI 应用工程（多工程并存）
- **lvgl**：LVGL 源码（v9.4，手动拷贝到此目录）

> **平台策略**：`str/platform/` 只维护 SDL 模拟器平台。其他运行平台（如 RK3506 Linux）
> 在各 UI 应用目录下的 `platform/<target>/` 中单独实现，与具体应用强绑定，互不干扰。

---

## 目录结构

```
.
├── lvframe/                        # LvFrame UI 框架（与平台无关）
│   ├── page.h / page.c
│   ├── page_manager.h / page_manager.c
│   ├── event_bus.h / event_bus.c
│   ├── swipe_container.h / swipe_container.c
│   ├── impl/
│   │   └── tileview_impl.c
│   ├── device/
│   │   ├── lv_device_model.h
│   │   ├── lv_device_store.h / lv_device_store.c
│   ├── README.md
│   ├── 详细设计文档.md
│   └── lvgl框架.md
│
├── platform/                       # 平台适配层（仅 SDL 模拟器）
│   ├── platform.h                  # 统一平台接口定义
│   └── sdl/                        # SDL 平台（Ubuntu PC 模拟器）
│       ├── platform_sdl.c          # SDL 显示/输入驱动 + LVGL tick
│       └── CMakeLists.txt
│
├── lvgl/                           # LVGL v9.4 源码（手动拷贝）
│   └── ...
│
├── examples/                       # 应用工程目录（多工程并存）
│   ├── box86/                      # box86 工程（智能家居中控屏）
│   │   ├── main.c                  # 应用入口
│   │   ├── pages/                  # 页面实现
│   │   ├── models/                 # 数据模型
│   │   ├── assets/                 # 图片、字体等资源
│   │   ├── platform/               # 应用自有平台支持
│   │   │   └── rk3506/             # RK3506 Linux 平台实现
│   │   │       ├── platform_rk3506.c
│   │   │       └── CMakeLists.txt
│   │   └── CMakeLists.txt
│   └── <other_app>/                # 其他工程（结构相同）
│
├── CMakeLists.txt                  # 顶层构建文件（提示入口）
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

**str/platform/ 只提供 SDL 模拟器平台**，供所有工程在 PC 上开发调试使用。

其他目标平台由各应用工程在自己的 `platform/<target>/` 目录下实现，例如：

| 位置 | 平台 | 用途 |
|------|------|------|
| `str/platform/sdl/` | Ubuntu + SDL2 | PC 端开发调试，模拟目标设备 UI |
| `examples/box86/platform/rk3506/` | RK3506 + Linux | box86 应用在目标硬件上运行 |

切换平台只需在编译时传入 `-DPLATFORM=sdl` 或 `-DPLATFORM=rk3506` 参数。
- `sdl`：链接 `str/platform/sdl/`
- `rk3506`：链接应用目录下的 `platform/rk3506/`

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
- `platform/`：应用自有平台支持（非 SDL 的目标平台在此实现）
- `CMakeLists.txt`：工程构建文件，通过 `PLATFORM` 参数决定链接哪个平台实现

---

## 构建方式

每个工程通过 `PLATFORM` 参数决定运行在哪个平台，与其他工程无关。

```bash
# box86 工程 —— 在 PC 上用 SDL 模拟器运行
cmake -DPLATFORM=sdl -S examples/box86 -B examples/box86/build
cmake --build examples/box86/build
./examples/box86/build/box86

# box86 工程 —— 交叉编译到 RK3506 Linux
cmake -DPLATFORM=rk3506 -DCMAKE_TOOLCHAIN_FILE=rk3506.cmake \
      -S examples/box86 -B examples/box86/build_rk
cmake --build examples/box86/build_rk
```

---

## LVGL 版本说明

使用 LVGL **v9.4**。

v9.x 相比 v7/v8 的主要变化（影响本项目的部分）：
- 显示驱动 API 改为 `lv_display_create()` / `lv_display_set_flush_cb()`
- 输入驱动 API 改为 `lv_indev_create()` / `lv_indev_set_read_cb()`
- `lv_task_t` 改为 `lv_timer_t`
- `lv_mutex_t` 线程 API 有所调整

---

开发过程中的踩坑记录和操作日志见 [DEVLOG.md](DEVLOG.md)。
