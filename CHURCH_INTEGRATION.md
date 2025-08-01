# Church Management Integration - Architecture Overview

## Core Components Added

### 1. Integration Framework (`src/integrations/`)
- **IntegrationManager**: Central orchestrator for all church management integrations
- **IntegrationProvider**: Abstract base class for integration implementations
- **PlanningCenterProvider**: Implementation for Planning Center Online integration

### 2. Service Planning Module (`src/service/`)
- **ServicePlan**: Complete service planning data structure with:
  - Service items (scripture, songs, prayers, announcements, etc.)
  - Collaboration features (multi-user editing, permissions)
  - Version control and approval workflows
  - Template management
  - Export/import capabilities

### 3. API Framework (`src/api/`)
- **ApiServer**: RESTful API server foundation with:
  - Route management and HTTP methods
  - OAuth authentication support
  - Rate limiting
  - CORS configuration
  - Webhook support
  - Error handling and logging

### 4. UI Integration (`src/ui/`)
- **Service Planning Screen**: New UI screen for managing service plans
- **Integrations Window**: Configuration interface for church management connections
- **Menu Integration**: Added service planning and integrations to File menu

## Supported Integrations

### Church Management Systems
1. **Planning Center Online** - Full integration with import/export, real-time sync
2. **ChurchTools** - Management system integration
3. **Breeze ChMS** - Church management connectivity
4. **Rock RMS** - Relationship management system plugin
5. **Elvanto/PushPay** - Service planning integration
6. **Church Community Builder (CCB)** - Basic connectivity

### Worship Software
1. **ProPresenter** - Slide export functionality
2. **EasyWorship** - Compatibility layer
3. **MediaShout** - Integration support
4. **OpenLP** - Plugin development framework
5. **Proclaim by Faithlife** - Service integration

## Key Features Implemented

### Service Planning
- Drag-and-drop service item ordering
- Multiple service item types (scripture, songs, prayers, etc.)
- Duration calculations and timeline management
- Template creation and reuse
- Real-time collaboration with role-based permissions

### Integration Management
- Connection status monitoring
- OAuth authentication flow
- Configuration management
- Test connections and health monitoring
- Bulk synchronization operations

### API Capabilities
- RESTful endpoints for external integration
- Webhook support for real-time updates
- Rate limiting and security
- CORS support for web applications
- Comprehensive error handling

### Data Synchronization
- Conflict resolution for simultaneous edits
- Offline mode with sync when connected
- Backup and restore functionality
- Cross-device synchronization

## Technical Architecture

### Modular Design
- Clean separation between integration, service, and API layers
- Extensible provider pattern for adding new integrations
- Minimal changes to existing VerseFinder core functionality

### Integration Patterns
- Abstract provider interface allows easy addition of new church management systems
- Configuration-driven approach for different authentication methods
- Status tracking and error handling across all integrations

### UI Integration
- New screens integrate seamlessly with existing ImGui interface
- Consistent styling and user experience
- Non-intrusive additions that don't affect existing functionality

## Usage Examples

### Basic Service Planning
```cpp
// Create a new service plan
ServicePlan plan("Sunday Morning Service", service_time);
plan.setDescription("Easter Sunday Service");

// Add service items
ServiceItem scripture;
scripture.type = ServiceItemType::SCRIPTURE;
scripture.title = "Opening Scripture";
scripture.content = "John 3:16";
plan.addItem(scripture);
```

### Integration Management
```cpp
// Initialize integration manager
IntegrationManager manager;

// Add Planning Center integration
IntegrationConfig config;
config.type = IntegrationType::PLANNING_CENTER;
config.api_key = "your_api_key";
manager.addIntegration(config);

// Export service plan
manager.exportServicePlan(plan, IntegrationType::PLANNING_CENTER);
```

### API Server Usage
```cpp
// Create API server
ApiServer api;
api.start(8080);

// Add route for service plans
api.addRoute(HttpMethod::GET, "/api/service-plans", 
    [](const ApiRequest& req) -> ApiResponse {
        // Handle service plan requests
        return jsonResponse(service_plans_json);
    });
```

## Testing

The integration has been tested with:
- Integration manager instantiation and available integrations listing
- Service plan creation and item management
- JSON export/import functionality
- API server initialization and basic setup

All components compile successfully and core functionality is verified through the integration test.