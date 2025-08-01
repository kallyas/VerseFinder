#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <GLFW/glfw3.h>
#include <vector>
#include <string>

struct MonitorInfo {
    GLFWmonitor* monitor;
    std::string name;
    int width;
    int height;
    int x;
    int y;
};

class WindowManager {
public:
    WindowManager();
    ~WindowManager();

    // Main window management
    bool initMainWindow(int width, int height, const char* title);
    void destroyMainWindow();
    GLFWwindow* getMainWindow() const { return main_window; }

    // Presentation window management
    bool initPresentationWindow(int monitor_index = -1);
    void destroyPresentationWindow();
    GLFWwindow* getPresentationWindow() const { return presentation_window; }
    bool hasPresentationWindow() const { return presentation_window != nullptr; }

    // Monitor management
    std::vector<MonitorInfo> getAvailableMonitors();
    void updatePresentationMonitorPosition(int monitor_index);

    // Window properties
    void setMainWindowSize(int width, int height);
    void getMainWindowSize(int& width, int& height);
    void setMainWindowPos(int x, int y);
    void getMainWindowPos(int& x, int& y);

    // Event handling
    bool shouldClose() const;
    void pollEvents();
    void swapBuffers();

private:
    GLFWwindow* main_window;
    GLFWwindow* presentation_window;
    int selected_monitor_index;

    static void errorCallback(int error, const char* description);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};

#endif // WINDOW_MANAGER_H