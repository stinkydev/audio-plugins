# Sesame Audio Compressor CLAP

A high-performance audio compressor CLAP plugin with SIMD optimizations.

## Features

- **Cross-platform**: Windows, macOS, and Linux support
- **C++20**: Modern C++ with Google code style
- **SIMD Optimized**: AVX2 instructions with automatic scalar fallback
- **CLAP**: Modern, open-source plugin format
- **No GUI**: Lightweight, host-controlled parameters
- **No SDK required**: CLAP headers fetched automatically via CMake

## Parameters

- **Threshold** (-60 to 0 dB): Level above which compression is applied
- **Ratio** (1:1 to 20:1): Amount of compression
- **Attack** (0.1 to 100 ms): How quickly compression responds
- **Release** (10 to 1000 ms): How quickly compression releases
- **Knee** (0 to 12 dB): Soft knee width (0 = hard knee)
- **Makeup Gain** (-12 to +24 dB): Output level compensation

## Building

### Prerequisites

1. **CMake** 3.20 or higher
2. **C++20 compatible compiler**:
   - Windows: Visual Studio 2019 or higher / MSVC 19.20+
   - macOS: Xcode 12+ / Clang 10+
   - Linux: GCC 10+ or Clang 10+
3. **Internet connection** (for fetching CLAP SDK during build)

### Build Instructions

1. Configure and build:
```powershell
# Windows
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# macOS/Linux
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

2. Disable SIMD (optional):
```powershell
cmake .. -DENABLE_SIMD=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Build Outputs

- **Windows**: `build/Release/StinkyCompressor.clap`
- **macOS**: `build/StinkyCompressor.clap` (bundle)
- **Linux**: `build/StinkyCompressor.clap`

## Installation

Copy the built plugin to your CLAP plugin directory:

- **Windows**: `C:\Program Files\Common Files\CLAP\` or `%COMMONPROGRAMFILES%\CLAP\`
- **macOS**: `~/Library/Audio/Plug-Ins/CLAP/` or `/Library/Audio/Plug-Ins/CLAP/`
- **Linux**: `~/.clap/` or `/usr/lib/clap/`

## Architecture

```
include/
├── compressor_processor.h  # Core DSP algorithm
├── compressor_clap.h       # CLAP wrapper interface
└── simd_utils.h            # SIMD operations

src/
├── compressor_processor.cc # Compression implementation
├── compressor_clap.cc      # CLAP plugin implementation
└── simd_utils.cc           # SIMD with scalar fallback
```

## Technical Details

### Compression Algorithm

- Peak detection with stereo linking
- Logarithmic domain processing for accuracy
- Smooth envelope follower (attack/release)
- Soft knee capability
- Makeup gain compensation

### SIMD Implementation

- Runtime CPU feature detection
- AVX2 vectorization for 8x float operations
- Automatic fallback to scalar code
- Operations vectorized:
  - Gain application
  - Multiply-add operations
  - Min/max operations

### Code Style

Follows Google C++ Style Guide:
- 2-space indentation
- Snake_case for functions and variables
- PascalCase for classes
- Pointer/reference alignment left
- Use `.clang-format` for automatic formatting

## License

This project is provided as-is for educational and commercial use. CLAP is an open-source plugin format under the MIT license.

## CLAP Advantages

- **Open-source**: No licensing restrictions or SDK downloads required
- **Modern design**: Built for today's audio workflows
- **Performance**: Efficient event handling and processing
- **Extensible**: Clean extension system for future features
- **Community-driven**: Active development and broad DAW support

## Supported DAWs

CLAP is supported by many modern DAWs including:
- Bitwig Studio
- Reaper
- FL Studio
- Ardour
- LMMS
- And many more (growing list)
