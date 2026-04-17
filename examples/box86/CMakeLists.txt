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

# ── LVGL ─────────────────────────────────────────────
add_subdirectory(${LVGL_DIR} ${CMAKE_BINARY_DIR}/lvgl_build)

# ── platform 库 ───────────────────────────────────────
add_library(platform STATIC)
target_include_directories(platform PUBLIC ${ROOT_DIR})
add_subdirectory(${ROOT_DIR}/platform/${PLATFORM} ${CMAKE_BINARY_DIR}/platform_build)

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

# 排除旧的 device_store.c，已迁移到 lvframe/device/
list(REMOVE_ITEM APP_SOURCES "${CMAKE_SOURCE_DIR}/models/device_store.c")
# 排除 old 目录下的所有源文件
list(FILTER APP_SOURCES EXCLUDE REGEX "/old/")
# 排除旧的设备页组件实现
list(REMOVE_ITEM APP_SOURCES "${CMAKE_SOURCE_DIR}/pages/device_page_component.c")

add_executable(box86 ${APP_SOURCES})
target_include_directories(box86 PRIVATE ${ROOT_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(box86 PRIVATE lvframe platform lvgl m pthread)
target_compile_options(box86 PRIVATE -Wall -Wno-unused-function)
