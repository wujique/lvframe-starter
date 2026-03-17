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
