cmake_minimum_required(VERSION 3.16)
project(music C)

set(CMAKE_C_STANDARD 99)

# -------------------------------------------------------
# 路径（转为绝对路径，避免相对路径解析问题）
# -------------------------------------------------------
get_filename_component(ROOT_DIR "${CMAKE_SOURCE_DIR}/../.." ABSOLUTE)
set(LVGL_DIR  "${ROOT_DIR}/lvgl/lvgl")

message(STATUS "ROOT_DIR = ${ROOT_DIR}")
message(STATUS "LVGL_DIR = ${LVGL_DIR}")

# -------------------------------------------------------
# lv_conf.h：让 LVGL 找到本工程的配置文件
# -------------------------------------------------------
include_directories(${CMAKE_SOURCE_DIR})
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE)

# 全局 include：让所有源文件（含 assets/*.c）都能找到 lvgl/lvgl.h
# ROOT_DIR/lvgl/ 下有 lvgl/ 子目录，所以 #include "lvgl/lvgl.h" 可以找到
include_directories(${ROOT_DIR}/lvgl)

# -------------------------------------------------------
# LVGL 库
# -------------------------------------------------------
add_subdirectory(${LVGL_DIR} ${CMAKE_BINARY_DIR}/lvgl)

# -------------------------------------------------------
# music demo 源文件
# -------------------------------------------------------
file(GLOB DEMO_SOURCES
    ${CMAKE_SOURCE_DIR}/lv_demo_music.c
    ${CMAKE_SOURCE_DIR}/lv_demo_music_list.c
    ${CMAKE_SOURCE_DIR}/lv_demo_music_main.c
    ${CMAKE_SOURCE_DIR}/assets/*.c
)

# -------------------------------------------------------
# 可执行文件
# -------------------------------------------------------
add_executable(music
    main.c
    ${DEMO_SOURCES}
)

target_include_directories(music PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${LVGL_DIR}
    ${ROOT_DIR}/lvgl
)

# -------------------------------------------------------
# 链接 LVGL 和 SDL2
# -------------------------------------------------------
find_package(SDL2 REQUIRED)

target_link_libraries(music PRIVATE
    lvgl
    SDL2::SDL2
    m
    pthread
)

# -------------------------------------------------------
# 编译选项
# -------------------------------------------------------
target_compile_options(music PRIVATE -Wall -Wno-unused-function)
