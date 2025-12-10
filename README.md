# Audio Plugins Collection

A collection of high-quality CLAP audio plugins built with modern C++20.

## Plugins

### 🎚️ Stinky Compressor
High-performance dynamics compressor with SIMD optimizations.
- Threshold, ratio, attack, release controls
- Soft knee compression
- Auto makeup gain
- [Learn more](compressor/README.md)

### 🎛️ Stinky EQ
4-band parametric equalizer with multiple filter types.
- 8 filter types per band (Low/High Pass, Low/High Shelf, Peak, Band Pass, Notch, All Pass)
- Independent frequency, gain, and Q controls
- Global output gain and bypass
- [Learn more](eq/README.md)

### 🔊 Stinky Limiter
Transparent brick-wall limiter for mastering and final stage processing.
- Independent threshold and output level controls
- Fast attack and release for transparent limiting
- [Learn more](limiter/README.md)

### 🔁 Stinky Delay
Simple stereo delay effect with synchronized channels.
- Delay time up to 2000ms
- Dry/wet mix control
- [Learn more](delay/README.md)

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
cmake .. -DBUILD_COMPRESSOR=ON -DBUILD_EQ=OFF -DBUILD_LIMITER=OFF -DBUILD_DELAY=OFF

# Disable tests
cmake .. -DBUILD_TESTS=OFF

# Disable SIMD optimizations (compressor only)
cmake .. -DENABLE_SIMD=OFF

# Combine options
cmake .. -DBUILD_COMPRESSOR=ON -DBUILD_EQ=ON -DBUILD_LIMITER=OFF -DBUILD_DELAY=OFF -DBUILD_TESTS=OFF
```

### Output Locations

After building, compiled plugins will be in:
- **Windows**: `build/{plugin}/Release/Stinky{Plugin}.clap`
  - Example: `build/compressor/Release/StinkyCompressor.clap`
- **macOS/Linux**: `build/{plugin}/Stinky{Plugin}.clap`
  - Example: `build/compressor/StinkyCompressor.clap`

All plugins also include TypeScript definitions for web integration:
- `{plugin}/{plugin}-plugin.ts` (e.g., `compressor/compressor-plugin.ts`)

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
├── plugins.ts              # Central TypeScript exports
├── compressor/             # Compressor plugin
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── compressor-plugin.ts
│   ├── include/
│   ├── src/
│   └── tests/
├── eq/                     # EQ plugin
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── eq-plugin.ts
│   ├── include/
│   ├── src/
│   ├── tests/
│   └── web-ui/            # Web-based EQ editor
├── limiter/                # Limiter plugin
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── limiter-plugin.ts
│   ├── include/
│   ├── src/
│   └── tests/
└── delay/                  # Delay plugin
    ├── CMakeLists.txt
    ├── README.md
    ├── delay-plugin.ts
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
ctest -C Release -R Limiter
ctest -C Release -R Delay
```

## Technical Details

- **Format**: CLAP (CLever Audio Plugin) 1.2.7
- **Language**: C++20
- **Build System**: CMake 3.20+
- **Testing**: Google Test 1.14.0
- **Dependencies**: Automatically fetched via CMake FetchContent
- **Parameter Architecture**: All parameters normalized to 0..1 range for DAW automation compatibility
- **TypeScript Integration**: Each plugin includes TypeScript definitions with conversion functions for web/host integration

## License

Copyright 2025 Stinky Computing

## Credits

- CLAP format by [Free Audio](https://github.com/free-audio/clap)
- Testing with [Google Test](https://github.com/google/googletest)
