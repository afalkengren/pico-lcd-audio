cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(Pico_ePaper_Code)
pico_sdk_init()

# Add a compilation subdirectory
add_subdirectory(lib/Config)
add_subdirectory(lib/LCD)

# Add a header directory
include_directories(./lib/Config)
include_directories(./lib/LCD)

# create an executable file
add_executable(main main.c)

# enable usb output, disable uart output
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(main)

target_link_libraries(main pico_stdlib LCD Config hardware_spi hardware_timer pico_multicore)