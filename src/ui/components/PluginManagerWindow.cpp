#include "PluginManagerWindow.h"
#include "imgui.h"
#include <algorithm>
#include <filesystem>

namespace PluginSystem {

PluginManagerWindow::PluginManagerWindow(PluginManager* manager) 
    : plugin_manager(manager) {
    refreshPluginList();
}

void PluginManagerWindow::render(bool* open) {
    if (!open || !*open) return;
    
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Plugin Manager", open)) {
        // Top toolbar
        if (ImGui::Button("Refresh")) {
            refresh();
        }
        ImGui::SameLine();
        if (ImGui::Button("Install Plugin")) {
            show_install_dialog = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Scan for Plugins")) {
            plugin_manager->scanForPlugins();
            refreshPluginList();
        }
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        ImGui::Checkbox("Show only loaded", &show_only_loaded);
        
        // Search filter
        ImGui::Separator();
        ImGui::Text("Filter:");
        ImGui::SameLine();
        if (ImGui::InputText("##filter", plugin_search_filter, sizeof(plugin_search_filter))) {
            refreshPluginList();
        }
        
        ImGui::Separator();
        
        // Main content area
        ImGui::Columns(2, "PluginManagerColumns");
        ImGui::SetColumnWidth(0, 300);
        
        // Left panel - Plugin list
        ImGui::Text("Plugins (%zu)", filtered_plugins.size());
        ImGui::Separator();
        
        renderPluginList();
        
        ImGui::NextColumn();
        
        // Right panel - Plugin details
        ImGui::Text("Plugin Details");
        ImGui::Separator();
        
        if (selected_plugin_index >= 0 && selected_plugin_index < filtered_plugins.size()) {
            renderPluginDetails();
        } else {
            ImGui::Text("No plugin selected");
        }
        
        ImGui::Columns(1);
    }
    ImGui::End();
    
    // Dialogs
    if (show_install_dialog) {
        renderInstallDialog();
    }
    
    if (show_uninstall_dialog) {
        renderUninstallDialog();
    }
}

void PluginManagerWindow::renderPluginList() {
    ImGui::BeginChild("PluginList", ImVec2(0, 0), true);
    
    for (size_t i = 0; i < filtered_plugins.size(); ++i) {
        const std::string& pluginName = filtered_plugins[i];
        PluginState state = plugin_manager->getPluginState(pluginName);
        
        // Plugin status icon
        const char* statusIcon = "○";
        ImVec4 statusColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        
        switch (state) {
            case PluginState::ACTIVE:
                statusIcon = "●";
                statusColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                break;
            case PluginState::LOADED:
                statusIcon = "◐";
                statusColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                break;
            case PluginState::ERROR:
                statusIcon = "✗";
                statusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                break;
            case PluginState::LOADING:
            case PluginState::UNLOADING:
                statusIcon = "◒";
                statusColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
                break;
            default:
                break;
        }
        
        ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
        ImGui::Text("%s", statusIcon);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        
        if (ImGui::Selectable(pluginName.c_str(), selected_plugin_index == i)) {
            selected_plugin_index = i;
            selected_plugin_name = pluginName;
            selected_plugin_info = plugin_manager->getPluginInfo(pluginName);
            plugin_permissions = plugin_manager->getPluginPermissions(pluginName);
        }
        
        // Context menu
        if (ImGui::BeginPopupContextItem()) {
            if (state == PluginState::UNLOADED) {
                if (ImGui::MenuItem("Load Plugin")) {
                    plugin_manager->loadPlugin(pluginName);
                    refreshPluginList();
                }
            } else if (state == PluginState::ACTIVE) {
                if (ImGui::MenuItem("Unload Plugin")) {
                    plugin_manager->unloadPlugin(pluginName);
                    refreshPluginList();
                }
                if (ImGui::MenuItem("Reload Plugin")) {
                    plugin_manager->reloadPlugin(pluginName);
                    refreshPluginList();
                }
            }
            
            if (ImGui::MenuItem("Uninstall Plugin")) {
                show_uninstall_dialog = true;
            }
            
            ImGui::EndPopup();
        }
    }
    
    ImGui::EndChild();
}

void PluginManagerWindow::renderPluginDetails() {
    ImGui::BeginChild("PluginDetails");
    
    ImGui::Text("Name: %s", selected_plugin_info.name.c_str());
    ImGui::Text("Version: %s", selected_plugin_info.version.toString().c_str());
    ImGui::Text("Author: %s", selected_plugin_info.author.c_str());
    ImGui::Text("Description:");
    ImGui::TextWrapped("%s", selected_plugin_info.description.c_str());
    
    if (!selected_plugin_info.website.empty()) {
        ImGui::Text("Website: %s", selected_plugin_info.website.c_str());
    }
    
    // Plugin state
    PluginState state = plugin_manager->getPluginState(selected_plugin_name);
    ImGui::Separator();
    ImGui::Text("Status: %s", [state]() {
        switch (state) {
            case PluginState::UNLOADED: return "Unloaded";
            case PluginState::LOADING: return "Loading...";
            case PluginState::LOADED: return "Loaded";
            case PluginState::ACTIVE: return "Active";
            case PluginState::ERROR: return "Error";
            case PluginState::UNLOADING: return "Unloading...";
            default: return "Unknown";
        }
    }());
    
    // Error message if any
    std::string error = plugin_manager->getPluginError(selected_plugin_name);
    if (!error.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", error.c_str());
    }
    
    // Control buttons
    ImGui::Separator();
    renderPluginControls();
    
    // Dependencies
    if (!selected_plugin_info.dependencies.empty()) {
        ImGui::Separator();
        ImGui::Text("Dependencies:");
        for (const auto& dep : selected_plugin_info.dependencies) {
            ImGui::BulletText("%s", dep.c_str());
        }
    }
    
    // Tags
    if (!selected_plugin_info.tags.empty()) {
        ImGui::Separator();
        ImGui::Text("Tags:");
        for (const auto& tag : selected_plugin_info.tags) {
            ImGui::SameLine();
            ImGui::Button(tag.c_str());
        }
    }
    
    // Permissions
    ImGui::Separator();
    renderPermissionsEditor();
    
    // Performance metrics
    ImGui::Separator();
    renderPluginMetrics();
    
    ImGui::EndChild();
}

void PluginManagerWindow::renderPluginControls() {
    PluginState state = plugin_manager->getPluginState(selected_plugin_name);
    
    if (state == PluginState::UNLOADED) {
        if (ImGui::Button("Load Plugin")) {
            plugin_manager->loadPlugin(selected_plugin_name);
        }
    } else if (state == PluginState::ACTIVE) {
        if (ImGui::Button("Unload Plugin")) {
            plugin_manager->unloadPlugin(selected_plugin_name);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload Plugin")) {
            plugin_manager->reloadPlugin(selected_plugin_name);
        }
    }
    
    ImGui::SameLine();
    bool enabled = plugin_manager->isPluginEnabled(selected_plugin_name);
    if (ImGui::Checkbox("Auto-start", &enabled)) {
        plugin_manager->enableAutoStart(selected_plugin_name, enabled);
    }
    
    ImGui::SameLine();
    bool trusted = plugin_manager->isPluginTrusted(selected_plugin_name);
    if (ImGui::Checkbox("Trusted", &trusted)) {
        if (trusted) {
            plugin_manager->trustPlugin(selected_plugin_name);
        } else {
            plugin_manager->untrustPlugin(selected_plugin_name);
        }
    }
}

void PluginManagerWindow::renderPermissionsEditor() {
    ImGui::Text("Permissions:");
    
    auto available_permissions = plugin_manager->getPluginPermissions(selected_plugin_name);
    
    for (const auto& permission : available_permissions) {
        bool granted = plugin_manager->hasPermission(selected_plugin_name, permission);
        if (ImGui::Checkbox(permission.c_str(), &granted)) {
            if (granted) {
                plugin_manager->grantPermission(selected_plugin_name, permission);
            } else {
                plugin_manager->revokePermission(selected_plugin_name, permission);
            }
        }
    }
}

void PluginManagerWindow::renderPluginMetrics() {
    ImGui::Text("Performance Metrics:");
    
    auto metrics = plugin_manager->getPluginMetrics(selected_plugin_name);
    ImGui::Text("Total calls: %zu", metrics.call_count);
    ImGui::Text("Average execution time: %.2f ms", metrics.average_execution_time_ms);
    ImGui::Text("Error count: %zu", metrics.error_count);
    
    if (metrics.call_count > 0) {
        float error_rate = static_cast<float>(metrics.error_count) / metrics.call_count * 100.0f;
        ImGui::Text("Error rate: %.1f%%", error_rate);
    }
}

void PluginManagerWindow::renderInstallDialog() {
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Install Plugin", &show_install_dialog, ImGuiWindowFlags_Modal)) {
        ImGui::Text("Plugin file path:");
        ImGui::InputText("##filepath", install_file_path, sizeof(install_file_path));
        
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            // Would open file dialog in real implementation
        }
        
        ImGui::Text("Plugin name (optional):");
        ImGui::InputText("##pluginname", install_plugin_name, sizeof(install_plugin_name));
        
        ImGui::Separator();
        
        if (ImGui::Button("Install")) {
            std::string name = strlen(install_plugin_name) > 0 ? install_plugin_name : "";
            if (plugin_manager->installPlugin(install_file_path, name)) {
                show_install_dialog = false;
                refreshPluginList();
                // Clear inputs
                install_file_path[0] = '\0';
                install_plugin_name[0] = '\0';
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            show_install_dialog = false;
        }
    }
    ImGui::End();
}

void PluginManagerWindow::renderUninstallDialog() {
    if (ImGui::BeginPopupModal("Uninstall Plugin", &show_uninstall_dialog)) {
        ImGui::Text("Are you sure you want to uninstall '%s'?", selected_plugin_name.c_str());
        ImGui::Text("This action cannot be undone.");
        
        ImGui::Separator();
        
        if (ImGui::Button("Uninstall")) {
            plugin_manager->uninstallPlugin(selected_plugin_name);
            show_uninstall_dialog = false;
            refreshPluginList();
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            show_uninstall_dialog = false;
        }
        
        ImGui::EndPopup();
    }
}

void PluginManagerWindow::refreshPluginList() {
    filtered_plugins.clear();
    
    auto available = plugin_manager->getAvailablePlugins();
    auto loaded = plugin_manager->getLoadedPlugins();
    
    // Combine available and loaded plugins
    std::set<std::string> all_plugins;
    all_plugins.insert(available.begin(), available.end());
    all_plugins.insert(loaded.begin(), loaded.end());
    
    for (const std::string& plugin : all_plugins) {
        if (show_only_loaded && !plugin_manager->isPluginLoaded(plugin)) {
            continue;
        }
        
        if (!matchesFilter(plugin)) {
            continue;
        }
        
        filtered_plugins.push_back(plugin);
    }
    
    // Sort plugins alphabetically
    std::sort(filtered_plugins.begin(), filtered_plugins.end());
}

bool PluginManagerWindow::matchesFilter(const std::string& pluginName) const {
    if (strlen(plugin_search_filter) == 0) {
        return true;
    }
    
    std::string filter = plugin_search_filter;
    std::string name = pluginName;
    
    // Convert to lowercase for case-insensitive search
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    
    return name.find(filter) != std::string::npos;
}

void PluginManagerWindow::setSelectedPlugin(const std::string& pluginName) {
    selected_plugin_name = pluginName;
    
    auto it = std::find(filtered_plugins.begin(), filtered_plugins.end(), pluginName);
    if (it != filtered_plugins.end()) {
        selected_plugin_index = std::distance(filtered_plugins.begin(), it);
        selected_plugin_info = plugin_manager->getPluginInfo(pluginName);
        plugin_permissions = plugin_manager->getPluginPermissions(pluginName);
    }
}

void PluginManagerWindow::refresh() {
    plugin_manager->scanForPlugins();
    refreshPluginList();
}

} // namespace PluginSystem