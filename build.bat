@echo off
REM VerseFinder Build Script for Windows
REM Automatically builds VerseFinder with all dependencies

echo 🚀 VerseFinder Build Script for Windows
echo =======================================

REM Check for CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo ❌ CMake not found. Please install CMake 3.16+ first.
    echo    Download from: https://cmake.org/download/
    pause
    exit /b 1
)

echo 🖥️  Platform: Windows
echo 🔧 Build jobs: %NUMBER_OF_PROCESSORS%
echo.

REM Create build directory
echo 📁 Creating build directory...
if exist build (
    echo    Cleaning existing build directory...
    rmdir /s /q build
)
mkdir build
cd build

REM Configure
echo ⚙️  Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo ❌ CMake configuration failed
    pause
    exit /b 1
)

REM Build
echo 🔨 Building VerseFinder...
cmake --build . --config Release --parallel %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo ❌ Build failed
    pause
    exit /b 1
)

echo.
echo ✅ Build completed successfully!
echo.

REM Check if executable exists
if exist "Release\VerseFinder.exe" (
    echo 🎉 VerseFinder is ready!
    echo    Executable: %CD%\Release\VerseFinder.exe
    echo.
    echo To run VerseFinder:
    echo    cd build ^&^& Release\VerseFinder.exe
    echo.
    echo Press any key to run VerseFinder now, or Ctrl+C to exit...
    pause >nul
    Release\VerseFinder.exe
) else (
    echo ⚠️  Warning: Executable not found at expected location
    pause
)

echo.
echo 📚 For more information, see README.md