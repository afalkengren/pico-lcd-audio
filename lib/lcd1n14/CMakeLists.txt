# Finds all source files in the current directory
aux_source_directory(. DIR_LCD_SRCS)

# Generate the link library
add_library(lcd1n14 ${DIR_LCD_SRCS})
target_link_libraries(lcd1n14 hardware_spi hardware_pwm)