cmake_minimum_required(VERSION 3.16)
project(box86 C)

# 平台选择：sdl | rk3506 | rtos
if(NOT DEFINED PLATFORM)
    set(PLATFORM "sdl")
    message(STATUS "PLATFORM not set, defaulting to: sdl")
endif()
message(STATUS "Building box86 for platform: ${PLATFORM}")

# LVGL
add_subdirectory(${CMAKE_SOURCE_DIR}/lvgl lvgl_build)

# platform 库
add_library(platform STATIC)
target_include_directories(platform PUBLIC ${CMAKE_SOURCE_DIR})
add_subdirectory(${CMAKE_SOURCE_DIR}/platform/${PLATFORM} platform_build)

# lvframe 库
add_library(lvframe STATIC
    ${CMAKE_SOURCE_DIR}/lvframe/page.c
    ${CMAKE_SOURCE_DIR}/lvframe/page_manager.c
    ${CMAKE_SOURCE_DIR}/lvframe/event_bus.c
    ${CMAKE_SOURCE_DIR}/lvframe/swipe_container.c
    ${CMAKE_SOURCE_DIR}/lvframe/device_manager.c
    ${CMAKE_SOURCE_DIR}/lvframe/impl/tileview_impl.c
)
target_include_directories(lvframe PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(lvframe PUBLIC lvgl)

# box86 可执行文件
file(GLOB_RECURSE APP_SOURCES
    main.c
    pages/*.c
    models/*.c
)

add_executable(box86 ${APP_SOURCES})
target_include_directories(box86 PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(box86 PRIVATE lvframe platform lvgl)
