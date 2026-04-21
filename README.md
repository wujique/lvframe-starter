# lvframe-starter

A lightweight LVGL application framework and smart home demo, targeting embedded Linux devices.

- **lvframe** — a reusable UI framework built on top of [LVGL 9.4](https://lvgl.io): page lifecycle management, navigation stack, thread-safe event bus, and a generic device store
- **box86** — a full-featured smart home control panel app built with lvframe, featuring multi-device swipe pages, Chinese text rendering, and a dual-thread architecture

---

## Repository Layout

```
.
├── lvframe/          # UI framework (platform-agnostic)
├── platform/
│   └── sdl/          # SDL2 simulator driver (PC development)
├── lvgl/             # LVGL v9.4 source (git submodule)
├── examples/
│   └── box86/        # Smart home control panel app
└── PROJECT_STRUCTURE.md
```

See [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) for a detailed breakdown of every directory.

---

## lvframe

A thin framework layer on top of LVGL providing:

| Module | Description |
|--------|-------------|
| `page.h` | Page struct with lifecycle callbacks (`on_create / on_destroy / on_event`) |
| `page_manager.h` | Navigation stack + LRU page cache |
| `event_bus.h` | Thread-safe publish/subscribe, driven by an LVGL timer |
| `swipe_container.h` | Abstract swipe-page container (backed by `lv_tileview`) |
| `device/lv_device_store.h` | Generic mutex-protected device store; concrete models defined by the app |

See [lvframe/README.md](lvframe/README.md) for the full API reference.

---

## box86 Example App

A 480×480 touchscreen smart home panel with:

- Left/right swipe between device pages (light, CCT light, motorized curtain)
- Pull-down gesture to open the settings page
- Chinese UI rendered via FreeType (Source Han Serif)
- Live device management through a terminal shell (add / delete / set / move)
- Rainbow border + flowing color animation on the light control page

### Quick Start (SDL simulator)

```bash
# 1. Install dependencies (Ubuntu / Debian)
sudo apt-get install build-essential cmake libsdl2-dev

# 2. Clone (include submodules)
git clone --recurse-submodules https://github.com/<you>/lvframe-starter.git
cd lvframe-starter

# 3. Build
cmake -DPLATFORM=sdl -S examples/box86 -B examples/box86/build
cmake --build examples/box86/build -j$(nproc)

# 4. Run
./examples/box86/build/box86
```

Once running, type shell commands in the same terminal to control the UI:

```
add light    "Living Room"   # add a light device
add cct_light "Bedroom"      # add a CCT light
add curtain  "Window"        # add a motorized curtain
list                         # list all devices
set 1 onoffsta 1             # turn device 1 on
set 2 color_temp 3000        # set CCT to 3000 K
set 3 command OPEN           # open the curtain
del 2                        # remove device 2
```

See [examples/box86/TESTING.md](examples/box86/TESTING.md) for the complete command reference.

### Cross-compile for RK3506 Linux

```bash
cmake -DPLATFORM=rk3506 \
      -DCMAKE_TOOLCHAIN_FILE=rk3506.cmake \
      -S examples/box86 -B examples/box86/build_rk
cmake --build examples/box86/build_rk -j$(nproc)
cmake --install examples/box86/build_rk   # installs to /usr/bin & /usr/share/box86
```

---

## Platform Support

| Platform | Driver location | How to select |
|----------|----------------|---------------|
| SDL2 (PC simulator) | `platform/sdl/` | `-DPLATFORM=sdl` |
| RK3506 Linux | `examples/box86/platform/rk3506/` | `-DPLATFORM=rk3506` |

Other target platforms can be added under `examples/<app>/platform/<target>/` without touching the framework.

---

## Requirements

| Dependency | Version | Notes |
|------------|---------|-------|
| CMake | ≥ 3.16 | |
| GCC / Clang | C99 | |
| LVGL | 9.4 | included as submodule |
| SDL2 | any recent | SDL simulator only |
| FreeType | 2.x | headers bundled in `third_party/freetype2/` |
| pthreads | POSIX | dual-thread architecture |

---

## License

This project is released under the MIT License.  
LVGL is licensed under the MIT License — see [lvgl/LICENSE](lvgl/lvgl/LICENSE).
