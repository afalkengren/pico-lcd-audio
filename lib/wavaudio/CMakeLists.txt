# Finds all source files in the current directory
aux_source_directory(. DIR_WAVAUDIO)

# Generate the link library
add_library(wavaudio ${DIR_WAVAUDIO})
target_link_libraries(wavaudio pico_stdlib)