# 项目结构

## 概述

本目录是一个基于 LVGL 的嵌入式 UI 开发工作区。包含：

- **lvframe**：自研的 LVGL UI 框架（页面生命周期栈 + 消息槽）
- **platform**：平台适配层，仅支持 SDL 模拟器平台（用于 PC 端开发调试）
- **examples**：基于 lvframe 开发的具体 UI 应用工程（多工程并存）
- **lvgl**：LVGL 源码（v9.4，手动拷贝到此目录）

> **框架边界**：lvframe 只提供两类能力——**消息槽**与**页面生命周期栈（含左右滑动页管理）**。
> 数据模型（设备或用户自定义结构体）的定义、存储、使用方式，以及"何时添加/删除页面"的决策逻辑，全部归应用层。

> **平台策略**：`str/platform/` 只维护 SDL 模拟器平台。其他运行平台（如 RK3506 Linux）
> 在各 UI 应用目录下的 `platform/<target>/` 中单独实现，与具体应用强绑定，互不干扰。

---

## 目录结构

```
.
├── lvframe/                        # LvFrame UI 框架（与平台无关）
│   ├── slot.h / slot.c             # 消息槽（signal + object 分发、payload 释放）
│   ├── page.h / page.c             # 页面生命周期 + 槽绑定
│   ├── page_manager.h / page_manager.c  # 页面管理器（导航栈、缓存）
│   ├── swipe_container.h / swipe_container.c  # 滑动容器抽象层
│   ├── impl/
│   │   └── tileview_impl.c         # lv_tileview 滑动容器实现
│   └── 设计文档.html                 # 框架设计 + API 参考
│
├── platform/                       # 平台适配层（仅 SDL 模拟器）
│   ├── platform.h                  # 统一平台接口定义
│   └── sdl/                        # SDL 平台（Ubuntu PC 模拟器）
│       ├── platform_sdl.c          # SDL 显示/输入驱动
│       └── CMakeLists.txt
│
├── lvgl/                           # LVGL v9.4 源码（手动拷贝）
│   └── ...
│
├── examples/                       # 应用工程目录（多工程并存）
│   ├── box86/                      # box86 工程（智能家居中控屏）
│   │   ├── main.c                  # 应用入口
│   │   ├── msg.h                   # 应用信号枚举
│   │   ├── slots.h / slots.c       # g_ui_slot / g_dev_slot 双槽实例
│   │   ├── business.h / business.c # 业务线程（消费 g_dev_slot + Shell）
│   │   ├── pages/                  # 页面实现
│   │   ├── models/                 # 应用数据模型（应用层，不归 lvframe）
│   │   ├── assets/                 # 图片、字体等资源
│   │   ├── platform/               # 应用自有平台支持
│   │   │   └── rk3506/             # RK3506 Linux 平台实现
│   │   ├── CMakeLists.txt
│   │   └── DESIGN.html             # 详细设计文档
│   └── <other_app>/                # 其他工程（结构相同）
│
├── CMakeLists.txt                  # 顶层构建文件（提示入口）
└── PROJECT_STRUCTURE.md            # 本文档
```

---

## 各模块说明

### lvframe/

与平台完全无关的 UI 框架层，不包含任何平台驱动、SDL、RTOS 相关代码。只提供：

- **slot.h / slot.c**：消息槽。任意线程 `lv_slot_send()` 发送；唯一消费者线程 `lv_slot_process()` 按 `signal + object` 分发（object 为 NULL 时双向通配）；payload 由槽负责释放（可选析构）；四元组退订。
- **page.h / page.c**：页面生命周期（`on_create/on_start/on_resume/on_pause/on_stop/on_destroy/on_msg`）+ 槽绑定（`page_bind_slot` / `page_unbind_slot`，销毁自动退订）。
- **page_manager.h / page_manager.c**：页面注册表、导航栈、LRU 缓存。
- **swipe_container.h / .c + impl/tileview_impl.c**：左右滑动页容器抽象层（ops 函数表 + tileview 实现）。

lvframe **不定义、不管理、不存储任何数据模型**；`object` 只是不透明路由键（仅指针比较）。

### platform/

平台适配层（HAL，Hardware Abstraction Layer），定义统一接口 `platform.h`：

- `platform_init(hor_res, ver_res)`：创建显示/输入驱动
- `platform_deinit()`：释放资源
- `platform_delay_ms(ms)`：主循环延时

**str/platform/ 只提供 SDL 模拟器平台**，供所有工程在 PC 上开发调试使用。其他目标平台由各应用工程在自己的 `platform/<target>/` 目录下实现：

| 位置 | 平台 | 用途 |
|------|------|------|
| `str/platform/sdl/` | Ubuntu + SDL2 | PC 端开发调试 |
| `examples/box86/platform/rk3506/` | RK3506 + Linux | box86 在目标硬件上运行 |

切换平台只需传入 `-DPLATFORM=sdl` 或 `-DPLATFORM=rk3506`。

### lvgl/

LVGL v9.4 源码目录。顶层 CMakeLists.txt 统一引用，各工程无需重复配置。

### examples/

多工程并存的应用目录，每个子目录是一个独立的 UI 应用工程。每个工程的标准结构：

- `main.c`：应用入口，初始化平台/槽/模型仓库/业务线程，注册页面，启动 LVGL 主循环
- `msg.h`：应用信号枚举
- `slots.h / slots.c`：双槽实例（`g_ui_slot` / `g_dev_slot`）
- `business.h / business.c`：业务线程（消费 g_dev_slot + Shell 解析）
- `pages/`：各页面实现（基于 lvframe 的 `Page` / `PageLifecycle`）
- `models/`：应用数据模型（`model_base` + `model_store`，与 UI/lvframe 解耦）
- `assets/`：图片、字体等静态资源
- `platform/`：应用自有平台支持（非 SDL 的目标平台在此实现）
- `CMakeLists.txt`：工程构建文件

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
- `lv_mutex_t` / `lv_thread_sync_t` 线程 API 有所调整
