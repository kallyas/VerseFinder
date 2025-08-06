#include "../interfaces/PluginInterfaces.h"
#include "../api/PluginAPI.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <chrono>

using namespace PluginSystem;

class PDFExportPlugin : public IExportPlugin {
private:
    PluginInfo info;
    PluginState state;
    PluginAPI* api;
    std::string last_error;
    PluginConfig config;
    
    // PDF Export options
    struct ExportOptions {
        std::string fontSize = "12";
        std::string fontFamily = "Arial";
        std::string pageSize = "A4";
        bool includeHeader = true;
        bool includeFooter = true;
        bool includePageNumbers = true;
        std::string headerText = "VerseFinder Export";
        std::string titleText = "Bible Verses";
        bool separateVerses = true;
    };
    
    ExportOptions defaultOptions;
    
public:
    PDFExportPlugin() : state(PluginState::UNLOADED), api(nullptr) {
        info.name = "PDF Export Plugin";
        info.description = "Export Bible verses and service plans to PDF format with customizable formatting";
        info.author = "VerseFinder Community";
        info.version = {1, 0, 0};
        info.website = "https://versefinder.com/plugins/pdf-export";
        info.tags = {"export", "pdf", "formatting", "service", "verses"};
    }
    
    // IPlugin interface
    bool initialize() override {
        try {
            state = PluginState::LOADED;
            last_error.clear();
            return true;
        } catch (const std::exception& e) {
            last_error = std::string("Initialization failed: ") + e.what();
            state = PluginState::ERROR;
            return false;
        }
    }
    
    void shutdown() override {
        state = PluginState::UNLOADED;
        api = nullptr;
    }
    
    const PluginInfo& getInfo() const override {
        return info;
    }
    
    bool configure(const PluginConfig& pluginConfig) override {
        config = pluginConfig;
        
        // Load configuration options
        defaultOptions.fontSize = config.getString("fontSize", "12");
        defaultOptions.fontFamily = config.getString("fontFamily", "Arial");
        defaultOptions.pageSize = config.getString("pageSize", "A4");
        defaultOptions.includeHeader = config.getBool("includeHeader", true);
        defaultOptions.includeFooter = config.getBool("includeFooter", true);
        defaultOptions.includePageNumbers = config.getBool("includePageNumbers", true);
        defaultOptions.headerText = config.getString("headerText", "VerseFinder Export");
        defaultOptions.titleText = config.getString("titleText", "Bible Verses");
        defaultOptions.separateVerses = config.getBool("separateVerses", true);
        
        return true;
    }
    
    void onActivate() override {
        state = PluginState::ACTIVE;
    }
    
    void onDeactivate() override {
        state = PluginState::LOADED;
    }
    
    PluginState getState() const override {
        return state;
    }
    
    std::string getLastError() const override {
        return last_error;
    }
    
    // IExportPlugin interface
    bool exportVerse(const std::string& verse, const std::string& reference, 
                    const std::string& filename) override {
        try {
            std::vector<std::string> verses = {verse};
            std::vector<std::string> references = {reference};
            return exportVerses(verses, references, filename);
        } catch (const std::exception& e) {
            last_error = std::string("Export verse failed: ") + e.what();
            return false;
        }
    }
    
    bool exportVerses(const std::vector<std::string>& verses, 
                     const std::vector<std::string>& references,
                     const std::string& filename) override {
        try {
            if (verses.empty() || references.empty() || verses.size() != references.size()) {
                last_error = "Invalid input: verses and references must have the same non-zero size";
                return false;
            }
            
            // Generate HTML content that can be converted to PDF
            std::string htmlContent = generateHTMLContent(verses, references, defaultOptions);
            
            // For this example, we'll save as HTML with PDF-like formatting
            // In a real implementation, you might use a PDF library like libharu or wkhtmltopdf
            std::string outputFile = filename;
            if (outputFile.find(".pdf") != std::string::npos) {
                outputFile = outputFile.substr(0, outputFile.find(".pdf")) + ".html";
            }
            
            std::ofstream file(outputFile);
            if (!file.is_open()) {
                last_error = "Cannot open file for writing: " + outputFile;
                return false;
            }
            
            file << htmlContent;
            file.close();
            
            // Log success
            if (api) {
                PluginEvent event("verse_exported", info.name);
                event.setData("filename", outputFile);
                event.setData("verse_count", std::to_string(verses.size()));
                // Note: In a real implementation, you'd emit this event through the API
            }
            
            last_error.clear();
            return true;
            
        } catch (const std::exception& e) {
            last_error = std::string("Export verses failed: ") + e.what();
            return false;
        }
    }
    
    bool exportServicePlan(const std::string& planData, const std::string& filename) override {
        try {
            // Parse service plan data and create formatted output
            std::string htmlContent = generateServicePlanHTML(planData, defaultOptions);
            
            std::string outputFile = filename;
            if (outputFile.find(".pdf") != std::string::npos) {
                outputFile = outputFile.substr(0, outputFile.find(".pdf")) + ".html";
            }
            
            std::ofstream file(outputFile);
            if (!file.is_open()) {
                last_error = "Cannot open file for writing: " + outputFile;
                return false;
            }
            
            file << htmlContent;
            file.close();
            
            last_error.clear();
            return true;
            
        } catch (const std::exception& e) {
            last_error = std::string("Export service plan failed: ") + e.what();
            return false;
        }
    }
    
    std::string getFormatName() const override {
        return "PDF Document";
    }
    
    std::string getFileExtension() const override {
        return ".pdf";
    }
    
    std::vector<std::string> getSupportedOptions() const override {
        return {
            "fontSize", "fontFamily", "pageSize", "includeHeader", "includeFooter",
            "includePageNumbers", "headerText", "titleText", "separateVerses"
        };
    }
    
    bool supportsMultipleVerses() const override {
        return true;
    }
    
    // Set the API reference
    void setAPI(PluginAPI* pluginAPI) {
        api = pluginAPI;
    }

private:
    std::string generateHTMLContent(const std::vector<std::string>& verses,
                                   const std::vector<std::string>& references,
                                   const ExportOptions& options) {
        std::ostringstream html;
        
        // HTML document start with CSS styling
        html << "<!DOCTYPE html>\n";
        html << "<html>\n<head>\n";
        html << "<meta charset=\"UTF-8\">\n";
        html << "<title>" << options.titleText << "</title>\n";
        html << "<style>\n";
        html << generateCSS(options);
        html << "</style>\n";
        html << "</head>\n<body>\n";
        
        // Header
        if (options.includeHeader) {
            html << "<div class=\"header\">\n";
            html << "<h1>" << options.headerText << "</h1>\n";
            html << "<h2>" << options.titleText << "</h2>\n";
            html << "<div class=\"export-info\">Exported on " << getCurrentDateTime() << "</div>\n";
            html << "</div>\n";
        }
        
        // Content
        html << "<div class=\"content\">\n";
        
        for (size_t i = 0; i < verses.size(); ++i) {
            if (options.separateVerses && i > 0) {
                html << "<div class=\"verse-separator\"></div>\n";
            }
            
            html << "<div class=\"verse-container\">\n";
            html << "<div class=\"verse-reference\">" << escapeHTML(references[i]) << "</div>\n";
            html << "<div class=\"verse-text\">" << escapeHTML(verses[i]) << "</div>\n";
            html << "</div>\n";
        }
        
        html << "</div>\n";
        
        // Footer
        if (options.includeFooter) {
            html << "<div class=\"footer\">\n";
            html << "<div class=\"footer-text\">Generated by VerseFinder PDF Export Plugin</div>\n";
            if (options.includePageNumbers) {
                html << "<div class=\"page-number\">Page 1</div>\n";
            }
            html << "</div>\n";
        }
        
        html << "</body>\n</html>";
        
        return html.str();
    }
    
    std::string generateServicePlanHTML(const std::string& planData, const ExportOptions& options) {
        std::ostringstream html;
        
        html << "<!DOCTYPE html>\n";
        html << "<html>\n<head>\n";
        html << "<meta charset=\"UTF-8\">\n";
        html << "<title>Service Plan</title>\n";
        html << "<style>\n";
        html << generateCSS(options);
        html << "\n.service-item { margin: 20px 0; padding: 15px; border-left: 4px solid #4CAF50; }\n";
        html << ".service-title { font-weight: bold; color: #2E7D32; margin-bottom: 10px; }\n";
        html << "</style>\n";
        html << "</head>\n<body>\n";
        
        if (options.includeHeader) {
            html << "<div class=\"header\">\n";
            html << "<h1>" << options.headerText << "</h1>\n";
            html << "<h2>Service Plan</h2>\n";
            html << "<div class=\"export-info\">Exported on " << getCurrentDateTime() << "</div>\n";
            html << "</div>\n";
        }
        
        html << "<div class=\"content\">\n";
        html << "<div class=\"service-item\">\n";
        html << "<div class=\"service-title\">Service Order</div>\n";
        html << "<div class=\"verse-text\">" << escapeHTML(planData) << "</div>\n";
        html << "</div>\n";
        html << "</div>\n";
        
        if (options.includeFooter) {
            html << "<div class=\"footer\">\n";
            html << "<div class=\"footer-text\">Generated by VerseFinder PDF Export Plugin</div>\n";
            html << "</div>\n";
        }
        
        html << "</body>\n</html>";
        
        return html.str();
    }
    
    std::string generateCSS(const ExportOptions& options) {
        std::ostringstream css;
        
        css << "body {\n";
        css << "  font-family: " << options.fontFamily << ", sans-serif;\n";
        css << "  font-size: " << options.fontSize << "pt;\n";
        css << "  line-height: 1.6;\n";
        css << "  margin: 0;\n";
        css << "  padding: 20px;\n";
        css << "  color: #333;\n";
        css << "  background-color: white;\n";
        css << "}\n\n";
        
        css << ".header {\n";
        css << "  text-align: center;\n";
        css << "  border-bottom: 2px solid #4CAF50;\n";
        css << "  padding-bottom: 20px;\n";
        css << "  margin-bottom: 30px;\n";
        css << "}\n\n";
        
        css << ".header h1 {\n";
        css << "  color: #2E7D32;\n";
        css << "  margin: 0 0 10px 0;\n";
        css << "  font-size: 24pt;\n";
        css << "}\n\n";
        
        css << ".header h2 {\n";
        css << "  color: #4CAF50;\n";
        css << "  margin: 0 0 15px 0;\n";
        css << "  font-size: 18pt;\n";
        css << "  font-weight: normal;\n";
        css << "}\n\n";
        
        css << ".export-info {\n";
        css << "  font-size: 10pt;\n";
        css << "  color: #666;\n";
        css << "  font-style: italic;\n";
        css << "}\n\n";
        
        css << ".content {\n";
        css << "  margin-bottom: 50px;\n";
        css << "}\n\n";
        
        css << ".verse-container {\n";
        css << "  margin: 25px 0;\n";
        css << "  padding: 15px;\n";
        css << "  border-left: 4px solid #2196F3;\n";
        css << "  background-color: #f8f9fa;\n";
        css << "}\n\n";
        
        css << ".verse-reference {\n";
        css << "  font-weight: bold;\n";
        css << "  color: #1976D2;\n";
        css << "  margin-bottom: 8px;\n";
        css << "  font-size: " << (std::stoi(options.fontSize) + 1) << "pt;\n";
        css << "}\n\n";
        
        css << ".verse-text {\n";
        css << "  text-align: justify;\n";
        css << "  line-height: 1.8;\n";
        css << "  color: #333;\n";
        css << "}\n\n";
        
        css << ".verse-separator {\n";
        css << "  height: 1px;\n";
        css << "  background-color: #ddd;\n";
        css << "  margin: 20px 0;\n";
        css << "}\n\n";
        
        css << ".footer {\n";
        css << "  position: fixed;\n";
        css << "  bottom: 20px;\n";
        css << "  left: 20px;\n";
        css << "  right: 20px;\n";
        css << "  border-top: 1px solid #ddd;\n";
        css << "  padding-top: 10px;\n";
        css << "  display: flex;\n";
        css << "  justify-content: space-between;\n";
        css << "  font-size: 9pt;\n";
        css << "  color: #666;\n";
        css << "}\n\n";
        
        css << "@media print {\n";
        css << "  body { margin: 0; }\n";
        css << "  .footer { position: fixed; bottom: 0; }\n";
        css << "}\n";
        
        return css.str();
    }
    
    std::string escapeHTML(const std::string& text) {
        std::string escaped = text;
        
        // Replace HTML special characters
        std::string::size_type pos = 0;
        while ((pos = escaped.find("&", pos)) != std::string::npos) {
            escaped.replace(pos, 1, "&amp;");
            pos += 5;
        }
        
        pos = 0;
        while ((pos = escaped.find("<", pos)) != std::string::npos) {
            escaped.replace(pos, 1, "&lt;");
            pos += 4;
        }
        
        pos = 0;
        while ((pos = escaped.find(">", pos)) != std::string::npos) {
            escaped.replace(pos, 1, "&gt;");
            pos += 4;
        }
        
        pos = 0;
        while ((pos = escaped.find("\"", pos)) != std::string::npos) {
            escaped.replace(pos, 1, "&quot;");
            pos += 6;
        }
        
        return escaped;
    }
    
    std::string getCurrentDateTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%B %d, %Y at %I:%M %p");
        return oss.str();
    }
};

// Required plugin export functions
extern "C" {
    PluginSystem::IPlugin* createPlugin() {
        return new PDFExportPlugin();
    }
    
    void destroyPlugin(PluginSystem::IPlugin* plugin) {
        delete plugin;
    }
    
    const char* getPluginApiVersion() {
        return "1.0";
    }
    
    const char* getPluginType() {
        return "export";
    }
    
    const char* getPluginName() {
        return "PDF Export Plugin";
    }
    
    const char* getPluginDescription() {
        return "Export Bible verses and service plans to PDF format with customizable formatting";
    }
    
    const char* getPluginVersion() {
        return "1.0.0";
    }
}