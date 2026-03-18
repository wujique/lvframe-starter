# 开发日志

## 2026-03-17 — music demo 编译运行

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

---

## 2026-03-17 — MobaXterm SSH 远程运行 SDL 程序

**现象**：通过 MobaXterm SSH 登录 WSL 后运行 SDL 程序，约两秒后报错退出：
```
X connection to localhost:10.0 broken (explicit kill or server shutdown)
```

**原因**：SDL2 默认启用硬件加速渲染，会调用 X11 的 MIT-SHM 共享内存扩展（`XShmPutImage`）。MobaXterm 内置的 X server 对该扩展支持不稳定，导致连接断开。

**解决方案**：运行前禁用硬件加速，强制使用软件渲染：
```bash
export SDL_RENDER_DRIVER=software
export LIBGL_ALWAYS_SOFTWARE=1
./build/music
```

---

## 2026-03-18 — box86 工程实现

### 主要工作

1. **platform 层重构**：`platform_init(void)` 改为 `platform_init(uint32_t hor_res, uint32_t ver_res)`，分辨率由各工程 `main.c` 传入，不再硬编码在平台层。`platform_sdl.c` 改用 LVGL 内置 SDL 驱动（`lv_sdl_window_create` + `lv_sdl_mouse_create`）。

2. **box86 工程完整实现**，目录结构：
   ```
   examples/box86/
   ├── main.c
   ├── lv_conf.h
   ├── CMakeLists.txt
   ├── app_bus.h/c          # 跨线程消息总线（UI ↔ 业务）
   ├── business.h/c         # 业务逻辑线程 + Shell 命令行接口
   ├── models/
   │   ├── device_model.h   # DeviceBase + LightModel + CctLightModel + CurtainModel
   │   └── device_store.h/c # 设备数据仓库（mutex 保护，快照读取）
   └── pages/
       ├── home_page.h/c
       ├── settings_page.h/c
       ├── more_settings_page.h/c
       ├── device_page.h/c          # 设备页调度层
       ├── device_page_internal.h
       ├── light_page.c
       ├── cct_light_page.c
       └── curtain_page.c
   ```

3. **设备模型设计**：每种设备独立结构体，`DeviceBase` 作为第一个成员（C 语言继承模式），支持 `void*` 指针数组统一存储。

4. **线程架构**：LVGL 主线程 + 业务逻辑线程，通过 `AppBus`（双向消息队列）通信，UI 通过快照（snapshot）读取数据，避免锁竞争。

5. **Shell 命令行接口**：业务线程读取 stdin，支持 `list / get / add / del / move / set` 命令。

6. **编译修复**：
   - `lvframe/event_bus.h`：修复 `Event` 类型重复定义冲突
   - `lvframe/page.h`：加入 `lv_os_private.h` include，提供 `lv_mutex_t` / `lv_thread_t` 类型
   - `lvframe/swipe_container.h`：修复 lvgl include 路径
   - `platform/platform.h`：修复 lvgl include 路径
   - `app_bus.h` / `device_store.h`：加入 `lv_os_private.h` include
   - `business.c`：修复 `lv_thread_init` 缺少 `name` 参数

7. **UI 文本全部改为英文**（LVGL 默认不支持中文字体）。

**编译命令**：
```bash
cd examples/box86
cmake -B build -DPLATFORM=sdl -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
```

**运行命令**：
```bash
SDL_RENDER_DRIVER=software ./build/box86
```

**Shell 使用示例**：
```
list
add light Bedroom
add cct_light Kitchen
add curtain LivingRoom
set 1 onoffsta 1
set 2 color_temp 3000
set 3 command OPEN
del 2
move 1 2
```
