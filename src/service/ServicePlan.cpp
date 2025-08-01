#include "ServicePlan.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

ServicePlan::ServicePlan() 
    : id_(generateItemId()),
      service_time_(std::chrono::system_clock::now()),
      created_at_(std::chrono::system_clock::now()),
      last_modified_(std::chrono::system_clock::now()),
      approval_status_(ApprovalStatus::DRAFT) {
}

ServicePlan::ServicePlan(const std::string& title, const std::chrono::system_clock::time_point& service_time)
    : id_(generateItemId()),
      title_(title),
      service_time_(service_time),
      created_at_(std::chrono::system_clock::now()),
      last_modified_(std::chrono::system_clock::now()),
      approval_status_(ApprovalStatus::DRAFT) {
}

void ServicePlan::setTitle(const std::string& title) {
    title_ = title;
    updateLastModified();
}

std::string ServicePlan::getTitle() const {
    return title_;
}

void ServicePlan::setServiceTime(const std::chrono::system_clock::time_point& time) {
    service_time_ = time;
    updateLastModified();
}

std::chrono::system_clock::time_point ServicePlan::getServiceTime() const {
    return service_time_;
}

void ServicePlan::setDescription(const std::string& description) {
    description_ = description;
    updateLastModified();
}

std::string ServicePlan::getDescription() const {
    return description_;
}

void ServicePlan::addItem(const ServiceItem& item) {
    // Validate item
    if (item.title.empty()) {
        return; // Don't add items without titles
    }
    
    ServiceItem new_item = item;
    if (new_item.id.empty()) {
        new_item.id = generateItemId();
    }
    
    // Validate scripture fields if it's a scripture item
    if (new_item.type == ServiceItemType::SCRIPTURE) {
        if (new_item.chapter < 0 || new_item.verse_start < 0 || new_item.verse_end < 0) {
            return; // Don't add items with invalid verse numbers
        }
        if (new_item.verse_start > new_item.verse_end && new_item.verse_end != 0) {
            return; // Invalid verse range
        }
    }
    
    items_.push_back(new_item);
    updateLastModified();
}

void ServicePlan::insertItem(size_t index, const ServiceItem& item) {
    ServiceItem new_item = item;
    if (new_item.id.empty()) {
        new_item.id = generateItemId();
    }
    if (index >= items_.size()) {
        items_.push_back(new_item);
    } else {
        items_.insert(items_.begin() + index, new_item);
    }
    updateLastModified();
}

void ServicePlan::removeItem(const std::string& item_id) {
    items_.erase(
        std::remove_if(items_.begin(), items_.end(),
            [&item_id](const ServiceItem& item) { return item.id == item_id; }),
        items_.end()
    );
    updateLastModified();
}

void ServicePlan::moveItem(const std::string& item_id, size_t new_index) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [&item_id](const ServiceItem& item) { return item.id == item_id; });
    
    if (it != items_.end()) {
        ServiceItem item = *it;
        size_t current_index = std::distance(items_.begin(), it);
        
        // Don't move if already at target position
        if (current_index == new_index) {
            return;
        }
        
        items_.erase(it);
        
        // Adjust new_index if it's beyond the new size after removal
        if (new_index >= items_.size()) {
            items_.push_back(item);
        } else {
            items_.insert(items_.begin() + new_index, item);
        }
        updateLastModified();
    }
}

void ServicePlan::updateItem(const ServiceItem& item) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [&item](const ServiceItem& existing) { return existing.id == item.id; });
    
    if (it != items_.end()) {
        *it = item;
        updateLastModified();
    }
}

std::vector<ServiceItem>& ServicePlan::getItems() {
    return items_;
}

const std::vector<ServiceItem>& ServicePlan::getItems() const {
    return items_;
}

ServiceItem* ServicePlan::getItem(const std::string& item_id) {
    auto it = std::find_if(items_.begin(), items_.end(),
        [&item_id](const ServiceItem& item) { return item.id == item_id; });
    return it != items_.end() ? &(*it) : nullptr;
}

void ServicePlan::saveAsTemplate(const std::string& template_name) {
    try {
        // Create templates directory if it doesn't exist
        std::filesystem::create_directories("templates");
        
        // Create template data (exclude ID and service time)
        nlohmann::json template_json;
        template_json["name"] = template_name;
        template_json["title"] = title_;
        template_json["description"] = description_;
        
        // Save items without IDs (they'll be generated when loaded)
        template_json["items"] = nlohmann::json::array();
        for (const auto& item : items_) {
            nlohmann::json item_json;
            item_json["title"] = item.title;
            item_json["type"] = static_cast<int>(item.type);
            item_json["content"] = item.content;
            item_json["description"] = item.description;
            item_json["duration_seconds"] = item.duration.count();
            item_json["assigned_role"] = static_cast<int>(item.assigned_role);
            item_json["notes"] = item.notes;
            item_json["tags"] = item.tags;
            item_json["is_transition"] = item.is_transition;
            
            if (item.type == ServiceItemType::SCRIPTURE) {
                item_json["translation"] = item.translation;
                item_json["book"] = item.book;
                item_json["chapter"] = item.chapter;
                item_json["verse_start"] = item.verse_start;
                item_json["verse_end"] = item.verse_end;
            }
            
            if (item.type == ServiceItemType::MEDIA) {
                item_json["media_path"] = item.media_path;
                item_json["media_type"] = item.media_type;
            }
            
            template_json["items"].push_back(item_json);
        }
        
        // Save to file
        std::string filename = "templates/" + template_name + ".json";
        std::ofstream file(filename);
        if (file.is_open()) {
            file << template_json.dump(2);
            file.close();
        }
        
    } catch (const std::exception&) {
        // Template save failed, but don't throw - this is not critical
    }
}

void ServicePlan::loadFromTemplate(const std::string& template_name) {
    try {
        std::string filename = "templates/" + template_name + ".json";
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            return; // Template not found, silently fail
        }
        
        std::string json_content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
        file.close();
        
        auto template_json = nlohmann::json::parse(json_content);
        
        // Load template data
        if (template_json.contains("title")) {
            title_ = template_json["title"].get<std::string>();
        }
        if (template_json.contains("description")) {
            description_ = template_json["description"].get<std::string>();
        }
        
        // Load items and generate new IDs
        if (template_json.contains("items") && template_json["items"].is_array()) {
            items_.clear();
            for (const auto& item_json : template_json["items"]) {
                ServiceItem item;
                item.id = generateItemId(); // Generate new ID
                
                if (item_json.contains("title")) {
                    item.title = item_json["title"].get<std::string>();
                }
                if (item_json.contains("type")) {
                    item.type = static_cast<ServiceItemType>(item_json["type"].get<int>());
                }
                if (item_json.contains("content")) {
                    item.content = item_json["content"].get<std::string>();
                }
                if (item_json.contains("description")) {
                    item.description = item_json["description"].get<std::string>();
                }
                if (item_json.contains("duration_seconds")) {
                    item.duration = std::chrono::seconds(item_json["duration_seconds"].get<int>());
                }
                if (item_json.contains("assigned_role")) {
                    item.assigned_role = static_cast<ServiceRole>(item_json["assigned_role"].get<int>());
                }
                if (item_json.contains("notes")) {
                    item.notes = item_json["notes"].get<std::string>();
                }
                if (item_json.contains("tags")) {
                    item.tags = item_json["tags"].get<std::vector<std::string>>();
                }
                if (item_json.contains("is_transition")) {
                    item.is_transition = item_json["is_transition"].get<bool>();
                }
                
                // Scripture-specific fields
                if (item.type == ServiceItemType::SCRIPTURE) {
                    if (item_json.contains("translation")) {
                        item.translation = item_json["translation"].get<std::string>();
                    }
                    if (item_json.contains("book")) {
                        item.book = item_json["book"].get<std::string>();
                    }
                    if (item_json.contains("chapter")) {
                        item.chapter = item_json["chapter"].get<int>();
                    }
                    if (item_json.contains("verse_start")) {
                        item.verse_start = item_json["verse_start"].get<int>();
                    }
                    if (item_json.contains("verse_end")) {
                        item.verse_end = item_json["verse_end"].get<int>();
                    }
                }
                
                // Media-specific fields
                if (item.type == ServiceItemType::MEDIA) {
                    if (item_json.contains("media_path")) {
                        item.media_path = item_json["media_path"].get<std::string>();
                    }
                    if (item_json.contains("media_type")) {
                        item.media_type = item_json["media_type"].get<std::string>();
                    }
                }
                
                items_.push_back(item);
            }
        }
        
        updateLastModified();
        
    } catch (const std::exception&) {
        // Template load failed, but don't throw - this is not critical
    }
}

std::vector<std::string> ServicePlan::getAvailableTemplates() {
    std::vector<std::string> templates;
    
    try {
        if (std::filesystem::exists("templates") && std::filesystem::is_directory("templates")) {
            for (const auto& entry : std::filesystem::directory_iterator("templates")) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    std::string filename = entry.path().stem().string();
                    templates.push_back(filename);
                }
            }
        }
    } catch (const std::exception&) {
        // If directory access fails, return default templates
    }
    
    // If no templates found, return some defaults
    if (templates.empty()) {
        templates = {"Sunday Morning Service", "Evening Service", "Youth Service", "Special Event"};
    }
    
    return templates;
}

void ServicePlan::addCollaborator(const ServiceCollaborator& collaborator) {
    collaborators_.push_back(collaborator);
    updateLastModified();
}

void ServicePlan::removeCollaborator(const std::string& user_id) {
    collaborators_.erase(
        std::remove_if(collaborators_.begin(), collaborators_.end(),
            [&user_id](const ServiceCollaborator& collab) { return collab.user_id == user_id; }),
        collaborators_.end()
    );
    updateLastModified();
}

void ServicePlan::updateCollaboratorPermissions(const std::string& user_id, bool can_edit, bool can_approve) {
    auto it = std::find_if(collaborators_.begin(), collaborators_.end(),
        [&user_id](const ServiceCollaborator& collab) { return collab.user_id == user_id; });
    
    if (it != collaborators_.end()) {
        it->can_edit = can_edit;
        it->can_approve = can_approve;
        updateLastModified();
    }
}

std::vector<ServiceCollaborator>& ServicePlan::getCollaborators() {
    return collaborators_;
}

const std::vector<ServiceCollaborator>& ServicePlan::getCollaborators() const {
    return collaborators_;
}

std::string ServicePlan::createVersion(const std::string& comment, const std::string& created_by) {
    ServiceVersion version;
    version.version_id = generateVersionId();
    version.created_by = created_by;
    version.created_at = std::chrono::system_clock::now();
    version.comment = comment;
    version.items = items_;
    
    versions_.push_back(version);
    return version.version_id;
}

void ServicePlan::revertToVersion(const std::string& version_id) {
    auto it = std::find_if(versions_.begin(), versions_.end(),
        [&version_id](const ServiceVersion& version) { return version.version_id == version_id; });
    
    if (it != versions_.end()) {
        items_ = it->items;
        updateLastModified();
    }
}

std::vector<ServiceVersion> ServicePlan::getVersionHistory() const {
    return versions_;
}

void ServicePlan::setApprovalStatus(ApprovalStatus status) {
    approval_status_ = status;
    updateLastModified();
}

ServicePlan::ApprovalStatus ServicePlan::getApprovalStatus() const {
    return approval_status_;
}

void ServicePlan::addApprovalComment(const std::string& comment, const std::string& user_id) {
    // Validate input
    if (comment.empty() || user_id.empty()) {
        return; // Don't add empty comments or comments without user ID
    }
    
    approval_comments_.push_back(user_id + ": " + comment);
    updateLastModified();
}

std::chrono::seconds ServicePlan::getTotalDuration() const {
    std::chrono::seconds total(0);
    for (const auto& item : items_) {
        total += item.duration;
    }
    return total;
}

std::chrono::system_clock::time_point ServicePlan::getEstimatedEndTime() const {
    return service_time_ + getTotalDuration();
}

std::vector<ServiceItem> ServicePlan::findItemsByType(ServiceItemType type) const {
    std::vector<ServiceItem> result;
    std::copy_if(items_.begin(), items_.end(), std::back_inserter(result),
        [type](const ServiceItem& item) { return item.type == type; });
    return result;
}

std::vector<ServiceItem> ServicePlan::findItemsByRole(ServiceRole role) const {
    std::vector<ServiceItem> result;
    std::copy_if(items_.begin(), items_.end(), std::back_inserter(result),
        [role](const ServiceItem& item) { return item.assigned_role == role; });
    return result;
}

std::vector<ServiceItem> ServicePlan::findItemsByTag(const std::string& tag) const {
    std::vector<ServiceItem> result;
    std::copy_if(items_.begin(), items_.end(), std::back_inserter(result),
        [&tag](const ServiceItem& item) { 
            return std::find(item.tags.begin(), item.tags.end(), tag) != item.tags.end(); 
        });
    return result;
}

std::string ServicePlan::exportToJson() const {
    try {
        nlohmann::json json;
        
        // Export basic properties
        json["id"] = id_;
        json["title"] = title_;
        json["description"] = description_;
        
        // Export service time as ISO 8601 string
        auto time_t = std::chrono::system_clock::to_time_t(service_time_);
        std::ostringstream time_stream;
        time_stream << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        json["service_time"] = time_stream.str();
        
        // Export items
        json["items"] = nlohmann::json::array();
        for (const auto& item : items_) {
            nlohmann::json item_json;
            item_json["id"] = item.id;
            item_json["title"] = item.title;
            item_json["type"] = static_cast<int>(item.type);
            item_json["content"] = item.content;
            item_json["description"] = item.description;
            item_json["duration_seconds"] = item.duration.count();
            item_json["assigned_to"] = item.assigned_to;
            item_json["assigned_role"] = static_cast<int>(item.assigned_role);
            item_json["notes"] = item.notes;
            item_json["tags"] = item.tags;
            item_json["is_transition"] = item.is_transition;
            
            // Scripture-specific fields
            if (item.type == ServiceItemType::SCRIPTURE) {
                item_json["translation"] = item.translation;
                item_json["book"] = item.book;
                item_json["chapter"] = item.chapter;
                item_json["verse_start"] = item.verse_start;
                item_json["verse_end"] = item.verse_end;
            }
            
            // Media-specific fields
            if (item.type == ServiceItemType::MEDIA) {
                item_json["media_path"] = item.media_path;
                item_json["media_type"] = item.media_type;
            }
            
            json["items"].push_back(item_json);
        }
        
        return json.dump(2); // Pretty print with 2-space indentation
        
    } catch (const std::exception&) {
        return "{}";
    }
}

bool ServicePlan::importFromJson(const std::string& json_data) {
    try {
        auto json = nlohmann::json::parse(json_data);
        
        // Validate required fields
        if (!json.contains("id") || !json.contains("title")) {
            return false;
        }
        
        // Import basic properties
        id_ = json["id"].get<std::string>();
        title_ = json["title"].get<std::string>();
        
        if (json.contains("description")) {
            description_ = json["description"].get<std::string>();
        }
        
        // Import items if present
        if (json.contains("items") && json["items"].is_array()) {
            items_.clear();
            for (const auto& item_json : json["items"]) {
                ServiceItem item;
                if (item_json.contains("id")) {
                    item.id = item_json["id"].get<std::string>();
                }
                if (item_json.contains("title")) {
                    item.title = item_json["title"].get<std::string>();
                }
                if (item_json.contains("type")) {
                    item.type = static_cast<ServiceItemType>(item_json["type"].get<int>());
                }
                if (item_json.contains("content")) {
                    item.content = item_json["content"].get<std::string>();
                }
                items_.push_back(item);
            }
        }
        
        updateLastModified();
        return true;
        
    } catch (const std::exception&) {
        return false;
    }
}

std::string ServicePlan::exportToPlainText() const {
    std::ostringstream text;
    text << "Service Plan: " << title_ << "\n";
    text << "Description: " << description_ << "\n";
    text << "===========================================\n\n";
    
    for (const auto& item : items_) {
        text << "• " << item.title << "\n";
        if (!item.content.empty()) {
            text << "  " << item.content << "\n";
        }
        text << "\n";
    }
    
    return text.str();
}

void ServicePlan::markAsSynced(const std::string& integration_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    sync_timestamps_[integration_type] = std::chrono::system_clock::now();
}

bool ServicePlan::needsSync(const std::string& integration_type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sync_timestamps_.find(integration_type);
    if (it == sync_timestamps_.end()) {
        return true;  // Never synced
    }
    return last_modified_ > it->second;
}

std::chrono::system_clock::time_point ServicePlan::getLastModified() const {
    return last_modified_;
}

std::string ServicePlan::getId() const {
    return id_;
}

void ServicePlan::setId(const std::string& id) {
    id_ = id;
}

void ServicePlan::updateLastModified() {
    std::lock_guard<std::mutex> lock(mutex_);
    last_modified_ = std::chrono::system_clock::now();
}

std::string ServicePlan::generateItemId() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::ostringstream oss;
    for (int i = 0; i < 8; ++i) {
        oss << std::hex << dis(gen);
    }
    return oss.str();
}

std::string ServicePlan::generateVersionId() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << "v" << std::put_time(std::gmtime(&time_t), "%Y%m%d_%H%M%S");
    return oss.str();
}