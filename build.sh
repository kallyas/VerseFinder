#!/bin/bash

# VerseFinder Build Script
# Automatically builds VerseFinder with all dependencies

set -e  # Exit on any error

echo "🚀 VerseFinder Build Script"
echo "========================="

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake not found. Please install CMake 3.16+ first."
    exit 1
fi

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
    JOBS=$(sysctl -n hw.ncpu)
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
    JOBS=$(nproc)
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    PLATFORM="Windows"
    JOBS=$NUMBER_OF_PROCESSORS
else
    PLATFORM="Unknown"
    JOBS=4
fi

echo "🖥️  Platform: $PLATFORM"
echo "🔧 Build jobs: $JOBS"
echo ""

# Create build directory
echo "📁 Creating build directory..."
if [ -d "build" ]; then
    echo "   Cleaning existing build directory..."
    rm -rf build
fi
mkdir build
cd build

# Configure
echo "⚙️  Configuring with CMake..."
if [[ "$PLATFORM" == "Windows" ]]; then
    cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
else
    cmake .. -DCMAKE_BUILD_TYPE=Release
fi

# Build
echo "🔨 Building VerseFinder..."
if [[ "$PLATFORM" == "Windows" ]]; then
    cmake --build . --config Release --parallel $JOBS
else
    make -j$JOBS
fi

echo ""
echo "✅ Build completed successfully!"
echo ""

# Check if executable exists
if [[ "$PLATFORM" == "Windows" ]]; then
    EXECUTABLE="Release/VerseFinder.exe"
else
    EXECUTABLE="./VerseFinder"
fi

if [ -f "$EXECUTABLE" ]; then
    echo "🎉 VerseFinder is ready!"
    echo "   Executable: $(pwd)/$EXECUTABLE"
    echo ""
    echo "To run VerseFinder:"
    if [[ "$PLATFORM" == "Windows" ]]; then
        echo "   cd build && Release\\VerseFinder.exe"
    else
        echo "   cd build && ./VerseFinder"
    fi
else
    echo "⚠️  Warning: Executable not found at expected location"
fi

echo ""
echo "📚 For more information, see README.md"