cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(pico_lcd_audio)
pico_sdk_init()

# Add a compilation subdirectory
add_subdirectory(lib/lcd1n14)
add_subdirectory(lib/mcp4922)
add_subdirectory(lib/wavaudio)

# Add a header directory
include_directories(./lib/lcd1n14)
include_directories(./lib/mcp4922)
include_directories(./lib/wavaudio)

# create an executable file
add_executable(main src/main.c src/channel.c)

# enable usb output, disable uart output
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(main)

target_link_libraries(main pico_stdlib lcd1n14 mcp4922 wavaudio hardware_spi hardware_timer pico_multicore)