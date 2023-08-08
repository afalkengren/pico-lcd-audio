# Finds all source files in the current directory
aux_source_directory(. DIR_MCP4922)

# Generate the link library
add_library(mcp4922 ${DIR_MCP4922})
target_link_libraries(mcp4922 hardware_spi hardware_gpio)