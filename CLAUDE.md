# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

VerseFinder is a C++ GUI application for churches to quickly search and display Bible verses during services. It uses SDL2 for the GUI, nlohmann/json for JSON parsing, and implements high-performance verse lookup with in-memory storage and keyword indexing.

## Build System and Common Commands

**Build the project:**
```bash
cmake . && make
```

**Run the application:**
```bash
./VerseFinder
```

**Clean build artifacts:**
```bash
rm -rf CMakeCache.txt CMakeFiles/
```

**Full rebuild:**
```bash
rm -rf CMakeCache.txt CMakeFiles/ && cmake . && make
```

## Dependencies

The project requires:
- Dear ImGui (immediate-mode GUI framework)
- GLFW3 (window management and OpenGL context)
- GLEW or system OpenGL (OpenGL function loading)
- nlohmann/json (JSON parsing for Bible data)
- C++20 standard

Install on macOS:
```bash
brew install glfw glew nlohmann-json
```

## Architecture Overview

**Core Components:**
- `src/core/VerseFinder.{h,cpp}` - Bible data management, search algorithms, and keyword indexing
- `src/ui/VerseFinderApp.{h,cpp}` - SDL2-based GUI application with search interface
- `src/main.cpp` - Application entry point

**Data Structures:**
- `verses`: Nested hash maps for O(1) verse lookups by translation and reference
- `keyword_index`: Inverted index mapping words to verse references for fast keyword search
- `book_aliases`: Normalization of book name variations

**Search Implementation:**
- Reference search: Direct hash map lookup (e.g., "John 3:16")
- Keyword search: Token intersection with phrase verification for multi-word queries
- Asynchronous data loading with atomic status checking

**GUI Architecture:**
- Dear ImGui immediate-mode interface with modern, responsive design
- GLFW window management with OpenGL rendering
- Tabbed settings interface, modal dialogs, and real-time search
- Professional dark theme with customizable appearance

## Data Format

The application expects `bible.json` with this structure:
```json
{
  "translation": "KJV",
  "abbreviation": "KJV", 
  "books": [
    {
      "name": "Genesis",
      "chapters": [
        {
          "chapter": 1,
          "verses": [
            {
              "verse": 1,
              "text": "In the beginning God created..."
            }
          ]
        }
      ]
    }
  ]
}
```

## Asset Requirements

- Font files in `assets/fonts/arial/` (multiple Arial variants for text rendering)
- Bible data as `bible.json` in project root or build directory
- CMake automatically copies assets to build directory

## Performance Considerations

- All Bible data loaded into memory on startup for sub-second search times
- Keyword index built during loading for fast phrase searches
- Asynchronous loading prevents GUI blocking during startup
- Book name normalization handles common variations automatically