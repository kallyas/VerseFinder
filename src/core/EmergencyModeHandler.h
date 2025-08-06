#ifndef EMERGENCY_MODE_HANDLER_H
#define EMERGENCY_MODE_HANDLER_H

#include <string>
#include <atomic>

class EmergencyModeHandler {
private:
    std::atomic<bool> is_initialized{false};
    std::atomic<bool> is_active{false};
    std::atomic<int> activation_count{0};
    std::string last_activation_reason;

public:
    EmergencyModeHandler() = default;
    ~EmergencyModeHandler() = default;

    bool initialize() {
        is_initialized.store(true);
        return true;
    }

    void shutdown() {
        is_initialized.store(false);
        is_active.store(false);
    }

    bool activate(const std::string& reason) {
        if (!is_initialized.load()) return false;
        
        is_active.store(true);
        last_activation_reason = reason;
        activation_count.fetch_add(1);
        return true;
    }

    bool deactivate() {
        if (!is_initialized.load()) return false;
        
        is_active.store(false);
        return true;
    }

    bool isActive() const {
        return is_active.load();
    }

    int getActivationCount() const {
        return activation_count.load();
    }

    std::string getLastActivationReason() const {
        return last_activation_reason;
    }

    bool selfTest() {
        return is_initialized.load();
    }

    void resetStats() {
        activation_count.store(0);
        last_activation_reason.clear();
    }
};

#endif // EMERGENCY_MODE_HANDLER_H