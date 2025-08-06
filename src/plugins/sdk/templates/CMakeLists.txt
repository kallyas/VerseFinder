cmake_minimum_required(VERSION 3.16)
project({{PLUGIN_NAME_LOWER}})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set VerseFinder root directory (adjust path as needed)
set(VERSEFINDER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# Include VerseFinder plugin headers
include_directories(
    ${VERSEFINDER_ROOT}/src/plugins/interfaces
    ${VERSEFINDER_ROOT}/src/plugins/api
    ${VERSEFINDER_ROOT}/src/core
    ${VERSEFINDER_ROOT}/src/plugins
)

# Create plugin library
add_library({{PLUGIN_NAME_LOWER}} SHARED
    {{PLUGIN_NAME_LOWER}}.cpp
    # Add additional source files here
)

# Platform-specific naming
if(WIN32)
    set_target_properties({{PLUGIN_NAME_LOWER}} PROPERTIES
        OUTPUT_NAME "{{PLUGIN_NAME_LOWER}}"
        PREFIX ""
        SUFFIX ".dll"
    )
elseif(APPLE)
    set_target_properties({{PLUGIN_NAME_LOWER}} PROPERTIES
        OUTPUT_NAME "{{PLUGIN_NAME_LOWER}}"
        PREFIX "lib"
        SUFFIX ".dylib"
    )
else()
    set_target_properties({{PLUGIN_NAME_LOWER}} PROPERTIES
        OUTPUT_NAME "{{PLUGIN_NAME_LOWER}}"
        PREFIX "lib"
        SUFFIX ".so"
    )
endif()

# Link additional libraries if needed
# target_link_libraries({{PLUGIN_NAME_LOWER}} your_library)

# Copy plugin to VerseFinder plugins directory (optional)
# add_custom_command(TARGET {{PLUGIN_NAME_LOWER}} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E make_directory ${VERSEFINDER_ROOT}/plugins
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:{{PLUGIN_NAME_LOWER}}> ${VERSEFINDER_ROOT}/plugins/
# )