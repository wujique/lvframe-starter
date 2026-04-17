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

## 2026-04-11 — box86 架构重构与 lvframe 集成

**目标**：统一 box86 与 lvframe 的架构，消除重复实现，提高代码健壮性，确保 LVGL v9.4 API 兼容性。

**完成的工作**：

### Phase 1: lvframe 框架增强
1. **修复资源泄漏**：在 `event_bus.c` 中添加 `event_bus_deinit()`，删除内部定时器和互斥锁。
2. **修复 LVGL v9.4 API 兼容性**：
   - 将 `lv_task_handler()` 替换为 `lv_timer_handler()`（`examples/light_control_app.c`）。
   - 将所有 `lv_obj_set_event_cb()` 调用替换为 `lv_obj_add_event_cb()`，并更新回调函数签名。
3. **增强错误处理**：修改 `page_manager_open()` 返回 `PageManagerError` 错误码，更新所有调用者。
4. **验证 OS 抽象层 API**（`lv_mutex_*`, `lv_thread_*`）与 v9.4 兼容。

### Phase 2: box86 项目重构
1. **标准化设备管理**：
   - 创建 `lvframe/device/lv_device_model.h` 统一设备模型定义。
   - 创建 `lvframe/device/lv_device_store.h/.c` 实现线程安全的设备数据存储（快照 API），替代原有的 `device_store`。
   - 更新 box86 所有文件使用新的 `lv_device_store_t` 和相关函数。
2. **实现 `app_bus_adapter`**：桥接 `AppBus` 消息到 `EventBus` 事件，扩展 `Event` 联合体支持 `EVENT_APP_MESSAGE`。
3. **重构 DevicePage 为 Page 对象**：
   - 创建 `examples/box86/pages/device_page.h/.c` 作为 Page 生命周期框架。
   - 更新 `light_page.c`、`cct_light_page.c`、`curtain_page.c` 支持 Page 生命周期。
   - 重构 `home_page.c` 管理 DevicePage Page 对象，实现完整的创建、事件订阅、销毁流程。
   - 确保事件传递和资源正确释放。

**架构改进**：
- 消除设备页面的组件/Page 架构不一致性，统一使用 lvframe Page 生命周期。
- 实现事件驱动的设备页面更新，每个设备页订阅 `EVENT_APP_MESSAGE` 并过滤设备 ID。
- 确保线程安全的数据访问（通过 `lv_device_store` 快照）。
- 编译成功并通过基本功能测试。

**验证结果**：
- 应用程序成功启动，平台初始化、事件总线、AppBus、业务线程正常工作。
- 设备页面正确创建（两个默认灯设备），生命周期回调被调用。
- 事件传递机制准备就绪（`app_bus_adapter` 集成）。

**下一步**：集成测试（添加/删除/刷新设备）和文档完善。

## 2026-04-11 — Phase 3: 测试、验证与文档

**目标**：完成 box86 项目的全面测试、验证 shell 输出缓冲问题修复，并完善测试文档。

### 测试与验证
1. **Shell 输出缓冲问题修复**：
   - 问题描述：点击 UI 按钮时，shell 输出（`[UI] set ...`）不立即显示，需等待用户按 Enter 键。
   - 根因分析：业务线程中 `stdout` 默认行缓冲，且 `fgets` 阻塞导致消息处理延迟。
   - 解决方案：
     - 在 `business_init()` 中添加 `setvbuf(stdout, NULL, _IONBF, 0)` 禁用输出缓冲。
     - 将 `fgets` 替换为非阻塞 I/O：使用 `select()` 检测 stdin 就绪，设置 10ms 超时。
     - 添加 `lv_sleep_ms(5)` 避免 CPU 空转。
     - 添加必要的头文件 (`sys/select.h`, `sys/time.h`, `lvgl/src/osal/lv_os.h`)。
   - 验证结果：通过 `test-ui` 模拟 UI 按钮点击，`[UI] set ...` 日志立即显示，问题已解决。

2. **集成测试**：
   - 创建 Python 测试脚本，自动执行全套 Shell 命令（`list`, `add`, `get`, `set`, `del`, `move`）。
   - 验证命令响应正确，输出立即显示。
   - 所有核心功能（设备管理、属性控制、页面生命周期）均工作正常。

3. **编译与运行验证**：
   - 使用 SDL dummy 驱动在无头环境中运行 box86，确保程序正常启动并进入主循环。
   - 业务线程成功启动，设备页面正确创建。
   - 事件传递机制正常工作（`app_bus_adapter` 桥接 AppBus ↔ EventBus）。

### 文档完善
1. **创建 `examples/box86/TESTING.md`**：
   - 详细说明环境要求、编译步骤、运行方法。
   - 提供完整的 Shell 命令参考（list, get, add, del, move, set）。
   - 包含测试用例和预期输出。

2. **更新 `PROJECT_STRUCTURE.md`** 和 `DEVLOG.md` 记录所有变更。

### 最终状态
- **Phase 1（框架增强）**：已完成，包括 event_bus 资源泄漏修复、LVGL v9.4 API 兼容性、page_manager 错误处理增强。
- **Phase 2（box86 重构）**：已完成，包括设备管理标准化、app_bus_adapter 实现、DevicePage 重构为 Page 对象。
- **Phase 3（测试验证）**：已完成，包括 shell 输出缓冲修复、集成测试、文档完善。

**项目现状**：
- lvframe 框架稳定，符合设计规范，API 与 LVGL v9.4 兼容。
- box86 应用程序架构统一，消除重复实现，代码健壮性显著提升。
- 线程安全的数据访问、事件驱动的页面更新、完整的生命周期管理均得到验证。
- 所有已知问题（资源泄漏、输出延迟）均已解决。

**下一步建议**：
- 进一步压力测试（长时间运行，大量设备操作）。
- 扩展更多设备类型（如传感器、空调）。
- 增加自动化单元测试（如使用 Unity 框架）。
