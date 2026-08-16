# lvframe-starter

A lightweight LVGL application framework and smart home demo, targeting embedded Linux devices.

- **lvframe** — a reusable UI framework on top of [LVGL 9.4](https://lvgl.io): page lifecycle, navigation stack, and a thread-safe **message slot** (`signal + object`). It intentionally does **not** manage data models.
- **box86** — a full-featured smart home control panel app built with lvframe: multi-device swipe pages, Chinese text rendering, and a dual-thread slot-based architecture.

---

## Repository Layout

```
.
├── lvframe/                    # UI framework (platform-agnostic)
│   ├── slot.h / slot.c         #   message slot (signal + object dispatch)
│   ├── page.h / page.c         #   page lifecycle + slot binding
│   ├── page_manager.h / .c     #   navigation stack + LRU page cache
│   ├── swipe_container.h / .c  #   abstract swipe-page container
│   ├── impl/tileview_impl.c    #   lv_tileview backend
│   └── 设计文档.html            #   framework design & API reference
├── platform/
│   └── sdl/                    # SDL2 simulator driver (PC development)
├── lvgl/                       # LVGL v9.4 source (git submodule)
└── examples/
    └── box86/                  # smart home control panel app
        ├── main.c              # entry: init slots / store / thread / pages
        ├── msg.h               # app signal enum
        ├── slots.h / slots.c   # g_ui_slot / g_dev_slot instances
        ├── business.h / .c     # business thread (consumes g_dev_slot + shell)
        ├── models/             # app data models (model_base + model_store)
        └── pages/              # page implementations
```

---

## lvframe

A thin framework layer providing exactly two capabilities:

| Capability | Modules |
|------------|---------|
| **Message slot** | `slot.h` — any thread sends; a single consumer thread dispatches by `signal` + `object` (NULL = broadcast); payload is freed by the slot; 4-tuple unsubscribe |
| **Page lifecycle stack** (incl. swipe pages) | `page.h` + `page_manager.h` + `swipe_container.h` |

Key boundary: lvframe does **not** define, manage, or store any data model. The slot's `object` field is an opaque routing key (compared by pointer only). Data models, and the decision of *when* to add/remove pages, belong entirely to the application.

See [lvframe/设计文档.html](lvframe/设计文档.html) for the full design and API reference.

---

## box86 Example App

A 480×480 touchscreen smart home panel with:

- Left/right swipe between device pages (light, CCT light, motorized curtain)
- Pull-down gesture to open the settings page
- Chinese UI rendered via FreeType (Source Han Serif)
- Live device management through a terminal shell (add / delete / set / move)
- Rainbow border + flowing color animation on the light control page
- Two message slots: `g_ui_slot` (UI thread consumes) and `g_dev_slot` (business thread consumes)

### Quick Start (SDL simulator)

```bash
# 1. Install dependencies (Ubuntu / Debian)
sudo apt-get install build-essential cmake libsdl2-dev

# 2. Clone (include submodules)
git clone --recurse-submodules https://github.com/wujique/lvframe-starter.git
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

See [examples/box86/TESTING.md](examples/box86/TESTING.md) for the complete command reference, and [examples/box86/DESIGN.html](examples/box86/DESIGN.html) for the app design.

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

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| No display available (CI / headless server) | `SDL_VIDEODRIVER=dummy ./examples/box86/build/box86` |
| SDL crashes over remote X11 (e.g. MobaXterm SSH): `X connection ... broken` | Force software rendering: `export SDL_RENDER_DRIVER=software; export LIBGL_ALWAYS_SOFTWARE=1` |

---

## License

This project is released under the MIT License.  
LVGL is licensed under the MIT License — see [lvgl/LICENSE](lvgl/lvgl/LICENSE).
