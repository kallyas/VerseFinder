#ifndef SERVICE_PLAN_H
#define SERVICE_PLAN_H

#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <mutex>

enum class ServiceItemType {
    SONG,
    SCRIPTURE,
    SERMON,
    PRAYER,
    ANNOUNCEMENT,
    OFFERING,
    BAPTISM,
    COMMUNION,
    MEDIA,
    CUSTOM
};

enum class ServiceRole {
    PASTOR,
    WORSHIP_LEADER,
    TECH_OPERATOR,
    READER,
    MUSICIAN,
    VOLUNTEER
};

struct ServiceItem {
    std::string id;
    ServiceItemType type;
    std::string title;
    std::string description;
    std::string content;  // Scripture reference, song lyrics, etc.
    std::chrono::seconds duration;
    std::string assigned_to;
    ServiceRole assigned_role;
    std::string notes;
    std::vector<std::string> tags;
    bool is_transition;
    
    // Scripture-specific fields
    std::string translation;
    std::string book;
    int chapter;
    int verse_start;
    int verse_end;
    
    // Media-specific fields
    std::string media_path;
    std::string media_type;
    
    ServiceItem() : type(ServiceItemType::CUSTOM), duration(std::chrono::seconds(0)), 
                   assigned_role(ServiceRole::PASTOR), is_transition(false),
                   chapter(0), verse_start(0), verse_end(0) {}
};

struct ServiceCollaborator {
    std::string user_id;
    std::string name;
    std::string email;
    ServiceRole role;
    bool can_edit;
    bool can_approve;
    std::chrono::system_clock::time_point last_active;
};

struct ServiceVersion {
    std::string version_id;
    std::string created_by;
    std::chrono::system_clock::time_point created_at;
    std::string comment;
    std::vector<ServiceItem> items;
};

class ServicePlan {
public:
    ServicePlan();
    ServicePlan(const std::string& title, const std::chrono::system_clock::time_point& service_time);
    
    // Basic properties
    void setTitle(const std::string& title);
    std::string getTitle() const;
    void setServiceTime(const std::chrono::system_clock::time_point& time);
    std::chrono::system_clock::time_point getServiceTime() const;
    void setDescription(const std::string& description);
    std::string getDescription() const;
    
    // Service items management
    void addItem(const ServiceItem& item);
    void insertItem(size_t index, const ServiceItem& item);
    void removeItem(const std::string& item_id);
    void moveItem(const std::string& item_id, size_t new_index);
    void updateItem(const ServiceItem& item);
    std::vector<ServiceItem>& getItems();
    const std::vector<ServiceItem>& getItems() const;
    ServiceItem* getItem(const std::string& item_id);
    
    // Template management
    void saveAsTemplate(const std::string& template_name);
    void loadFromTemplate(const std::string& template_name);
    static std::vector<std::string> getAvailableTemplates();
    
    // Collaboration
    void addCollaborator(const ServiceCollaborator& collaborator);
    void removeCollaborator(const std::string& user_id);
    void updateCollaboratorPermissions(const std::string& user_id, bool can_edit, bool can_approve);
    std::vector<ServiceCollaborator>& getCollaborators();
    const std::vector<ServiceCollaborator>& getCollaborators() const;
    
    // Version control
    std::string createVersion(const std::string& comment, const std::string& created_by);
    void revertToVersion(const std::string& version_id);
    std::vector<ServiceVersion> getVersionHistory() const;
    
    // Status and approval
    enum class ApprovalStatus { DRAFT, PENDING_APPROVAL, APPROVED, REJECTED };
    void setApprovalStatus(ApprovalStatus status);
    ApprovalStatus getApprovalStatus() const;
    void addApprovalComment(const std::string& comment, const std::string& user_id);
    
    // Duration calculations
    std::chrono::seconds getTotalDuration() const;
    std::chrono::system_clock::time_point getEstimatedEndTime() const;
    
    // Search and filtering
    std::vector<ServiceItem> findItemsByType(ServiceItemType type) const;
    std::vector<ServiceItem> findItemsByRole(ServiceRole role) const;
    std::vector<ServiceItem> findItemsByTag(const std::string& tag) const;
    
    // Export/Import
    std::string exportToJson() const;
    bool importFromJson(const std::string& json_data);
    std::string exportToPlainText() const;
    
    // Synchronization
    void markAsSynced(const std::string& integration_type);
    bool needsSync(const std::string& integration_type) const;
    std::chrono::system_clock::time_point getLastModified() const;
    
    // ID and metadata
    std::string getId() const;
    void setId(const std::string& id);

private:
    std::string id_;
    std::string title_;
    std::string description_;
    std::chrono::system_clock::time_point service_time_;
    std::chrono::system_clock::time_point created_at_;
    std::chrono::system_clock::time_point last_modified_;
    ApprovalStatus approval_status_;
    
    std::vector<ServiceItem> items_;
    std::vector<ServiceCollaborator> collaborators_;
    std::vector<ServiceVersion> versions_;
    std::vector<std::string> approval_comments_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> sync_timestamps_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    void updateLastModified();
    std::string generateItemId() const;
    std::string generateVersionId() const;
};

#endif // SERVICE_PLAN_H