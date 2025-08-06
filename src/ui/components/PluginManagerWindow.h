#ifndef PLUGIN_MANAGER_WINDOW_H
#define PLUGIN_MANAGER_WINDOW_H

#include "../plugins/manager/PluginManager.h"
#include "imgui.h"
#include <vector>
#include <string>

namespace PluginSystem {

class PluginManagerWindow {
private:
    PluginManager* plugin_manager;
    
    // UI state
    int selected_plugin_index = -1;
    char plugin_search_filter[256] = "";
    bool show_only_loaded = false;
    bool show_plugin_details = false;
    bool show_install_dialog = false;
    bool show_uninstall_dialog = false;
    
    // Plugin installation
    char install_file_path[512] = "";
    char install_plugin_name[256] = "";
    
    // Plugin details
    std::string selected_plugin_name;
    PluginInfo selected_plugin_info;
    std::vector<std::string> plugin_permissions;
    
    // Helper methods
    void renderPluginList();
    void renderPluginDetails();
    void renderInstallDialog();
    void renderUninstallDialog();
    void renderPermissionsEditor();
    void renderPluginControls();
    void renderPluginMetrics();
    void refreshPluginList();
    bool matchesFilter(const std::string& pluginName) const;
    
    std::vector<std::string> filtered_plugins;
    
public:
    explicit PluginManagerWindow(PluginManager* manager);
    ~PluginManagerWindow() = default;
    
    void render(bool* open);
    void setSelectedPlugin(const std::string& pluginName);
    void refresh();
};

} // namespace PluginSystem

#endif // PLUGIN_MANAGER_WINDOW_H