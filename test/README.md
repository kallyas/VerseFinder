# VerseFinder Tests

This directory contains all test files for the VerseFinder project. Tests are organized into different categories based on functionality.

## Test Files

### Core Tests
- `performance_test.cpp` - Performance benchmarks and timing tests
- `quick_test.cpp` - Fast smoke tests for basic functionality
- `test_advanced_features.cpp` - Tests for advanced search features
- `integration_test.cpp` - Integration tests for core components
- `integration_test_simple.cpp` - Simple integration tests

### UI Tests
- `validation_test.cpp` - Validation tests for animation system and visual effects
- `ui_test.cpp` - Basic UI component tests

### Translation Tests
- `test_comparison.cpp` - Translation comparison functionality tests
- `test_multi_translation.cpp` - Multi-translation handling tests

## Running Tests

### Build All Tests
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Run Individual Tests
```bash
cd build
./performance_test     # Performance benchmarks
./quick_test          # Quick smoke tests
./validation_test     # Animation and effects validation
./integration_test    # Core integration tests
```

### Run All Tests with CTest
```bash
cd build
ctest --output-on-failure --verbose
```

## Test Data

Tests use `sample_bible.json` for test data. Make sure this file is available in the build directory before running tests.

## GitHub Actions Integration

Tests are automatically run on all supported platforms (Linux, Windows, macOS) through GitHub Actions workflows:

- **CI Workflow** (`.github/workflows/ci.yml`) - Runs all tests on push/PR
- **Release Workflow** (`.github/workflows/release.yml`) - Builds executables for all platforms

## Platform-Specific Notes

### Linux
- Uses Xvfb for headless GUI testing in CI
- Requires X11 development libraries for building UI tests

### Windows
- Uses FetchContent for dependency management
- All dependencies downloaded automatically

### macOS
- Uses Homebrew for system dependencies
- Requires manual installation of development tools

## Test Categories

### Performance Tests
Focus on timing, memory usage, and scalability:
- Search performance (must be < 50ms)
- Memory usage monitoring
- Cache effectiveness
- Auto-complete responsiveness

### Validation Tests
Verify advanced presentation features:
- Animation system functionality
- Visual effects rendering
- Media format support
- Configuration loading

### Integration Tests
Test component interactions:
- Core system integration
- API functionality
- Service plan management
- Translation management

## Contributing

When adding new tests:
1. Place test files in the `test/` directory
2. Update `CMakeLists.txt` to include new test executables
3. Add test targets to CTest configuration
4. Update this README with test descriptions
5. Ensure tests work on all supported platforms