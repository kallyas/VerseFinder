# VerseFinder User Customization Features

## Overview
VerseFinder now includes comprehensive user customization options that allow users to personalize their Bible search experience. All settings are automatically saved and restored between sessions.

## Features Implemented

### 🎨 Appearance Customization
- **Font Size Adjustment**: Slider control (8-36pt) with real-time preview
- **Color Themes**: Four complete themes (Dark, Light, Blue, Green)
- **Highlight Colors**: Custom color picker for search result highlighting
- **Window Memory**: Automatic saving and restoring of window size and position

### 🔍 Search Preferences
- **Default Translation**: Dropdown selection of available Bible translations
- **Search Result Limits**: Configurable maximum results (10-200)
- **Search Format**: Choose between "Reference + Text", "Text Only", or "Reference Only"
- **Auto Search**: Toggle automatic searching as you type
- **Fuzzy Search**: Enable/disable with advanced configuration options
- **Performance Stats**: Toggle display of search timing information

### 📚 Content Management
- **Favorites System**: Right-click any verse to add/remove from favorites
- **Search History**: Automatic tracking with configurable limits (10-500 entries)
- **Recent Translations**: Track last 10 used translations
- **Quick Access Dropdown**: Recent searches available in search area
- **Display Formats**: Standard, Compact, or Detailed verse formatting

### ⚙️ Technical Features
- **Cross-Platform Settings**: Platform-appropriate storage locations
- **Import/Export**: Save and share settings configurations
- **Reset to Defaults**: One-click restoration of default settings
- **Validation**: Automatic error handling with graceful fallbacks
- **Real-time Preview**: Immediate application of theme and font changes

## Keyboard Shortcuts
- `Ctrl+K` - Clear search
- `Ctrl+C` - Copy selected verse
- `Ctrl+P` - Toggle performance statistics
- `Ctrl+,` - Open settings
- `F1` - Show help
- `Escape` - Close dialogs

## Settings File Location
Settings are automatically saved to platform-appropriate locations:
- **Windows**: `%APPDATA%\VerseFinder\settings.json`
- **macOS**: `~/Library/Application Support/VerseFinder/settings.json`
- **Linux**: `~/.config/VerseFinder/settings.json`

## Usage
1. Open settings with `Ctrl+,` or via the Settings button
2. Navigate between tabs: Translations, Appearance, Search, Content, Shortcuts
3. Changes are applied immediately where possible
4. Click "Save Settings" to persist changes
5. Use Import/Export for sharing configurations
6. Right-click search results for context menu options

## Implementation Notes
The customization system is built on a robust JSON-based settings architecture that provides:
- Type-safe settings with validation
- Automatic migration for future versions
- Comprehensive error handling
- Minimal performance impact
- Clean separation of concerns

All customization features integrate seamlessly with the existing VerseFinder interface while maintaining the application's performance and reliability.