find_package(SDL2 REQUIRED)

target_sources(platform PRIVATE platform_sdl.c)
target_link_libraries(platform PUBLIC SDL2::SDL2)
target_include_directories(platform PUBLIC ${SDL2_INCLUDE_DIRS})
