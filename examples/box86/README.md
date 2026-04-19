# box86

智能家居中控屏 UI 应用，基于 [LVGL 9.4](https://lvgl.io) 和自研的 [lvframe](../../lvframe) 页面管理框架开发。

---

## 工程概述

box86 实现了一个触控式智能家居控制面板，支持：

- 多设备页左右滑动切换（普通灯 / 色温灯 / 电动窗帘）
- 下拉手势进入设置页，上拉返回
- 通过终端 Shell 命令动态添加、删除、控制设备
- UI 与业务逻辑双线程解耦，通过消息总线通信
- 中文界面显示（FreeType 引擎，思源宋体）

详细设计见 [DESIGN.md](DESIGN.md)，测试操作见 [TESTING.md](TESTING.md)。

---

## 工程结构

```
box86/
├── main.c                  # 应用入口，初始化平台、字体、页面、业务线程
├── font.h / font.c         # 中文字体加载与注入
├── config.h                # 编译期配置（assets 路径、字体路径等）
├── lv_conf.h               # LVGL 功能配置
├── app_bus.h / app_bus.c   # UI ↔ 业务线程消息队列
├── app_bus_adapter.h/c     # 将 AppBus 消息桥接到 lvframe EventBus
├── business.h / business.c # 业务逻辑线程（Shell 命令解析、数据模型管理）
├── pages/                  # 各页面实现
│   ├── home_page.c         # 主页（设备页容器 + 下拉设置页）
│   ├── device_page.c       # 设备页分发（按类型路由到具体实现）
│   ├── light_page.c        # 普通灯控制页
│   ├── cct_light_page.c    # 色温灯控制页
│   ├── curtain_page.c      # 电动窗帘控制页
│   ├── settings_page.c     # 设置页
│   └── more_settings_page.c# 更多设置页
├── models/                 # 数据模型定义
├── assets/                 # 资源文件
│   └── font/               # 字体文件
│       └── SourceHanSerifCN-Regular.ttf
├── platform/               # 应用自有平台支持
│   └── rk3506/             # RK3506 Linux 平台实现
│       ├── platform_rk3506.c
│       └── CMakeLists.txt
├── CMakeLists.txt
├── DESIGN.md               # 详细设计文档
├── README.md               # 本文件
└── TESTING.md              # 测试操作指南
```

---

## 依赖

| 依赖 | 用途 | SDL 模拟器 | RK3506 |
|------|------|-----------|--------|
| SDL2 | 显示与输入驱动 | 必须 | 不需要 |
| FreeType | 中文字体渲染 | 需要头文件（见下方说明） | 系统预装 |
| pthreads | 双线程架构 | 必须 | 必须 |
| CMake 3.16+ | 构建系统 | 必须 | 必须 |

### SDL 模拟器环境安装依赖

```bash
sudo apt-get install build-essential cmake libsdl2-dev
```

FreeType 运行库通常已预装（`libfreetype6`）。头文件已内置于工程 `third_party/freetype2/`，无需额外操作。

---

## 编译

### SDL 模拟器（PC 开发调试）

```bash
cmake -DPLATFORM=sdl -S examples/box86 -B examples/box86/build
cmake --build examples/box86/build -j$(nproc)
```

### RK3506 Linux（交叉编译）

```bash
cmake -DPLATFORM=rk3506 \
      -DCMAKE_TOOLCHAIN_FILE=rk3506.cmake \
      -S examples/box86 -B examples/box86/build_rk
cmake --build examples/box86/build_rk -j$(nproc)
cmake --install examples/box86/build_rk
```

安装后：
- 可执行文件：`/usr/bin/box86`
- 资源文件：`/usr/share/box86/assets/`
- LVGL 共享库：`/usr/lib/liblvgl.so`

---

## 运行

```bash
# 图形界面运行
./examples/box86/build/box86

# 无头模式（无显示环境）
SDL_VIDEODRIVER=dummy ./examples/box86/build/box86
```

启动成功后终端可直接输入 Shell 命令与应用交互，详见 [TESTING.md](TESTING.md)。

---

## 平台适配

| 平台 | 字体路径 | assets 路径 |
|------|---------|------------|
| SDL 模拟器 | `<工程目录>/assets/font/` | `<工程目录>/assets/` |
| RK3506 Linux | `/usr/share/box86/assets/font/` | `/usr/share/box86/assets/` |

路径由 CMake 构建时注入 `BOX86_ASSETS_PATH` 宏，代码通过 `config.h` 引用，无需手动修改。
