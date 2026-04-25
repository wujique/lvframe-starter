cmake_minimum_required(VERSION 3.16)
project(box86 C)

set(CMAKE_C_STANDARD 99)

# ── 平台选择 ──────────────────────────────────────────
if(NOT DEFINED PLATFORM)
    set(PLATFORM "sdl")
    message(STATUS "PLATFORM not set, defaulting to: sdl")
endif()
message(STATUS "Building box86 for platform: ${PLATFORM}")

# ── 根目录（绝对路径，避免子目录相对路径解析错误） ────
get_filename_component(ROOT_DIR "${CMAKE_SOURCE_DIR}/../.." ABSOLUTE)
set(LVGL_DIR "${ROOT_DIR}/lvgl/lvgl")

message(STATUS "ROOT_DIR = ${ROOT_DIR}")

# ── lv_conf.h：让 LVGL 找到本工程的配置文件 ──────────
include_directories(${CMAKE_SOURCE_DIR})
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE)

# 全局 include：让所有源文件都能找到 lvgl/lvgl.h
include_directories(${ROOT_DIR}/lvgl)
include_directories(${ROOT_DIR})

# ── assets 路径 ────────────────────────────────────────
# SDL 模拟器：直接引用工程源码树下的 assets/
# RK3506 部署：assets 安装到 /usr/share/box86/assets
if(PLATFORM STREQUAL "sdl")
    set(BOX86_ASSETS_PATH "${CMAKE_SOURCE_DIR}/assets")
else()
    set(BOX86_ASSETS_PATH "/usr/share/box86/assets")
endif()
message(STATUS "BOX86_ASSETS_PATH = ${BOX86_ASSETS_PATH}")
add_compile_definitions(BOX86_ASSETS_PATH="${BOX86_ASSETS_PATH}")

# ── LVGL ─────────────────────────────────────────────
if(PLATFORM STREQUAL "rk3506")
    # RK3506：将 LVGL 编译为共享库，安装路径 /usr/lib
    set(BUILD_SHARED_LIBS ON)
    add_subdirectory(${LVGL_DIR} ${CMAKE_BINARY_DIR}/lvgl_build)
    set_target_properties(lvgl PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
    install(TARGETS lvgl LIBRARY DESTINATION /usr/lib)
else()
    # SDL 模拟器：静态链接 LVGL
    add_subdirectory(${LVGL_DIR} ${CMAKE_BINARY_DIR}/lvgl_build)
endif()

# ── platform 库 ───────────────────────────────────────
# sdl 平台：使用 str/platform/sdl（模拟器，通用）
# 其他平台（如 rk3506）：使用应用自有的 platform/<target>/
add_library(platform STATIC)
target_include_directories(platform PUBLIC ${ROOT_DIR})
if(PLATFORM STREQUAL "sdl")
    add_subdirectory(${ROOT_DIR}/platform/sdl ${CMAKE_BINARY_DIR}/platform_build)
else()
    set(APP_PLATFORM_DIR "${CMAKE_SOURCE_DIR}/platform/${PLATFORM}")
    if(NOT EXISTS "${APP_PLATFORM_DIR}")
        message(FATAL_ERROR "Platform '${PLATFORM}' not found in ${APP_PLATFORM_DIR}")
    endif()
    add_subdirectory(${APP_PLATFORM_DIR} ${CMAKE_BINARY_DIR}/platform_build)
endif()

# ── 字体引擎：两个平台都用 FreeType（支持 CFF/OTF 格式） ──
add_compile_definitions(BOX86_USE_FREETYPE)
if(PLATFORM STREQUAL "rk3506")
    # rk3506 目标机预装 freetype，使用系统默认路径
    find_library(FREETYPE_LIB freetype HINTS /usr/lib /usr/local/lib)
    if(NOT FREETYPE_LIB)
        message(WARNING "freetype library not found; set FREETYPE_LIB manually for cross-compilation")
        set(FREETYPE_LIB freetype)
    endif()
    message(STATUS "Font engine: FreeType (${FREETYPE_LIB})")
else()
    # SDL 模拟器：使用工程内置的 FreeType 头文件 + 系统已有的 libfreetype.so
    set(FT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/freetype2")
    include_directories(${FT_INCLUDE_DIR})
    find_library(FREETYPE_LIB freetype
        HINTS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib)
    if(NOT FREETYPE_LIB)
        set(FREETYPE_LIB /usr/lib/x86_64-linux-gnu/libfreetype.so.6)
    endif()
    message(STATUS "Font engine: FreeType (${FREETYPE_LIB}), headers: ${FT_INCLUDE_DIR}")
endif()
set(FONT_ENGINE_LIBS ${FREETYPE_LIB})

# LVGL FreeType 集成源文件（LV_USE_FREETYPE=1 时需手动加入编译）
set(LVGL_FREETYPE_SRCS
    ${LVGL_DIR}/src/libs/freetype/lv_freetype.c
    ${LVGL_DIR}/src/libs/freetype/lv_freetype_glyph.c
    ${LVGL_DIR}/src/libs/freetype/lv_freetype_image.c
    ${LVGL_DIR}/src/libs/freetype/lv_freetype_outline.c
    ${LVGL_DIR}/src/libs/freetype/lv_ftsystem.c
)

# ── lvframe 库 ────────────────────────────────────────
add_library(lvframe STATIC
    ${ROOT_DIR}/lvframe/page.c
    ${ROOT_DIR}/lvframe/page_manager.c
    ${ROOT_DIR}/lvframe/event_bus.c
    ${ROOT_DIR}/lvframe/swipe_container.c
    ${ROOT_DIR}/lvframe/impl/tileview_impl.c
    ${ROOT_DIR}/lvframe/device/lv_device_store.c
)
target_include_directories(lvframe PUBLIC ${ROOT_DIR})
target_link_libraries(lvframe PUBLIC lvgl)

# ── box86 可执行文件 ──────────────────────────────────
file(GLOB_RECURSE APP_SOURCES
    ${CMAKE_SOURCE_DIR}/main.c
    ${CMAKE_SOURCE_DIR}/app_bus.c
    ${CMAKE_SOURCE_DIR}/app_bus_adapter.c
    ${CMAKE_SOURCE_DIR}/business.c
    ${CMAKE_SOURCE_DIR}/pages/*.c
    ${CMAKE_SOURCE_DIR}/models/*.c
)

# 排除 old 目录下的所有源文件
list(FILTER APP_SOURCES EXCLUDE REGEX "/old/")
# 排除旧的设备页组件实现
list(REMOVE_ITEM APP_SOURCES "${CMAKE_SOURCE_DIR}/pages/device_page_component.c")

add_executable(box86 ${APP_SOURCES} ${LVGL_FREETYPE_SRCS}
    ${CMAKE_SOURCE_DIR}/font.c
    ${CMAKE_SOURCE_DIR}/screensaver.c
)
target_include_directories(box86 PRIVATE ${ROOT_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(box86 PRIVATE lvframe platform lvgl ${FONT_ENGINE_LIBS} m pthread)
target_compile_options(box86 PRIVATE -Wall -Wno-unused-function)

# ── rk3506 安装规则 ───────────────────────────────────
if(PLATFORM STREQUAL "rk3506")
    install(TARGETS box86 RUNTIME DESTINATION /usr/bin)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/
            DESTINATION /usr/share/box86/assets)
endif()
