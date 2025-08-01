#include "WindowManager.h"
#include <iostream>
#include <algorithm>

WindowManager::WindowManager() 
    : main_window(nullptr)
    , presentation_window(nullptr)
    , selected_monitor_index(-1) {
}

WindowManager::~WindowManager() {
    destroyPresentationWindow();
    destroyMainWindow();
    glfwTerminate();
}

void WindowManager::errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void WindowManager::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

bool WindowManager::initMainWindow(int width, int height, const char* title) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwSetErrorCallback(errorCallback);

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    main_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!main_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(main_window);
    glfwSetFramebufferSizeCallback(main_window, framebufferSizeCallback);
    glfwSwapInterval(1); // Enable vsync

    return true;
}

void WindowManager::destroyMainWindow() {
    if (main_window) {
        glfwDestroyWindow(main_window);
        main_window = nullptr;
    }
}

bool WindowManager::initPresentationWindow(int monitor_index) {
    if (presentation_window) {
        destroyPresentationWindow();
    }

    auto monitors = getAvailableMonitors();
    if (monitors.empty()) {
        std::cerr << "No monitors available for presentation mode" << std::endl;
        return false;
    }

    // Use specified monitor or default to first available
    if (monitor_index < 0 || monitor_index >= static_cast<int>(monitors.size())) {
        monitor_index = 0;
    }
    
    selected_monitor_index = monitor_index;
    const auto& monitor = monitors[monitor_index];

    // Create fullscreen window on selected monitor
    presentation_window = glfwCreateWindow(
        monitor.width, 
        monitor.height, 
        "VerseFinder Presentation", 
        monitor.monitor, 
        main_window
    );

    if (!presentation_window) {
        std::cerr << "Failed to create presentation window" << std::endl;
        return false;
    }

    return true;
}

void WindowManager::destroyPresentationWindow() {
    if (presentation_window) {
        glfwDestroyWindow(presentation_window);
        presentation_window = nullptr;
    }
}

std::vector<MonitorInfo> WindowManager::getAvailableMonitors() {
    std::vector<MonitorInfo> monitors;
    
    int monitor_count;
    GLFWmonitor** glfw_monitors = glfwGetMonitors(&monitor_count);
    
    for (int i = 0; i < monitor_count; ++i) {
        MonitorInfo info;
        info.monitor = glfw_monitors[i];
        info.name = glfwGetMonitorName(glfw_monitors[i]);
        
        const GLFWvidmode* mode = glfwGetVideoMode(glfw_monitors[i]);
        info.width = mode->width;
        info.height = mode->height;
        
        glfwGetMonitorPos(glfw_monitors[i], &info.x, &info.y);
        
        monitors.push_back(info);
    }
    
    return monitors;
}

void WindowManager::updatePresentationMonitorPosition(int monitor_index) {
    if (presentation_window && monitor_index >= 0) {
        auto monitors = getAvailableMonitors();
        if (monitor_index < static_cast<int>(monitors.size())) {
            const auto& monitor = monitors[monitor_index];
            glfwSetWindowMonitor(
                presentation_window,
                monitor.monitor,
                0, 0,
                monitor.width,
                monitor.height,
                GLFW_DONT_CARE
            );
            selected_monitor_index = monitor_index;
        }
    }
}

void WindowManager::setMainWindowSize(int width, int height) {
    if (main_window) {
        glfwSetWindowSize(main_window, width, height);
    }
}

void WindowManager::getMainWindowSize(int& width, int& height) {
    if (main_window) {
        glfwGetWindowSize(main_window, &width, &height);
    } else {
        width = height = 0;
    }
}

void WindowManager::setMainWindowPos(int x, int y) {
    if (main_window) {
        glfwSetWindowPos(main_window, x, y);
    }
}

void WindowManager::getMainWindowPos(int& x, int& y) {
    if (main_window) {
        glfwGetWindowPos(main_window, &x, &y);
    } else {
        x = y = 0;
    }
}

bool WindowManager::shouldClose() const {
    return main_window && glfwWindowShouldClose(main_window);
}

void WindowManager::pollEvents() {
    glfwPollEvents();
}

void WindowManager::swapBuffers() {
    if (main_window) {
        glfwSwapBuffers(main_window);
    }
    if (presentation_window) {
        glfwMakeContextCurrent(presentation_window);
        glfwSwapBuffers(presentation_window);
        glfwMakeContextCurrent(main_window);
    }
}