# Audio Plugins Collection

A collection of high-quality CLAP audio plugins built with modern C++20.

## Plugins

### 🎚️ Sesame Compressor
High-performance dynamics compressor with SIMD optimizations.
- Threshold, ratio, attack, release controls
- Soft knee compression
- Auto makeup gain
- [Learn more](compressor/README.md)

### 🎛️ Sesame EQ
4-band parametric equalizer with multiple filter types.
- 8 filter types per band (Low/High Pass, Low/High Shelf, Peak, Band Pass, Notch, All Pass)
- Independent frequency, gain, and Q controls
- Global output gain and bypass
- [Learn more](eq/README.md)

## Building All Plugins

### Prerequisites
- CMake 3.20 or higher
- C++20 compatible compiler (MSVC, GCC, or Clang)
- Git (for fetching dependencies)

### Quick Start

```powershell
# Clone the repository
git clone https://github.com/stinkydev/audio-plugins.git
cd audio-plugins

# Create build directory
mkdir build
cd build

# Configure (builds all plugins by default)
cmake ..

# Build all plugins in Release mode
cmake --build . --config Release

# Run all tests
ctest -C Release
```

### Build Options

You can customize the build using CMake options:

```powershell
# Build specific plugins
cmake .. -DBUILD_COMPRESSOR=ON -DBUILD_EQ=OFF

# Disable tests
cmake .. -DBUILD_TESTS=OFF

# Disable SIMD optimizations (compressor only)
cmake .. -DENABLE_SIMD=OFF

# Combine options
cmake .. -DBUILD_EQ=OFF -DBUILD_TESTS=OFF
```

### Output Locations

After building, compiled plugins will be in:
- **Windows**: `build/compressor/Release/SesameCompressor.clap` and `build/eq/Release/SesameEQ.clap`
- **macOS**: `build/compressor/SesameCompressor.clap` and `build/eq/SesameEQ.clap`
- **Linux**: `build/compressor/SesameCompressor.clap` and `build/eq/SesameEQ.clap`

## Installation

### Windows
Copy `.clap` files to:
```
C:\Program Files\Common Files\CLAP\
```

### macOS
Copy `.clap` bundles to:
```
/Library/Audio/Plug-Ins/CLAP/
~/Library/Audio/Plug-Ins/CLAP/
```

### Linux
Copy `.clap` files to:
```
~/.clap/
/usr/lib/clap/
```

## Project Structure

```
audio-plugins/
├── CMakeLists.txt          # Root build configuration
├── README.md               # This file
├── compressor/             # Compressor plugin
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── include/
│   ├── src/
│   └── tests/
└── eq/                     # EQ plugin
    ├── CMakeLists.txt
    ├── README.md
    ├── include/
    ├── src/
    └── tests/
```

## Development

### Adding a New Plugin

1. Create a new directory for your plugin
2. Add a `CMakeLists.txt` following the pattern in `compressor/` or `eq/`
3. Add your plugin to the root `CMakeLists.txt`:
   ```cmake
   option(BUILD_MYPLUGIN "Build My Plugin" ON)
   if(BUILD_MYPLUGIN)
       add_subdirectory(myplugin)
   endif()
   ```

### Running Tests

```powershell
# Run all tests
cd build
ctest -C Release --output-on-failure

# Run tests for a specific plugin
ctest -C Release -R Compressor
ctest -C Release -R Eq
```

## Technical Details

- **Format**: CLAP (CLever Audio Plugin)
- **Language**: C++20
- **Build System**: CMake 3.20+
- **Testing**: Google Test
- **Dependencies**: Automatically fetched via CMake FetchContent

## License

Copyright 2025 Stinky Computing

## Credits

- CLAP format by [Free Audio](https://github.com/free-audio/clap)
- Testing with [Google Test](https://github.com/google/googletest)
