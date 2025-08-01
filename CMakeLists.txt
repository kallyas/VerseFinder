cmake_minimum_required(VERSION 3.16)
project(VerseFinder VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent for downloading dependencies
include(FetchContent)

# Find OpenGL (should be available on all platforms)
find_package(OpenGL REQUIRED)

# Try to find system packages first, fall back to FetchContent if not found
find_package(PkgConfig QUIET)

# GLFW - Try system package first
if(PkgConfig_FOUND)
    pkg_check_modules(GLFW3 QUIET glfw3)
endif()

if(NOT GLFW3_FOUND)
    message(STATUS "GLFW not found via pkg-config, downloading...")
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(glfw)
    
    set(GLFW3_LIBRARIES glfw)
    set(GLFW3_INCLUDE_DIRS ${glfw_SOURCE_DIR}/include)
    set(GLFW3_FOUND TRUE)
    message(STATUS "GLFW downloaded and configured")
endif()

# nlohmann/json - Try system package first
find_package(nlohmann_json 3.12.0 QUIET)

if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann/json not found, downloading...")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    set(JSON_Install OFF CACHE INTERNAL "")
    
    FetchContent_MakeAvailable(nlohmann_json)
    message(STATUS "nlohmann/json downloaded and configured")
endif()

# Dear ImGui setup
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)

# Check if ImGui exists, if not download it
if(NOT EXISTS ${IMGUI_DIR})
    message(STATUS "Dear ImGui not found, downloading...")
    
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.4
        SOURCE_DIR ${IMGUI_DIR}
    )
    
    FetchContent_MakeAvailable(imgui)
    message(STATUS "Dear ImGui downloaded to ${IMGUI_DIR}")
endif()

# ImGui sources
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Configure OpenGL loader based on platform
if(APPLE)
    # macOS uses system OpenGL
    set(OPENGL_LOADER_FOUND TRUE)
    set(OPENGL_LOADER_LIBRARIES "")
    set(OPENGL_LOADER_INCLUDE_DIRS "")
    set(OPENGL_LOADER_LINK_DIRS "")
    set(OPENGL_LOADER_DEFINITIONS "IMGUI_IMPL_OPENGL_LOADER_CUSTOM;GL_SILENCE_DEPRECATION")
    message(STATUS "Using macOS system OpenGL (no external loader needed)")
    
elseif(WIN32)
    # Windows - try to find GLEW, download if not found
    find_package(GLEW QUIET)
    
    if(NOT GLEW_FOUND AND PkgConfig_FOUND)
        pkg_check_modules(GLEW QUIET glew)
    endif()
    
    if(NOT GLEW_FOUND)
        message(STATUS "GLEW not found, downloading...")
        FetchContent_Declare(
            glew
            GIT_REPOSITORY https://github.com/nigels-com/glew.git
            GIT_TAG glew-2.2.0
            SOURCE_SUBDIR build/cmake
        )
        
        set(BUILD_UTILS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glew)
        
        set(OPENGL_LOADER_LIBRARIES GLEW::GLEW)
        set(OPENGL_LOADER_DEFINITIONS IMGUI_IMPL_OPENGL_LOADER_GLEW)
        message(STATUS "GLEW downloaded and configured")
    else()
        set(OPENGL_LOADER_LIBRARIES ${GLEW_LIBRARIES})
        set(OPENGL_LOADER_INCLUDE_DIRS ${GLEW_INCLUDE_DIRS})
        set(OPENGL_LOADER_LINK_DIRS ${GLEW_LIBRARY_DIRS})
        set(OPENGL_LOADER_DEFINITIONS IMGUI_IMPL_OPENGL_LOADER_GLEW)
        message(STATUS "Found system GLEW")
    endif()
    
    set(OPENGL_LOADER_FOUND TRUE)
    
else()
    # Linux and other Unix-like systems
    if(PkgConfig_FOUND)
        pkg_check_modules(GLEW QUIET glew)
    endif()
    
    if(NOT GLEW_FOUND)
        find_package(GLEW QUIET)
    endif()
    
    if(NOT GLEW_FOUND)
        message(STATUS "GLEW not found, downloading...")
        FetchContent_Declare(
            glew
            GIT_REPOSITORY https://github.com/nigels-com/glew.git
            GIT_TAG glew-2.2.0
            SOURCE_SUBDIR build/cmake
        )
        
        set(BUILD_UTILS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glew)
        
        set(OPENGL_LOADER_LIBRARIES GLEW::GLEW)
        set(OPENGL_LOADER_DEFINITIONS IMGUI_IMPL_OPENGL_LOADER_GLEW)
        message(STATUS "GLEW downloaded and configured")
    else()
        set(OPENGL_LOADER_LIBRARIES ${GLEW_LIBRARIES})
        set(OPENGL_LOADER_INCLUDE_DIRS ${GLEW_INCLUDE_DIRS})
        set(OPENGL_LOADER_LINK_DIRS ${GLEW_LIBRARY_DIRS})
        set(OPENGL_LOADER_DEFINITIONS IMGUI_IMPL_OPENGL_LOADER_GLEW)
        message(STATUS "Found system GLEW")
    endif()
    
    set(OPENGL_LOADER_FOUND TRUE)
endif()

# Include directories
include_directories(
    src/core
    src/ui
    src/integrations
    src/service
    src/api
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${OPENGL_LOADER_INCLUDE_DIRS}
    ${GLFW3_INCLUDE_DIRS}
)

# Add library directories if needed
if(OPENGL_LOADER_LINK_DIRS)
    link_directories(${OPENGL_LOADER_LINK_DIRS})
endif()

# Add executable
add_executable(VerseFinder
    src/main.cpp
    src/core/VerseFinder.cpp
    src/core/SearchCache.cpp
    src/core/SearchOptimizer.cpp
    src/core/PerformanceBenchmark.cpp
    src/core/FuzzySearch.cpp
    src/core/UserSettings.cpp
    src/core/AutoComplete.cpp
    src/core/MemoryMonitor.cpp
    src/core/IncrementalSearch.cpp
    src/core/SemanticSearch.cpp
    src/core/CrossReferenceSystem.cpp
    src/core/SearchAnalytics.cpp
    src/core/TopicManager.cpp
    src/ui/VerseFinderApp.cpp
    src/ui/components/SearchComponent.cpp
    src/ui/components/TranslationSelector.cpp
    src/ui/settings/ThemeManager.cpp
    src/ui/system/FontManager.cpp
    src/ui/system/WindowManager.cpp
    src/ui/system/PlatformUtils.cpp
    src/ui/modals/SettingsModal.cpp
    src/integrations/IntegrationManager.cpp
    src/integrations/PlanningCenterProvider.cpp
    src/service/ServicePlan.cpp
    src/api/ApiServer.cpp
    ${IMGUI_SOURCES}
)

# Compiler definitions for OpenGL loader
target_compile_definitions(VerseFinder PRIVATE 
    ${OPENGL_LOADER_DEFINITIONS}
    GENTIUM_FONT_PATH="${GENTIUM_FONT_PATH}"
    FONT_BASE_PATH="${FONT_BASE_PATH}"
)

# On macOS with custom loader, force include our OpenGL header
if(APPLE AND OPENGL_LOADER_DEFINITIONS MATCHES "IMGUI_IMPL_OPENGL_LOADER_CUSTOM")
    target_compile_options(VerseFinder PRIVATE "-include${CMAKE_CURRENT_SOURCE_DIR}/src/opengl_loader.h")
endif()

# Link standard libraries
target_link_libraries(VerseFinder
    ${GLFW3_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

# Link OpenGL loader libraries
if(OPENGL_LOADER_LIBRARIES)
    target_link_libraries(VerseFinder ${OPENGL_LOADER_LIBRARIES})
endif()

# Link JSON library
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(VerseFinder nlohmann_json::nlohmann_json)
elseif(TARGET nlohmann_json)
    target_link_libraries(VerseFinder nlohmann_json)
endif()

# Platform-specific linking
if(APPLE)
    target_link_libraries(VerseFinder
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(WIN32)
    # Windows-specific libraries
    target_link_libraries(VerseFinder
        opengl32
        gdi32
        shell32
        user32
        kernel32
    )
elseif(UNIX AND NOT APPLE)
    # Linux-specific libraries
    find_package(Threads REQUIRED)
    target_link_libraries(VerseFinder
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )
    
    # Check for X11 libraries (commonly needed on Linux)
    find_package(X11 QUIET)
    if(X11_FOUND)
        target_link_libraries(VerseFinder ${X11_LIBRARIES})
    endif()
endif()

# Add link directories for pkg-config libraries
if(GLFW3_LIBRARY_DIRS)
    target_link_directories(VerseFinder PRIVATE ${GLFW3_LIBRARY_DIRS})
endif()
if(OPENGL_LOADER_LINK_DIRS)
    target_link_directories(VerseFinder PRIVATE ${OPENGL_LOADER_LINK_DIRS})
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(VerseFinder PRIVATE -Wall -Wextra)
endif()

# Copy assets and data to the build directory
file(COPY data/ DESTINATION ${CMAKE_BINARY_DIR})
file(COPY assets/ DESTINATION ${CMAKE_BINARY_DIR})
file(COPY fonts/ DESTINATION ${CMAKE_BINARY_DIR}/fonts)

# Define font paths for compile-time configuration
set(FONT_BASE_PATH "${CMAKE_BINARY_DIR}/fonts")
set(GENTIUM_FONT_PATH "${FONT_BASE_PATH}/Gentium_Plus/GentiumPlus-Regular.ttf")

# Create translations directory structure
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/translations)

# Copy bible.json if it exists (for backward compatibility)
if(EXISTS ${CMAKE_SOURCE_DIR}/bible.json)
    file(COPY bible.json DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Add performance test executable
add_executable(performance_test
    performance_test.cpp
    src/core/VerseFinder.cpp
    src/core/SearchCache.cpp
    src/core/SearchOptimizer.cpp
    src/core/PerformanceBenchmark.cpp
    src/core/FuzzySearch.cpp
    src/core/UserSettings.cpp
    src/core/AutoComplete.cpp
    src/core/MemoryMonitor.cpp
    src/core/IncrementalSearch.cpp
    src/core/SemanticSearch.cpp
    src/core/CrossReferenceSystem.cpp
    src/core/SearchAnalytics.cpp
    src/core/TopicManager.cpp
)

# Link libraries for performance test
target_link_libraries(performance_test
    ${GLFW3_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(performance_test nlohmann_json::nlohmann_json)
elseif(TARGET nlohmann_json)
    target_link_libraries(performance_test nlohmann_json)
endif()

# Platform-specific linking for performance test
if(APPLE)
    target_link_libraries(performance_test
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(WIN32)
    target_link_libraries(performance_test
        opengl32
        gdi32
        shell32
        user32
        kernel32
    )
elseif(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(performance_test
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )
    
    if(X11_FOUND)
        target_link_libraries(performance_test ${X11_LIBRARIES})
    endif()
endif()
