# Sesame Limiter

A high-performance look-ahead brickwall limiter CLAP plugin with SIMD optimization.

## Features

- **Brickwall Limiting**: Hard ceiling prevents any signal from exceeding the threshold
- **5ms Lookahead**: Fixed lookahead buffer for transparent limiting without distortion
- **Instant Attack**: 0.1ms attack time for catching all transients
- **Fast Release**: 50ms release for natural dynamics recovery
- **Output Level Control**: Set target output level independently of threshold
- **SIMD Optimized**: AVX2 acceleration for efficient processing
- **Stereo Linking**: True stereo processing with linked peak detection
- **Zero Latency Option**: Minimal 5ms latency for real-time performance

## Parameters

### Threshold
- **Range**: -60.0 dB to 0.0 dB
- **Default**: -0.1 dB
- **Description**: The brickwall ceiling. No signal will exceed this level. Set this to your desired maximum output level (e.g., -0.1 dB to prevent clipping).

### Output Level
- **Range**: -60.0 dB to 0.0 dB
- **Default**: -0.1 dB
- **Description**: The target output level after limiting. This applies makeup gain to bring the limited signal to your desired level. Set this equal to threshold for transparent limiting, or lower for additional headroom.

## Building

### Requirements
- CMake 3.20 or later
- C++20 compatible compiler (MSVC, GCC, Clang)
- AVX2-capable CPU (for SIMD optimization)

### Build Instructions

```bash
# Configure
cmake -B build -DBUILD_LIMITER=ON -DENABLE_SIMD=ON

# Build
cmake --build build --config Release

# The plugin will be at: build/limiter/Release/StinkyLimiter.clap
```

### Build Options
- `BUILD_LIMITER`: Enable building the limiter plugin (default: ON)
- `BUILD_TESTS`: Build unit tests (default: ON)
- `ENABLE_SIMD`: Enable AVX2 SIMD optimizations (default: ON)

## Installation

### Windows
Copy `StinkyLimiter.clap` to:
```
%COMMONPROGRAMFILES%\CLAP\
```

### macOS
Copy `StinkyLimiter.clap` to:
```
/Library/Audio/Plug-Ins/CLAP/
```

### Linux
Copy `StinkyLimiter.clap` to:
```
~/.clap/
```

## Usage

The Sesame Limiter is a simple, effective brickwall limiter designed for mastering and final stage limiting:

1. **Basic Mastering**: Set threshold to -0.1 dB and output level to -0.1 dB for maximum loudness without clipping
2. **Conservative Mastering**: Set threshold to -0.3 dB and output level to -0.3 dB for safer headroom
3. **Streaming Prep**: Set threshold to -1.0 dB and output level to -1.0 dB to meet platform requirements
4. **Safety Limiter**: Set threshold to -0.1 dB and output level to -6.0 dB for broadcast/streaming with extra headroom

### Tips
- The limiter has a fixed 5ms lookahead for optimal transparency
- Attack time is instant (0.1ms) to catch all peaks
- Release time is fixed at 50ms for natural dynamics
- Output level can be set below threshold for additional safety margin
- Use on master bus or individual tracks for peak control

## How It Works

The limiter uses a 5ms lookahead delay buffer to analyze incoming audio before it reaches the output. When peaks are detected that would exceed the threshold:

1. The detector calculates required gain reduction
2. The envelope follower applies instant attack and fast release
3. Gain reduction is applied to the delayed (past) audio
4. Output is scaled to the target output level
5. Final signal never exceeds the threshold (brickwall)

The fixed lookahead allows the limiter to "see into the future" and apply gain reduction preemptively, preventing peaks while maintaining clarity. The output level control provides independent control over the final output gain.

## Technical Details

- **Processing**: 32-bit floating point
- **Latency**: Fixed 5ms (220 samples @ 44.1kHz)
- **Attack Time**: 0.1ms (instant)
- **Release Time**: 50ms (fixed)
- **Sample Rates**: All standard rates supported
- **SIMD**: AVX2 with scalar fallback
- **Algorithm**: Look-ahead feedforward brickwall limiter

## License

Copyright 2025 Stinky Computing

## Support

- GitHub: https://github.com/fastlimiter
- Issues: Report bugs and request features on GitHub
