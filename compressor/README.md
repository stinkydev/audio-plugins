# Stinky Compressor

A feed-forward compressor CLAP plugin for live production, built for ambience
and commentary buses in sports broadcast.

## Features

- **Clean on low frequencies**: the detector holds its gain for 15 ms before
  releasing, so it never chases the half-cycles of crowd rumble or PA bass. No
  measurable distortion on a 50 Hz tone at 14 dB over threshold.
- **Correct soft knee**: quadratic knee that is continuous at both edges and
  never boosts below threshold.
- **Linked stereo detection** with optional external sidechain.
- **Predictable auto makeup**: a fixed function of threshold, ratio and knee.
  No adaptive gain, no level creep, no jump when switched on.
- **Click-free parameter changes**: makeup gain moves are smoothed over 10 ms.
- **Cross-platform**: Windows, macOS, and Linux
- **CLAP**: open-source plugin format, headers fetched automatically via CMake

## Parameters

- **Threshold** (-60 to 0 dB): Level above which compression is applied
- **Ratio** (1:1 to 100:1): Amount of compression
- **Attack** (0.05 to 250 ms): How quickly gain reduction builds up
- **Release** (10 to 2500 ms): How quickly gain reduction recovers, after a
  fixed 15 ms hold
- **Knee** (0 to 12 dB): Soft knee width (0 = hard knee)
- **Makeup Gain** (-12 to +24 dB): Output level compensation
- **Auto Makeup** (on/off): Replaces Makeup Gain with half the gain reduction a
  full-scale steady signal would receive, so a mix that peaks near full scale
  comes out at roughly the same level

### Starting points

- **Commentary**: threshold around -24 dB, ratio 3:1, attack 5 ms, release
  120 ms, knee 6 dB. Enough to even out a commentator without pumping.
- **Ambience**: threshold set so the crowd's normal level sits just under it,
  ratio 2:1 to 3:1, attack 20 ms, release 300 to 500 ms, knee 12 dB. Slow
  release keeps a roar from breathing.

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

Feed-forward topology after Giannoulis, Massberg & Reiss (2012), with the
level detector placed after the gain computer in the dB domain:

1. Linked peak level of the sidechain (or main input) is converted to dB.
2. The static curve computes the gain reduction for that level, using a
   quadratic soft knee.
3. The gain reduction is smoothed in dB. It attacks with the attack time
   whenever the target is at or above the current value. When the target
   falls below, the current value is held for 15 ms, then released with the
   release time. The hold is what keeps low-frequency content from
   modulating the gain within a cycle.
4. Makeup gain (manual or auto) is smoothed over 10 ms and applied together
   with the gain reduction.

The gain reduction meter reports the deepest reduction reached in the most
recent block.

### SIMD utilities

`simd_utils` provides AVX2 vector helpers with runtime detection and a scalar
fallback. They are built and tested but the compressor's own loop is scalar;
its cost is dominated by one logarithm and one power per frame.

### Code Style

Follows Google C++ Style Guide:
- 2-space indentation
- Snake_case for functions and variables
- PascalCase for classes
- Pointer/reference alignment left
- Use `.clang-format` for automatic formatting

## License

MIT License - see [LICENSE](../LICENSE) file for details.

Copyright (c) 2025 Stinky Computing

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
