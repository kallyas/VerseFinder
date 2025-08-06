cmake_minimum_required(VERSION 3.16)
project(enhanced_search_plugin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set VerseFinder root directory
set(VERSEFINDER_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Include VerseFinder plugin headers
include_directories(
    ${VERSEFINDER_ROOT}/src/plugins/interfaces
    ${VERSEFINDER_ROOT}/src/plugins/api
    ${VERSEFINDER_ROOT}/src/core
    ${VERSEFINDER_ROOT}/src/plugins
)

# Create plugin library
add_library(enhanced_search_plugin SHARED
    enhanced_search_plugin.cpp
)

# Platform-specific naming
if(WIN32)
    set_target_properties(enhanced_search_plugin PROPERTIES
        OUTPUT_NAME "enhanced_search_plugin"
        PREFIX ""
        SUFFIX ".dll"
    )
elseif(APPLE)
    set_target_properties(enhanced_search_plugin PROPERTIES
        OUTPUT_NAME "enhanced_search_plugin"
        PREFIX "lib"
        SUFFIX ".dylib"
    )
else()
    set_target_properties(enhanced_search_plugin PROPERTIES
        OUTPUT_NAME "enhanced_search_plugin"
        PREFIX "lib"
        SUFFIX ".so"
    )
endif()

# Copy to plugins directory
add_custom_command(TARGET enhanced_search_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${VERSEFINDER_ROOT}/plugins
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:enhanced_search_plugin> ${VERSEFINDER_ROOT}/plugins/
)