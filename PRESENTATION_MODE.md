# Presentation Mode Documentation

## Overview

VerseFinder now includes a powerful presentation mode designed specifically for church services, live streaming, and projector display. This feature provides a dual-window system optimized for OBS Studio integration and professional presentation environments.

## Core Features

### 🖥️ Dual Window System
- **Main Control Interface**: Search, select, and manage verses
- **Presentation Display**: Clean, minimal window optimized for audience viewing
- **Seamless Integration**: Control presentation from main interface

### 📺 OBS Studio Integration
- **Optimized Window Capture**: Properly named windows for easy OBS source identification
- **Borderless Mode**: Clean capture without window decorations
- **Transparent Background Support**: For overlay applications
- **Resolution Independent**: Works with any screen size or aspect ratio

### 🎨 Customizable Display
- **Font Configuration**: Adjustable size (24-120px) and family selection
- **Color Customization**: Background, text, and reference colors
- **Text Alignment**: Left, center, or right alignment options
- **Padding Control**: Adjustable text margins for optimal display

### 🖱️ Multi-Monitor Support
- **Monitor Selection**: Choose target display from available monitors
- **Automatic Positioning**: Smart placement on secondary displays
- **Fullscreen Mode**: True fullscreen for dedicated projection
- **Windowed Mode**: Flexible sizing for streaming setups

## Quick Start Guide

### 1. Enable Presentation Mode
1. Open **Settings** (Ctrl+, or File → Settings)
2. Go to the **📺 Presentation** tab
3. Check **"Enable Presentation Mode"**
4. Configure your preferred settings

### 2. Start Presenting
1. Search for a verse in the main interface
2. Press **F5** to start presentation mode, or
3. Click **"🚀 Start Presentation Mode"** in the preview panel

### 3. Display Verses
- **Right-click** on any search result → **"📺 Display on Presentation"**
- **Select a verse** and press **F7**
- Use the presentation preview panel for one-click display

### 4. Presentation Controls
- **F5**: Toggle presentation mode on/off
- **F6**: Toggle blank screen (hide/show content)
- **F7**: Display selected verse on presentation

## Settings Configuration

### Monitor Settings
- **Target Monitor**: Select which monitor to use for presentation
- **Fullscreen Mode**: Enable for dedicated projection screens
- **Window Size**: Custom dimensions when not in fullscreen

### Appearance Settings
- **Font Size**: 24-120px range for optimal readability
- **Text Alignment**: Left, center, or right positioning
- **Text Padding**: Margin control from screen edges
- **Show Bible Reference**: Toggle verse citation display

### Color Configuration
- **Background Color**: Custom background (default: black)
- **Text Color**: Main verse text color (default: white)
- **Reference Color**: Bible citation color (default: light gray)

### Advanced Options
- **OBS Studio Optimization**: Enhanced window capture compatibility
- **Auto-hide Cursor**: Hide mouse cursor on presentation display
- **Fade Transition Time**: Smooth transitions between verses (0-2 seconds)
- **Window Title**: Custom title for OBS source identification

## OBS Studio Setup

### Window Capture Method
1. Add **"Window Capture"** source in OBS
2. Select **"VerseFinder - Presentation"** from the window list
3. Configure capture options as needed

### Display Capture Method
1. Add **"Display Capture"** source in OBS
2. Select the monitor used for presentation
3. Crop to presentation window if needed

### Recommended OBS Settings
- **Capture Method**: Window Capture (more reliable)
- **Window Priority**: Match title if possible
- **Capture Cursor**: Disable (auto-hidden by VerseFinder)

## Church Service Workflow

### Preparation
1. **Setup Dual Monitors**: Connect projector/streaming display
2. **Configure VerseFinder**: Set target monitor and appearance
3. **Test OBS**: Verify capture source is working
4. **Prepare Verses**: Search and bookmark key verses beforehand

### During Service
1. **Search**: Use main interface to find verses
2. **Preview**: Check appearance in preview panel
3. **Display**: Right-click → "Display on Presentation" or press F7
4. **Navigate**: Switch between verses as needed
5. **Blank**: Press F6 to hide display during transitions

### Common Use Cases
- **Scripture Reading**: Display verses for congregation reading
- **Sermon References**: Quick display of referenced verses
- **Song Lyrics**: (Future feature) Display hymn verses
- **Announcements**: Use blank screen between segments

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| **F5** | Toggle presentation mode on/off |
| **F6** | Toggle blank screen (presentation only) |
| **F7** | Display selected verse on presentation |
| **Ctrl+,** | Open settings |
| **Ctrl+K** | Clear search |
| **Ctrl+C** | Copy selected verse |

## Troubleshooting

### Presentation Window Not Appearing
- Check if multiple monitors are detected
- Verify target monitor selection in settings
- Try windowed mode instead of fullscreen

### OBS Not Capturing Window
- Ensure "VerseFinder - Presentation" appears in window list
- Try "Display Capture" as alternative method
- Check window title in presentation settings

### Text Too Small/Large
- Adjust font size in presentation settings (24-120px)
- Consider screen resolution and viewing distance
- Test with sample verses for optimal sizing

### Performance Issues
- Close unnecessary applications
- Check if hardware acceleration is available
- Consider windowed mode for better performance

## Technical Details

### System Requirements
- **Operating System**: Windows, macOS, or Linux
- **Graphics**: OpenGL 3.2+ support
- **Memory**: 2GB RAM minimum
- **Display**: Multi-monitor support recommended

### Architecture
- **Dual GLFW Windows**: Separate rendering contexts
- **OpenGL Rendering**: Hardware-accelerated graphics
- **ImGui Interface**: Immediate-mode GUI framework
- **JSON Configuration**: Persistent settings storage

### Future Enhancements
- **Slide Templates**: Pre-designed layouts
- **Animation Effects**: Advanced transitions
- **Remote Control**: Mobile app integration
- **Lyrics Support**: Song and hymn display
- **Multi-language**: International Bible versions

## Support

For technical support or feature requests, please visit:
- **GitHub Repository**: [kallyas/VerseFinder](https://github.com/kallyas/VerseFinder)
- **Issues**: Report bugs or request features
- **Documentation**: Additional guides and tutorials

---

**VerseFinder Presentation Mode** - Bringing Bible verses to life in your church services! 🙏