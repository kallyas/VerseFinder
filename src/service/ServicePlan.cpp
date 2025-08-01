#include "ServicePlan.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <unordered_map>

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
    ServiceItem new_item = item;
    if (new_item.id.empty()) {
        new_item.id = generateItemId();
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
    
    if (it != items_.end() && new_index < items_.size()) {
        ServiceItem item = *it;
        items_.erase(it);
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
    // Implementation would save the current plan as a template
    // This is a placeholder for the actual template saving logic
}

void ServicePlan::loadFromTemplate(const std::string& template_name) {
    // Implementation would load a plan from a saved template
    // This is a placeholder for the actual template loading logic
}

std::vector<std::string> ServicePlan::getAvailableTemplates() {
    // Return list of available templates
    return {"Sunday Morning Service", "Evening Service", "Youth Service", "Special Event"};
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
    // Basic JSON export - in a real implementation, this would use nlohmann::json
    std::ostringstream json;
    json << "{\n";
    json << "  \"id\": \"" << id_ << "\",\n";
    json << "  \"title\": \"" << title_ << "\",\n";
    json << "  \"description\": \"" << description_ << "\",\n";
    json << "  \"items\": [\n";
    
    for (size_t i = 0; i < items_.size(); ++i) {
        const auto& item = items_[i];
        json << "    {\n";
        json << "      \"id\": \"" << item.id << "\",\n";
        json << "      \"title\": \"" << item.title << "\",\n";
        json << "      \"type\": " << static_cast<int>(item.type) << ",\n";
        json << "      \"content\": \"" << item.content << "\"\n";
        json << "    }";
        if (i < items_.size() - 1) json << ",";
        json << "\n";
    }
    
    json << "  ]\n";
    json << "}";
    return json.str();
}

bool ServicePlan::importFromJson(const std::string& json_data) {
    // Placeholder for JSON import implementation
    return false;
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
    sync_timestamps_[integration_type] = std::chrono::system_clock::now();
}

bool ServicePlan::needsSync(const std::string& integration_type) const {
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