cmake_minimum_required(VERSION 3.16)
project(lvgl_workspace C)

# 顶层 CMake 仅作为入口，实际构建从各工程目录发起
# 用法示例：
#   cd examples/box86
#   cmake -B build -DPLATFORM=sdl
#   cmake --build build

message(STATUS "请进入 examples/<工程名> 目录执行 cmake 构建")
message(STATUS "示例：cmake -B build -DPLATFORM=sdl")
