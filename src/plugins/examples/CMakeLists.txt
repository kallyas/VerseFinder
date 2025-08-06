cmake_minimum_required(VERSION 3.16)
project(example_plugins)

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
.)

# Enhanced Search Plugin
add_library(enhanced_search_plugin SHARED
    enhanced_search_plugin.cpp
)

# Simple UI Plugin
add_library(simple_ui_plugin SHARED
    simple_ui_plugin.cpp
)

# PDF Export Plugin
add_library(pdf_export_plugin SHARED
    pdf_export_plugin.cpp
)

# Platform-specific naming function
function(set_plugin_properties target_name)
    if(WIN32)
        set_target_properties(${target_name} PROPERTIES
            OUTPUT_NAME "${target_name}"
            PREFIX ""
            SUFFIX ".dll"
        )
    elseif(APPLE)
        set_target_properties(${target_name} PROPERTIES
            OUTPUT_NAME "${target_name}"
            PREFIX "lib"
            SUFFIX ".dylib"
        )
    else()
        set_target_properties(${target_name} PROPERTIES
            OUTPUT_NAME "${target_name}"
            PREFIX "lib"
            SUFFIX ".so"
        )
    endif()
    
    # Copy to plugins directory
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${VERSEFINDER_ROOT}/plugins
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${target_name}> ${VERSEFINDER_ROOT}/plugins/
    )
endfunction()

# Apply properties to all plugins
set_plugin_properties(enhanced_search_plugin)
set_plugin_properties(simple_ui_plugin)
set_plugin_properties(pdf_export_plugin)